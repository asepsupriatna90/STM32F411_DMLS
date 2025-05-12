/**
  ******************************************************************************
  * @file           : peq_filter.c
  * @brief          : Parametric Equalizer (PEQ) filter implementation
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Implementation of 5-band parametric equalizer for audio processing
  * Supports Bell, Low Shelf, High Shelf, Low Pass, and High Pass filter types
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "peq.h"
#include "peq_types.h"
#include "biquad.h"
#include "math_utils.h"
#include "debug.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define PEQ_MAX_BANDS_PER_CHANNEL    5
#define PEQ_UNUSED_BAND_FLAG         0xFF

/* Max. possible Q-factor - guard against division by near-zero */
#define PEQ_MAX_Q_FACTOR             20.0f

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* EQ band configurations for each output channel */
static PEQBand_TypeDef PEQBands[AUDIO_OUTPUT_CHANNELS][PEQ_MAX_BANDS_PER_CHANNEL];

/* Biquad filter states for each EQ band */
static BiquadState_TypeDef PEQBiquadStates[AUDIO_OUTPUT_CHANNELS][PEQ_MAX_BANDS_PER_CHANNEL];

/* Private function prototypes -----------------------------------------------*/
static void PEQ_CalculateBell(float centerFreq, float gain, float Q, float fs, BiquadCoeffs_TypeDef *coeffs);
static void PEQ_CalculateLowShelf(float cutoffFreq, float gain, float Q, float fs, BiquadCoeffs_TypeDef *coeffs);
static void PEQ_CalculateHighShelf(float cutoffFreq, float gain, float Q, float fs, BiquadCoeffs_TypeDef *coeffs);
static void PEQ_CalculateLowPass(float cutoffFreq, float Q, float fs, BiquadCoeffs_TypeDef *coeffs);
static void PEQ_CalculateHighPass(float cutoffFreq, float Q, float fs, BiquadCoeffs_TypeDef *coeffs);
static void PEQ_UpdateFilterCoefficients(uint8_t channel, uint8_t band);

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  Initialize all PEQ bands with default values
  * @param  None
  * @retval None
  */
void PEQ_Init(void)
{
  /* Initialize all PEQ bands with default values */
  for (uint8_t channel = 0; channel < AUDIO_OUTPUT_CHANNELS; channel++) {
    for (uint8_t band = 0; band < PEQ_MAX_BANDS_PER_CHANNEL; band++) {
      /* Default values for EQ bands */
      PEQBands[channel][band].type = PEQ_TYPE_BELL;
      PEQBands[channel][band].frequency = 1000.0f;  /* 1kHz */
      PEQBands[channel][band].gain = 0.0f;         /* 0dB - flat response */
      PEQBands[channel][band].q = 1.414f;          /* Q = 1.414 (sqrt(2)) */
      PEQBands[channel][band].enabled = 0;         /* Disabled by default */
      
      /* Initialize biquad filter states */
      Biquad_ResetState(&PEQBiquadStates[channel][band]);
      
      /* Calculate initial coefficients */
      PEQ_UpdateFilterCoefficients(channel, band);
    }
  }
  
  DEBUG_PRINT("PEQ: Initialized all PEQ bands with default values\r\n");
}

/**
  * @brief  Configure a specific PEQ band
  * @param  channel: Output channel index (0-3)
  * @param  band: Band index (0-4)
  * @param  config: Pointer to PEQ band configuration
  * @retval HAL status
  */
