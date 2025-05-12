/**
  ******************************************************************************
  * @file           : dsp_common.c
  * @brief          : Common DSP utilities implementation
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Common DSP functions implementation for the STM32F411 DSP Audio Crossover system.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "dsp_common.h"
#include <string.h>

/**
  * @brief  Convert from dB to linear scale
  * @param  db_value: Value in dB
  * @retval Linear scale value
  */
float DSP_DbToLinear(float db_value)
{
    return powf(10.0f, db_value / 20.0f);
}

/**
  * @brief  Convert from linear to dB scale
  * @param  linear_value: Value in linear scale
  * @retval dB value
  */
float DSP_LinearToDb(float linear_value)
{
    if (linear_value < DSP_EPSILON) {
        return DSP_MIN_GAIN_DB;
    }
    return 20.0f * log10f(linear_value);
}

/**
  * @brief  Calculate peak of audio buffer
  * @param  buffer: Audio data buffer
  * @param  length: Length of buffer in samples
  * @retval Peak value
  */
float DSP_CalculatePeak(float* buffer, uint16_t length)
{
    float peak = 0.0f;
    
    for (uint16_t i = 0; i < length; i++) {
        float abs_val = fabsf(buffer[i]);
        if (abs_val > peak) {
            peak = abs_val;
        }
    }
    
    return peak;
}

/**
  * @brief  Calculate RMS of audio buffer
  * @param  buffer: Audio data buffer
  * @param  length: Length of buffer in samples
  * @retval RMS value
  */
float DSP_CalculateRMS(float* buffer, uint16_t length)
{
    float sum = 0.0f;
    
    for (uint16_t i = 0; i < length; i++) {
        sum += buffer[i] * buffer[i];
    }
    
    if (length > 0) {
        return sqrtf(sum / length);
    } else {
        return 0.0f;
    }
}

/**
  * @brief  Apply gain to audio buffer
  * @param  buffer: Audio data buffer
  * @param  length: Length of buffer in samples
  * @param  gain: Gain value in linear scale
  * @retval None
  */
void DSP_ApplyGain(float* buffer, uint16_t length, float gain)
{
    for (uint16_t i = 0; i < length; i++) {
        buffer[i] *= gain;
    }
}

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
void DSP_MixBuffers(float* dst, float* src1, float gain1, float* src2, float gain2, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++) {
        dst[i] = (src1[i] * gain1) + (src2[i] * gain2);
    }
}

/**
  * @brief  Apply soft clipping/limiting to audio buffer
  * @param  buffer: Audio data buffer
  * @param  length: Length of buffer in samples
  * @param  threshold: Threshold in linear scale (0.0 to 1.0)
  * @retval None
  */
void DSP_SoftClip(float* buffer, uint16_t length, float threshold)
{
    /* Tanh-based soft clipping */
    for (uint16_t i = 0; i < length; i++) {
        if (fabsf(buffer[i]) > threshold) {
            /* Apply soft clipping using tanh function */
            buffer[i] = threshold * tanhf(buffer[i] / threshold);
        }
    }
}

/**
  * @brief  Reset filter state (clear history)
  * @param  filter: Pointer to filter instance
  * @retval None
  */
void DSP_ResetFilter(DSP_Filter* filter)
{
    for (uint8_t i = 0; i < filter->numStages; i++) {
        /* Clear state (history) variables */
        filter->stages[i].state.x1 = 0.0f;
        filter->stages[i].state.x2 = 0.0f;
        filter->stages[i].state.y1 = 0.0f;
        filter->stages[i].state.y2 = 0.0f;
    }
}

/**
  * @brief  Process a single sample through a biquad filter
  * @param  biquad: Pointer to biquad filter structure
  * @param  input: Input sample
  * @retval Filtered output sample
  */
float DSP_ProcessBiquad(DSP_Biquad* biquad, float input)
{
    float output;
    
    /* Direct Form II Transposed implementation */
    output = input * biquad->coeff.a0 + biquad->state.x1;
    biquad->state.x1 = input * biquad->coeff.a1 + biquad->state.x2 - biquad->coeff.b1 * output;
    biquad->state.x2 = input * biquad->coeff.a2 - biquad->coeff.b2 * output;
    
    return output;
}

/**
  * @brief  Process a buffer through a multi-stage filter
  * @param  filter: Pointer to filter instance
  * @param  buffer: Audio data buffer (in-place processing)
  * @param  length: Length of buffer in samples
  * @retval None
  */
void DSP_ProcessFilter(DSP_Filter* filter, float* buffer, uint16_t length)
{
    /* Early return if filter is disabled */
    if (!filter->enabled) {
        return;
    }
    
    /* Process each sample */
    for (uint16_t i = 0; i < length; i++) {
        float sample = buffer[i];
        
        /* Process through all biquad stages in cascade */
        for (uint8_t stage = 0; stage < filter->numStages; stage++) {
            sample = DSP_ProcessBiquad(&filter->stages[stage], sample);
        }
        
        buffer[i] = sample;
    }
}

/**
  * @brief  Calculate the number of samples for a given delay time
  * @param  delay_ms: Delay time in milliseconds
  * @param  sample_rate: Sample rate in Hz
  * @retval Number of samples
  */
uint32_t DSP_CalculateDelaySamples(float delay_ms, uint32_t sample_rate)
{
    /* Convert milliseconds to samples based on sample rate */
    return (uint32_t)((delay_ms * sample_rate) / 1000.0f);
}

/**
  * @brief  Calculate frequency value from MIDI note
  * @param  midi_note: MIDI note number (0-127)
  * @retval Frequency in Hz
  */
float DSP_MidiNoteToFrequency(uint8_t midi_note)
{
    /* A4 = MIDI note 69 = 440 Hz */
    return 440.0f * powf(2.0f, (midi_note - 69.0f) / 12.0f);
}

/**
  * @brief  Convert frequency to MIDI note (approximate)
  * @param  frequency: Frequency in Hz
  * @retval MIDI note number
  */
uint8_t DSP_FrequencyToMidiNote(float frequency)
{
    if (frequency <= 0.0f) {
        return 0;
    }
    
    /* A4 = MIDI note 69 = 440 Hz */
    float note = 69.0f + 12.0f * log2f(frequency / 440.0f);
    
    /* Round to nearest integer and clamp to valid MIDI range */
    int32_t midi_note = (int32_t)(note + 0.5f);
    
    if (midi_note < 0) {
        midi_note = 0;
    } else if (midi_note > 127) {
        midi_note = 127;
    }
    
    return (uint8_t)midi_note;
}

/**
  * @brief  Interpolate smoothly between two values
  * @param  start: Start value
  * @param  end: End value
  * @param  position: Position (0.0 to 1.0)
  * @retval Interpolated value
  */
float DSP_SmoothInterpolate(float start, float end, float position)
{
    /* Clamp position between 0 and 1 */
    if (position <= 0.0f) {
        return start;
    }
    if (position >= 1.0f) {
        return end;
    }
    
    /* Smooth cubic interpolation (S-curve) */
    float pos2 = position * position;
    float pos3 = pos2 * position;
    
    /* Apply cubic function: 3*t^2 - 2*t^3 */
    float smooth_pos = 3.0f * pos2 - 2.0f * pos3;
    
    /* Linear interpolation with smoothed position */
    return start + (end - start) * smooth_pos;
}