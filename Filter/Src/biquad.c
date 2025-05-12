/**
  ******************************************************************************
  * @file           : biquad.c
  * @brief          : Biquad filter implementation
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Biquad filter library for STM32F411 Audio DSP Crossover
  * Implements various IIR filter types (low pass, high pass, band pass,
  * notch, peak, low shelf, high shelf) using cascaded biquad filters
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "biquad.h"
#include "math_utils.h"  // For dB conversion utilities

/* Private defines -----------------------------------------------------------*/
#define PI 3.14159265358979323846f

/* Private function prototypes -----------------------------------------------*/
static float Biquad_DirectFormII(BiquadState_t *state, float input);

/**
 * @brief  Initialize a biquad filter with the specified configuration
 * @param  state: Pointer to the biquad filter state structure
 * @param  config: Pointer to the biquad filter configuration
 * @retval None
 */
void Biquad_Init(BiquadState_t *state, const BiquadConfig_t *config)
{
  /* Clear delay lines */
  Biquad_Reset(state);
  
  /* Calculate coefficients based on configuration */
  Biquad_CalculateCoefficients(state, config);
}

/**
 * @brief  Reset biquad filter state (clear delay lines)
 * @param  state: Pointer to the biquad filter state structure
 * @retval None
 */
void Biquad_Reset(BiquadState_t *state)
{
  /* Reset filter state/history */
  state->x1 = 0.0f;
  state->x2 = 0.0f;
  state->y1 = 0.0f;
  state->y2 = 0.0f;
}

/**
 * @brief  Calculate biquad filter coefficients for the specified configuration
 * @param  state: Pointer to the biquad filter state structure
 * @param  config: Pointer to the biquad filter configuration
 * @retval None
 */
void Biquad_CalculateCoefficients(BiquadState_t *state, const BiquadConfig_t *config)
{
  float omega, sinOmega, cosOmega;
  float alpha, beta, A, sqrtA;
  
  /* Constraint check to avoid division by zero */
  if (config->sampleRate <= 0.0f) {
    return;
  }
  
  /* Calculate intermediate values */
  omega = 2.0f * PI * config->frequency / config->sampleRate;
  sinOmega = sinf(omega);
  cosOmega = cosf(omega);
  
  /* Alpha value based on Q factor */
  alpha = sinOmega / (2.0f * config->Q);
  
  /* Calculate coefficients based on filter type */
  switch (config->type) {
    case BIQUAD_TYPE_LPF:  /* Low Pass Filter */
      state->b0 = (1.0f - cosOmega) / 2.0f;
      state->b1 = 1.0f - cosOmega;
      state->b2 = (1.0f - cosOmega) / 2.0f;
      state->a1 = -2.0f * cosOmega;
      state->a2 = 1.0f - alpha;
      break;
      
    case BIQUAD_TYPE_HPF:  /* High Pass Filter */
      state->b0 = (1.0f + cosOmega) / 2.0f;
      state->b1 = -(1.0f + cosOmega);
      state->b2 = (1.0f + cosOmega) / 2.0f;
      state->a1 = -2.0f * cosOmega;
      state->a2 = 1.0f - alpha;
      break;
      
    case BIQUAD_TYPE_BPF:  /* Band Pass Filter */
      state->b0 = alpha;
      state->b1 = 0.0f;
      state->b2 = -alpha;
      state->a1 = -2.0f * cosOmega;
      state->a2 = 1.0f - alpha;
      break;
      
    case BIQUAD_TYPE_NOTCH:  /* Notch Filter */
      state->b0 = 1.0f;
      state->b1 = -2.0f * cosOmega;
      state->b2 = 1.0f;
      state->a1 = -2.0f * cosOmega;
      state->a2 = 1.0f - alpha;
      break;
      
    case BIQUAD_TYPE_PEAK:  /* Peaking EQ Filter */
      A = powf(10.0f, config->gainDB / 40.0f);  // Convert dB to linear
      state->b0 = 1.0f + alpha * A;
      state->b1 = -2.0f * cosOmega;
      state->b2 = 1.0f - alpha * A;
      state->a1 = -2.0f * cosOmega;
      state->a2 = 1.0f - alpha / A;
      break;
      
    case BIQUAD_TYPE_LOWSHELF:  /* Low Shelf Filter */
      A = powf(10.0f, config->gainDB / 40.0f);  // Convert dB to linear
      sqrtA = sqrtf(A);
      beta = 2.0f * sqrtA * alpha;
      
      state->b0 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega + beta);
      state->b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosOmega);
      state->b2 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega - beta);
      state->a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosOmega);
      state->a2 = (A + 1.0f) + (A - 1.0f) * cosOmega - beta;
      break;
      
    case BIQUAD_TYPE_HIGHSHELF:  /* High Shelf Filter */
      A = powf(10.0f, config->gainDB / 40.0f);  // Convert dB to linear
      sqrtA = sqrtf(A);
      beta = 2.0f * sqrtA * alpha;
      
      state->b0 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega + beta);
      state->b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosOmega);
      state->b2 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega - beta);
      state->a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosOmega);
      state->a2 = (A + 1.0f) - (A - 1.0f) * cosOmega - beta;
      break;
      
    default:
      // Invalid filter type, set as passthrough
      state->b0 = 1.0f;
      state->b1 = 0.0f;
      state->b2 = 0.0f;
      state->a1 = 0.0f;
      state->a2 = 0.0f;
      break;
  }
  
  /* Normalize coefficients to a0 = 1.0 */
  float a0 = 1.0f + alpha;
  state->b0 /= a0;
  state->b1 /= a0;
  state->b2 /= a0;
  state->a1 /= a0;
  state->a2 /= a0;
}