HAL_StatusTypeDef PEQ_ConfigureBand(uint8_t channel, uint8_t band, PEQBand_TypeDef *config)
{
  /* Check parameters */
  if (channel >= AUDIO_OUTPUT_CHANNELS || band >= PEQ_MAX_BANDS_PER_CHANNEL || config == NULL) {
    DEBUG_PRINT("PEQ: Invalid parameters in PEQ_ConfigureBand\r\n");
    return HAL_ERROR;
  }
  
  /* Safety checks on frequency range */
  if (config->frequency < 20.0f) {
    config->frequency = 20.0f;
  } else if (config->frequency > 20000.0f) {
    config->frequency = 20000.0f;
  }
  
  /* Safety checks on Q-factor */
  if (config->q < 0.1f) {
    config->q = 0.1f;
  } else if (config->q > 10.0f) {
    config->q = 10.0f;
  }
  
  /* Safety checks on gain range */
  if (config->gain < -12.0f) {
    config->gain = -12.0f;
  } else if (config->gain > 12.0f) {
    config->gain = 12.0f;
  }
  
  /* Update band configuration */
  PEQBands[channel][band].type = config->type;
  PEQBands[channel][band].frequency = config->frequency;
  PEQBands[channel][band].gain = config->gain;
  PEQBands[channel][band].q = config->q;
  PEQBands[channel][band].enabled = config->enabled;
  
  /* Update filter coefficients */
  PEQ_UpdateFilterCoefficients(channel, band);
  
  DEBUG_PRINT("PEQ: Configured channel %d band %d: type=%d, freq=%.1f, gain=%.1f, Q=%.2f, enabled=%d\r\n", 
              channel, band, config->type, config->frequency, config->gain, config->q, config->enabled);
  
  return HAL_OK;
}

/**
  * @brief  Get the configuration of a specific PEQ band
  * @param  channel: Output channel index (0-3)
  * @param  band: Band index (0-4)
  * @param  config: Pointer to store PEQ band configuration
  * @retval HAL status
  */
HAL_StatusTypeDef PEQ_GetBandConfig(uint8_t channel, uint8_t band, PEQBand_TypeDef *config)
{
  /* Check parameters */
  if (channel >= AUDIO_OUTPUT_CHANNELS || band >= PEQ_MAX_BANDS_PER_CHANNEL || config == NULL) {
    DEBUG_PRINT("PEQ: Invalid parameters in PEQ_GetBandConfig\r\n");
    return HAL_ERROR;
  }
  
  /* Copy band configuration */
  config->type = PEQBands[channel][band].type;
  config->frequency = PEQBands[channel][band].frequency;
  config->gain = PEQBands[channel][band].gain;
  config->q = PEQBands[channel][band].q;
  config->enabled = PEQBands[channel][band].enabled;
  
  return HAL_OK;
}

/**
  * @brief  Enable or disable a specific PEQ band
  * @param  channel: Output channel index (0-3)
  * @param  band: Band index (0-4)
  * @param  enabled: 1 to enable, 0 to disable
  * @retval HAL status
  */
HAL_StatusTypeDef PEQ_SetBandEnabled(uint8_t channel, uint8_t band, uint8_t enabled)
{
  /* Check parameters */
  if (channel >= AUDIO_OUTPUT_CHANNELS || band >= PEQ_MAX_BANDS_PER_CHANNEL) {
    DEBUG_PRINT("PEQ: Invalid parameters in PEQ_SetBandEnabled\r\n");
    return HAL_ERROR;
  }
  
  /* Update enabled state */
  PEQBands[channel][band].enabled = enabled ? 1 : 0;
  
  DEBUG_PRINT("PEQ: Channel %d band %d %s\r\n", 
              channel, band, enabled ? "enabled" : "disabled");
  
  return HAL_OK;
}

/**
  * @brief  Process audio data through the PEQ filter chain for one channel
  * @param  channel: Output channel index (0-3)
  * @param  audioBuffer: Pointer to audio buffer structure
  * @retval None
  */
void PEQ_ProcessChannel(uint8_t channel, AudioBuffer_TypeDef *audioBuffer)
{
  float *samples;
  uint32_t numSamples;
  float tempSample;
  
  /* Check parameters */
  if (channel >= AUDIO_OUTPUT_CHANNELS || audioBuffer == NULL) {
    DEBUG_PRINT("PEQ: Invalid parameters in PEQ_ProcessChannel\r\n");
    return;
  }
  
  /* Get output samples array for this channel */
  samples = audioBuffer->outputChannels[channel];
  numSamples = audioBuffer->frameSize;
  
  /* Process each sample through all enabled EQ bands */
  for (uint32_t i = 0; i < numSamples; i++) {
    tempSample = samples[i];
    
    /* Apply each EQ band in series */
    for (uint8_t band = 0; band < PEQ_MAX_BANDS_PER_CHANNEL; band++) {
      /* Skip disabled bands */
      if (!PEQBands[channel][band].enabled) {
        continue;
      }
      
      /* Process through biquad filter */
      tempSample = Biquad_Process(&PEQBiquadStates[channel][band], tempSample);
    }
    
    /* Write back to output buffer */
    samples[i] = tempSample;
  }
}

