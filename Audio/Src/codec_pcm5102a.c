/**
  ******************************************************************************
  * @file           : codec_pcm5102a.c
  * @brief          : Implementation of PCM5102A DAC driver
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * PCM5102A DAC driver implementation for the STM32F411 Audio DSP Crossover
  * Handles initialization, control, and configuration of PCM5102A DAC
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "codec_pcm5102a.h"
#include "i2s.h"
#include "debug.h"

/* Private define ------------------------------------------------------------*/
#define PCM5102A_DEFAULT_SAMPLE_RATE  48000
#define PCM5102A_DEFAULT_VOLUME       80
#define PCM5102A_VOLUME_MAX           100

/* Private variables ---------------------------------------------------------*/
PCM5102A_Config_t PCM5102A_Config = {
    .SampleRate = PCM5102A_DEFAULT_SAMPLE_RATE,
    .Volume = PCM5102A_DEFAULT_VOLUME,
    .Mute = 0,
    .OutputMode = PCM5102A_FORMAT_I2S,
    .DacFilter = PCM5102A_FILTER_FAST
};

static uint8_t pcm5102a_initialized = 0;

/* Private function prototypes -----------------------------------------------*/
static PCM5102A_Status_t PCM5102A_InitGPIO(void);
static PCM5102A_Status_t PCM5102A_ConfigureControlPins(PCM5102A_Config_t *config);
static void PCM5102A_SetMutePin(uint8_t state);
static void PCM5102A_SetFilterPin(uint8_t filterMode);
static void PCM5102A_SetFormatPin(uint8_t formatMode);
static void PCM5102A_SetDempPin(uint8_t dempMode);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief Initialize GPIO pins for controlling PCM5102A
  * @retval PCM5102A_Status_t
  */
static PCM5102A_Status_t PCM5102A_InitGPIO(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* Enable GPIO clocks */
    /* Note: In a real implementation, check if these ports are already enabled */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* Configure all control pins as outputs */
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    
    /* FMT pin */
    GPIO_InitStruct.Pin = PCM5102A_FMT_PIN;
    HAL_GPIO_Init(PCM5102A_FMT_GPIO_PORT, &GPIO_InitStruct);
    
    /* FLT pin */
    GPIO_InitStruct.Pin = PCM5102A_FLT_PIN;
    HAL_GPIO_Init(PCM5102A_FLT_GPIO_PORT, &GPIO_InitStruct);
    
    /* DEMP pin */
    GPIO_InitStruct.Pin = PCM5102A_DEMP_PIN;
    HAL_GPIO_Init(PCM5102A_DEMP_GPIO_PORT, &GPIO_InitStruct);
    
    /* XSMT pin */
    GPIO_InitStruct.Pin = PCM5102A_XSMT_PIN;
    HAL_GPIO_Init(PCM5102A_XSMT_GPIO_PORT, &GPIO_InitStruct);
    
    return PCM5102A_OK;
}

/**
  * @brief Configure PCM5102A control pins based on configuration
  * @param config PCM5102A configuration structure
  * @retval PCM5102A_Status_t
  */
static PCM5102A_Status_t PCM5102A_ConfigureControlPins(PCM5102A_Config_t *config)
{
    if (config == NULL) {
        return PCM5102A_ERROR;
    }
    
    /* Configure FMT pin (output format) */
    PCM5102A_SetFormatPin(config->OutputMode);
    
    /* Configure FLT pin (filter mode) */
    PCM5102A_SetFilterPin(config->DacFilter);
    
    /* Configure DEMP pin (de-emphasis) - set to off by default */
    PCM5102A_SetDempPin(PCM5102A_DEMP_OFF);
    
    /* Configure XSMT pin (mute) */
    PCM5102A_SetMutePin(!config->Mute); /* Inverted logic: 0 = mute, 1 = unmute */
    
    return PCM5102A_OK;
}

/**
  * @brief Set the mute pin state
  * @param state Mute state (0=muted, 1=unmuted)
  * @retval None
  * @note PCM5102A XSMT pin: 0=mute, 1=normal operation
  */
