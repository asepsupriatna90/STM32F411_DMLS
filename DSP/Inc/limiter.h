/**
  ******************************************************************************
  * @file           : limiter.h
  * @brief          : Interface for the audio limiter module
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Audio limiter interface for the DSP Audio Crossover system
  * Provides protection against signal clipping
  *
  ******************************************************************************
  */

#ifndef __LIMITER_H
#define __LIMITER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "limiter_types.h"
#include "audio_config.h"

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initialize limiter module for a channel
 * @param outputChannel Output channel index
 * @param thresholdDb Default threshold in dB (typically -0.1 to -6.0 dB)
 * @param attackMs Default attack time in milliseconds (typically 0.1 to 10ms)
 * @param releaseMs Default release time in milliseconds (typically 10 to 500ms)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Limiter_Init(uint8_t outputChannel, float_t thresholdDb, float_t attackMs, float_t releaseMs);

/**
 * @brief Process audio through limiter
 * @param outputChannel Output channel index
 * @param pAudioBuffer Pointer to audio buffer structure
 * @retval None
 */
void DSP_Limiter_Process(uint8_t outputChannel, AudioBuffer_TypeDef *pAudioBuffer);

/**
 * @brief Set limiter threshold
 * @param outputChannel Output channel index
 * @param thresholdDb Threshold in dB (typically -0.1 to -6.0 dB)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Limiter_SetThreshold(uint8_t outputChannel, float_t thresholdDb);

/**
 * @brief Set limiter attack time
 * @param outputChannel Output channel index
 * @param attackMs Attack time in milliseconds (0.1ms to 10ms)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Limiter_SetAttack(uint8_t outputChannel, float_t attackMs);

/**
 * @brief Set limiter release time
 * @param outputChannel Output channel index
 * @param releaseMs Release time in milliseconds (10ms to 500ms)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Limiter_SetRelease(uint8_t outputChannel, float_t releaseMs);

/**
 * @brief Set limiter makeup gain
 * @param outputChannel Output channel index
 * @param makeupGainDb Makeup gain in dB (0 to 6 dB)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Limiter_SetMakeupGain(uint8_t outputChannel, float_t makeupGainDb);

/**
 * @brief Enable or disable lookahead (if CPU allows)
 * @param outputChannel Output channel index
 * @param enableLookahead Enable flag (1 = enabled)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Limiter_EnableLookahead(uint8_t outputChannel, uint8_t enableLookahead);

/**
 * @brief Enable or disable limiter
 * @param outputChannel Output channel index
 * @param enableFlag Enable flag (1 = enabled)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Limiter_Enable(uint8_t outputChannel, uint8_t enableFlag);

/**
 * @brief Bypass limiter (pass through)
 * @param outputChannel Output channel index
 * @param bypassFlag Bypass flag (1 = bypassed)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Limiter_Bypass(uint8_t outputChannel, uint8_t bypassFlag);

/**
 * @brief Get current gain reduction amount
 * @param outputChannel Output channel index
 * @retval Gain reduction in dB (negative value)
 */
float_t DSP_Limiter_GetGainReduction(uint8_t outputChannel);

/**
 * @brief Reset limiter state
 * @param outputChannel Output channel index
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Limiter_Reset(uint8_t outputChannel);

/**
 * @brief Link channels for stereo operation
 * @param outputChannel1 First output channel index
 * @param outputChannel2 Second output channel index
 * @param linkFlag Link flag (1 = linked)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Limiter_LinkChannels(uint8_t outputChannel1, uint8_t outputChannel2, uint8_t linkFlag);

/**
 * @brief Get limiter configuration for preset storage
 * @param outputChannel Output channel index
 * @param pConfig Pointer to limiter configuration structure
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Limiter_GetConfig(uint8_t outputChannel, LimiterParams_TypeDef *pConfig);

/**
 * @brief Set limiter configuration from preset
 * @param outputChannel Output channel index
 * @param pConfig Pointer to limiter configuration structure
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Limiter_SetConfig(uint8_t outputChannel, LimiterParams_TypeDef *pConfig);

#ifdef __cplusplus
}
#endif

#endif /* __LIMITER_H */