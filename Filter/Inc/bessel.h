/**
  ******************************************************************************
  * @file           : bessel.h
  * @brief          : Bessel filter implementation for audio crossover
  * @author         : asepsupriatna90
  ******************************************************************************
  * @attention
  *
  * Bessel filter module for the DSP Audio Crossover system
  * Provides optimal phase response with linear phase in the passband,
  * minimizing signal distortion due to phase shifts.
  *
  ******************************************************************************
  */

#ifndef BESSEL_H
#define BESSEL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "filter_types.h"
#include "biquad.h"
#include <stdint.h>

/* Defines -------------------------------------------------------------------*/
#define MAX_BESSEL_ORDER    8       /* Maximum supported filter order */

/* Types ---------------------------------------------------------------------*/
/**
 * @brief Bessel filter design parameters
 */
typedef struct {
    float cutoffFrequency;          /* Cutoff frequency in Hz */
    uint8_t order;                  /* Filter order (1-8) */
    FilterType_t type;              /* LOW_PASS or HIGH_PASS */
    float sampleRate;               /* Sample rate in Hz */
} BesselParams_t;

/**
 * @brief Bessel filter state
 */
typedef struct {
    BesselParams_t params;          /* Filter parameters */
    uint8_t numBiquads;             /* Number of biquad sections needed */
    BiquadFilter_t biquads[MAX_BESSEL_ORDER/2 + 1]; /* Array of biquad filters */
} BesselFilter_t;

/* Function prototypes -------------------------------------------------------*/
/**
 * @brief Initialize Bessel filter with given parameters
 * @param filter Pointer to filter structure
 * @param params Filter design parameters
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t Bessel_Init(BesselFilter_t *filter, const BesselParams_t *params);

/**
 * @brief Reset filter state (clear delay lines)
 * @param filter Pointer to filter structure
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t Bessel_Reset(BesselFilter_t *filter);

/**
 * @brief Process a single sample through the filter
 * @param filter Pointer to filter structure
 * @param input Input sample
 * @return Filtered output sample
 */
float Bessel_ProcessSample(BesselFilter_t *filter, float input);

/**
 * @brief Process a block of samples through the filter
 * @param filter Pointer to filter structure
 * @param input Pointer to input buffer
 * @param output Pointer to output buffer
 * @param length Number of samples to process
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t Bessel_ProcessBlock(BesselFilter_t *filter, 
                                const float *input, 
                                float *output, 
                                uint32_t length);

/**
 * @brief Update the filter cutoff frequency
 * @param filter Pointer to filter structure
 * @param cutoffFrequency New cutoff frequency in Hz
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t Bessel_UpdateFrequency(BesselFilter_t *filter, float cutoffFrequency);

/**
 * @brief Get the filter's magnitude response at a specific frequency
 * @param filter Pointer to filter structure
 * @param frequency The frequency to evaluate (in Hz)
 * @return Magnitude response (linear scale)
 */
float Bessel_GetMagnitudeResponse(const BesselFilter_t *filter, float frequency);

/**
 * @brief Get the filter's phase response at a specific frequency
 * @param filter Pointer to filter structure
 * @param frequency The frequency to evaluate (in Hz)
 * @return Phase response (in radians)
 */
float Bessel_GetPhaseResponse(const BesselFilter_t *filter, float frequency);

/**
 * @brief Calculate the group delay at a specific frequency
 * @param filter Pointer to filter structure
 * @param frequency The frequency to evaluate (in Hz)
 * @return Group delay (in seconds)
 */
float Bessel_GetGroupDelay(const BesselFilter_t *filter, float frequency);

/**
 * @brief Calculate Bessel filter coefficients
 * @note This is typically an internal function but exposed for testing
 * @param params Bessel parameters
 * @param coeffs Array to store calculated coefficients 
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t Bessel_CalculateCoefficients(const BesselParams_t *params, 
                                         BiquadCoefficients_t *coeffs);

/**
 * @brief Create a Bessel crossover with matched phase between low and high pass
 * @param lpFilter Pointer to low-pass filter structure
 * @param hpFilter Pointer to high-pass filter structure
 * @param crossoverFreq Crossover frequency in Hz
 * @param order Filter order
 * @param sampleRate Sample rate in Hz
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t Bessel_CreateCrossover(BesselFilter_t *lpFilter, 
                                   BesselFilter_t *hpFilter,
                                   float crossoverFreq,
                                   uint8_t order,
                                   float sampleRate);

#ifdef __cplusplus
}
#endif

#endif /* BESSEL_H */