/**
  ******************************************************************************
  * @file           : crossover_types.h
  * @brief          : Crossover filter type definitions
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Type definitions for audio crossover filter implementation
  * Part of STM32F411 Audio DSP Crossover
  *
  ******************************************************************************
  */

#ifndef CROSSOVER_TYPES_H
#define CROSSOVER_TYPES_H

#include <stdint.h>
#include "filter_types.h"

/**
  * @brief  Crossover filter types
  */
typedef enum {
  CROSSOVER_FILTER_BUTTERWORTH = 0, /**< Butterworth filter (maximally flat) */
  CROSSOVER_FILTER_LINKWITZ_RILEY,  /**< Linkwitz-Riley filter (-6dB at crossover) */
  CROSSOVER_FILTER_BESSEL,          /**< Bessel filter (linear phase) */
  CROSSOVER_FILTER_MAX              /**< Maximum filter type value */
} CrossoverFilterType_t;

/**
  * @brief  Crossover filter slopes
  */
typedef enum {
  CROSSOVER_SLOPE_6DB = 0,     /**< 6dB/octave slope (1st order) */
  CROSSOVER_SLOPE_12DB,        /**< 12dB/octave slope (2nd order) */
  CROSSOVER_SLOPE_18DB,        /**< 18dB/octave slope (3rd order) */
  CROSSOVER_SLOPE_24DB,        /**< 24dB/octave slope (4th order) */
  CROSSOVER_SLOPE_36DB,        /**< 36dB/octave slope (6th order) */
  CROSSOVER_SLOPE_48DB,        /**< 48dB/octave slope (8th order) */
  CROSSOVER_SLOPE_MAX          /**< Maximum slope value */
} CrossoverSlope_t;

/**
  * @brief  Crossover band configuration
  */
typedef enum {
  CROSSOVER_BAND_BYPASS = 0, /**< Band is bypassed (full range) */
  CROSSOVER_BAND_LOW_PASS,   /**< Low pass filter */
  CROSSOVER_BAND_HIGH_PASS,  /**< High pass filter */
  CROSSOVER_BAND_BAND_PASS,  /**< Band pass filter (combination of LP and HP) */
  CROSSOVER_BAND_MAX         /**< Maximum band type value */
} CrossoverBandType_t;

/**
  * @brief  Crossover mode configuration
  */
typedef enum {
  CROSSOVER_MODE_OFF = 0,    /**< Crossover disabled (full range) */
  CROSSOVER_MODE_2WAY,       /**< 2-way crossover */
  CROSSOVER_MODE_3WAY,       /**< 3-way crossover */
  CROSSOVER_MODE_4WAY,       /**< 4-way crossover */
  CROSSOVER_MODE_CUSTOM,     /**< Custom crossover configuration */
  CROSSOVER_MODE_MAX         /**< Maximum mode value */
} CrossoverMode_t;

/**
  * @brief  Crossover band parameters
  */
typedef struct {
  CrossoverBandType_t type;            /**< Band type (LP, HP, BP, Bypass) */
  uint8_t enabled;                      /**< Band enabled flag */
  float frequency;                      /**< Crossover frequency (Hz) */
  float frequencyHigh;                  /**< High crossover frequency for bandpass (Hz) */
  CrossoverFilterType_t filterType;     /**< Filter type (Butterworth, LR, Bessel) */
  CrossoverSlope_t slope;               /**< Filter slope (6dB to 48dB/oct) */
  float gain;                           /**< Band gain (dB) */
} CrossoverBandParams_t;

/**
  * @brief  Crossover configuration parameters
  */
typedef struct {
  CrossoverMode_t mode;                             /**< Crossover mode */
  uint8_t enabled;                                  /**< Crossover enabled flag */
  uint8_t bandCount;                                /**< Number of active bands */
  CrossoverBandParams_t bands[4];                   /**< Band parameters (max 4 bands) */
  uint8_t linkChannels;                             /**< Link stereo channels flag */
} CrossoverParams_t;

/**
  * @brief  Filter state for each band
  */
typedef struct {
  BiquadFilterState_t filters[4];       /**< Biquad filters for each band (max 4 cascaded) */
  uint8_t filterCount;                  /**< Number of active filters in the cascade */
} CrossoverBandState_t;

/**
  * @brief  Crossover filter state
  */
typedef struct {
  CrossoverBandState_t bands[4];        /**< Filter states for each band */
  float bandOutput[4];                  /**< Per-band output for visualization */
  uint8_t initialized;                  /**< Initialization flag */
} CrossoverState_t;

#endif /* CROSSOVER_TYPES_H */