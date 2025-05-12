/**
  ******************************************************************************
  * @file           : compressor_proc.c
  * @brief          : Compressor processing implementation
  * @author         : asepsupriatna90
  ******************************************************************************
  * @attention
  *
  * Audio dynamic range compressor implementation for STM32F411 DSP Audio Crossover
  * Features:
  * - Threshold based compression
  * - Variable ratio (1:1 to 20:1)
  * - Adjustable attack and release
  * - Soft/Hard knee
  * - Make-up gain 
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "compressor.h"
#include "compressor_types.h"
#include "math_utils.h"
#include "debug.h"
#include <math.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define COMP_DB_MIN                 -120.0f    /* Minimum dB level (considered as silence) */
#define COMP_ENVELOPE_INIT          -120.0f    /* Initial envelope value */
#define COMP_GAIN_SMOOTHING_COEF    0.9995f    /* Smoothing coefficient for gain changes */
#define COMP_RMS_WINDOW_SIZE        32         /* Window size for RMS calculation */
#define COMP_MIN_GAIN_DB            -60.0f     /* Minimum gain in dB */
#define COMP_MAX_GAIN_DB            0.0f       /* Maximum gain in dB */

/* Private macro -------------------------------------------------------------*/
#define DB_TO_LINEAR(x)             ((x) <= COMP_DB_MIN ? 0.0f : powf(10.0f, (x) / 20.0f))
#define LINEAR_TO_DB(x)             ((x) <= 0.0f ? COMP_DB_MIN : 20.0f * log10f(x))
#define MAX(a,b)                    ((a) > (b) ? (a) : (b))
#define MIN(a,b)                    ((a) < (b) ? (a) : (b))
#define CLAMP(x, min, max)          (MIN(MAX((x), (min)), (max)))

/* Private variables ---------------------------------------------------------*/
/* Compressor states for all output channels */
static CompressorState_TypeDef compressorState[AUDIO_OUTPUT_CHANNELS];

/* RMS calculation buffers */
static float rmsBuffer[AUDIO_OUTPUT_CHANNELS][COMP_RMS_WINDOW_SIZE];
static uint16_t rmsBufferIndex[AUDIO_OUTPUT_CHANNELS];

/* Private function prototypes -----------------------------------------------*/
static float Compressor_CalculateRMS(uint8_t channel, float sample);
static float Compressor_CalculateEnvelope(uint8_t channel, float level_db);
static float Compressor_CalculateGain(uint8_t channel, float envelope_db);
static float Compressor_ApplySoftKnee(CompressorParameters_TypeDef *params, float input_db);

/**
  * @brief  Initialize the compressor module
  * @param  None
  * @retval None
  */
void Compressor_Init(void)
{
  DEBUG_PRINT("Initializing compressor module\r\n");
  
  /* Initialize compressor states for all channels */
  for (uint8_t channel = 0; channel < AUDIO_OUTPUT_CHANNELS; channel++) {
    /* Initialize envelope */
    compressorState[channel].envelope_db = COMP_ENVELOPE_INIT;
    compressorState[channel].currentGain = 1.0f;
    compressorState[channel].prevGainDb = 0.0f;
    compressorState[channel].rmsValue = 0.0f;
    
    /* Clear RMS buffer */
    memset(rmsBuffer[channel], 0, sizeof(float) * COMP_RMS_WINDOW_SIZE);
    rmsBufferIndex[channel] = 0;
    
    /* Initialize with default parameters */
    CompressorParameters_TypeDef *params = &compressorState[channel].params;
    params->enabled = 0;
    params->threshold_db = -20.0f;
    params->ratio = 4.0f;
    params->attackTime_ms = 20.0f;
    params->releaseTime_ms = 200.0f;
    params->makeupGain_db = 0.0f;
    params->kneeWidth_db = 6.0f;  /* Default soft knee */
    params->useSoftKnee = 1;      /* Enable soft knee by default */
    params->useRMS = 1;           /* Use RMS detection by default */
    
    /* Calculate attack and release coefficients */
    Compressor_UpdateParameters(channel);
  }
  
  DEBUG_PRINT("Compressor module initialized\r\n");
}

/**
  * @brief  Update compressor parameters for a specific channel
  * @param  channel: Channel index
  * @retval None
  */