/**
  * @brief  Process all channels through their respective PEQ filters
  * @param  audioBuffer: Pointer to audio buffer structure
  * @retval None
  */
void PEQ_ProcessAllChannels(AudioBuffer_TypeDef *audioBuffer)
{
  /* Process each output channel */
  for (uint8_t channel = 0; channel < AUDIO_OUTPUT_CHANNELS; channel++) {
    PEQ_ProcessChannel(channel, audioBuffer);
  }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Update filter coefficients for a specific PEQ band
  * @param  channel: Output channel index (0-3)
  * @param  band: Band index (0-4)
  * @retval None
  */
static void PEQ_UpdateFilterCoefficients(uint8_t channel, uint8_t band)
{
  /* Get system sample rate */
  float fs = (float)AUDIO_SAMPLE_RATE;
  
  /* Get references to band and its coefficients */
  PEQBand_TypeDef *peqBand = &PEQBands[channel][band];
  BiquadCoeffs_TypeDef coeffs;
  
  /* Calculate coefficients based on filter type */
  switch (peqBand->type) {
    case PEQ_TYPE_BELL:
      PEQ_CalculateBell(peqBand->frequency, peqBand->gain, peqBand->q, fs, &coeffs);
      break;
      
    case PEQ_TYPE_LOW_SHELF:
      PEQ_CalculateLowShelf(peqBand->frequency, peqBand->gain, peqBand->q, fs, &coeffs);
      break;
      
    case PEQ_TYPE_HIGH_SHELF:
      PEQ_CalculateHighShelf(peqBand->frequency, peqBand->gain, peqBand->q, fs, &coeffs);
      break;
      
    case PEQ_TYPE_LOW_PASS:
      PEQ_CalculateLowPass(peqBand->frequency, peqBand->q, fs, &coeffs);
      break;
      
    case PEQ_TYPE_HIGH_PASS:
      PEQ_CalculateHighPass(peqBand->frequency, peqBand->q, fs, &coeffs);
      break;
      
    default:
      /* Invalid filter type, use pass-through (flat response) */
      coeffs.b0 = 1.0f;
      coeffs.b1 = 0.0f;
      coeffs.b2 = 0.0f;
      coeffs.a1 = 0.0f;
      coeffs.a2 = 0.0f;
      break;
  }
  
  /* Set coefficients to the biquad filter */
  Biquad_SetCoefficients(&PEQBiquadStates[channel][band], &coeffs);
}

/**
  * @brief  Calculate coefficients for Bell (Peaking) filter
  * @param  centerFreq: Center frequency in Hz
  * @param  gain: Gain in dB
  * @param  Q: Q-factor
  * @param  fs: Sample rate in Hz
  * @param  coeffs: Pointer to store calculated coefficients
  * @retval None
  */
static void PEQ_CalculateBell(float centerFreq, float gain, float Q, float fs, BiquadCoeffs_TypeDef *coeffs)
{
  float A, omega, sn, cs, alpha, beta;
  
  /* Limit parameters to safe values */
  if (Q > PEQ_MAX_Q_FACTOR) Q = PEQ_MAX_Q_FACTOR;
  if (centerFreq <= 0.0f) centerFreq = 20.0f;
  if (centerFreq >= fs/2.0f) centerFreq = fs/2.0f - 1.0f;
  
  /* Convert gain from dB to linear scale */
  A = powf(10.0f, gain / 40.0f); /* 40 instead of 20 because A is squared in some terms */
  
  /* Calculate intermediate values */
  omega = 2.0f * M_PI * centerFreq / fs;
  sn = sinf(omega);
  cs = cosf(omega);
  alpha = sn / (2.0f * Q);
  beta = sqrtf(A) / Q;
  
  /* Calculate coefficients */
  coeffs->b0 = 1.0f + alpha * A;
  coeffs->b1 = -2.0f * cs;
  coeffs->b2 = 1.0f - alpha * A;
  coeffs->a0 = 1.0f + alpha / A;
  coeffs->a1 = -2.0f * cs;
  coeffs->a2 = 1.0f - alpha / A;
  
  /* Normalize by a0 */
  coeffs->b0 /= coeffs->a0;
  coeffs->b1 /= coeffs->a0;
  coeffs->b2 /= coeffs->a0;
  coeffs->a1 /= coeffs->a0;
  coeffs->a2 /= coeffs->a0;
  coeffs->a0 = 1.0f;
}

/**
  * @brief  Calculate coefficients for Low Shelf filter
  * @param  cutoffFreq: Cutoff frequency in Hz
  * @param  gain: Gain in dB
  * @param  Q: Q-factor (slope control)
  * @param  fs: Sample rate in Hz
  * @param  coeffs: Pointer to store calculated coefficients
  * @retval None
  */
static void PEQ_CalculateLowShelf(float cutoffFreq, float gain, float Q, float fs, BiquadCoeffs_TypeDef *coeffs)
{
  float A, omega, sn, cs, alpha, beta;
  
  /* Limit parameters to safe values */
  if (Q > PEQ_MAX_Q_FACTOR) Q = PEQ_MAX_Q_FACTOR;
  if (cutoffFreq <= 0.0f) cutoffFreq = 20.0f;
  if (cutoffFreq >= fs/2.0f) cutoffFreq = fs/2.0f - 1.0f;
  
  /* Convert gain from dB to linear scale */
  A = powf(10.0f, gain / 40.0f);
  
  /* Calculate intermediate values */
  omega = 2.0f * M_PI * cutoffFreq / fs;
  sn = sinf(omega);
  cs = cosf(omega);
  alpha = sn / (2.0f * Q);
  beta = 2.0f * sqrtf(A) * alpha;
  
  /* Calculate coefficients */
  coeffs->b0 = A * ((A + 1.0f) - (A - 1.0f) * cs + beta);
  coeffs->b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cs);
  coeffs->b2 = A * ((A + 1.0f) - (A - 1.0f) * cs - beta);
  coeffs->a0 = (A + 1.0f) + (A - 1.0f) * cs + beta;
  coeffs->a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cs);
  coeffs->a2 = (A + 1.0f) + (A - 1.0f) * cs - beta;
  
  /* Normalize by a0 */
  coeffs->b0 /= coeffs->a0;
  coeffs->b1 /= coeffs->a0;
  coeffs->b2 /= coeffs->a0;
  coeffs->a1 /= coeffs->a0;
  coeffs->a2 /= coeffs->a0;
  coeffs->a0 = 1.0f;
}

