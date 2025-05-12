/**
  ******************************************************************************
  * @file           : delay.h
  * @brief          : Interface for the audio delay module
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Audio delay interface for the DSP Audio Crossover system
  * Used for time alignment between drivers
  *
  ******************************************************************************
  */

#ifndef __DELAY_H
#define __DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "delay_types.h"
#include "audio_config.h"

/* Constants -----------------------------------------------------------------*/
#define SPEED_OF_SOUND_CM_S   34300    /* Speed of sound in cm/s at 20°C */
#define SPEED_OF_SOUND_IN_S   13504    /* Speed of sound in inches/s at 20°C */

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initialize delay module for a channel
 * @param outputChannel Output channel index
 * @param initialDelayMs Initial delay in milliseconds (0 to MAX_DELAY_MS)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_Init(uint8_t outputChannel, float_t initialDelayMs);

/**
 * @brief Process audio through delay
 * @param outputChannel Output channel index
 * @param pAudioBuffer Pointer to audio buffer structure
 * @retval None
 */
void DSP_Delay_Process(uint8_t outputChannel, AudioBuffer_TypeDef *pAudioBuffer);

/**
 * @brief Set delay time in milliseconds
 * @param outputChannel Output channel index
 * @param delayMs Delay time in milliseconds (0 to MAX_DELAY_MS)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_SetTimeMs(uint8_t outputChannel, float_t delayMs);

/**
 * @brief Set delay time in samples
 * @param outputChannel Output channel index
 * @param delaySamples Delay time in samples
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_SetTimeSamples(uint8_t outputChannel, uint32_t delaySamples);

/**
 * @brief Set delay as distance in centimeters (for time alignment)
 * @param outputChannel Output channel index
 * @param distanceCm Distance in centimeters
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_SetDistanceCm(uint8_t outputChannel, float_t distanceCm);

/**
 * @brief Set delay as distance in inches (for time alignment)
 * @param outputChannel Output channel index
 * @param distanceInches Distance in inches
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_SetDistanceInches(uint8_t outputChannel, float_t distanceInches);

/**
 * @brief Set preferred display unit for user interface
 * @param outputChannel Output channel index
 * @param unit Preferred unit (ms, samples, cm, inches)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_SetDisplayUnit(uint8_t outputChannel, DelayUnit_TypeDef unit);

/**
 * @brief Enable or disable delay
 * @param outputChannel Output channel index
 * @param enableFlag Enable flag (1 = enabled)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_Enable(uint8_t outputChannel, uint8_t enableFlag);

/**
 * @brief Bypass delay (pass through)
 * @param outputChannel Output channel index
 * @param bypassFlag Bypass flag (1 = bypassed)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_Bypass(uint8_t outputChannel, uint8_t bypassFlag);

/**
 * @brief Set phase inversion (polarity)
 * @param outputChannel Output channel index
 * @param invertFlag Invert flag (0 = normal, 1 = inverted)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_SetPolarity(uint8_t outputChannel, uint8_t invertFlag);

/**
 * @brief Set mix percentage (wet/dry balance)
 * @param outputChannel Output channel index
 * @param mixPct Mix percentage (0 to 100, typically 100 for crossover)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_SetMix(uint8_t outputChannel, float_t mixPct);

/**
 * @brief Set feedback percentage (typically 0 for crossover)
 * @param outputChannel Output channel index
 * @param feedbackPct Feedback percentage (0 to 100)
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_SetFeedback(uint8_t outputChannel, float_t feedbackPct);

/**
 * @brief Get current delay in milliseconds
 * @param outputChannel Output channel index
 * @retval Current delay in milliseconds
 */
float_t DSP_Delay_GetTimeMs(uint8_t outputChannel);

/**
 * @brief Get current delay in samples
 * @param outputChannel Output channel index
 * @retval Current delay in samples
 */
uint32_t DSP_Delay_GetTimeSamples(uint8_t outputChannel);

/**
 * @brief Get current delay as distance in centimeters
 * @param outputChannel Output channel index
 * @retval Current distance in centimeters
 */
float_t DSP_Delay_GetDistanceCm(uint8_t outputChannel);

/**
 * @brief Get current delay as distance in inches
 * @param outputChannel Output channel index
 * @retval Current distance in inches
 */
float_t DSP_Delay_GetDistanceInches(uint8_t outputChannel);

/**
 * @brief Reset delay state (clear buffer)
 * @param outputChannel Output channel index
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_Reset(uint8_t outputChannel);

/**
 * @brief Convert between delay units
 * @param value Value to convert
 * @param fromUnit Source unit
 * @param toUnit Target unit
 * @retval Converted value
 */
float_t DSP_Delay_ConvertUnit(float_t value, DelayUnit_TypeDef fromUnit, DelayUnit_TypeDef toUnit);

/**
 * @brief Get delay configuration for preset storage
 * @param outputChannel Output channel index
 * @param pConfig Pointer to delay configuration structure
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_GetConfig(uint8_t outputChannel, DelayParams_TypeDef *pConfig);

/**
 * @brief Set delay configuration from preset
 * @param outputChannel Output channel index
 * @param pConfig Pointer to delay configuration structure
 * @retval HAL status
 */
HAL_StatusTypeDef DSP_Delay_SetConfig(uint8_t outputChannel, DelayParams_TypeDef *pConfig);

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_H */