void Compressor_UpdateParameters(uint8_t channel)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS) {
    return;
  }
  
  CompressorParameters_TypeDef *params = &compressorState[channel].params;
  
  /* Calculate attack coefficient: e^(-1/(sampleRate * attackTime)) */
  float attackCoeff;
  if (params->attackTime_ms <= 0.1f) {
    /* Very fast attack, almost immediate */
    attackCoeff = 0.0f;
  } else {
    attackCoeff = expf(-1.0f / (AUDIO_SAMPLE_RATE * params->attackTime_ms / 1000.0f));
  }
  compressorState[channel].attackCoeff = attackCoeff;
  
  /* Calculate release coefficient: e^(-1/(sampleRate * releaseTime)) */
  float releaseCoeff;
  if (params->releaseTime_ms <= 0.1f) {
    /* Very fast release, almost immediate */
    releaseCoeff = 0.0f;
  } else {
    releaseCoeff = expf(-1.0f / (AUDIO_SAMPLE_RATE * params->releaseTime_ms / 1000.0f));
  }
  compressorState[channel].releaseCoeff = releaseCoeff;
  
  DEBUG_PRINT("Updated compressor ch%d: thr=%.1f, ratio=%.1f, att=%.1f, rel=%.1f\r\n", 
               channel, params->threshold_db, params->ratio, 
               params->attackTime_ms, params->releaseTime_ms);
}

/**
  * @brief  Set compressor parameters for a specific channel
  * @param  channel: Channel index
  * @param  params: Pointer to compressor parameters
  * @retval None
  */
void Compressor_SetParameters(uint8_t channel, CompressorParameters_TypeDef *params)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS || params == NULL) {
    return;
  }
  
  /* Copy parameters */
  memcpy(&compressorState[channel].params, params, sizeof(CompressorParameters_TypeDef));
  
  /* Update coefficients */
  Compressor_UpdateParameters(channel);
}

/**
  * @brief  Get compressor parameters for a specific channel
  * @param  channel: Channel index
  * @param  params: Pointer to store compressor parameters
  * @retval None
  */
void Compressor_GetParameters(uint8_t channel, CompressorParameters_TypeDef *params)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS || params == NULL) {
    return;
  }
  
  /* Copy parameters */
  memcpy(params, &compressorState[channel].params, sizeof(CompressorParameters_TypeDef));
}

/**
  * @brief  Enable/disable compressor for a specific channel
  * @param  channel: Channel index
  * @param  enabled: Enable (1) or disable (0)
  * @retval None
  */
void Compressor_SetEnabled(uint8_t channel, uint8_t enabled)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS) {
    return;
  }
  
  compressorState[channel].params.enabled = enabled;
  
  /* Reset state when disabling to avoid gain jumping when re-enabled */
  if (!enabled) {
    compressorState[channel].envelope_db = COMP_ENVELOPE_INIT;
    compressorState[channel].currentGain = 1.0f;
    compressorState[channel].prevGainDb = 0.0f;
  }
  
  DEBUG_PRINT("Compressor channel %d %s\r\n", channel, enabled ? "enabled" : "disabled");
}

/**
  * @brief  Process audio samples through the compressor
  * @param  channel: Channel index
  * @param  audioBuffer: Pointer to audio buffer
  * @retval None
  */
void Compressor_Process(uint8_t channel, AudioBuffer_TypeDef *audioBuffer)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS || audioBuffer == NULL) {
    return;
  }
  
  /* Quick return if compressor is disabled */
  if (!compressorState[channel].params.enabled) {
    return;
  }
  
  CompressorParameters_TypeDef *params = &compressorState[channel].params;
  
  /* Process each sample */
  for (uint16_t i = 0; i < audioBuffer->size; i++) {
    float sample = audioBuffer->data[channel][i];
    float level;
    
    /* Calculate signal level - RMS or peak based on configuration */
    if (params->useRMS) {
      level = Compressor_CalculateRMS(channel, sample);
    } else {
      /* Peak detection */
      level = fabsf(sample);
    }
    
    /* Convert to dB */
    float level_db = level > 0.0f ? LINEAR_TO_DB(level) : COMP_DB_MIN;
    
    /* Calculate envelope */
    float envelope_db = Compressor_CalculateEnvelope(channel, level_db);
    
    /* Calculate gain reduction */
    float gain_db = Compressor_CalculateGain(channel, envelope_db);
    
    /* Apply makeup gain */
    gain_db += params->makeupGain_db;
    
    /* Smooth gain changes to avoid artifacts */
    compressorState[channel].prevGainDb = compressorState[channel].prevGainDb * COMP_GAIN_SMOOTHING_COEF + 
                                          gain_db * (1.0f - COMP_GAIN_SMOOTHING_COEF);
    
    /* Convert back to linear gain */
    float gain = DB_TO_LINEAR(compressorState[channel].prevGainDb);
    compressorState[channel].currentGain = gain;
    
    /* Apply gain to the sample */
    audioBuffer->data[channel][i] = sample * gain;
  }
  
  /* Store statistics for metering and monitoring */
  compressorState[channel].stats.gainReduction_db = -compressorState[channel].prevGainDb + params->makeupGain_db;
  compressorState[channel].stats.inputLevel_db = LINEAR_TO_DB(compressorState[channel].rmsValue);
  compressorState[channel].stats.outputLevel_db = compressorState[channel].stats.inputLevel_db - 
                                                 compressorState[channel].stats.gainReduction_db;
}

