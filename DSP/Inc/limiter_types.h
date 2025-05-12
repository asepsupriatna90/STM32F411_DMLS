/**
  ******************************************************************************
  * @file           : limiter_types.h
  * @brief          : Type definitions for the audio limiter module
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Limiter data types and structures for DSP Audio Crossover system
  *
  ******************************************************************************
  */

#ifndef __LIMITER_TYPES_H
#define __LIMITER_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "dsp_common.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Limiter state structure for runtime operation
 */
typedef struct {
    float_t threshold;          /* Limiter threshold in linear scale */
    float_t attack;             /* Attack time constant */
    float_t release;            /* Release time constant */
    float_t attackCoef;         /* Calculated attack coefficient */
    float_t releaseCoef;        /* Calculated release coefficient */
    float_t envValue;           /* Current envelope follower value */
    float_t gainReduction;      /* Current gain reduction amount */
    float_t makeupGain;         /* Makeup gain in linear scale */
    uint8_t bypass;             /* Bypass flag (1 = bypassed) */
    uint8_t enabled;            /* Enabled flag (1 = enabled) */
} LimiterState_TypeDef;

/**
 * @brief Limiter parameters structure for user configuration
 */
typedef struct {
    float_t thresholdDb;        /* Limiter threshold in dB (typically -0.1 to -6.0 dB) */
    float_t attackMs;           /* Attack time in milliseconds (0.1ms to 10ms) */
    float_t releaseMs;          /* Release time in milliseconds (10ms to 500ms) */
    float_t makeupGainDb;       /* Makeup gain in dB (0 to 6 dB) */
    uint8_t enableLookahead;    /* Enable lookahead (1 = enabled) - if CPU allows */
    uint8_t linkChannels;       /* Link channels for stereo operation (1 = linked) */
} LimiterParams_TypeDef;

/**
 * @brief Limiter runtime configuration for a channel
 */
typedef struct {
    LimiterState_TypeDef state;    /* Current runtime state */
    LimiterParams_TypeDef params;  /* User parameters */
    float_t preDelayBuffer[MAX_LIMITER_LOOKAHEAD_SAMPLES]; /* Lookahead buffer */
    uint16_t preDelayIndex;         /* Current index in the lookahead buffer */
    uint16_t lookaheadSamples;      /* Number of lookahead samples (if enabled) */
} LimiterConfig_TypeDef;

#ifdef __cplusplus
}
#endif

#endif /* __LIMITER_TYPES_H */