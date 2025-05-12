/**
  ******************************************************************************
  * @file           : compressor_types.h
  * @brief          : Type definitions for dynamics compressor
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Type definitions for dynamic range compressor functionality
  * Used by the DSP audio processing chain
  *
  ******************************************************************************
  */

#ifndef COMPRESSOR_TYPES_H
#define COMPRESSOR_TYPES_H

#include <stdint.h>
#include "dsp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compressor knee types
 */
typedef enum {
  COMP_KNEE_HARD = 0,       /*!< Hard knee (sharp transition at threshold) */
  COMP_KNEE_SOFT,           /*!< Soft knee (gradual transition around threshold) */
  COMP_KNEE_MAX             /*!< Number of knee types */
} Compressor_KneeType_t;

/**
 * @brief Compressor detection modes
 */
typedef enum {
  COMP_DETECTION_RMS = 0,   /*!< RMS level detection (slower, more musical) */
  COMP_DETECTION_PEAK,      /*!< Peak level detection (faster, more protective) */
  COMP_DETECTION_MAX        /*!< Number of detection modes */
} Compressor_DetectionMode_t;

/**
 * @brief Compressor parameters
 */
typedef struct {
  float threshold;          /*!< Threshold in dB (-60 to 0) */
  float ratio;              /*!< Compression ratio (1.0 to 20.0) */
  float attackTime;         /*!< Attack time in ms (0.1 to 100) */
  float releaseTime;        /*!< Release time in ms (10 to 1000) */
  float makeupGain;         /*!< Makeup gain in dB (0 to 24) */
  Compressor_KneeType_t kneeType;          /*!< Knee type */
  Compressor_DetectionMode_t detectionMode; /*!< Detection mode */
  uint8_t enabled;          /*!< Compressor enabled flag */
  
  /* Internal state variables */
  float envDb;              /*!< Current envelope level in dB */
  float gainReduction;      /*!< Current gain reduction in dB */
  float sampleRate;         /*!< Current sample rate */
  float attackCoef;         /*!< Attack coefficient (calculated from time) */
  float releaseCoef;        /*!< Release coefficient (calculated from time) */
  float kneeWidth;          /*!< Knee width in dB (for soft knee) */
  float prevInputSample;    /*!< Previous input sample (for envelope detection) */
  float prevOutputSample;   /*!< Previous output sample */
} Compressor_Channel_t;

/**
 * @brief Compressor system configuration
 */
typedef struct {
  Compressor_Channel_t channels[AUDIO_OUTPUT_CHANNELS]; /*!< Channel configurations */
} Compressor_Config_t;

#ifdef __cplusplus
}
#endif

#endif /* COMPRESSOR_TYPES_H */