/**
  ******************************************************************************
  * @file           : compressor.h
  * @brief          : Interface for dynamics compressor functionality
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Interface for dynamic range compressor functionality
  * Provides functions for configuring and processing audio compression
  *
  ******************************************************************************
  */

#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <stdint.h>
#include "audio_config.h"
#include "compressor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the compressor module
 * @param sampleRate Current audio sample rate
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_Init(float sampleRate);

/**
 * @brief Process a single sample through the compressor for specific channel
 * @param channelIndex Output channel index
 * @param sample Input sample
 * @return Processed sample
 */
float DSP_Compressor_ProcessSample(uint8_t channelIndex, float sample);

/**
 * @brief Process a buffer of samples through the compressor
 * @param channelIndex Output channel index
 * @param buffer Audio buffer containing input and output samples
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_Process(uint8_t channelIndex, AudioBuffer_TypeDef* buffer);

/**
 * @brief Enable or disable the compressor for specific channel
 * @param channelIndex Output channel index
 * @param state Enable state (0: disable, 1: enable)
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_SetEnabled(uint8_t channelIndex, uint8_t state);

/**
 * @brief Get the enabled state of the compressor for specific channel
 * @param channelIndex Output channel index
 * @return Enabled state (0: disabled, 1: enabled)
 */
uint8_t DSP_Compressor_GetEnabled(uint8_t channelIndex);

/**
 * @brief Set compressor threshold level
 * @param channelIndex Output channel index
 * @param thresholdDb Threshold level in dB (-60 to 0)
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_SetThreshold(uint8_t channelIndex, float thresholdDb);

/**
 * @brief Set compressor ratio
 * @param channelIndex Output channel index
 * @param ratio Compression ratio (1.0 to 20.0)
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_SetRatio(uint8_t channelIndex, float ratio);

/**
 * @brief Set compressor attack time
 * @param channelIndex Output channel index
 * @param attackMs Attack time in milliseconds (0.1 to 100)
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_SetAttack(uint8_t channelIndex, float attackMs);

/**
 * @brief Set compressor release time
 * @param channelIndex Output channel index
 * @param releaseMs Release time in milliseconds (10 to 1000)
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_SetRelease(uint8_t channelIndex, float releaseMs);

/**
 * @brief Set compressor makeup gain
 * @param channelIndex Output channel index
 * @param gainDb Makeup gain in dB (0 to 24)
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_SetMakeupGain(uint8_t channelIndex, float gainDb);

/**
 * @brief Set compressor knee type
 * @param channelIndex Output channel index
 * @param kneeType Knee type (COMP_KNEE_HARD or COMP_KNEE_SOFT)
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_SetKneeType(uint8_t channelIndex, Compressor_KneeType_t kneeType);

/**
 * @brief Set compressor detection mode
 * @param channelIndex Output channel index
 * @param detectionMode Detection mode (COMP_DETECTION_RMS or COMP_DETECTION_PEAK)
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_SetDetectionMode(uint8_t channelIndex, Compressor_DetectionMode_t detectionMode);

/**
 * @brief Get current compressor configuration for specific channel
 * @param channelIndex Output channel index
 * @param [out] threshold Threshold level in dB
 * @param [out] ratio Compression ratio 
 * @param [out] attackMs Attack time in milliseconds
 * @param [out] releaseMs Release time in milliseconds
 * @param [out] makeupGain Makeup gain in dB
 * @param [out] kneeType Knee type
 * @param [out] detectionMode Detection mode
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_GetConfig(uint8_t channelIndex, float* threshold, 
                                           float* ratio, float* attackMs, 
                                           float* releaseMs, float* makeupGain,
                                           Compressor_KneeType_t* kneeType,
                                           Compressor_DetectionMode_t* detectionMode);

/**
 * @brief Get current gain reduction for specific channel
 * @param channelIndex Output channel index
 * @return Current gain reduction in dB (negative value)
 */
float DSP_Compressor_GetGainReduction(uint8_t channelIndex);

/**
 * @brief Reset compressor to default state for specific channel
 * @param channelIndex Output channel index
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_Reset(uint8_t channelIndex);

/**
 * @brief Update sample rate for compressor processing (recalculates coefficients)
 * @param sampleRate New sample rate in Hz
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_UpdateSampleRate(float sampleRate);

/**
 * @brief Get current compressor configuration for all channels
 * @return Pointer to the compressor configuration structure
 */
const Compressor_Config_t* DSP_Compressor_GetAllConfig(void);

/**
 * @brief Set compressor configuration from saved preset
 * @param config Pointer to configuration structure with preset values
 * @return HAL status
 */
HAL_StatusTypeDef DSP_Compressor_SetAllConfig(const Compressor_Config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* COMPRESSOR_H */