/**
 * @brief  Process a single sample through the biquad filter
 * @param  state: Pointer to the biquad filter state structure
 * @param  input: Input sample
 * @retval float: Filtered output sample
 */
float Biquad_ProcessSample(BiquadState_t *state, float input)
{
  return Biquad_DirectFormII(state, input);
}

/**
 * @brief  DirectForm II implementation of biquad filter
 * @note   This implementation minimizes the number of state variables and memory usage
 * @param  state: Pointer to the biquad filter state structure
 * @param  input: Input sample
 * @retval float: Filtered output sample
 */
static float Biquad_DirectFormII(BiquadState_t *state, float input)
{
  float w0, output;
  
  /* Direct Form II implementation */
  w0 = input - state->a1 * state->y1 - state->a2 * state->y2;
  output = state->b0 * w0 + state->b1 * state->y1 + state->b2 * state->y2;
  
  /* Update delay lines */
  state->y2 = state->y1;
  state->y1 = w0;
  
  return output;
}

/**
 * @brief  Process a block of samples through the biquad filter
 * @param  state: Pointer to the biquad filter state structure
 * @param  input: Pointer to input buffer
 * @param  output: Pointer to output buffer
 * @param  blockSize: Number of samples to process
 * @retval None
 */
void Biquad_ProcessBlock(BiquadState_t *state, const float *input, float *output, uint32_t blockSize)
{
  uint32_t i;
  
  /* Process each sample in the block */
  for (i = 0; i < blockSize; i++) {
    output[i] = Biquad_ProcessSample(state, input[i]);
  }
}

/**
 * @brief  Create a chain of biquad filters for higher order filters
 * @param  chain: Pointer to array of biquad states
 * @param  config: Base configuration for all filters in chain
 * @param  order: Filter order (determines number of biquads required)
 * @retval uint8_t: Number of biquads created in the chain
 */
uint8_t Biquad_CreateFilterChain(BiquadState_t *chain, const BiquadConfig_t *config, uint8_t order)
{
  uint8_t numStages;
  uint8_t i;
  BiquadConfig_t stageConfig;
  
  /* Calculate number of biquad stages needed */
  numStages = (order + 1) / 2;  // Ceiling division for odd orders
  
  /* Initialize each biquad in the chain with the same base configuration */
  for (i = 0; i < numStages; i++) {
    /* Copy the base configuration */
    stageConfig = *config;
    
    /* For multi-stage filters, Q factor needs adjustment based on filter design */
    /* This is a simplified approach - actual implementation should use proper filter design methods */
    if (numStages > 1) {
      /* Adjust Q based on Butterworth design (simplified) */
      stageConfig.Q = 1.0f / (2.0f * sinf(PI / (order * 2.0f)));
    }
    
    /* Initialize this stage */
    Biquad_Init(&chain[i], &stageConfig);
  }
  
  return numStages;
}