/**
  * @brief  Calculate coefficients for High Shelf filter
  * @param  cutoffFreq: Cutoff frequency in Hz
  * @param  gain: Gain in dB
  * @param  Q: Q-factor (slope control)
  * @param  fs: Sample rate in Hz
  * @param  coeffs: Pointer to store calculated coefficients
  * @retval None
  */
static void PEQ_CalculateHighShelf(float cutoffFreq, float gain, float Q, float fs, BiquadCoeffs_TypeDef *coeffs)
{
  float A, omega, sn, cs, alpha, beta;
  
  /* Limit parameters to safe values */
  if (Q > PEQ_MAX_Q_FACTOR) Q = PEQ_MAX_Q_FACTOR;
  if (cutoffFreq <= 0.0f) cutoffFreq = 20.0f;
  if (cutoffFreq >= fs/2.0f) cutoffFreq = fs/2.0f - 1.0f;
  
  /* Convert gain from dB to linear scale */
  A = powf(10.0f, gain / 40.0f);
  
  /* Calculate intermediate values */
  omega = 2.0f * M_PI * cutoffFreq / fs;
  sn = sinf(omega);
  cs = cosf(omega);
  alpha = sn / (2.0f * Q);
  beta = 2.0f * sqrtf(A) * alpha;
  
  /* Calculate coefficients */
  coeffs->b0 = A * ((A + 1.0f) + (A - 1.0f) * cs + beta);
  coeffs->b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs);
  coeffs->b2 = A * ((A + 1.0f) + (A - 1.0f) * cs - beta);
  coeffs->a0 = (A + 1.0f) - (A - 1.0f) * cs + beta;
  coeffs->a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cs);
  coeffs->a2 = (A + 1.0f) - (A - 1.0f) * cs - beta;
  
  /* Normalize by a0 */
  coeffs->b0 /= coeffs->a0;
  coeffs->b1 /= coeffs->a0;
  coeffs->b2 /= coeffs->a0;
  coeffs->a1 /= coeffs->a0;
  coeffs->a2 /= coeffs->a0;
  coeffs->a0 = 1.0f;
}

