/**
  ******************************************************************************
  * @file           : crossover.h
  * @brief          : Crossover filter API
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * API for audio crossover filter implementation
  * Part of STM32F411 Audio DSP Crossover
  *
  ******************************************************************************
  */

#ifndef CROSSOVER_H
#define CROSSOVER_H

#include "crossover_types.h"
#include "audio_config.h"

/**
  * @brief  Initialize the crossover filter module
  * @param  sampleRate: Audio sample rate in Hz
  * @retval None
  */
void Crossover_Init(uint32_t sampleRate);

/**
  * @brief  Reset the crossover filter state
  * @param  channel: Channel index
  * @retval None
  */
void Crossover_Reset(uint8_t channel);

/**
  * @brief  Reset all crossover filter states
  * @retval None
  */
void Crossover_ResetAll(void);

/**
  * @brief  Configure the crossover parameters
  * @param  channel: Channel index
  * @param  params: Pointer to crossover parameters
  * @retval Status: 0 if OK, non-zero if error
  */
uint8_t Crossover_Configure(uint8_t channel, CrossoverParams_t *params);

/**
  * @brief  Set a specific crossover band parameter
  * @param  channel: Channel index
  * @param  band: Band index (0-3)
  * @param  type: Band type (LP, HP, BP, Bypass)
  * @param  freq: Crossover frequency (Hz)
  * @param  filterType: Filter type (Butterworth, LR, Bessel)
  * @param  slope: Filter slope (6dB to 48dB/oct)
  * @retval Status: 0 if OK, non-zero if error
  */
uint8_t Crossover_SetBand(uint8_t channel, uint8_t band, CrossoverBandType_t type, 
                          float freq, CrossoverFilterType_t filterType, CrossoverSlope_t slope);

/**
  * @brief  Set band pass parameters
  * @param  channel: Channel index
  * @param  band: Band index (0-3)
  * @param  freqLow: Low crossover frequency (Hz)
  * @param  freqHigh: High crossover frequency (Hz)
  * @param  filterType: Filter type (Butterworth, LR, Bessel)
  * @param  slope: Filter slope (6dB to 48dB/oct)
  * @retval Status: 0 if OK, non-zero if error
  */
uint8_t Crossover_SetBandPass(uint8_t channel, uint8_t band, 
                             float freqLow, float freqHigh,
                             CrossoverFilterType_t filterType, CrossoverSlope_t slope);

/**
  * @brief  Set band gain in dB
  * @param  channel: Channel index
  * @param  band: Band index (0-3)
  * @param  gain: Gain in dB
  * @retval Status: 0 if OK, non-zero if error
  */
uint8_t Crossover_SetBandGain(uint8_t channel, uint8_t band, float gain);

/**
  * @brief  Enable/disable a specific crossover band
  * @param  channel: Channel index
  * @param  band: Band index (0-3)
  * @param  enable: 1 to enable, 0 to disable
  * @retval Status: 0 if OK, non-zero if error
  */
uint8_t Crossover_EnableBand(uint8_t channel, uint8_t band, uint8_t enable);

/**
  * @brief  Get the current crossover parameters
  * @param  channel: Channel index
  * @param  params: Pointer to store parameters
  * @retval Status: 0 if OK, non-zero if error
  */
uint8_t Crossover_GetParams(uint8_t channel, CrossoverParams_t *params);

/**
  * @brief  Set crossover mode (2-way, 3-way, etc.)
  * @param  channel: Channel index
  * @param  mode: Crossover mode
  * @retval Status: 0 if OK, non-zero if error
  */
uint8_t Crossover_SetMode(uint8_t channel, CrossoverMode_t mode);

/**
  * @brief  Enable/disable the crossover processing
  * @param  channel: Channel index
  * @param  enable: 1 to enable, 0 to disable (bypass)
  * @retval Status: 0 if OK, non-zero if error
  */
