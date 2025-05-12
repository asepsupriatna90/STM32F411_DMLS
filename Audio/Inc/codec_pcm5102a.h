/**
  ******************************************************************************
  * @file           : codec_pcm5102a.h
  * @brief          : Header for PCM5102A DAC driver
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CODEC_PCM5102A_H
#define __CODEC_PCM5102A_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "audio_config.h"
#include "stm32f4xx_hal.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief PCM5102A Configuration structure
  */
typedef struct {
    uint32_t SampleRate;      /* Sample rate in Hz (e.g., 44100, 48000) */
    uint8_t Volume;           /* Output volume setting (0-100) */
    uint8_t Mute;             /* Mute state (0=unmuted, 1=muted) */
    uint8_t OutputMode;       /* Output mode configuration */
    uint8_t DacFilter;        /* DAC filter setting */
} PCM5102A_Config_t;

/**
  * @brief PCM5102A Status enumeration
  */
typedef enum {
    PCM5102A_OK       = 0x00, /* Operation successful */
    PCM5102A_ERROR    = 0x01, /* General error */
    PCM5102A_BUSY     = 0x02, /* Device busy */
    PCM5102A_TIMEOUT  = 0x03  /* Operation timeout */
} PCM5102A_Status_t;

/* Exported constants --------------------------------------------------------*/
/* PCM5102A pin control macros (using direct GPIO control) */
/* Note: These are defined assuming these control pins are connected to GPIO */
/* Default pin mappings (should be revised based on actual hardware connections) */
/* FMT pin: set output format (I2S vs. left-justified) */
#ifndef PCM5102A_FMT_PIN
#define PCM5102A_FMT_PIN          GPIO_PIN_12
#define PCM5102A_FMT_GPIO_PORT    GPIOB
#endif

/* FLT pin: set filter mode between Fast Roll-off and Slow Roll-off filter */
#ifndef PCM5102A_FLT_PIN
#define PCM5102A_FLT_PIN          GPIO_PIN_13
#define PCM5102A_FLT_GPIO_PORT    GPIOB
#endif

/* DEMP pin: set de-emphasis filter: 0 = off, 1 = 44.1kHz sampling rate */
#ifndef PCM5102A_DEMP_PIN
#define PCM5102A_DEMP_PIN         GPIO_PIN_14
#define PCM5102A_DEMP_GPIO_PORT   GPIOB
#endif

/* XSMT pin: mute control: 0 = mute, 1 = normal operation */
#ifndef PCM5102A_XSMT_PIN
#define PCM5102A_XSMT_PIN         GPIO_PIN_15
#define PCM5102A_XSMT_GPIO_PORT   GPIOB
#endif

/* PCM5102A configuration values */
/* Filter modes */
#define PCM5102A_FILTER_FAST      0
#define PCM5102A_FILTER_SLOW      1

/* Format modes */
#define PCM5102A_FORMAT_I2S       0
#define PCM5102A_FORMAT_LJ        1

/* De-emphasis modes */
#define PCM5102A_DEMP_OFF         0
#define PCM5102A_DEMP_44K         1

/* Exported variables --------------------------------------------------------*/
extern PCM5102A_Config_t PCM5102A_Config;

/* Exported functions prototypes ---------------------------------------------*/
PCM5102A_Status_t PCM5102A_Init(PCM5102A_Config_t *config);
PCM5102A_Status_t PCM5102A_DeInit(void);
PCM5102A_Status_t PCM5102A_Play(void);
PCM5102A_Status_t PCM5102A_Stop(void);
PCM5102A_Status_t PCM5102A_Pause(void);
PCM5102A_Status_t PCM5102A_Resume(void);
PCM5102A_Status_t PCM5102A_SetVolume(uint8_t volume);
PCM5102A_Status_t PCM5102A_SetMute(uint8_t state);
PCM5102A_Status_t PCM5102A_SetSampleRate(uint32_t sampleRate);
PCM5102A_Status_t PCM5102A_SetFilter(uint8_t filterMode);
uint8_t PCM5102A_GetVolume(void);
uint8_t PCM5102A_GetMute(void);
uint32_t PCM5102A_GetSampleRate(void);
PCM5102A_Status_t PCM5102A_Reset(void);
PCM5102A_Status_t PCM5102A_CheckReady(void);

#ifdef __cplusplus
}
#endif

#endif /* __CODEC_PCM5102A_H */