/**
  * @brief  Calculate coefficients for Low Pass filter
  * @param  cutoffFreq: Cutoff frequency in Hz
  * @param  Q: Q-factor (resonance control)
  * @param  fs: Sample rate in Hz
  * @param  coeffs: Pointer to store calculated coefficients
  * @retval None
  */
static void PEQ_CalculateLowPass(float cutoffFreq, float Q, float fs, BiquadCoeffs_TypeDef *coeffs)
{
  float omega, sn, cs, alpha;
  
  /* Limit parameters to safe values */
  if (Q > PEQ_MAX_Q_FACTOR) Q = PEQ_MAX_Q_FACTOR;
  if (cutoffFreq <= 0.0f) cutoffFreq = 20.0f;
  if (cutoffFreq >= fs/2.0f) cutoffFreq = fs/2.0f - 1.0f;
  
  /* Calculate intermediate values */
  omega = 2.0f * M_PI * cutoffFreq / fs;
  sn = sinf(omega);
  cs = cosf(omega);
  alpha = sn / (2.0f * Q);
  
  /* Calculate coefficients */
  coeffs->b0 = (1.0f - cs) / 2.0f;
  coeffs->b1 = 1.0f - cs;
  coeffs->b2 = (1.0f - cs) / 2.0f;
  coeffs->a0 = 1.0f + alpha;
  coeffs->a1 = -2.0f * cs;
  coeffs->a2 = 1.0f - alpha;
  
  /* Normalize by a0 */
  coeffs->b0 /= coeffs->a0;
  coeffs->b1 /= coeffs->a0;
  coeffs->b2 /= coeffs->a0;
  coeffs->a1 /= coeffs->a0;
  coeffs->a2 /= coeffs->a0;
  coeffs->a0 = 1.0f;
}

/**
  * @brief  Calculate coefficients for High Pass filter
  * @param  cutoffFreq: Cutoff frequency in Hz
  * @param  Q: Q-factor (resonance control)
  * @param  fs: Sample rate in Hz
  * @param  coeffs: Pointer to store calculated coefficients
  * @retval None
  */
static void PEQ_CalculateHighPass(float cutoffFreq, float Q, float fs, BiquadCoeffs_TypeDef *coeffs)
{
  float omega, sn, cs, alpha;
  
  /* Limit parameters to safe values */
  if (Q > PEQ_MAX_Q_FACTOR) Q = PEQ_MAX_Q_FACTOR;
  if (cutoffFreq <= 0.0f) cutoffFreq = 20.0f;
  if (cutoffFreq >= fs/2.0f) cutoffFreq = fs/2.0f - 1.0f;
  
  /* Calculate intermediate values */
  omega = 2.0f * M_PI * cutoffFreq / fs;
  sn = sinf(omega);
  cs = cosf(omega);
  alpha = sn / (2.0f * Q);
  
  /* Calculate coefficients */
  coeffs->b0 = (1.0f + cs) / 2.0f;
  coeffs->b1 = -(1.0f + cs);
  coeffs->b2 = (1.0f + cs) / 2.0f;
  coeffs->a0 = 1.0f + alpha;
  coeffs->a1 = -2.0f * cs;
  coeffs->a2 = 1.0f - alpha;
  
  /* Normalize by a0 */
  coeffs->b0 /= coeffs->a0;
  coeffs->b1 /= coeffs->a0;
  coeffs->b2 /= coeffs->a0;
  coeffs->a1 /= coeffs->a0;
  coeffs->a2 /= coeffs->a0;
  coeffs->a0 = 1.0f;
}