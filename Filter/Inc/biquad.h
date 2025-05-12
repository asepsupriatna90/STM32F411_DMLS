/**
  ******************************************************************************
  * @file           : biquad.h
  * @brief          : Header for biquad filter implementation
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Biquad filter library for STM32F411 Audio DSP Crossover
  * Implements various IIR filter types (low pass, high pass, band pass,
  * notch, peak, low shelf, high shelf) using cascaded biquad filters
  *
  ******************************************************************************
  */

#ifndef __BIQUAD_H
#define __BIQUAD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "filter_types.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

/**
 * @brief  Biquad filter types enumeration
 */
typedef enum {
  BIQUAD_TYPE_LPF = 0,      /*!< Low Pass Filter */
  BIQUAD_TYPE_HPF,          /*!< High Pass Filter */
  BIQUAD_TYPE_BPF,          /*!< Band Pass Filter */
  BIQUAD_TYPE_NOTCH,        /*!< Notch Filter */
  BIQUAD_TYPE_PEAK,         /*!< Peaking EQ Filter */
  BIQUAD_TYPE_LOWSHELF,     /*!< Low Shelf Filter */
  BIQUAD_TYPE_HIGHSHELF     /*!< High Shelf Filter */
} BiquadType_t;

/**
 * @brief  Biquad filter state structure
 */
typedef struct {
  float b0;    /*!< Feedforward coefficient b0 */
  float b1;    /*!< Feedforward coefficient b1 */
  float b2;    /*!< Feedforward coefficient b2 */
  float a1;    /*!< Feedback coefficient a1 */
  float a2;    /*!< Feedback coefficient a2 */
  float x1;    /*!< Previous input sample x[n-1] */
  float x2;    /*!< Previous input sample x[n-2] */
  float y1;    /*!< Previous output sample y[n-1] */
  float y2;    /*!< Previous output sample y[n-2] */
} BiquadState_t;

/**
 * @brief  Biquad filter configuration structure
 */
typedef struct {
  BiquadType_t type;        /*!< Filter type */
  float frequency;          /*!< Center/cutoff frequency in Hz */
  float Q;                  /*!< Quality factor */
  float gainDB;             /*!< Gain in dB (for peaking/shelf filters) */
  float sampleRate;         /*!< Sample rate in Hz */
} BiquadConfig_t;

/* Function prototypes -------------------------------------------------------*/

/**
 * @brief  Initialize a biquad filter with the specified configuration
 * @param  state: Pointer to the biquad filter state structure
 * @param  config: Pointer to the biquad filter configuration
 * @retval None
 */
void Biquad_Init(BiquadState_t *state, const BiquadConfig_t *config);

/**
 * @brief  Calculate biquad filter coefficients for the specified configuration
 * @param  state: Pointer to the biquad filter state structure
 * @param  config: Pointer to the biquad filter configuration
 * @retval None
 */
void Biquad_CalculateCoefficients(BiquadState_t *state, const BiquadConfig_t *config);

/**
 * @brief  Reset biquad filter state (clear delay lines)
 * @param  state: Pointer to the biquad filter state structure
 * @retval None
 */
void Biquad_Reset(BiquadState_t *state);

/**
 * @brief  Process a single sample through the biquad filter
 * @param  state: Pointer to the biquad filter state structure
 * @param  input: Input sample
 * @retval float: Filtered output sample
 */
float Biquad_ProcessSample(BiquadState_t *state, float input);

/**
 * @brief  Process a block of samples through the biquad filter
 * @param  state: Pointer to the biquad filter state structure
 * @param  input: Pointer to input buffer
 * @param  output: Pointer to output buffer
 * @param  blockSize: Number of samples to process
 * @retval None
 */
void Biquad_ProcessBlock(BiquadState_t *state, const float *input, float *output, uint32_t blockSize);

/**
 * @brief  Create a chain of biquad filters for higher order filters
 * @param  chain: Pointer to array of biquad states
 * @param  config: Base configuration for all filters in chain
 * @param  order: Filter order (determines number of biquads required)
 * @retval uint8_t: Number of biquads created in the chain
 */
uint8_t Biquad_CreateFilterChain(BiquadState_t *chain, const BiquadConfig_t *config, uint8_t order);

/**
 * @brief  Process a sample through a chain of biquad filters
 * @param  chain: Pointer to array of biquad states
 * @param  numStages: Number of biquad stages in the chain
 * @param  input: Input sample
 * @retval float: Filtered output sample
 */
float Biquad_ProcessChain(BiquadState_t *chain, uint8_t numStages, float input);

/**
 * @brief  Get the frequency response of a biquad filter at a specific frequency
 * @param  state: Pointer to the biquad filter state structure
 * @param  frequency: Frequency at which to calculate response (Hz)
 * @param  sampleRate: Sample rate in Hz
 * @retval float: Magnitude response in dB
 */
float Biquad_GetFrequencyResponse(const BiquadState_t *state, float frequency, float sampleRate);

/**
 * @brief  Get the phase response of a biquad filter at a specific frequency
 * @param  state: Pointer to the biquad filter state structure
 * @param  frequency: Frequency at which to calculate response (Hz)
 * @param  sampleRate: Sample rate in Hz
 * @retval float: Phase response in radians
 */
float Biquad_GetPhaseResponse(const BiquadState_t *state, float frequency, float sampleRate);

#ifdef __cplusplus
}
#endif

#endif /* __BIQUAD_H */