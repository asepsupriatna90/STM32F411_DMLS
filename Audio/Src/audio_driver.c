/**
  ******************************************************************************
  * @file           : audio_driver.c
  * @brief          : Audio driver implementation
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Audio driver for STM32F411 DSP Crossover System
  * Handles audio I/O and buffer management for PCM1808 (ADC) and PCM5102A (DAC)
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "audio_driver.h"
#include "math_utils.h"
#include "debug.h"
#include <math.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
typedef enum {
    BUFFER_HALF = 0,
    BUFFER_FULL
} BufferState_TypeDef;

/* Private define ------------------------------------------------------------*/
/* DMA buffer sizes, each sample is 32-bit (24-bit audio in 32-bit container) */
#define DMA_INPUT_BUFFER_SIZE     (AUDIO_BUFFER_SIZE * AUDIO_INPUT_CHANNELS)
#define DMA_OUTPUT_BUFFER_SIZE    (AUDIO_BUFFER_SIZE * AUDIO_OUTPUT_CHANNELS)

/* Private macro -------------------------------------------------------------*/
#define FLOAT_TO_INT24(x)     ((int32_t)((x) * AUDIO_MAX_VALUE))
#define INT24_TO_FLOAT(x)     ((float)(x) / AUDIO_MAX_VALUE)

/* Private variables ---------------------------------------------------------*/
/* DMA buffers for input and output */
static int32_t inputDmaBuffer[DMA_INPUT_BUFFER_SIZE];
static int32_t outputDmaBuffer[DMA_OUTPUT_BUFFER_SIZE];

/* Buffer management */
static volatile BufferState_TypeDef inputBufferState = BUFFER_HALF;
static volatile BufferState_TypeDef outputBufferState = BUFFER_HALF;
static volatile uint8_t audioProcessingNeeded = 0;

/* Driver status */
static AudioDriverStatus_TypeDef audioStatus;

/* RMS calculation buffers */
static float rmsValues[AUDIO_OUTPUT_CHANNELS] = {0.0f};

/* Private function prototypes -----------------------------------------------*/
static void Audio_ProcessInputSamples(uint32_t offset, AudioBuffer_TypeDef *buffer);
static void Audio_PrepareOutputSamples(uint32_t offset, AudioBuffer_TypeDef *buffer);
static void Audio_ResetBuffers(void);

/**
  * @brief  Initialize audio driver and codec hardware
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_Init(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    
    /* Initialize status structure */
    audioStatus.state = AUDIO_STATE_IDLE;
    audioStatus.sampleRate = AUDIO_SAMPLING_RATE;
    audioStatus.inputUnderflows = 0;
    audioStatus.outputOverflows = 0;
    
    /* Set default gains to 0dB (1.0) */
    for (uint8_t i = 0; i < AUDIO_INPUT_CHANNELS; i++) {
        audioStatus.inputGain[i] = 1.0f;
        audioStatus.inputMute[i] = 0;
    }
    
    for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
        audioStatus.outputGain[i] = 1.0f;
        audioStatus.outputMute[i] = 0;
    }
    
    /* Initialize input codec (PCM1808) */
    status = PCM1808_Init();
    if (status != HAL_OK) {
        DEBUG_PRINT("PCM1808 initialization failed\r\n");
        return status;
    }
    
    /* Initialize output codec (PCM5102A) */
    status = PCM5102A_Init();
    if (status != HAL_OK) {
        DEBUG_PRINT("PCM5102A initialization failed\r\n");
        return status;
    }
    
    /* Reset audio buffers */
    Audio_ResetBuffers();
    
    DEBUG_PRINT("Audio driver initialized at %dHz\r\n", audioStatus.sampleRate);
    
    return status;
}