static void PCM5102A_SetMutePin(uint8_t state)
{
    /* Note: For PCM5102A, XSMT=0 means muted, XSMT=1 means normal operation */
    HAL_GPIO_WritePin(PCM5102A_XSMT_GPIO_PORT, PCM5102A_XSMT_PIN, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
  * @brief Set the filter mode pin state
  * @param filterMode Filter mode (0=fast roll-off, 1=slow roll-off)
  * @retval None
  */
static void PCM5102A_SetFilterPin(uint8_t filterMode)
{
    HAL_GPIO_WritePin(PCM5102A_FLT_GPIO_PORT, PCM5102A_FLT_PIN, 
                     filterMode == PCM5102A_FILTER_SLOW ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
  * @brief Set the format mode pin state
  * @param formatMode Format mode (0=I2S, 1=Left Justified)
  * @retval None
  */
static void PCM5102A_SetFormatPin(uint8_t formatMode)
{
    HAL_GPIO_WritePin(PCM5102A_FMT_GPIO_PORT, PCM5102A_FMT_PIN, 
                     formatMode == PCM5102A_FORMAT_LJ ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
  * @brief Set the de-emphasis pin state
  * @param dempMode De-emphasis mode (0=off, 1=44.1kHz)
  * @retval None
  */
static void PCM5102A_SetDempPin(uint8_t dempMode)
{
    HAL_GPIO_WritePin(PCM5102A_DEMP_GPIO_PORT, PCM5102A_DEMP_PIN, 
                     dempMode == PCM5102A_DEMP_44K ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* Exported functions --------------------------------------------------------*/

/**
  * @brief Initialize PCM5102A DAC
  * @param config PCM5102A configuration structure
  * @retval PCM5102A_Status_t
  */
PCM5102A_Status_t PCM5102A_Init(PCM5102A_Config_t *config)
{
    PCM5102A_Status_t status;
    
    /* Check if already initialized */
    if (pcm5102a_initialized) {
        return PCM5102A_OK;
    }
    
    /* If no config provided, use default */
    if (config == NULL) {
        config = &PCM5102A_Config;
    } else {
        /* Copy config to global structure */
        PCM5102A_Config = *config;
    }
    
    DEBUG_PRINT("Initializing PCM5102A DAC...\r\n");
    
    /* Initialize GPIO pins */
    status = PCM5102A_InitGPIO();
    if (status != PCM5102A_OK) {
        DEBUG_PRINT("PCM5102A GPIO init failed\r\n");
        return status;
    }
    
    /* Configure control pins based on config */
    status = PCM5102A_ConfigureControlPins(&PCM5102A_Config);
    if (status != PCM5102A_OK) {
        DEBUG_PRINT("PCM5102A control pin configuration failed\r\n");
        return status;
    }
    
    /* Initial state: Unmuted */
    PCM5102A_SetMute(0);
    
    /* Mark as initialized */
    pcm5102a_initialized = 1;
    
    DEBUG_PRINT("PCM5102A initialized successfully\r\n");
    return PCM5102A_OK;
}

/**
  * @brief De-initialize PCM5102A DAC
  * @retval PCM5102A_Status_t
  */
PCM5102A_Status_t PCM5102A_DeInit(void)
{
    if (!pcm5102a_initialized) {
        return PCM5102A_OK;
    }
    
    /* Mute output before deinitialization */
    PCM5102A_SetMute(1);
    
    /* Reset GPIO pins if needed */
    /* Note: In a real implementation, you might need to de-initialize GPIO */
    
    pcm5102a_initialized = 0;
    DEBUG_PRINT("PCM5102A de-initialized\r\n");
    
    return PCM5102A_OK;
}

/**
  * @brief Start PCM5102A DAC playback
  * @retval PCM5102A_Status_t
  */
PCM5102A_Status_t PCM5102A_Play(void)
{
    if (!pcm5102a_initialized) {
        return PCM5102A_ERROR;
    }
    
    /* Unmute the DAC if it was muted */
    if (PCM5102A_Config.Mute) {
        PCM5102A_SetMute(0);
    }
    
    DEBUG_PRINT("PCM5102A playback started\r\n");
    return PCM5102A_OK;
}

/**
  * @brief Stop PCM5102A DAC playback
  * @retval PCM5102A_Status_t
  */
PCM5102A_Status_t PCM5102A_Stop(void)
{
    if (!pcm5102a_initialized) {
        return PCM5102A_ERROR;
    }
    
    /* Mute the DAC output */
    PCM5102A_SetMute(1);
    
    DEBUG_PRINT("PCM5102A playback stopped\r\n");
    return PCM5102A_OK;
}

/**
  * @brief Pause PCM5102A DAC playback (mute output)
  * @retval PCM5102A_Status_t
  */
PCM5102A_Status_t PCM5102A_Pause(void)
{
    if (!pcm5102a_initialized) {
        return PCM5102A_ERROR;
    }
    
    /* Mute the DAC output */
    PCM5102A_SetMute(1);
    
    DEBUG_PRINT("PCM5102A playback paused\r\n");
    return PCM5102A_OK;
}

/**
  * @brief Resume PCM5102A DAC playback (unmute output)
  * @retval PCM5102A_Status_t
  */
PCM5102A_Status_t PCM5102A_Resume(void)
{
    if (!pcm5102a_initialized) {
        return PCM5102A_ERROR;
    }
    
    /* Unmute the DAC output */
    PCM5102A_SetMute(0);
    
    DEBUG_PRINT("PCM5102A playback resumed\r\n");
    return PCM5102A_OK;
}

/**
  * @brief Set PCM5102A volume
  * @param volume Volume level (0-100)
  * @retval PCM5102A_Status_t
  * @note PCM5102A doesn't have hardware volume control, this is for
  *       software volume control implemented in the DSP path
  */
PCM5102A_Status_t PCM5102A_SetVolume(uint8_t volume)
{
    if (!pcm5102a_initialized) {
        return PCM5102A_ERROR;
    }
    
    /* Limit volume to valid range */
    if (volume > PCM5102A_VOLUME_MAX) {
        volume = PCM5102A_VOLUME_MAX;
    }
    
    /* Store volume setting */
    PCM5102A_Config.Volume = volume;
    
    DEBUG_PRINT("PCM5102A volume set to %d%%\r\n", volume);
    return PCM5102A_OK;
}

/**
  * @brief Set PCM5102A mute state
  * @param state Mute state (0=unmuted, 1=muted)
  * @retval PCM5102A_Status_t
  */
PCM5102A_Status_t PCM5102A_SetMute(uint8_t state)
{
    if (!pcm5102a_initialized) {
        return PCM5102A_ERROR;
    }
    
    /* Update configuration */
    PCM5102A_Config.Mute = state ? 1 : 0;
    
    /* Set mute pin state */
    PCM5102A_SetMutePin(!state); /* Inverted logic: 0 = mute, 1 = unmute */
    
    DEBUG_PRINT("PCM5102A mute %s\r\n", state ? "enabled" : "disabled");
    return PCM5102A_OK;
}

/**
  * @brief Set PCM5102A sample rate
  * @param sampleRate Sample rate in Hz
  * @retval PCM5102A_Status_t
  * @note PCM5102A auto-detects sample rate from I2S clock
  *       This function updates the sample rate in configuration
  *       and configures I2S peripherals if needed
  */
PCM5102A_Status_t PCM5102A_SetSampleRate(uint32_t sampleRate)
{
    if (!pcm5102a_initialized) {
        return PCM5102A_ERROR;
    }
    
    /* Store sample rate */
    PCM5102A_Config.SampleRate = sampleRate;
    
    /* Set de-emphasis if 44.1kHz */
    if (sampleRate == 44100) {
        PCM5102A_SetDempPin(PCM5102A_DEMP_44K);
    } else {
        PCM5102A_SetDempPin(PCM5102A_DEMP_OFF);
    }
    
    DEBUG_PRINT("PCM5102A sample rate set to %lu Hz\r\n", sampleRate);
    return PCM5102A_OK;
}

/**
  * @brief Set PCM5102A filter mode
  * @param filterMode Filter mode (0=fast roll-off, 1=slow roll-off)
  * @retval PCM5102A_Status_t
  */
PCM5102A_Status_t PCM5102A_SetFilter(uint8_t filterMode)
{
    if (!pcm5102a_initialized) {
        return PCM5102A_ERROR;
    }
    
    /* Update configuration */
    PCM5102A_Config.DacFilter = filterMode;
    
    /* Set filter pin state */
    PCM5102A_SetFilterPin(filterMode);
    
    DEBUG_PRINT("PCM5102A filter mode set to %s\r\n", 
                filterMode == PCM5102A_FILTER_SLOW ? "slow roll-off" : "fast roll-off");
    return PCM5102A_OK;
}

/**
  * @brief Get PCM5102A current volume
  * @retval Current volume (0-100)
  */
uint8_t PCM5102A_GetVolume(void)
{
    return PCM5102A_Config.Volume;
}

/**
  * @brief Get PCM5102A current mute state
  * @retval Mute state (0=unmuted, 1=muted)
  */
uint8_t PCM5102A_GetMute(void)
{
    return PCM5102A_Config.Mute;
}

/**
  * @brief Get PCM5102A current sample rate
  * @retval Sample rate in Hz
  */
uint32_t PCM5102A_GetSampleRate(void)
{
    return PCM5102A_Config.SampleRate;
}

/**
  * @brief Reset PCM5102A DAC
  * @retval PCM5102A_Status_t
  */
PCM5102A_Status_t PCM5102A_Reset(void)
{
    PCM5102A_Status_t status;
    
    /* De-initialize first */
    status = PCM5102A_DeInit();
    if (status != PCM5102A_OK) {
        return status;
    }
    
    /* Small delay for power stabilization */
    HAL_Delay(10);
    
    /* Re-initialize with current configuration */
    status = PCM5102A_Init(&PCM5102A_Config);
    
    DEBUG_PRINT("PCM5102A reset complete\r\n");
    return status;
}

/**
  * @brief Check if PCM5102A is ready
  * @retval PCM5102A_Status_t
  */
PCM5102A_Status_t PCM5102A_CheckReady(void)
{
    if (!pcm5102a_initialized) {
        return PCM5102A_ERROR;
    }
    
    /* PCM5102A doesn't have a ready pin or status register
       we can only check if it's initialized */
    return PCM5102A_OK;
}