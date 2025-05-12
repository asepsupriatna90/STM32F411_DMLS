/**
  ******************************************************************************
  * @file           : audio_config.c
  * @brief          : Audio configuration implementation
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Implementation of audio configuration and utility functions
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "audio_config.h"
#include <string.h>
#include <math.h>

/* Private defines -----------------------------------------------------------*/
#define LOG10_20    0.0301029996f  /* 1/log(10)*20 for dB conversion */
#define DB_TO_LINEAR(x) powf(10.0f, (x) / 20.0f)
#define LINEAR_TO_DB(x) (20.0f * log10f(x > 0.0001f ? x : 0.0001f))

/* Private variables ---------------------------------------------------------*/
static const char* ChannelNames[AUDIO_OUTPUT_CHANNELS] = {
  "OUT1", "OUT2", "OUT3", "OUT4"
};

/* Global variables ----------------------------------------------------------*/
AudioChannel_TypeDef AudioChannels[AUDIO_OUTPUT_CHANNELS];
AudioRouting_TypeDef AudioRouting;

/**
  * @brief  Initialize audio configuration
  * @param  None
  * @retval HAL status
  */
HAL_StatusTypeDef Audio_Config_Init(void)
{
  /* Set default configuration values */
  Audio_Config_SetDefaults();
  
  /* Initialize channel names */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    strncpy(AudioChannels[i].name, ChannelNames[i], sizeof(AudioChannels[i].name) - 1);
    AudioChannels[i].name[sizeof(AudioChannels[i].name) - 1] = '\0'; /* Ensure null termination */
  }
  
  return HAL_OK;
}

/**
  * @brief  Set default audio configuration values
  * @param  None
  * @retval None
  */
void Audio_Config_SetDefaults(void)
{
  /* Initialize channel configurations with defaults */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    AudioChannels[i].gainDb = 0.0f;         /* 0 dB gain by default */
    AudioChannels[i].gainLinear = 1.0f;     /* Unity gain */
    AudioChannels[i].mute = 0;              /* Not muted */
    AudioChannels[i].phase = 0;             /* Normal phase */
  }
  
  /* Setup default routing */
  if (AUDIO_OUTPUT_CHANNELS >= 2 && AUDIO_INPUT_CHANNELS >= 2) {
    /* Default routing for stereo setup */
    AudioRouting.source[0] = 0;             /* OUT1 from IN1 (Left) */
    AudioRouting.source[1] = 1;             /* OUT2 from IN2 (Right) */
    
    /* If we have 4 outputs, setup default 2-way stereo */
    if (AUDIO_OUTPUT_CHANNELS >= 4) {
      AudioRouting.source[2] = 0;           /* OUT3 from IN1 (Left) */
      AudioRouting.source[3] = 1;           /* OUT4 from IN2 (Right) */
    }
    
    /* Default mix ratios for summing mode */
    for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
      AudioRouting.mixRatio[i] = 0.5f;      /* Equal mix of IN1 and IN2 if using summing */
    }
  } else {
    /* Simple mono routing for limited channel counts */
    for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
      AudioRouting.source[i] = 0;           /* All outputs from IN1 */
      AudioRouting.mixRatio[i] = 0.5f;      /* Default mix ratio */
    }
  }
}

/**
  * @brief  Convert linear gain to dB value
  * @param  linear: Linear gain value
  * @retval Gain in dB
  */
float Audio_LinearToDB(float linear)
{
  if (linear < 0.0001f) {
    return AUDIO_MIN_GAIN_DB;  /* Return minimum dB for very low values */
  }
  
  return 20.0f * log10f(linear);
}

/**
  * @brief  Convert dB value to linear gain
  * @param  dB: Gain in dB
  * @retval Linear gain value
  */
float Audio_DBToLinear(float dB)
{
  if (dB <= AUDIO_MIN_GAIN_DB) {
    return 0.0f;  /* Return 0 for very low gain values */
  }
  
  return powf(10.0f, dB / 20.0f);
}

/**
  * @brief  Calculate RMS value of an audio buffer
  * @param  pBuffer: Pointer to float buffer
  * @param  size: Buffer size in samples
  * @retval RMS value (0.0 to 1.0)
  */
float Audio_CalculateRMS(float *pBuffer, uint16_t size)
{
  float sum = 0.0f;
  
  /* Sum squares of all samples */
  for (uint16_t i = 0; i < size; i++) {
    sum += pBuffer[i] * pBuffer[i];
  }
  
  /* Calculate RMS and return */
  if (size > 0) {
    return sqrtf(sum / (float)size);
  } else {
    return 0.0f;
  }
}

/**
  * @brief  Clip audio sample to valid range (-1.0 to 1.0)
  * @param  sample: Input sample
  * @retval Clipped sample
  */
float Audio_ClipSample(float sample)
{
  if (sample > 1.0f) {
    return 1.0f;
  } else if (sample < -1.0f) {
    return -1.0f;
  }
  
  return sample;
}

/**
  * @brief  Get channel display name
  * @param  channel: Channel index
  * @retval Channel name string pointer
  */
const char* Audio_GetChannelName(uint8_t channel)
{
  if (channel < AUDIO_OUTPUT_CHANNELS) {
    return AudioChannels[channel].name;
  }
  
  return "---";  /* Return placeholder for invalid channel */
}

/**
  * @brief  Convert frequency to logarithmic scale percentage (for UI)
  * @param  frequency: Frequency in Hz
  * @retval Percentage value (0-100%)
  */
uint8_t Audio_FreqToPercent(float frequency)
{
  float minLog = log10f(AUDIO_MIN_FREQ);
  float maxLog = log10f(AUDIO_MAX_FREQ);
  float freqLog = log10f(frequency);
  
  /* Clamp frequency to valid range */
  if (frequency < AUDIO_MIN_FREQ) {
    return 0;
  } else if (frequency > AUDIO_MAX_FREQ) {
    return 100;
  }
  
  /* Convert logarithmic scale to percentage */
  float percent = (freqLog - minLog) / (maxLog - minLog) * 100.0f;
  
  return (uint8_t)(percent + 0.5f);  /* Round to nearest integer */
}

/**
  * @brief  Convert percentage to logarithmic frequency (for UI)
  * @param  percent: Percentage value (0-100%)
  * @retval Frequency in Hz
  */
float Audio_PercentToFreq(uint8_t percent)
{
  float minLog = log10f(AUDIO_MIN_FREQ);
  float maxLog = log10f(AUDIO_MAX_FREQ);
  
  /* Clamp percentage to valid range */
  if (percent > 100) {
    percent = 100;
  }
  
  /* Convert percentage to logarithmic scale */
  float freqLog = minLog + (maxLog - minLog) * ((float)percent / 100.0f);
  
  return powf(10.0f, freqLog);
}