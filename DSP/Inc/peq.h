/**
  ******************************************************************************
  * @file           : peq.h
  * @brief          : Interface for parametric equalizer functionality
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Interface for parametric equalizer (PEQ) functionality
  * Provides functions for configuring and processing audio through EQ bands
  *
  ******************************************************************************
  */

#ifndef PEQ_H
#define PEQ_H

#include <stdint.h>
#include "audio_config.h"
#include "peq_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the PEQ module
 * @param sampleRate Current audio sample rate
 * @return HAL status
 */
HAL_StatusTypeDef DSP_EQ_Init(float sampleRate);

/**
 * @brief Process a single sample through the parametric EQ for specific channel
 * @param channelIndex Output channel index
 * @param sample Input sample
 * @return Processed sample
 */
float DSP_EQ_ProcessSample(uint8_t channelIndex, float sample);

/**
 * @brief Process a buffer of samples through the parametric EQ
 * @param channelIndex Output channel index
 * @param buffer Audio buffer containing input and output samples
 * @return HAL status
 */
HAL_StatusTypeDef DSP_EQ_Process(uint8_t channelIndex, AudioBuffer_TypeDef* buffer);

/**
 * @brief Enable or disable the parametric EQ for specific channel
 * @param channelIndex Output channel index
 * @param state Enable state (0: disable, 1: enable)
 * @return HAL status
 */
HAL_StatusTypeDef DSP_EQ_SetEnabled(uint8_t channelIndex, uint8_t state);

/**
 * @brief Get the enabled state of the parametric EQ for specific channel
 * @param channelIndex Output channel index
 * @return Enabled state (0: disabled, 1: enabled)
 */
uint8_t DSP_EQ_GetEnabled(uint8_t channelIndex);

/**
 * @brief Configure a specific EQ band
 * @param channelIndex Output channel index
 * @param bandIndex Band index (0 to MAX_PEQ_BANDS_PER_CHANNEL-1)
 * @param filterType Type of filter (PEQ_FilterType_t)
 * @param frequency Center/corner frequency in Hz (20-20000)
 * @param gain Gain in dB (-12 to +12)
 * @param q Q-factor (0.1 to 10.0)
 * @return HAL status
 */
HAL_StatusTypeDef DSP_EQ_ConfigureBand(uint8_t channelIndex, uint8_t bandIndex, 
                                      PEQ_FilterType_t filterType, float frequency, 
                                      float gain, float q);

/**
 * @brief Enable or disable a specific EQ band
 * @param channelIndex Output channel index
 * @param bandIndex Band index (0 to MAX_PEQ_BANDS_PER_CHANNEL-1)
 * @param state Enable state (0: disable, 1: enable)
 * @return HAL status
 */
HAL_StatusTypeDef DSP_EQ_SetBandEnabled(uint8_t channelIndex, uint8_t bandIndex, uint8_t state);

/**
 * @brief Get EQ band configuration for specific channel and band
 * @param channelIndex Output channel index
 * @param bandIndex Band index (0 to MAX_PEQ_BANDS_PER_CHANNEL-1)
 * @param [out] filterType Type of filter 
 * @param [out] frequency Center/corner frequency in Hz
 * @param [out] gain Gain in dB
 * @param [out] q Q-factor
 * @param [out] enabled Band enabled state
 * @return HAL status
 */
HAL_StatusTypeDef DSP_EQ_GetBandConfig(uint8_t channelIndex, uint8_t bandIndex,
                                      PEQ_FilterType_t* filterType, float* frequency, 
                                      float* gain, float* q, uint8_t* enabled);

/**
 * @brief Set channel pre-gain (gain applied before EQ processing)
 * @param channelIndex Output channel index
 * @param gain Pre-gain in dB
 * @return HAL status
 */
HAL_StatusTypeDef DSP_EQ_SetPreGain(uint8_t channelIndex, float gain);

/**
 * @brief Reset all EQ bands for specific channel to pass-through state
 * @param channelIndex Output channel index
 * @return HAL status
 */
HAL_StatusTypeDef DSP_EQ_Reset(uint8_t channelIndex);

/**
 * @brief Update sample rate for EQ processing (recalculates coefficients)
 * @param sampleRate New sample rate in Hz
 * @return HAL status
 */
HAL_StatusTypeDef DSP_EQ_UpdateSampleRate(float sampleRate);

/**
 * @brief Get current EQ configuration for all channels
 * @return Pointer to the PEQ configuration structure
 */
const PEQ_Config_t* DSP_EQ_GetConfig(void);

/**
 * @brief Set EQ configuration from saved preset
 * @param config Pointer to configuration structure with preset values
 * @return HAL status
 */
HAL_StatusTypeDef DSP_EQ_SetConfig(const PEQ_Config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* PEQ_H */