/**
  * @brief  Start audio processing
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_Start(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    
    /* Start input codec */
    status = PCM1808_Start();
    if (status != HAL_OK) {
        DEBUG_PRINT("PCM1808 start failed\r\n");
        return status;
    }
    
    /* Start output codec */
    status = PCM5102A_Start();
    if (status != HAL_OK) {
        DEBUG_PRINT("PCM5102A start failed\r\n");
        return status;
    }
    
    /* Start I2S DMA for receiving audio */
    status = HAL_I2S_Receive_DMA(&hi2s2, (uint16_t*)inputDmaBuffer, DMA_INPUT_BUFFER_SIZE);
    if (status != HAL_OK) {
        DEBUG_PRINT("I2S receive DMA start failed\r\n");
        return status;
    }
    
    /* Start I2S DMA for transmitting audio */
    status = HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)outputDmaBuffer, DMA_OUTPUT_BUFFER_SIZE);
    if (status != HAL_OK) {
        DEBUG_PRINT("I2S transmit DMA start failed\r\n");
        return status;
    }
    
    audioStatus.state = AUDIO_STATE_RUNNING;
    DEBUG_PRINT("Audio processing started\r\n");
    
    return status;
}

/**
  * @brief  Stop audio processing
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_Stop(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    
    /* Stop I2S DMA */
    status = HAL_I2S_DMAStop(&hi2s2);
    if (status != HAL_OK) {
        return status;
    }
    
    status = HAL_I2S_DMAStop(&hi2s3);
    if (status != HAL_OK) {
        return status;
    }
    
    /* Stop codecs */
    PCM1808_Stop();
    PCM5102A_Stop();
    
    audioStatus.state = AUDIO_STATE_IDLE;
    DEBUG_PRINT("Audio processing stopped\r\n");
    
    return status;
}

/**
  * @brief  Pause audio processing
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_Pause(void)
{
    if (audioStatus.state != AUDIO_STATE_RUNNING) {
        return HAL_ERROR;
    }
    
    /* Pause output codec */
    PCM5102A_Mute(1);
    
    DEBUG_PRINT("Audio processing paused\r\n");
    return HAL_OK;
}

/**
  * @brief  Resume audio processing
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_Resume(void)
{
    if (audioStatus.state != AUDIO_STATE_RUNNING) {
        return HAL_ERROR;
    }
    
    /* Resume output codec */
    PCM5102A_Mute(0);
    
    DEBUG_PRINT("Audio processing resumed\r\n");
    return HAL_OK;
}

/**
  * @brief  Get input samples from DMA buffer
  * @param  buffer: Pointer to audio buffer
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_GetInputSamples(AudioBuffer_TypeDef *buffer)
{
    uint32_t offset;
    
    if (audioStatus.state != AUDIO_STATE_RUNNING) {
        return HAL_ERROR;
    }
    
    /* Determine which half of the buffer to process */
    offset = (inputBufferState == BUFFER_HALF) ? 0 : (DMA_INPUT_BUFFER_SIZE / 2);
    
    /* Process input samples - convert from int24 to float */
    Audio_ProcessInputSamples(offset, buffer);
    
    return HAL_OK;
}

/**
  * @brief  Send output samples to DMA buffer
  * @param  buffer: Pointer to audio buffer
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_SendOutputSamples(AudioBuffer_TypeDef *buffer)
{
    uint32_t offset;
    
    if (audioStatus.state != AUDIO_STATE_RUNNING) {
        return HAL_ERROR;
    }
    
    /* Determine which half of the buffer to process */
    offset = (outputBufferState == BUFFER_HALF) ? 0 : (DMA_OUTPUT_BUFFER_SIZE / 2);
    
    /* Prepare output samples - convert from float to int24 */
    Audio_PrepareOutputSamples(offset, buffer);
    
    return HAL_OK;
}

