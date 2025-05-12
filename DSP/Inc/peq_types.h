/**
  ******************************************************************************
  * @file           : peq_types.h
  * @brief          : Type definitions for parametric equalizer
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Type definitions for parametric equalizer (PEQ) functionality
  * Used by the DSP audio processing chain
  *
  ******************************************************************************
  */

#ifndef PEQ_TYPES_H
#define PEQ_TYPES_H

#include <stdint.h>
#include "dsp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EQ filter types
 */
typedef enum {
  PEQ_FILTER_BELL = 0,      /*!< Bell/Peak filter */
  PEQ_FILTER_LOW_SHELF,     /*!< Low shelf filter */
  PEQ_FILTER_HIGH_SHELF,    /*!< High shelf filter */
  PEQ_FILTER_LOW_PASS,      /*!< Low pass filter */
  PEQ_FILTER_HIGH_PASS,     /*!< High pass filter */
  PEQ_FILTER_ALL_PASS,      /*!< All pass filter (phase adjustment) */
  PEQ_FILTER_NOTCH,         /*!< Notch filter */
  PEQ_FILTER_BAND_PASS,     /*!< Band pass filter */
  PEQ_FILTER_MAX            /*!< Number of filter types */
} PEQ_FilterType_t;

/**
 * @brief EQ band configuration
 */
typedef struct {
  PEQ_FilterType_t filterType;  /*!< Type of EQ filter */
  float frequency;              /*!< Center/corner frequency in Hz (20-20000) */
  float gain;                   /*!< Gain in dB (-12 to +12) */
  float q;                      /*!< Q-factor (0.1 to 10.0) */
  uint8_t enabled;              /*!< Band enabled flag */
  
  /* Filter coefficients (calculated internally) */
  float b0;                     /*!< Biquad coefficient b0 */
  float b1;                     /*!< Biquad coefficient b1 */
  float b2;                     /*!< Biquad coefficient b2 */
  float a1;                     /*!< Biquad coefficient a1 */
  float a2;                     /*!< Biquad coefficient a2 */
  
  /* Filter state variables */
  float x1;                     /*!< Previous input sample */
  float x2;                     /*!< Second previous input sample */
  float y1;                     /*!< Previous output sample */
  float y2;                     /*!< Second previous output sample */
} PEQ_Band_t;

/**
 * @brief PEQ channel configuration
 */
typedef struct {
  PEQ_Band_t bands[MAX_PEQ_BANDS_PER_CHANNEL]; /*!< EQ bands per channel */
  float preGain;                               /*!< Pre-gain in dB */
  uint8_t enabled;                             /*!< EQ enabled flag */
  uint8_t numActiveBands;                      /*!< Number of active bands */
} PEQ_Channel_t;

/**
 * @brief PEQ system configuration
 */
typedef struct {
  PEQ_Channel_t channels[AUDIO_OUTPUT_CHANNELS]; /*!< Channel configurations */
  float sampleRate;                              /*!< Current sample rate */
} PEQ_Config_t;

#ifdef __cplusplus
}
#endif

#endif /* PEQ_TYPES_H */