/**
 * @brief  Process a sample through a chain of biquad filters
 * @param  chain: Pointer to array of biquad states
 * @param  numStages: Number of biquad stages in the chain
 * @param  input: Input sample
 * @retval float: Filtered output sample
 */
float Biquad_ProcessChain(BiquadState_t *chain, uint8_t numStages, float input)
{
  float sample = input;
  uint8_t i;
  
  /* Process sample through each biquad in the chain */
  for (i = 0; i < numStages; i++) {
    sample = Biquad_ProcessSample(&chain[i], sample);
  }
  
  return sample;
}

/**
 * @brief  Get the frequency response of a biquad filter at a specific frequency
 * @param  state: Pointer to the biquad filter state structure
 * @param  frequency: Frequency at which to calculate response (Hz)
 * @param  sampleRate: Sample rate in Hz
 * @retval float: Magnitude response in dB
 */
float Biquad_GetFrequencyResponse(const BiquadState_t *state, float frequency, float sampleRate)
{
  float omega, sinOmega, cosOmega;
  float realNum, imagNum, realDen, imagDen;
  float real, imag, magnitude;
  
  /* Calculate frequency in radians */
  omega = 2.0f * PI * frequency / sampleRate;
  sinOmega = sinf(omega);
  cosOmega = cosf(omega);
  
  /* Calculate real and imaginary parts of numerator */
  realNum = state->b0 + state->b1 * cosOmega + state->b2 * cosf(2.0f * omega);
  imagNum = -(state->b1 * sinOmega + state->b2 * sinf(2.0f * omega));
  
  /* Calculate real and imaginary parts of denominator */
  realDen = 1.0f + state->a1 * cosOmega + state->a2 * cosf(2.0f * omega);
  imagDen = -(state->a1 * sinOmega + state->a2 * sinf(2.0f * omega));
  
  /* Complex division */
  real = (realNum * realDen + imagNum * imagDen) / (realDen * realDen + imagDen * imagDen);
  imag = (imagNum * realDen - realNum * imagDen) / (realDen * realDen + imagDen * imagDen);
  
  /* Magnitude (absolute value of complex number) */
  magnitude = sqrtf(real * real + imag * imag);
  
  /* Convert to dB */
  return 20.0f * log10f(magnitude);
}

/**
 * @brief  Get the phase response of a biquad filter at a specific frequency
 * @param  state: Pointer to the biquad filter state structure
 * @param  frequency: Frequency at which to calculate response (Hz)
 * @param  sampleRate: Sample rate in Hz
 * @retval float: Phase response in radians
 */
float Biquad_GetPhaseResponse(const BiquadState_t *state, float frequency, float sampleRate)
{
  float omega, sinOmega, cosOmega;
  float realNum, imagNum, realDen, imagDen;
  float real, imag;
  
  /* Calculate frequency in radians */
  omega = 2.0f * PI * frequency / sampleRate;
  sinOmega = sinf(omega);
  cosOmega = cosf(omega);
  
  /* Calculate real and imaginary parts of numerator */
  realNum = state->b0 + state->b1 * cosOmega + state->b2 * cosf(2.0f * omega);
  imagNum = -(state->b1 * sinOmega + state->b2 * sinf(2.0f * omega));
  
  /* Calculate real and imaginary parts of denominator */
  realDen = 1.0f + state->a1 * cosOmega + state->a2 * cosf(2.0f * omega);
  imagDen = -(state->a1 * sinOmega + state->a2 * sinf(2.0f * omega));
  
  /* Complex division */
  real = (realNum * realDen + imagNum * imagDen) / (realDen * realDen + imagDen * imagDen);
  imag = (imagNum * realDen - realNum * imagDen) / (realDen * realDen + imagDen * imagDen);
  
  /* Phase (arctan of imag/real) */
  return atan2f(imag, real);
}