/**
  ******************************************************************************
  * @file           : peq_init.c
  * @brief          : Parametric EQ initialization implementation
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Implementation of parametric equalizer initialization functions for the
  * DSP Audio Crossover system. Handles parameter setup and filter coefficient
  * calculations.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "peq.h"
#include "peq_types.h"
#include "dsp_common.h"
#include "math_utils.h"
#include "biquad.h"
#include "debug.h"

/* Private defines -----------------------------------------------------------*/
#define PEQ_MINIMAL_Q         0.1f
#define PEQ_MAXIMAL_Q         10.0f
#define PEQ_MINIMAL_GAIN      -12.0f
#define PEQ_MAXIMAL_GAIN      12.0f
#define PEQ_MINIMAL_FREQ      20.0f
#define PEQ_MAXIMAL_FREQ      20000.0f

/* Private variables ---------------------------------------------------------*/
static PEQ_Instance_TypeDef peqInstances[AUDIO_OUTPUT_CHANNELS][PEQ_BANDS_PER_CHANNEL];
static uint8_t peqInitialized = 0;

/* Private function prototypes -----------------------------------------------*/
static void PEQ_SetDefaultParams(uint8_t channel, uint8_t band);
static void PEQ_CalculateCoefficients(PEQ_Instance_TypeDef *instance);
static void PEQ_ValidateParams(PEQ_Instance_TypeDef *instance);

/* Functions -----------------------------------------------------------------*/

/**
  * @brief  Initialize all PEQ instances for all channels
  * @param  None
  * @retval DSP_StatusTypeDef status
  */
DSP_StatusTypeDef PEQ_Init(void)
{
  if (peqInitialized) {
    DEBUG_PRINT("[PEQ] Already initialized\r\n");
    return DSP_OK;
  }
  
  DEBUG_PRINT("[PEQ] Initializing parametric EQ module\r\n");
  
  /* Initialize all channels and bands with default parameters */
  for (uint8_t channel = 0; channel < AUDIO_OUTPUT_CHANNELS; channel++) {
    for (uint8_t band = 0; band < PEQ_BANDS_PER_CHANNEL; band++) {
      /* Reset states */
      peqInstances[channel][band].state.x1 = 0.0f;
      peqInstances[channel][band].state.x2 = 0.0f;
      peqInstances[channel][band].state.y1 = 0.0f;
      peqInstances[channel][band].state.y2 = 0.0f;
      
      /* Set default parameters */
      PEQ_SetDefaultParams(channel, band);
      
      /* Calculate initial coefficients */
      PEQ_CalculateCoefficients(&peqInstances[channel][band]);
      
      DEBUG_PRINT("[PEQ] Channel %d Band %d initialized\r\n", channel, band);
    }
  }
  
  peqInitialized = 1;
  
  DEBUG_PRINT("[PEQ] Module initialization complete\r\n");
  return DSP_OK;
}

/**
  * @brief  Reset a single PEQ instance
  * @param  channel: Output channel number
  * @param  band: EQ band number
  * @retval DSP_StatusTypeDef status
  */
DSP_StatusTypeDef PEQ_ResetBand(uint8_t channel, uint8_t band)
{
  if (!peqInitialized) {
    DEBUG_PRINT("[PEQ] Module not initialized\r\n");
    return DSP_ERROR;
  }
  
  if (channel >= AUDIO_OUTPUT_CHANNELS || band >= PEQ_BANDS_PER_CHANNEL) {
    DEBUG_PRINT("[PEQ] Invalid channel or band index\r\n");
    return DSP_ERROR;
  }
  
  /* Reset states */
  peqInstances[channel][band].state.x1 = 0.0f;
  peqInstances[channel][band].state.x2 = 0.0f;
  peqInstances[channel][band].state.y1 = 0.0f;
  peqInstances[channel][band].state.y2 = 0.0f;
  
  DEBUG_PRINT("[PEQ] Channel %d Band %d states reset\r\n", channel, band);
  
  return DSP_OK;
}

/**
  * @brief  Reset all PEQ instances for a channel
  * @param  channel: Output channel number
  * @retval DSP_StatusTypeDef status
  */
