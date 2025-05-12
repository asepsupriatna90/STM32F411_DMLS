/**
  ******************************************************************************
  * @file           : dsp_common.h
  * @brief          : Common DSP utilities and definitions
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Common definitions, structures, and utility functions for DSP processing
  * in the STM32F411 DSP Audio Crossover system.
  *
  ******************************************************************************
  */

#ifndef DSP_COMMON_H
#define DSP_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>
#include "audio_config.h"

/* Constants -----------------------------------------------------------------*/
#define DSP_PI                      3.14159265358979323846f
#define DSP_TWOPI                   (2.0f * DSP_PI)
#define DSP_HALFPI                  (0.5f * DSP_PI)

#define DSP_LN2                     0.69314718056f
#define DSP_LN10                    2.30258509299f

#define DSP_MIN_FREQUENCY           20.0f     /* Minimum audible frequency in Hz */
#define DSP_MAX_FREQUENCY           20000.0f  /* Maximum audible frequency in Hz */

#define DSP_MIN_GAIN_DB             -80.0f    /* Minimum gain in dB */
#define DSP_MAX_GAIN_DB             12.0f     /* Maximum gain in dB */

#define DSP_MIN_Q_FACTOR            0.1f      /* Minimum Q factor */
#define DSP_MAX_Q_FACTOR            10.0f     /* Maximum Q factor */

#define DSP_MIN_DELAY_MS            0.0f      /* Minimum delay in milliseconds */
#define DSP_MAX_DELAY_MS            20.0f     /* Maximum delay in milliseconds */

#define DSP_EPSILON                 1e-10f    /* Small value to prevent division by zero */

#define DSP_MAX_BIQUADS_PER_FILTER  4         /* Maximum number of biquad stages per filter */

/* Type Definitions ----------------------------------------------------------*/

/**
 * @brief Filter types enumeration
 */
typedef enum {
    FILTER_TYPE_NONE = 0,
    FILTER_TYPE_LOWPASS,
    FILTER_TYPE_HIGHPASS,
    FILTER_TYPE_BANDPASS,
    FILTER_TYPE_NOTCH,
    FILTER_TYPE_PEAK,
    FILTER_TYPE_LOWSHELF,
    FILTER_TYPE_HIGHSHELF,
    FILTER_TYPE_ALLPASS,
    FILTER_TYPE_MAX
} DSP_FilterType;

/**
 * @brief Filter response types enumeration
 */
typedef enum {
    FILTER_RESPONSE_BUTTERWORTH = 0,
    FILTER_RESPONSE_LINKWITZ_RILEY,
    FILTER_RESPONSE_BESSEL,
    FILTER_RESPONSE_MAX
} DSP_FilterResponse;

/**
 * @brief Filter slope enumeration (dB/octave)
 */
typedef enum {
    FILTER_SLOPE_6DB = 0,   /* 1st order, 6dB/octave */
    FILTER_SLOPE_12DB,      /* 2nd order, 12dB/octave */
    FILTER_SLOPE_18DB,      /* 3rd order, 18dB/octave */
    FILTER_SLOPE_24DB,      /* 4th order, 24dB/octave */
    FILTER_SLOPE_36DB,      /* 6th order, 36dB/octave */
    FILTER_SLOPE_48DB,      /* 8th order, 48dB/octave */
    FILTER_SLOPE_MAX
} DSP_FilterSlope;

/**
 * @brief Biquad filter coefficient structure
 */
typedef struct {
    float a0;
    float a1;
    float a2;
    float b1;
    float b2;
} DSP_BiquadCoeff;

/**
 * @brief Biquad filter state structure
 */
typedef struct {
    float x1;  /* x[n-1] */
    float x2;  /* x[n-2] */
    float y1;  /* y[n-1] */
    float y2;  /* y[n-2] */
} DSP_BiquadState;

/**
 * @brief Biquad filter instance structure
 */
typedef struct {
    DSP_BiquadCoeff coeff;
    DSP_BiquadState state;
} DSP_Biquad;

/**
 * @brief Multi-stage filter structure (cascaded biquads)
 */
