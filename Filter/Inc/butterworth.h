/**
  ******************************************************************************
  * @file           : butterworth.h
  * @brief          : Butterworth filter implementation for audio crossover
  * @author         : asepsupriatna90
  ******************************************************************************
  * @attention
  *
  * Butterworth filter module for the DSP Audio Crossover system
  * Provides maximally flat amplitude response in the passband.
  *
  ******************************************************************************
  */

#ifndef BUTTERWORTH_H
#define BUTTERWORTH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "filter_types.h"
#include "biquad.h"
#include <stdint.h>

/* Defines -------------------------------------------------------------------*/
#define MAX_BUTTERWORTH_ORDER    8   /* Maximum supported filter order */

/* Types ---------------------------------------------------------------------*/
/**
 * @brief Butterworth filter design parameters
 */
typedef struct {
    float cutoffFrequency;          /* Cutoff frequency in Hz */
    uint8_t order;                  /* Filter order (1-8) */
    FilterType_t type;              /* LOW_PASS or HIGH_PASS */
    float sampleRate;               /* Sample rate in Hz */
} ButterworthParams_t;

/**
 * @brief Butterworth filter state
 */
typedef struct {
    ButterworthParams_t params;     /* Filter parameters */
    uint8_t numBiquads;             /* Number of biquad sections needed */
    BiquadFilter_t biquads[MAX_BUTTERWORTH_ORDER/2 + 1]; /* Array of biquad filters */
} ButterworthFilter_t;

/* Function prototypes -------------------------------------------------------*/
/**
 * @brief Initialize Butterworth filter with given parameters
 * @param filter Pointer to filter structure
 * @param params Filter design parameters
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t Butterworth_Init(ButterworthFilter_t *filter, const ButterworthParams_t *params);

/**
 * @brief Reset filter state (clear delay lines)
 * @param filter Pointer to filter structure
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t Butterworth_Reset(ButterworthFilter_t *filter);

/**
 * @brief Process a single sample through the filter
 * @param filter Pointer to filter structure
 * @param input Input sample
 * @return Filtered output sample
 */
float Butterworth_ProcessSample(ButterworthFilter_t *filter, float input);

/**
 * @brief Process a block of samples through the filter
 * @param filter Pointer to filter structure
 * @param input Pointer to input buffer
 * @param output Pointer to output buffer
 * @param length Number of samples to process
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t Butterworth_ProcessBlock(ButterworthFilter_t *filter, 
                                     const float *input, 
                                     float *output, 
                                     uint32_t length);

/**
 * @brief Update the filter cutoff frequency
 * @param filter Pointer to filter structure
 * @param cutoffFrequency New cutoff frequency in Hz
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t Butterworth_UpdateFrequency(ButterworthFilter_t *filter, float cutoffFrequency);

/**
 * @brief Get the filter's magnitude response at a specific frequency
 * @param filter Pointer to filter structure
 * @param frequency The frequency to evaluate (in Hz)
 * @return Magnitude response (linear scale)
 */
float Butterworth_GetMagnitudeResponse(const ButterworthFilter_t *filter, float frequency);

/**
 * @brief Calculate Butterworth filter coefficients
 * @note This is typically an internal function but exposed for testing
 * @param params Butterworth parameters
 * @param coeffs Array to store calculated coefficients 
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t Butterworth_CalculateCoefficients(const ButterworthParams_t *params, 
                                              BiquadCoefficients_t *coeffs);

#ifdef __cplusplus
}
#endif

#endif /* BUTTERWORTH_H */