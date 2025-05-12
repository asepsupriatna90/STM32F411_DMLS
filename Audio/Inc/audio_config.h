/**
  ******************************************************************************
  * @file           : audio_config.h
  * @brief          : Audio configuration header file
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * General audio configuration for the DSP Crossover System
  * Defines sample rates, buffer sizes, and basic audio parameters
  *
  ******************************************************************************
  */

#ifndef __AUDIO_CONFIG_H
#define __AUDIO_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
/* Audio sample rate */
#define AUDIO_SAMPLE_RATE           48000U  /* Sample rate in Hz (48kHz) */
#define AUDIO_BIT_DEPTH             24      /* Audio bit depth */
#define AUDIO_FRAME_SIZE            32      /* Audio frame size in samples */

/* Channel configuration */
#define AUDIO_INPUT_CHANNELS        2       /* Number of input channels (stereo) */
#define AUDIO_OUTPUT_CHANNELS       4       /* Number of output channels */

/* Buffer sizes */
#define AUDIO_BUFFER_SIZE           (AUDIO_FRAME_SIZE * 2)  /* Double buffer for ping-pong */
#define AUDIO_MAX_DELAY_MS          20      /* Maximum delay in milliseconds */
#define AUDIO_MAX_DELAY_SAMPLES     (AUDIO_SAMPLE_RATE * AUDIO_MAX_DELAY_MS / 1000)

/* Audio level thresholds */
#define AUDIO_SILENCE_THRESHOLD     0.001f  /* -60 dB */
#define AUDIO_CLIP_THRESHOLD        0.95f   /* Near full scale to avoid clipping */

/* Audio processing constants */
#define AUDIO_MIN_FREQ              20.0f   /* Minimum frequency in Hz */
#define AUDIO_MAX_FREQ              20000.0f /* Maximum frequency in Hz */
#define AUDIO_MIN_GAIN_DB           -80.0f  /* Minimum gain in dB */
#define AUDIO_MAX_GAIN_DB           12.0f   /* Maximum gain in dB */

/* Audio signal flow enumeration */
typedef enum {
  AUDIO_STAGE_INPUT = 0,
  AUDIO_STAGE_ROUTING,
  AUDIO_STAGE_CROSSOVER,
  AUDIO_STAGE_EQ,
  AUDIO_STAGE_COMPRESSOR,
  AUDIO_STAGE_LIMITER,
  AUDIO_STAGE_DELAY,
  AUDIO_STAGE_GAIN,
  AUDIO_STAGE_OUTPUT
} AudioStage_TypeDef;

/* Audio buffer structure */
typedef struct {
  float samples[AUDIO_OUTPUT_CHANNELS][AUDIO_BUFFER_SIZE];
  uint16_t position;
  uint8_t bufferHalf;  /* 0 for first half, 1 for second half */
} AudioBuffer_TypeDef;

/* Audio channel configuration */
typedef struct {
  float gainLinear;        /* Linear gain multiplier */
  float gainDb;            /* Gain in dB */
  uint8_t mute;            /* Mute status (1=muted, 0=active) */
  uint8_t phase;           /* Phase invert (0=normal, 1=inverted) */
  char name[16];           /* Channel name for display */
} AudioChannel_TypeDef;

/* Audio routing configuration */
typedef struct {
  uint8_t source[AUDIO_OUTPUT_CHANNELS];  /* Input source for each output (0=IN1, 1=IN2, 2=IN1+IN2) */
  float mixRatio[AUDIO_OUTPUT_CHANNELS];  /* Mix ratio for IN1+IN2 mode (0.0 to 1.0) */
} AudioRouting_TypeDef;

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Initialize audio configuration
  * @param  None
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_Config_Init(void);

/**
  * @brief  Set default audio configuration values
  * @param  None
  * @retval None
  */
void Audio_Config_SetDefaults(void);

/**
  * @brief  Convert linear gain to dB value
  * @param  linear: Linear gain value
  * @retval Gain in dB
  */
float Audio_LinearToDB(float linear);

/**
  * @brief  Convert dB value to linear gain
  * @param  dB: Gain in dB
  * @retval Linear gain value
  */
float Audio_DBToLinear(float dB);

/**
  * @brief  Calculate RMS value of an audio buffer
  * @param  pBuffer: Pointer to float buffer
  * @param  size: Buffer size in samples
  * @retval RMS value (0.0 to 1.0)
  */
float Audio_CalculateRMS(float *pBuffer, uint16_t size);

/**
  * @brief  Clip audio sample to valid range (-1.0 to 1.0)
  * @param  sample: Input sample
  * @retval Clipped sample
  */
float Audio_ClipSample(float sample);

/**
  * @brief  Get channel display name
  * @param  channel: Channel index
  * @retval Channel name string pointer
  */
const char* Audio_GetChannelName(uint8_t channel);

/**
  * @brief  Convert frequency to logarithmic scale percentage (for UI)
  * @param  frequency: Frequency in Hz
  * @retval Percentage value (0-100%)
  */
uint8_t Audio_FreqToPercent(float frequency);

/**
  * @brief  Convert percentage to logarithmic frequency (for UI)
  * @param  percent: Percentage value (0-100%)
  * @retval Frequency in Hz
  */
float Audio_PercentToFreq(uint8_t percent);

/* External variables --------------------------------------------------------*/
extern AudioChannel_TypeDef AudioChannels[AUDIO_OUTPUT_CHANNELS];
extern AudioRouting_TypeDef AudioRouting;

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_CONFIG_H */