typedef struct {
    DSP_Biquad stages[DSP_MAX_BIQUADS_PER_FILTER];
    uint8_t numStages;
    uint8_t enabled;
} DSP_Filter;

/* Function Prototypes -------------------------------------------------------*/

/**
 * @brief  Convert from dB to linear scale
 * @param  db_value: Value in dB
 * @retval Linear scale value
 */
float DSP_DbToLinear(float db_value);

/**
 * @brief  Convert from linear to dB scale
 * @param  linear_value: Value in linear scale
 * @retval dB value
 */
float DSP_LinearToDb(float linear_value);

/**
 * @brief  Calculate peak of audio buffer
 * @param  buffer: Audio data buffer
 * @param  length: Length of buffer in samples
 * @retval Peak value
 */
float DSP_CalculatePeak(float* buffer, uint16_t length);

/**
 * @brief  Calculate RMS of audio buffer
 * @param  buffer: Audio data buffer
 * @param  length: Length of buffer in samples
 * @retval RMS value
 */
float DSP_CalculateRMS(float* buffer, uint16_t length);

/**
 * @brief  Apply gain to audio buffer
 * @param  buffer: Audio data buffer
 * @param  length: Length of buffer in samples
 * @param  gain: Gain value in linear scale
 * @retval None
 */
void DSP_ApplyGain(float* buffer, uint16_t length, float gain);

/**
 * @brief  Mix two audio buffers with specified gains
 * @param  dst: Destination buffer
 * @param  src1: First source buffer
 * @param  gain1: Gain for first source (linear)
 * @param  src2: Second source buffer
 * @param  gain2: Gain for second source (linear)
 * @param  length: Length of buffers in samples
 * @retval None
 */
void DSP_MixBuffers(float* dst, float* src1, float gain1, float* src2, float gain2, uint16_t length);

/**
 * @brief  Apply soft clipping/limiting to audio buffer
 * @param  buffer: Audio data buffer
 * @param  length: Length of buffer in samples
 * @param  threshold: Threshold in linear scale (0.0 to 1.0)
 * @retval None
 */
void DSP_SoftClip(float* buffer, uint16_t length, float threshold);

/**
 * @brief  Reset filter state (clear history)
 * @param  filter: Pointer to filter instance
 * @retval None
 */
void DSP_ResetFilter(DSP_Filter* filter);

/**
 * @brief  Process a single sample through a biquad filter
 * @param  biquad: Pointer to biquad filter structure
 * @param  input: Input sample
 * @retval Filtered output sample
 */
float DSP_ProcessBiquad(DSP_Biquad* biquad, float input);

/**
 * @brief  Process a buffer through a multi-stage filter
 * @param  filter: Pointer to filter instance
 * @param  buffer: Audio data buffer (in-place processing)
 * @param  length: Length of buffer in samples
 * @retval None
 */
void DSP_ProcessFilter(DSP_Filter* filter, float* buffer, uint16_t length);

/**
 * @brief  Calculate the number of samples for a given delay time
 * @param  delay_ms: Delay time in milliseconds
 * @param  sample_rate: Sample rate in Hz
 * @retval Number of samples
 */
uint32_t DSP_CalculateDelaySamples(float delay_ms, uint32_t sample_rate);

/**
 * @brief  Calculate frequency value from MIDI note
 * @param  midi_note: MIDI note number (0-127)
 * @retval Frequency in Hz
 */
float DSP_MidiNoteToFrequency(uint8_t midi_note);

/**
 * @brief  Convert frequency to MIDI note (approximate)
 * @param  frequency: Frequency in Hz
 * @retval MIDI note number
 */
uint8_t DSP_FrequencyToMidiNote(float frequency);

/**
 * @brief  Interpolate smoothly between two values
 * @param  start: Start value
 * @param  end: End value
 * @param  position: Position (0.0 to 1.0)
 * @retval Interpolated value
 */
float DSP_SmoothInterpolate(float start, float end, float position);

#ifdef __cplusplus
}
#endif

#endif /* DSP_COMMON_H */