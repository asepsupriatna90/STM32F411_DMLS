/**
  ******************************************************************************
  * @file           : audio_driver.h
  * @brief          : Header for audio_driver.c file.
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Audio driver for STM32F411 DSP Crossover System
  * Handles audio I/O and buffer management
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AUDIO_DRIVER_H
#define __AUDIO_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2s.h"
#include "dma.h"
#include "codec_pcm1808.h"
#include "codec_pcm5102a.h"
#include "audio_config.h"

/* Exported defines ----------------------------------------------------------*/
#define AUDIO_SAMPLING_RATE       48000U         /* 48kHz sampling rate */
#define AUDIO_BUFFER_SIZE         128U           /* Number of samples per channel in buffer */
#define AUDIO_FRAME_SIZE          (AUDIO_BUFFER_SIZE / 2)  /* Half-buffer processing frame */
#define AUDIO_BIT_DEPTH           24U            /* Audio bit depth */
#define AUDIO_MAX_VALUE           8388607.0f     /* 2^23 - 1 for 24-bit audio */
#define AUDIO_INPUT_CHANNELS      2U             /* Number of input channels */
#define AUDIO_OUTPUT_CHANNELS     4U             /* Number of output channels */

/* RMS calculation parameters */
#define AUDIO_RMS_WINDOW_SIZE     32U            /* RMS window size in samples */
#define AUDIO_RMS_DECAY           0.9f           /* Decay factor for RMS smoothing */

/* Exported types ------------------------------------------------------------*/
typedef struct {
    float channels[AUDIO_INPUT_CHANNELS][AUDIO_BUFFER_SIZE];   /* Input/Output channel buffer */
} AudioBuffer_TypeDef;

typedef enum {
    AUDIO_STATE_IDLE = 0,
    AUDIO_STATE_RUNNING,
    AUDIO_STATE_ERROR
} AudioState_TypeDef;

typedef struct {
    AudioState_TypeDef state;
    uint32_t sampleRate;
    uint32_t inputUnderflows;
    uint32_t outputOverflows;
    float inputGain[AUDIO_INPUT_CHANNELS];
    float outputGain[AUDIO_OUTPUT_CHANNELS];
    uint8_t inputMute[AUDIO_INPUT_CHANNELS];
    uint8_t outputMute[AUDIO_OUTPUT_CHANNELS];
} AudioDriverStatus_TypeDef;

/* Exported functions prototypes ---------------------------------------------*/
HAL_StatusTypeDef Audio_Init(void);
HAL_StatusTypeDef Audio_Start(void);
HAL_StatusTypeDef Audio_Stop(void);
HAL_StatusTypeDef Audio_Pause(void);
HAL_StatusTypeDef Audio_Resume(void);

HAL_StatusTypeDef Audio_GetInputSamples(AudioBuffer_TypeDef *buffer);
HAL_StatusTypeDef Audio_SendOutputSamples(AudioBuffer_TypeDef *buffer);

HAL_StatusTypeDef Audio_SetInputGain(uint8_t channel, float gain);
HAL_StatusTypeDef Audio_SetOutputGain(uint8_t channel, float gain);
HAL_StatusTypeDef Audio_MuteInput(uint8_t channel, uint8_t state);
HAL_StatusTypeDef Audio_MuteOutput(uint8_t channel, uint8_t state);

void Audio_ProcessCallback(void);  /* DMA callback for audio processing */
float Audio_CalculateRMS(uint8_t channel, AudioBuffer_TypeDef *buffer);

AudioDriverStatus_TypeDef Audio_GetStatus(void);
HAL_StatusTypeDef Audio_Reset(void);

/* Private functions ---------------------------------------------------------*/
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_DRIVER_H */