/**
  * @brief  Calculate RMS value of audio samples using a moving window
  * @param  channel: Channel index
  * @param  sample: New sample to add to the RMS calculation
  * @retval RMS value
  */
static float Compressor_CalculateRMS(uint8_t channel, float sample)
{
  /* Replace oldest sample with new sample */
  rmsBuffer[channel][rmsBufferIndex[channel]] = sample * sample;
  
  /* Update index */
  rmsBufferIndex[channel] = (rmsBufferIndex[channel] + 1) % COMP_RMS_WINDOW_SIZE;
  
  /* Calculate sum of squares */
  float sum = 0.0f;
  for (uint16_t i = 0; i < COMP_RMS_WINDOW_SIZE; i++) {
    sum += rmsBuffer[channel][i];
  }
  
  /* Calculate RMS */
  float rms = sqrtf(sum / COMP_RMS_WINDOW_SIZE);
  
  /* Store for statistics */
  compressorState[channel].rmsValue = rms;
  
  return rms;
}

/**
  * @brief  Calculate envelope with attack and release time
  * @param  channel: Channel index
  * @param  level_db: Current signal level in dB
  * @retval Envelope value in dB
  */
static float Compressor_CalculateEnvelope(uint8_t channel, float level_db)
{
  float envelope_db = compressorState[channel].envelope_db;
  
  /* Apply attack or release based on whether level is above envelope */
  if (level_db > envelope_db) {
    /* Attack phase */
    envelope_db = compressorState[channel].attackCoeff * envelope_db + 
                  (1.0f - compressorState[channel].attackCoeff) * level_db;
  } else {
    /* Release phase */
    envelope_db = compressorState[channel].releaseCoeff * envelope_db + 
                  (1.0f - compressorState[channel].releaseCoeff) * level_db;
  }
  
  /* Update state */
  compressorState[channel].envelope_db = envelope_db;
  
  return envelope_db;
}

/**
  * @brief  Calculate gain based on envelope and compressor parameters
  * @param  channel: Channel index
  * @param  envelope_db: Signal envelope in dB
  * @retval Gain in dB
  */
static float Compressor_CalculateGain(uint8_t channel, float envelope_db)
{
  CompressorParameters_TypeDef *params = &compressorState[channel].params;
  float threshold_db = params->threshold_db;
  float gain_db = 0.0f;  /* No gain reduction by default */
  
  /* Determine if signal is above threshold */
  if (params->useSoftKnee) {
    /* Soft knee implementation */
    gain_db = Compressor_ApplySoftKnee(params, envelope_db);
  } else {
    /* Hard knee (standard) implementation */
    if (envelope_db > threshold_db) {
      /* Calculate gain reduction */
      float excess_db = envelope_db - threshold_db;
      gain_db = excess_db - (excess_db / params->ratio);
      
      /* Apply gain (negative value for reduction) */
      gain_db = -gain_db;
    }
  }
  
  /* Limit gain reduction */
  gain_db = CLAMP(gain_db, COMP_MIN_GAIN_DB, COMP_MAX_GAIN_DB);
  
  return gain_db;
}

/**
  * @brief  Apply soft knee to compressor curve
  * @param  params: Compressor parameters
  * @param  input_db: Input level in dB
  * @retval Output gain in dB
  */