/**
  * @brief  Set input gain for a specific channel
  * @param  channel: Input channel (0-1)
  * @param  gain: Linear gain value (typically 0.0-2.0)
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_SetInputGain(uint8_t channel, float gain)
{
    if (channel >= AUDIO_INPUT_CHANNELS) {
        return HAL_ERROR;
    }
    
    audioStatus.inputGain[channel] = gain;
    return HAL_OK;
}

/**
  * @brief  Set output gain for a specific channel
  * @param  channel: Output channel (0-3)
  * @param  gain: Linear gain value (typically 0.0-2.0)
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_SetOutputGain(uint8_t channel, float gain)
{
    if (channel >= AUDIO_OUTPUT_CHANNELS) {
        return HAL_ERROR;
    }
    
    audioStatus.outputGain[channel] = gain;
    return HAL_OK;
}

/**
  * @brief  Mute or unmute input channel
  * @param  channel: Input channel (0-1)
  * @param  state: Mute state (0=unmute, 1=mute)
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_MuteInput(uint8_t channel, uint8_t state)
{
    if (channel >= AUDIO_INPUT_CHANNELS) {
        return HAL_ERROR;
    }
    
    audioStatus.inputMute[channel] = state ? 1 : 0;
    return HAL_OK;
}

/**
  * @brief  Mute or unmute output channel
  * @param  channel: Output channel (0-3)
  * @param  state: Mute state (0=unmute, 1=mute)
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_MuteOutput(uint8_t channel, uint8_t state)
{
    if (channel >= AUDIO_OUTPUT_CHANNELS) {
        return HAL_ERROR;
    }
    
    audioStatus.outputMute[channel] = state ? 1 : 0;
    return HAL_OK;
}

/**
  * @brief  Get audio driver status
  * @retval Audio driver status structure
  */
AudioDriverStatus_TypeDef Audio_GetStatus(void)
{
    return audioStatus;
}

/**
  * @brief  Reset audio driver
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_Reset(void)
{
    HAL_StatusTypeDef status;
    
    /* Stop audio processing */
    status = Audio_Stop();
    if (status != HAL_OK) {
        return status;
    }
    
    /* Reset buffers */
    Audio_ResetBuffers();
    
    /* Reset status counters */
    audioStatus.inputUnderflows = 0;
    audioStatus.outputOverflows = 0;
    
    /* Reset RMS values */
    for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
        rmsValues[i] = 0.0f;
    }
    
    /* Restart audio */
    status = Audio_Start();
    
    return status;
}

/**
  * @brief  Calculate RMS value for a specific output channel
  * @param  channel: Output channel (0-3)
  * @param  buffer: Pointer to audio buffer
  * @retval RMS value (0.0-1.0)
  */
