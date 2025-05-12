/**
  ******************************************************************************
  * @file           : linkwitz_riley.h
  * @brief          : Linkwitz-Riley filter implementation for audio crossover
  * @author         : asepsupriatna90
  ******************************************************************************
  * @attention
  *
  * Linkwitz-Riley filter module for the DSP Audio Crossover system
  * Provides a -6dB amplitude response at the crossover frequency with 
  * a flat summed response when low-pass and high-pass outputs are combined.
  *
  ******************************************************************************
  */

#ifndef LINKWITZ_RILEY_H
#define LINKWITZ_RILEY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "filter_types.h"
#include "butterworth.h"
#include "biquad.h"
#include <stdint.h>

/* Defines -------------------------------------------------------------------*/
#define MAX_LR_ORDER       48      /* Maximum supported filter order */
#define MAX_LR_BIQUADS     (MAX_LR_ORDER/4 + 1)  /* Max biquads needed */

/* Types ---------------------------------------------------------------------*/
/**
 * @brief Linkwitz-Riley filter design parameters
 */
typedef struct {
    float cutoffFrequency;          /* Crossover frequency in Hz */
    uint8_t order;                  /* Filter order (must be even: 12, 24, 36, 48) */
    FilterType_t type;              /* LOW_PASS or HIGH_PASS */
    float sampleRate;               /* Sample rate in Hz */
} LinkwitzRileyParams_t;

/**
 * @brief Linkwitz-Riley filter state
 */
typedef struct {
    LinkwitzRileyParams_t params;   /* Filter parameters */
    uint8_t numBiquads;             /* Number of biquad sections needed */
    BiquadFilter_t biquads[MAX_LR_BIQUADS]; /* Array of biquad filters */
} LinkwitzRileyFilter_t;

/* Function prototypes -------------------------------------------------------*/
/**
 * @brief Initialize Linkwitz-Riley filter with given parameters
 * @param filter Pointer to filter structure
 * @param params Filter design parameters
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t LinkwitzRiley_Init(LinkwitzRileyFilter_t *filter, const LinkwitzRileyParams_t *params);

/**
 * @brief Reset filter state (clear delay lines)
 * @param filter Pointer to filter structure
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t LinkwitzRiley_Reset(LinkwitzRileyFilter_t *filter);

/**
 * @brief Process a single sample through the filter
 * @param filter Pointer to filter structure
 * @param input Input sample
 * @return Filtered output sample
 */
float LinkwitzRiley_ProcessSample(LinkwitzRileyFilter_t *filter, float input);

/**
 * @brief Process a block of samples through the filter
 * @param filter Pointer to filter structure
 * @param input Pointer to input buffer
 * @param output Pointer to output buffer
 * @param length Number of samples to process
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t LinkwitzRiley_ProcessBlock(LinkwitzRileyFilter_t *filter, 
                                       const float *input, 
                                       float *output, 
                                       uint32_t length);

/**
 * @brief Update the filter crossover frequency
 * @param filter Pointer to filter structure
 * @param cutoffFrequency New crossover frequency in Hz
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t LinkwitzRiley_UpdateFrequency(LinkwitzRileyFilter_t *filter, float cutoffFrequency);

/**
 * @brief Get the filter's magnitude response at a specific frequency
 * @param filter Pointer to filter structure
 * @param frequency The frequency to evaluate (in Hz)
 * @return Magnitude response (linear scale)
 */
float LinkwitzRiley_GetMagnitudeResponse(const LinkwitzRileyFilter_t *filter, float frequency);

/**
 * @brief Apply a Linkwitz-Riley crossover with both low-pass and high-pass outputs
 * @param lpFilter Pointer to low-pass filter structure
 * @param hpFilter Pointer to high-pass filter structure
 * @param input Input sample
 * @param lpOutput Pointer to store low-pass output
 * @param hpOutput Pointer to store high-pass output
 */
void LinkwitzRiley_CrossoverSample(LinkwitzRileyFilter_t *lpFilter, 
                                 LinkwitzRileyFilter_t *hpFilter,
                                 float input,
                                 float *lpOutput,
                                 float *hpOutput);

/**
 * @brief Apply a Linkwitz-Riley crossover with both low-pass and high-pass outputs for a block
 * @param lpFilter Pointer to low-pass filter structure
 * @param hpFilter Pointer to high-pass filter structure
 * @param input Pointer to input buffer
 * @param lpOutput Pointer to store low-pass output buffer
 * @param hpOutput Pointer to store high-pass output buffer
 * @param length Number of samples to process
 * @return FILTER_OK if successful, error code otherwise
 */
FilterStatus_t LinkwitzRiley_CrossoverBlock(LinkwitzRileyFilter_t *lpFilter, 
                                         LinkwitzRileyFilter_t *hpFilter,
                                         const float *input, 
                                         float *lpOutput, 
                                         float *hpOutput,
                                         uint32_t length);

/**
 * @brief Convert Linkwitz-Riley order to slope in dB/octave
 * @param order L-R filter order
 * @return Slope in dB/octave
 */
uint8_t LinkwitzRiley_OrderToSlope(uint8_t order);

/**
 * @brief Convert slope in dB/octave to Linkwitz-Riley order
 * @param slope Slope in dB/octave
 * @return L-R filter order
 */
uint8_t LinkwitzRiley_SlopeToOrder(uint8_t slope);

#ifdef __cplusplus
}
#endif

#endif /* LINKWITZ_RILEY_H */