uint8_t Crossover_Enable(uint8_t channel, uint8_t enable);

/**
  * @brief  Link/unlink stereo channels (for synchronizing parameters)
  * @param  link: 1 to link, 0 to unlink
  * @retval None
  */
void Crossover_LinkChannels(uint8_t link);

/**
  * @brief  Apply 2-way crossover preset
  * @param  channel: Channel index
  * @param  frequency: Crossover frequency (Hz)
  * @param  filterType: Filter type (Butterworth, LR, Bessel)
  * @param  slope: Filter slope (6dB to 48dB/oct)
  * @retval Status: 0 if OK, non-zero if error
  */
uint8_t Crossover_Apply2Way(uint8_t channel, float frequency, 
                           CrossoverFilterType_t filterType, CrossoverSlope_t slope);

/**
  * @brief  Apply 3-way crossover preset
  * @param  channel: Channel index
  * @param  freqLow: Low crossover frequency (Hz)
  * @param  freqHigh: High crossover frequency (Hz)
  * @param  filterType: Filter type (Butterworth, LR, Bessel)
  * @param  slope: Filter slope (6dB to 48dB/oct)
  * @retval Status: 0 if OK, non-zero if error
  */
uint8_t Crossover_Apply3Way(uint8_t channel, float freqLow, float freqHigh,
                           CrossoverFilterType_t filterType, CrossoverSlope_t slope);

/**
  * @brief  Apply subwoofer + fullrange preset
  * @param  channel: Channel index
  * @param  frequency: Crossover frequency (Hz)
  * @param  filterType: Filter type (Butterworth, LR, Bessel)
  * @param  slope: Filter slope (6dB to 48dB/oct)
  * @retval Status: 0 if OK, non-zero if error
  */
uint8_t Crossover_ApplySubPlusFull(uint8_t channel, float frequency,
                                  CrossoverFilterType_t filterType, CrossoverSlope_t slope);

/**
  * @brief  Process a single sample through the crossover filters
  * @param  channel: Channel index
  * @param  input: Input sample
  * @retval Output sample (mixed output of enabled bands)
  */
float Crossover_ProcessSample(uint8_t channel, float input);

/**
  * @brief  Process multiple samples through the crossover filters
  * @param  channel: Channel index
  * @param  input: Pointer to input samples
  * @param  output: Pointer to output samples
  * @param  size: Number of samples to process
  * @retval None
  */
void Crossover_ProcessBuffer(uint8_t channel, float *input, float *output, uint16_t size);

/**
  * @brief  Get a specific band output (for visualization or multi-output use)
  * @param  channel: Channel index
  * @param  band: Band index (0-3)
  * @retval Last processed sample for the specified band
  */
float Crossover_GetBandOutput(uint8_t channel, uint8_t band);

/**
  * @brief  Get text description of crossover filter type
  * @param  type: Filter type
  * @retval Pointer to constant string describing the filter type
  */
const char* Crossover_GetFilterTypeText(CrossoverFilterType_t type);

/**
  * @brief  Get text description of crossover slope
  * @param  slope: Filter slope
  * @retval Pointer to constant string describing the slope
  */
const char* Crossover_GetSlopeText(CrossoverSlope_t slope);

/**
  * @brief  Get text description of crossover mode
  * @param  mode: Crossover mode
  * @retval Pointer to constant string describing the mode
  */
const char* Crossover_GetModeText(CrossoverMode_t mode);

/**
  * @brief  Check if frequency is in valid range
  * @param  frequency: Frequency to check (Hz)
  * @retval 1 if valid, 0 if invalid
  */
uint8_t Crossover_IsValidFrequency(float frequency);

/**
  * @brief  Calculate filter order from slope
  * @param  slope: Crossover slope
  * @retval Filter order
  */
uint8_t Crossover_GetFilterOrder(CrossoverSlope_t slope);

#endif /* CROSSOVER_H */