float Audio_CalculateRMS(uint8_t channel, AudioBuffer_TypeDef *buffer)
{
    float sum = 0.0f;
    float sample;
    
    if (channel >= AUDIO_OUTPUT_CHANNELS) {
        return 0.0f;
    }
    
    /* Use only a window of samples to reduce computational load */
    for (uint32_t i = 0; i < AUDIO_RMS_WINDOW_SIZE; i++) {
        sample = buffer->channels[channel][i];
        sum += sample * sample;
    }
    
    /* Calculate RMS */
    float newRMS = sqrtf(sum / AUDIO_RMS_WINDOW_SIZE);
    
    /* Apply smoothing */
    rmsValues[channel] = (rmsValues[channel] * AUDIO_RMS_DECAY) + (newRMS * (1.0f - AUDIO_RMS_DECAY));
    
    return rmsValues[channel];
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Process input samples from DMA buffer to audio buffer
  * @param  offset: Offset in DMA buffer
  * @param  buffer: Pointer to audio buffer
  * @retval None
  */
static void Audio_ProcessInputSamples(uint32_t offset, AudioBuffer_TypeDef *buffer)
{
    uint32_t sample_idx = 0;
    int32_t sample;
    float sample_float;
    
    /* Process each sample in the current buffer half */
    for (uint32_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
        /* Process each input channel */
        for (uint8_t ch = 0; ch < AUDIO_INPUT_CHANNELS; ch++) {
            /* Get sample from DMA buffer */
            sample = inputDmaBuffer[offset + sample_idx++];
            
            /* Convert to float in range [-1.0, 1.0] */
            sample_float = INT24_TO_FLOAT(sample);
            
            /* Apply input gain and mute */
            if (audioStatus.inputMute[ch]) {
                sample_float = 0.0f;
            } else {
                sample_float *= audioStatus.inputGain[ch];
            }
            
            /* Store in audio buffer */
            buffer->channels[ch][i] = sample_float;
        }
    }
}

/**
  * @brief  Prepare output samples from audio buffer to DMA buffer
  * @param  offset: Offset in DMA buffer
  * @param  buffer: Pointer to audio buffer
  * @retval None
  */
static void Audio_PrepareOutputSamples(uint32_t offset, AudioBuffer_TypeDef *buffer)
{
    uint32_t sample_idx = 0;
    float sample_float;
    int32_t sample_int;
    
    /* Process each sample in the current buffer half */
    for (uint32_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
        /* Process each output channel */
        for (uint8_t ch = 0; ch < AUDIO_OUTPUT_CHANNELS; ch++) {
            /* Get sample from audio buffer */
            sample_float = buffer->channels[ch][i];
            
            /* Apply output gain and mute */
            if (audioStatus.outputMute[ch]) {
                sample_float = 0.0f;
            } else {
                sample_float *= audioStatus.outputGain[ch];
            }
            
            /* Hard limit to prevent clipping */
            sample_float = CLAMP(sample_float, -1.0f, 1.0f);
            
            /* Convert float to int24 */
            sample_int = FLOAT_TO_INT24(sample_float);
            
            /* Store in DMA buffer */
            outputDmaBuffer[offset + sample_idx++] = sample_int;
        }
    }
}

/**
  * @brief  Reset all audio buffers
  * @retval None
  */
static void Audio_ResetBuffers(void)
{
    /* Clear DMA buffers */
    memset(inputDmaBuffer, 0, sizeof(inputDmaBuffer));
    memset(outputDmaBuffer, 0, sizeof(outputDmaBuffer));
    
    /* Reset buffer states */
    inputBufferState = BUFFER_HALF;
    outputBufferState = BUFFER_HALF;
    audioProcessingNeeded = 0;
}

/* I2S DMA Callbacks ---------------------------------------------------------*/

/**
  * @brief  I2S Rx Half Complete callback
  * @param  hi2s: I2S handle
  * @retval None
  */
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2) {
        inputBufferState = BUFFER_HALF;
        audioProcessingNeeded = 1;
        
        /* Call the audio processing callback */
        Audio_ProcessCallback();
    }
}

/**
  * @brief  I2S Rx Complete callback
  * @param  hi2s: I2S handle
  * @retval None
  */
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2) {
        inputBufferState = BUFFER_FULL;
        audioProcessingNeeded = 1;
        
        /* Call the audio processing callback */
        Audio_ProcessCallback();
    }
}

/**
  * @brief  I2S Tx Half Complete callback
  * @param  hi2s: I2S handle
  * @retval None
  */
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI3) {
        outputBufferState = BUFFER_HALF;
    }
}

/**
  * @brief  I2S Tx Complete callback
  * @param  hi2s: I2S handle
  * @retval None
  */
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI3) {
        outputBufferState = BUFFER_FULL;
    }
}

/**
  * @brief  I2S Error callback
  * @param  hi2s: I2S handle
  * @retval None
  */
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2) {
        /* Input error */
        audioStatus.inputUnderflows++;
        DEBUG_PRINT("I2S input error\r\n");
    } else if (hi2s->Instance == SPI3) {
        /* Output error */
        audioStatus.outputOverflows++;
        DEBUG_PRINT("I2S output error\r\n");
    }
    
    if (audioStatus.state == AUDIO_STATE_RUNNING) {
        audioStatus.state = AUDIO_STATE_ERROR;
        
        /* Try to recover */
        Audio_Reset();
    }
}