DSP_StatusTypeDef PEQ_ResetChannel(uint8_t channel)
{
  if (!peqInitialized) {
    DEBUG_PRINT("[PEQ] Module not initialized\r\n");
    return DSP_ERROR;
  }
  
  if (channel >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("[PEQ] Invalid channel index\r\n");
    return DSP_ERROR;
  }
  
  for (uint8_t band = 0; band < PEQ_BANDS_PER_CHANNEL; band++) {
    PEQ_ResetBand(channel, band);
  }
  
  DEBUG_PRINT("[PEQ] All bands for channel %d reset\r\n", channel);
  
  return DSP_OK;
}

/**
  * @brief  Configure PEQ band parameters
  * @param  channel: Output channel number
  * @param  band: EQ band number
  * @param  config: Pointer to configuration parameters
  * @retval DSP_StatusTypeDef status
  */
DSP_StatusTypeDef PEQ_ConfigureBand(uint8_t channel, uint8_t band, PEQ_Config_TypeDef *config)
{
  if (!peqInitialized) {
    DEBUG_PRINT("[PEQ] Module not initialized\r\n");
    return DSP_ERROR;
  }
  
  if (channel >= AUDIO_OUTPUT_CHANNELS || band >= PEQ_BANDS_PER_CHANNEL || config == NULL) {
    DEBUG_PRINT("[PEQ] Invalid parameters\r\n");
    return DSP_ERROR;
  }
  
  /* Copy configuration parameters */
  peqInstances[channel][band].config.type = config->type;
  peqInstances[channel][band].config.frequency = config->frequency;
  peqInstances[channel][band].config.gain = config->gain;
  peqInstances[channel][band].config.q = config->q;
  peqInstances[channel][band].config.enabled = config->enabled;
  
  /* Validate parameters */
  PEQ_ValidateParams(&peqInstances[channel][band]);
  
  /* Calculate coefficients with new parameters */
  PEQ_CalculateCoefficients(&peqInstances[channel][band]);
  
  DEBUG_PRINT("[PEQ] Channel %d Band %d configured: Type=%d, F=%dHz, G=%.1fdB, Q=%.2f\r\n", 
              channel, band, 
              config->type, 
              (int)config->frequency, 
              config->gain, 
              config->q);
  
  return DSP_OK;
}

/**
  * @brief  Enable/disable a specific PEQ band
  * @param  channel: Output channel number
  * @param  band: EQ band number
  * @param  enabled: 1 to enable, 0 to disable
  * @retval DSP_StatusTypeDef status
  */
DSP_StatusTypeDef PEQ_SetBandEnabled(uint8_t channel, uint8_t band, uint8_t enabled)
{
  if (!peqInitialized) {
    DEBUG_PRINT("[PEQ] Module not initialized\r\n");
    return DSP_ERROR;
  }
  
  if (channel >= AUDIO_OUTPUT_CHANNELS || band >= PEQ_BANDS_PER_CHANNEL) {
    DEBUG_PRINT("[PEQ] Invalid channel or band index\r\n");
    return DSP_ERROR;
  }
  
  /* Update enabled status */
  peqInstances[channel][band].config.enabled = enabled ? 1 : 0;
  
  DEBUG_PRINT("[PEQ] Channel %d Band %d %s\r\n", 
              channel, band, 
              enabled ? "enabled" : "disabled");
  
  return DSP_OK;
}

/**
  * @brief  Get current PEQ configuration parameters
  * @param  channel: Output channel number
  * @param  band: EQ band number
  * @param  config: Pointer to store configuration
  * @retval DSP_StatusTypeDef status
  */
DSP_StatusTypeDef PEQ_GetBandConfig(uint8_t channel, uint8_t band, PEQ_Config_TypeDef *config)
{
  if (!peqInitialized) {
    DEBUG_PRINT("[PEQ] Module not initialized\r\n");
    return DSP_ERROR;
  }
  
  if (channel >= AUDIO_OUTPUT_CHANNELS || band >= PEQ_BANDS_PER_CHANNEL || config == NULL) {
    DEBUG_PRINT("[PEQ] Invalid parameters\r\n");
    return DSP_ERROR;
  }
  
  /* Copy configuration parameters */
  config->type = peqInstances[channel][band].config.type;
  config->frequency = peqInstances[channel][band].config.frequency;
  config->gain = peqInstances[channel][band].config.gain;
  config->q = peqInstances[channel][band].config.q;
  config->enabled = peqInstances[channel][band].config.enabled;
  
  return DSP_OK;
}