static float Compressor_ApplySoftKnee(CompressorParameters_TypeDef *params, float input_db)
{
  float threshold_db = params->threshold_db;
  float knee_width = params->kneeWidth_db;
  float knee_lower = threshold_db - (knee_width / 2.0f);
  float knee_upper = threshold_db + (knee_width / 2.0f);
  float gain_db = 0.0f;
  
  if (input_db < knee_lower) {
    /* Below knee - no compression */
    gain_db = 0.0f;
  } else if (input_db > knee_upper) {
    /* Above knee - full compression based on ratio */
    float excess_db = input_db - threshold_db;
    gain_db = excess_db - (excess_db / params->ratio);
    gain_db = -gain_db;  /* Convert to gain reduction (negative) */
  } else {
    /* Within knee - gradual transition */
    float knee_position = (input_db - knee_lower) / knee_width;  /* 0 to 1 */
    float transition_ratio = 1.0f + ((params->ratio - 1.0f) * knee_position);
    float excess_db = input_db - knee_lower;
    gain_db = excess_db - (excess_db / transition_ratio);
    gain_db = -gain_db;  /* Convert to gain reduction (negative) */
  }
  
  return gain_db;
}

/**
  * @brief  Get compressor statistics for metering
  * @param  channel: Channel index
  * @param  stats: Pointer to store statistics
  * @retval None
  */
void Compressor_GetStatistics(uint8_t channel, CompressorStatistics_TypeDef *stats)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS || stats == NULL) {
    return;
  }
  
  /* Copy statistics */
  memcpy(stats, &compressorState[channel].stats, sizeof(CompressorStatistics_TypeDef));
}

/**
  * @brief  Reset compressor state for all channels
  * @param  None
  * @retval None
  */
void Compressor_Reset(void)
{
  for (uint8_t channel = 0; channel < AUDIO_OUTPUT_CHANNELS; channel++) {
    /* Reset envelope and gain */
    compressorState[channel].envelope_db = COMP_ENVELOPE_INIT;
    compressorState[channel].currentGain = 1.0f;
    compressorState[channel].prevGainDb = 0.0f;
    compressorState[channel].rmsValue = 0.0f;
    
    /* Clear RMS buffer */
    memset(rmsBuffer[channel], 0, sizeof(float) * COMP_RMS_WINDOW_SIZE);
    rmsBufferIndex[channel] = 0;
    
    /* Reset statistics */
    memset(&compressorState[channel].stats, 0, sizeof(CompressorStatistics_TypeDef));
  }
  
  DEBUG_PRINT("Compressor states reset\r\n");
}

/**
  * @brief  Reset compressor state for a specific channel
  * @param  channel: Channel index
  * @retval None
  */
void Compressor_ResetChannel(uint8_t channel)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS) {
    return;
  }
  
  /* Reset envelope and gain */
  compressorState[channel].envelope_db = COMP_ENVELOPE_INIT;
  compressorState[channel].currentGain = 1.0f;
  compressorState[channel].prevGainDb = 0.0f;
  compressorState[channel].rmsValue = 0.0f;
  
  /* Clear RMS buffer */
  memset(rmsBuffer[channel], 0, sizeof(float) * COMP_RMS_WINDOW_SIZE);
  rmsBufferIndex[channel] = 0;
  
  /* Reset statistics */
  memset(&compressorState[channel].stats, 0, sizeof(CompressorStatistics_TypeDef));
  
  DEBUG_PRINT("Compressor channel %d reset\r\n", channel);
}

/**
  * @brief  Get real-time compression factor for metering
  * @param  channel: Channel index
  * @retval Compression factor (0.0 to 1.0, where 1.0 is max compression)
  */
float Compressor_GetCompressionFactor(uint8_t channel)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS || !compressorState[channel].params.enabled) {
    return 0.0f;
  }
  
  /* Convert gain reduction from dB to linear scale 0-1 */
  float reduction_db = compressorState[channel].stats.gainReduction_db;
  
  /* Map typical range (0-20dB) to 0-1 for display */
  float factor = reduction_db / 20.0f;
  factor = CLAMP(factor, 0.0f, 1.0f);
  
  return factor;
}

/**
  * @brief  This function is called by DSP_Compressor_Process in audio_processing.c
  * @param  channel: Channel index
  * @param  audioBuffer: Pointer to audio buffer
  * @retval None
  */
void DSP_Compressor_ProcessChannel(uint8_t channel, AudioBuffer_TypeDef *audioBuffer)
{
  /* Call the main process function */
  Compressor_Process(channel, audioBuffer);
}