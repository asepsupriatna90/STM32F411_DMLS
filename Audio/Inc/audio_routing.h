/**
  ******************************************************************************
  * @file           : audio_routing.h
  * @brief          : Header for audio_routing.c file.
  *                   This file contains the common defines and functions for
  *                   audio signal routing matrix.
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Panel Kontrol DSP STM32F411 untuk Sistem Audio Crossover Aktif
  * Routing matrix for 2-input, 4-output DSP processing system
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AUDIO_ROUTING_H
#define __AUDIO_ROUTING_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "audio_config.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  Input sources enum
  */
typedef enum {
  AUDIO_SOURCE_NONE = 0,
  AUDIO_SOURCE_IN1,
  AUDIO_SOURCE_IN2,
  AUDIO_SOURCE_IN1_IN2_MIX,
  AUDIO_SOURCE_IN1_ONLY_LEFT,
  AUDIO_SOURCE_IN1_ONLY_RIGHT,
  AUDIO_SOURCE_IN2_ONLY_LEFT,
  AUDIO_SOURCE_IN2_ONLY_RIGHT,
  AUDIO_SOURCE_MAX
} AudioSource_TypeDef;

/**
  * @brief  Audio routing configuration struct
  */
typedef struct {
  AudioSource_TypeDef source[AUDIO_OUTPUT_CHANNELS];  /* Source for each output channel */
  float mixLevel[AUDIO_OUTPUT_CHANNELS];              /* Mix level when mixing inputs (0.0 to 1.0) */
  float inputGain[AUDIO_INPUT_CHANNELS];              /* Input gain (0.0 to 4.0) */
  uint8_t outputMute[AUDIO_OUTPUT_CHANNELS];          /* Mute state for each output (0=unmuted, 1=muted) */
  uint8_t stereoLink[AUDIO_OUTPUT_CHANNELS/2];        /* Stereo link for channel pairs (0=independent, 1=linked) */
  uint8_t monoSumInputs;                              /* Mix inputs to mono (0=stereo, 1=mono) */
} AudioRouting_TypeDef;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initialize the audio routing matrix
  * @param  None
  * @retval None
  */
void AudioRouting_Init(void);

/**
  * @brief  Configure routing for a specific output channel
  * @param  outputChannel: Output channel number (0 to AUDIO_OUTPUT_CHANNELS-1)
  * @param  source: Audio source to connect
  * @retval None
  */
void AudioRouting_ConfigOutput(uint8_t outputChannel, AudioSource_TypeDef source);

/**
  * @brief  Configure input gain for a specific input channel
  * @param  inputChannel: Input channel number (0 to AUDIO_INPUT_CHANNELS-1)
  * @param  gain: Gain value (0.0 to 4.0)
  * @retval None
  */
void AudioRouting_SetInputGain(uint8_t inputChannel, float gain);

/**
  * @brief  Set mix level for an output when mixing inputs
  * @param  outputChannel: Output channel number (0 to AUDIO_OUTPUT_CHANNELS-1)
  * @param  level: Mix level (0.0 to 1.0)
  * @retval None
  */
void AudioRouting_SetMixLevel(uint8_t outputChannel, float level);

/**
  * @brief  Set mute state for an output channel
  * @param  outputChannel: Output channel number (0 to AUDIO_OUTPUT_CHANNELS-1)
  * @param  state: Mute state (0=unmuted, 1=muted)
  * @retval None
  */
void AudioRouting_SetMute(uint8_t outputChannel, uint8_t state);

/**
  * @brief  Set stereo link for channel pair
  * @param  pairIndex: Pair index (0 to AUDIO_OUTPUT_CHANNELS/2-1)
  * @param  state: Link state (0=independent, 1=linked)
  * @retval None
  */
void AudioRouting_SetStereoLink(uint8_t pairIndex, uint8_t state);

/**
  * @brief  Set mono sum mode for inputs
  * @param  state: Mono sum state (0=stereo, 1=mono)
  * @retval None
  */
void AudioRouting_SetMonoSum(uint8_t state);

/**
  * @brief  Process audio through the routing matrix
  * @param  inputBuffer: Pointer to input buffer
  * @param  outputBuffer: Pointer to output buffer
  * @retval None
  */
void AudioRouting_Process(AudioBuffer_TypeDef *inputBuffer, AudioBuffer_TypeDef *outputBuffer);

/**
  * @brief  Get current routing configuration
  * @param  config: Pointer to configuration structure
  * @retval None
  */
void AudioRouting_GetConfig(AudioRouting_TypeDef *config);

/**
  * @brief  Set routing configuration
  * @param  config: Pointer to configuration structure
  * @retval None
  */
void AudioRouting_SetConfig(AudioRouting_TypeDef *config);

/**
  * @brief  Reset routing to default configuration
  * @param  None
  * @retval None
  */
void AudioRouting_Reset(void);

/**
  * @brief  Get source name string for UI display
  * @param  source: Source enum value
  * @retval Pointer to source name string
  */
const char* AudioRouting_GetSourceName(AudioSource_TypeDef source);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_ROUTING_H */