/**
  * @brief  Get PEQ instance pointer for direct access
  * @param  channel: Output channel number
  * @param  band: EQ band number
  * @retval Pointer to PEQ instance or NULL if invalid
  */
PEQ_Instance_TypeDef* PEQ_GetInstance(uint8_t channel, uint8_t band)
{
  if (!peqInitialized || channel >= AUDIO_OUTPUT_CHANNELS || band >= PEQ_BANDS_PER_CHANNEL) {
    return NULL;
  }
  
  return &peqInstances[channel][band];
}

/* Private Functions ---------------------------------------------------------*/

/**
  * @brief  Set default parameters for a PEQ band
  * @param  channel: Output channel number
  * @param  band: EQ band number
  * @retval None
  */
static void PEQ_SetDefaultParams(uint8_t channel, uint8_t band)
{
  PEQ_Instance_TypeDef *instance = &peqInstances[channel][band];
  
  /* Default parameters based on band number */
  switch (band) {
    case 0: /* Low shelf */
      instance->config.type = PEQ_TYPE_LOW_SHELF;
      instance->config.frequency = 80.0f;
      instance->config.gain = 0.0f;
      instance->config.q = 0.707f;
      break;
      
    case 1: /* Low-mid bell */
      instance->config.type = PEQ_TYPE_BELL;
      instance->config.frequency = 250.0f;
      instance->config.gain = 0.0f;
      instance->config.q = 1.0f;
      break;
      
    case 2: /* Mid bell */
      instance->config.type = PEQ_TYPE_BELL;
      instance->config.frequency = 1000.0f;
      instance->config.gain = 0.0f;
      instance->config.q = 1.0f;
      break;
      
    case 3: /* High-mid bell */
      instance->config.type = PEQ_TYPE_BELL;
      instance->config.frequency = 3500.0f;
      instance->config.gain = 0.0f;
      instance->config.q = 1.0f;
      break;
      
    case 4: /* High shelf */
      instance->config.type = PEQ_TYPE_HIGH_SHELF;
      instance->config.frequency = 10000.0f;
      instance->config.gain = 0.0f;
      instance->config.q = 0.707f;
      break;
      
    default:
      instance->config.type = PEQ_TYPE_BELL;
      instance->config.frequency = 1000.0f;
      instance->config.gain = 0.0f;
      instance->config.q = 1.0f;
      break;
  }
  
  /* By default, all bands are enabled */
  instance->config.enabled = 1;
}

/**
  * @brief  Validate PEQ parameters to ensure they are within acceptable ranges
  * @param  instance: Pointer to PEQ instance
  * @retval None
  */
static void PEQ_ValidateParams(PEQ_Instance_TypeDef *instance)
{
  /* Clamp frequency to valid range */
  if (instance->config.frequency < PEQ_MINIMAL_FREQ) {
    instance->config.frequency = PEQ_MINIMAL_FREQ;
  } else if (instance->config.frequency > PEQ_MAXIMAL_FREQ) {
    instance->config.frequency = PEQ_MAXIMAL_FREQ;
  }
  
  /* Clamp Q to valid range */
  if (instance->config.q < PEQ_MINIMAL_Q) {
    instance->config.q = PEQ_MINIMAL_Q;
  } else if (instance->config.q > PEQ_MAXIMAL_Q) {
    instance->config.q = PEQ_MAXIMAL_Q;
  }
  
  /* Clamp gain to valid range */
  if (instance->config.gain < PEQ_MINIMAL_GAIN) {
    instance->config.gain = PEQ_MINIMAL_GAIN;
  } else if (instance->config.gain > PEQ_MAXIMAL_GAIN) {
    instance->config.gain = PEQ_MAXIMAL_GAIN;
  }
  
  /* Ensure type is valid */
  if (instance->config.type >= PEQ_TYPE_COUNT) {
    instance->config.type = PEQ_TYPE_BELL; /* Default to bell filter */
  }
}

