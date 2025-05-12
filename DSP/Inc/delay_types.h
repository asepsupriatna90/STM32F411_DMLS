/**
  ******************************************************************************
  * @file           : delay_types.h
  * @brief          : Type definitions for the audio delay module
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Delay data types and structures for DSP Audio Crossover system
  *
  ******************************************************************************
  */

#ifndef __DELAY_TYPES_H
#define __DELAY_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "dsp_common.h"

/* Constants -----------------------------------------------------------------*/
#define MAX_DELAY_MS           20        /* Maximum delay time in milliseconds */
#define DELAY_BUFFER_SIZE      (MAX_DELAY_MS * (AUDIO_SAMPLE_RATE / 1000) + 1) /* Buffer size based on max delay */

/* Units for delay representation */
typedef enum {
    DELAY_UNIT_MS,       /* Milliseconds */
    DELAY_UNIT_SAMPLES,  /* Samples */
    DELAY_UNIT_CM,       /* Centimeters (for distance-based time alignment) */
    DELAY_UNIT_INCHES,   /* Inches (for distance-based time alignment) */
} DelayUnit_TypeDef;

/**
 * @brief Delay state structure for runtime operation
 */
typedef struct {
    float_t delayBuffer[DELAY_BUFFER_SIZE]; /* Circular buffer for delay samples */
    uint32_t writeIndex;         /* Current write position in buffer */
    uint32_t delaySamples;       /* Current delay in samples */
    uint8_t bypass;              /* Bypass flag (1 = bypassed) */
    uint8_t enabled;             /* Enabled flag (1 = enabled) */
    float_t feedback;            /* Feedback amount (0.0 to 1.0) - typically 0 for crossover */
    float_t mix;                 /* Wet/dry mix (0.0 to 1.0) - typically 1.0 for crossover */
} DelayState_TypeDef;

/**
 * @brief Delay parameters structure for user configuration
 */
typedef struct {
    float_t delayTimeMs;        /* Delay time in milliseconds */
    float_t delayDistanceCm;    /* Delay as distance in centimeters */
    float_t feedbackPct;        /* Feedback percentage (typically 0% for crossover) */
    float_t mixPct;             /* Wet/dry mix percentage (typically 100% for crossover) */
    DelayUnit_TypeDef delayUnit; /* Preferred unit for user interface */
    uint8_t invertPolarity;     /* Phase inversion (0 = normal, 1 = inverted) */
} DelayParams_TypeDef;

/**
 * @brief Delay runtime configuration for a channel
 */
typedef struct {
    DelayState_TypeDef state;    /* Current runtime state */
    DelayParams_TypeDef params;  /* User parameters */
} DelayConfig_TypeDef;

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_TYPES_H */