/**
  * @brief  Calculate biquad filter coefficients for the PEQ
  * @param  instance: Pointer to PEQ instance
  * @retval None
  */
static void PEQ_CalculateCoefficients(PEQ_Instance_TypeDef *instance)
{
  float fs = AUDIO_SAMPLE_RATE;
  float f0 = instance->config.frequency;
  float Q = instance->config.q;
  float gainDB = instance->config.gain;
  float A = powf(10.0f, gainDB / 40.0f); /* Convert dB to linear amplitude */
  
  /* Angular frequency */
  float w0 = 2.0f * PI * f0 / fs;
  float alpha = sinf(w0) / (2.0f * Q);
  float cosw0 = cosf(w0);
  
  /* Calculate coefficients based on filter type */
  switch (instance->config.type) {
    case PEQ_TYPE_LOW_SHELF:
      /* Low Shelf filter */
      {
        float sqrtA = sqrtf(A);
        float b0 = A * ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha);
        float b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0);
        float b2 = A * ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha);
        float a0 = (A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha;
        float a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0);
        float a2 = (A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha;
        
        /* Normalize by a0 */
        instance->coef.b0 = b0 / a0;
        instance->coef.b1 = b1 / a0;
        instance->coef.b2 = b2 / a0;
        instance->coef.a1 = a1 / a0;
        instance->coef.a2 = a2 / a0;
      }
      break;
      
    case PEQ_TYPE_HIGH_SHELF:
      /* High Shelf filter */
      {
        float sqrtA = sqrtf(A);
        float b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha);
        float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0);
        float b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha);
        float a0 = (A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha;
        float a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0);
        float a2 = (A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha;
        
        /* Normalize by a0 */
        instance->coef.b0 = b0 / a0;
        instance->coef.b1 = b1 / a0;
        instance->coef.b2 = b2 / a0;
        instance->coef.a1 = a1 / a0;
        instance->coef.a2 = a2 / a0;
      }
      break;
      
    case PEQ_TYPE_LOW_PASS:
      /* Low Pass filter */
      {
        float b0 = (1.0f - cosw0) / 2.0f;
        float b1 = 1.0f - cosw0;
        float b2 = (1.0f - cosw0) / 2.0f;
        float a0 = 1.0f + alpha;
        float a1 = -2.0f * cosw0;
        float a2 = 1.0f - alpha;
        
        /* Normalize by a0 */
        instance->coef.b0 = b0 / a0;
        instance->coef.b1 = b1 / a0;
        instance->coef.b2 = b2 / a0;
        instance->coef.a1 = a1 / a0;
        instance->coef.a2 = a2 / a0;
      }
      break;
      
    case PEQ_TYPE_HIGH_PASS:
      /* High Pass filter */
      {
        float b0 = (1.0f + cosw0) / 2.0f;
        float b1 = -(1.0f + cosw0);
        float b2 = (1.0f + cosw0) / 2.0f;
        float a0 = 1.0f + alpha;
        float a1 = -2.0f * cosw0;
        float a2 = 1.0f - alpha;
        
        /* Normalize by a0 */
        instance->coef.b0 = b0 / a0;
        instance->coef.b1 = b1 / a0;
        instance->coef.b2 = b2 / a0;
        instance->coef.a1 = a1 / a0;
        instance->coef.a2 = a2 / a0;
      }
      break;
      
    case PEQ_TYPE_BELL:
    default:
      /* Bell (Peaking) filter */
      {
        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * cosw0;
        float b2 = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * cosw0;
        float a2 = 1.0f - alpha / A;
        
        /* Normalize by a0 */
        instance->coef.b0 = b0 / a0;
        instance->coef.b1 = b1 / a0;
        instance->coef.b2 = b2 / a0;
        instance->coef.a1 = a1 / a0;
        instance->coef.a2 = a2 / a0;
      }
      break;
  }
  
  /* If band is disabled, set to unity gain bypass */
  if (!instance->config.enabled) {
    instance->coef.b0 = 1.0f;
    instance->coef.b1 = 0.0f;
    instance->coef.b2 = 0.0f;
    instance->coef.a1 = 0.0f;
    instance->coef.a2 = 0.0f;
  }
}