/**
  ******************************************************************************
  * @file           : compressor_init.c
  * @brief          : Compressor initialization implementation
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Implementation of the compressor initialization for the DSP chain
  * Used in Panel Kontrol DSP STM32F411 untuk Sistem Audio Crossover Aktif
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "compressor.h"
#include "compressor_types.h"
#include "math_utils.h"
#include "debug.h"
#include <string.h>
#include <math.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define COMPRESSOR_DEFAULT_THRESHOLD   -20.0f  /* Default threshold in dB */
#define COMPRESSOR_DEFAULT_RATIO       4.0f    /* Default ratio (e.g., 4:1) */
#define COMPRESSOR_DEFAULT_ATTACK      10.0f   /* Default attack time in ms */
#define COMPRESSOR_DEFAULT_RELEASE     100.0f  /* Default release time in ms */
#define COMPRESSOR_DEFAULT_KNEE_WIDTH  6.0f    /* Default soft knee width in dB */
#define COMPRESSOR_DEFAULT_MAKEUP_GAIN 0.0f    /* Default makeup gain in dB */
#define COMPRESSOR_MIN_THRESHOLD       -60.0f  /* Minimum threshold value in dB */
#define COMPRESSOR_MAX_THRESHOLD       0.0f    /* Maximum threshold value in dB */
#define COMPRESSOR_MIN_RATIO           1.0f    /* Minimum ratio value */
#define COMPRESSOR_MAX_RATIO           20.0f   /* Maximum ratio value */
#define COMPRESSOR_MIN_ATTACK          0.1f    /* Minimum attack time in ms */
#define COMPRESSOR_MAX_ATTACK          100.0f  /* Maximum attack time in ms */
#define COMPRESSOR_MIN_RELEASE         10.0f   /* Minimum release time in ms */
#define COMPRESSOR_MAX_RELEASE         1000.0f /* Maximum release time in ms */
#define COMPRESSOR_MIN_KNEE            0.0f    /* Minimum knee width (hard knee) */
#define COMPRESSOR_MAX_KNEE            12.0f   /* Maximum knee width in dB */
#define COMPRESSOR_MIN_MAKEUP          -12.0f  /* Minimum makeup gain in dB */
#define COMPRESSOR_MAX_MAKEUP          12.0f   /* Maximum makeup gain in dB */

/* Private variables ---------------------------------------------------------*/
static Compressor_State_TypeDef compressorState[AUDIO_OUTPUT_CHANNELS];
static Compressor_Params_TypeDef compressorParams[AUDIO_OUTPUT_CHANNELS];

/* Private function prototypes -----------------------------------------------*/
static void Compressor_CalculateCoefficients(uint8_t channelIndex);
static float Compressor_CalculateAttackCoefficient(float attackTimeMs, float sampleRate);
static float Compressor_CalculateReleaseCoefficient(float releaseTimeMs, float sampleRate);

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Calculate the attack coefficient for the compressor
  * @param  attackTimeMs: Attack time in milliseconds
  * @param  sampleRate: Audio sample rate
  * @retval Calculated attack coefficient
  */
static float Compressor_CalculateAttackCoefficient(float attackTimeMs, float sampleRate)
{
  /* Convert attack time from milliseconds to samples */
  float attackTimeSamples = (attackTimeMs * sampleRate) / 1000.0f;
  
  /* Calculate and return the attack coefficient */
  return exp(-1.0f / attackTimeSamples);
}

/**
  * @brief  Calculate the release coefficient for the compressor
  * @param  releaseTimeMs: Release time in milliseconds
  * @param  sampleRate: Audio sample rate
  * @retval Calculated release coefficient
  */
static float Compressor_CalculateReleaseCoefficient(float releaseTimeMs, float sampleRate)
{
  /* Convert release time from milliseconds to samples */
  float releaseTimeSamples = (releaseTimeMs * sampleRate) / 1000.0f;
  
  /* Calculate and return the release coefficient */
  return exp(-1.0f / releaseTimeSamples);
}

/**
  * @brief  Calculate all coefficients used by the compressor
  * @param  channelIndex: Index of the channel to calculate coefficients for
  * @retval None
  */
static void Compressor_CalculateCoefficients(uint8_t channelIndex)
{
  /* Validate channel index */
  if (channelIndex >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("Compressor_CalculateCoefficients: Invalid channel index %d\r\n", channelIndex);
    return;
  }
  
  Compressor_Params_TypeDef* params = &compressorParams[channelIndex];
  Compressor_State_TypeDef* state = &compressorState[channelIndex];
  
  /* Calculate threshold in linear scale */
  state->thresholdLin = DB_TO_LINEAR(params->thresholdDb);
  
  /* Calculate attack and release coefficients */
  state->attackCoeff = Compressor_CalculateAttackCoefficient(params->attackTimeMs, params->sampleRate);
  state->releaseCoeff = Compressor_CalculateReleaseCoefficient(params->releaseTimeMs, params->sampleRate);
  
  /* Calculate makeup gain in linear scale */
  state->makeupGainLin = DB_TO_LINEAR(params->makeupGainDb);
  
  /* Calculate knee-related parameters for soft knee */
  if (params->kneeWidthDb > 0.0f) {
    state->kneeStart = params->thresholdDb - (params->kneeWidthDb / 2.0f);
    state->kneeEnd = params->thresholdDb + (params->kneeWidthDb / 2.0f);
    state->kneeStartLin = DB_TO_LINEAR(state->kneeStart);
    state->kneeEndLin = DB_TO_LINEAR(state->kneeEnd);
    state->kneeWidthLin = state->kneeEndLin - state->kneeStartLin;
  } else {
    /* For hard knee, set knee width to 0 */
    state->kneeStart = params->thresholdDb;
    state->kneeEnd = params->thresholdDb;
    state->kneeStartLin = state->thresholdLin;
    state->kneeEndLin = state->thresholdLin;
    state->kneeWidthLin = 0.0f;
  }
  
  /* Calculate 1 - (1/ratio) for gain computation */
  state->ratioFactor = 1.0f - (1.0f / params->ratio);
  
  /* Update the compressor's internal state */
  state->calculatedGain = 1.0f;      /* Start with unity gain */
  state->currentEnvelope = 0.0f;     /* Reset envelope follower */
  state->isEnabled = params->enabled; /* Set enabled state */
  
  DEBUG_PRINT("Compressor[%d]: Coefficients calculated (T=%0.1fdB, R=%0.1f:1, A=%0.1fms, R=%0.1fms)\r\n", 
              channelIndex, params->thresholdDb, params->ratio, params->attackTimeMs, params->releaseTimeMs);
}

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Initialize the compressor module
  * @retval Status (0 if OK, -1 if error)
  */
int8_t Compressor_Init(void)
{
  DEBUG_PRINT("Compressor_Init: Initializing compressors for %d channels\r\n", AUDIO_OUTPUT_CHANNELS);
  
  /* Initialize all channels with default parameters */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    /* Clear structures */
    memset(&compressorParams[i], 0, sizeof(Compressor_Params_TypeDef));
    memset(&compressorState[i], 0, sizeof(Compressor_State_TypeDef));
    
    /* Set default parameters */
    compressorParams[i].thresholdDb = COMPRESSOR_DEFAULT_THRESHOLD;
    compressorParams[i].ratio = COMPRESSOR_DEFAULT_RATIO;
    compressorParams[i].attackTimeMs = COMPRESSOR_DEFAULT_ATTACK;
    compressorParams[i].releaseTimeMs = COMPRESSOR_DEFAULT_RELEASE;
    compressorParams[i].kneeWidthDb = COMPRESSOR_DEFAULT_KNEE_WIDTH;
    compressorParams[i].makeupGainDb = COMPRESSOR_DEFAULT_MAKEUP_GAIN;
    compressorParams[i].sampleRate = AUDIO_SAMPLE_RATE;
    compressorParams[i].enabled = 0; /* Disabled by default */
    
    /* Calculate initial coefficients */
    Compressor_CalculateCoefficients(i);
  }
  
  DEBUG_PRINT("Compressor_Init: Initialization complete\r\n");
  return 0;
}

/**
  * @brief  Reset the compressor module to default values
  * @param  channelIndex: Channel to reset (or ALL_CHANNELS for all)
  * @retval Status (0 if OK, -1 if error)
  */
int8_t Compressor_Reset(uint8_t channelIndex)
{
  DEBUG_PRINT("Compressor_Reset: Resetting channel %d\r\n", channelIndex);
  
  /* Check if we reset all channels or just one */
  uint8_t startChannel = (channelIndex == ALL_CHANNELS) ? 0 : channelIndex;
  uint8_t endChannel = (channelIndex == ALL_CHANNELS) ? AUDIO_OUTPUT_CHANNELS - 1 : channelIndex;
  
  /* Reset each specified channel */
  for (uint8_t i = startChannel; i <= endChannel; i++) {
    if (i >= AUDIO_OUTPUT_CHANNELS) {
      DEBUG_PRINT("Compressor_Reset: Invalid channel index %d\r\n", i);
      return -1;
    }
    
    /* Reset to default parameters */
    compressorParams[i].thresholdDb = COMPRESSOR_DEFAULT_THRESHOLD;
    compressorParams[i].ratio = COMPRESSOR_DEFAULT_RATIO;
    compressorParams[i].attackTimeMs = COMPRESSOR_DEFAULT_ATTACK;
    compressorParams[i].releaseTimeMs = COMPRESSOR_DEFAULT_RELEASE;
    compressorParams[i].kneeWidthDb = COMPRESSOR_DEFAULT_KNEE_WIDTH;
    compressorParams[i].makeupGainDb = COMPRESSOR_DEFAULT_MAKEUP_GAIN;
    compressorParams[i].enabled = 0; /* Disabled by default */
    
    /* Reset state variables */
    compressorState[i].currentEnvelope = 0.0f;
    compressorState[i].calculatedGain = 1.0f;
    
    /* Recalculate coefficients */
    Compressor_CalculateCoefficients(i);
  }
  
  return 0;
}

/**
  * @brief  Set compressor threshold for a channel
  * @param  channelIndex: Channel to set threshold for
  * @param  thresholdDb: Threshold value in dB
  * @retval Status (0 if OK, -1 if error)
  */
int8_t Compressor_SetThreshold(uint8_t channelIndex, float thresholdDb)
{
  /* Validate channel index */
  if (channelIndex >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("Compressor_SetThreshold: Invalid channel index %d\r\n", channelIndex);
    return -1;
  }
  
  /* Clamp threshold to valid range */
  thresholdDb = CLAMP(thresholdDb, COMPRESSOR_MIN_THRESHOLD, COMPRESSOR_MAX_THRESHOLD);
  
  /* Update parameter */
  compressorParams[channelIndex].thresholdDb = thresholdDb;
  
  /* Recalculate coefficients */
  Compressor_CalculateCoefficients(channelIndex);
  
  DEBUG_PRINT("Compressor[%d]: Set threshold to %0.1f dB\r\n", channelIndex, thresholdDb);
  return 0;
}

/**
  * @brief  Set compressor ratio for a channel
  * @param  channelIndex: Channel to set ratio for
  * @param  ratio: Ratio value (e.g., 4.0 for 4:1)
  * @retval Status (0 if OK, -1 if error)
  */
int8_t Compressor_SetRatio(uint8_t channelIndex, float ratio)
{
  /* Validate channel index */
  if (channelIndex >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("Compressor_SetRatio: Invalid channel index %d\r\n", channelIndex);
    return -1;
  }
  
  /* Clamp ratio to valid range */
  ratio = CLAMP(ratio, COMPRESSOR_MIN_RATIO, COMPRESSOR_MAX_RATIO);
  
  /* Update parameter */
  compressorParams[channelIndex].ratio = ratio;
  
  /* Recalculate coefficients */
  Compressor_CalculateCoefficients(channelIndex);
  
  DEBUG_PRINT("Compressor[%d]: Set ratio to %0.1f:1\r\n", channelIndex, ratio);
  return 0;
}

/**
  * @brief  Set compressor attack time for a channel
  * @param  channelIndex: Channel to set attack time for
  * @param  attackTimeMs: Attack time in milliseconds
  * @retval Status (0 if OK, -1 if error)
  */
int8_t Compressor_SetAttackTime(uint8_t channelIndex, float attackTimeMs)
{
  /* Validate channel index */
  if (channelIndex >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("Compressor_SetAttackTime: Invalid channel index %d\r\n", channelIndex);
    return -1;
  }
  
  /* Clamp attack time to valid range */
  attackTimeMs = CLAMP(attackTimeMs, COMPRESSOR_MIN_ATTACK, COMPRESSOR_MAX_ATTACK);
  
  /* Update parameter */
  compressorParams[channelIndex].attackTimeMs = attackTimeMs;
  
  /* Recalculate coefficients */
  Compressor_CalculateCoefficients(channelIndex);
  
  DEBUG_PRINT("Compressor[%d]: Set attack time to %0.1f ms\r\n", channelIndex, attackTimeMs);
  return 0;
}

/**
  * @brief  Set compressor release time for a channel
  * @param  channelIndex: Channel to set release time for
  * @param  releaseTimeMs: Release time in milliseconds
  * @retval Status (0 if OK, -1 if error)
  */
int8_t Compressor_SetReleaseTime(uint8_t channelIndex, float releaseTimeMs)
{
  /* Validate channel index */
  if (channelIndex >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("Compressor_SetReleaseTime: Invalid channel index %d\r\n", channelIndex);
    return -1;
  }
  
  /* Clamp release time to valid range */
  releaseTimeMs = CLAMP(releaseTimeMs, COMPRESSOR_MIN_RELEASE, COMPRESSOR_MAX_RELEASE);
  
  /* Update parameter */
  compressorParams[channelIndex].releaseTimeMs = releaseTimeMs;
  
  /* Recalculate coefficients */
  Compressor_CalculateCoefficients(channelIndex);
  
  DEBUG_PRINT("Compressor[%d]: Set release time to %0.1f ms\r\n", channelIndex, releaseTimeMs);
  return 0;
}

/**
  * @brief  Set compressor knee width for a channel
  * @param  channelIndex: Channel to set knee width for
  * @param  kneeWidthDb: Knee width in dB (0 for hard knee)
  * @retval Status (0 if OK, -1 if error)
  */
int8_t Compressor_SetKneeWidth(uint8_t channelIndex, float kneeWidthDb)
{
  /* Validate channel index */
  if (channelIndex >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("Compressor_SetKneeWidth: Invalid channel index %d\r\n", channelIndex);
    return -1;
  }
  
  /* Clamp knee width to valid range */
  kneeWidthDb = CLAMP(kneeWidthDb, COMPRESSOR_MIN_KNEE, COMPRESSOR_MAX_KNEE);
  
  /* Update parameter */
  compressorParams[channelIndex].kneeWidthDb = kneeWidthDb;
  
  /* Recalculate coefficients */
  Compressor_CalculateCoefficients(channelIndex);
  
  DEBUG_PRINT("Compressor[%d]: Set knee width to %0.1f dB\r\n", channelIndex, kneeWidthDb);
  return 0;
}

/**
  * @brief  Set compressor makeup gain for a channel
  * @param  channelIndex: Channel to set makeup gain for
  * @param  makeupGainDb: Makeup gain in dB
  * @retval Status (0 if OK, -1 if error)
  */
int8_t Compressor_SetMakeupGain(uint8_t channelIndex, float makeupGainDb)
{
  /* Validate channel index */
  if (channelIndex >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("Compressor_SetMakeupGain: Invalid channel index %d\r\n", channelIndex);
    return -1;
  }
  
  /* Clamp makeup gain to valid range */
  makeupGainDb = CLAMP(makeupGainDb, COMPRESSOR_MIN_MAKEUP, COMPRESSOR_MAX_MAKEUP);
  
  /* Update parameter */
  compressorParams[channelIndex].makeupGainDb = makeupGainDb;
  
  /* Recalculate coefficients */
  Compressor_CalculateCoefficients(channelIndex);
  
  DEBUG_PRINT("Compressor[%d]: Set makeup gain to %0.1f dB\r\n", channelIndex, makeupGainDb);
  return 0;
}

/**
  * @brief  Enable or disable compressor for a channel
  * @param  channelIndex: Channel to enable/disable
  * @param  enabled: 1 to enable, 0 to disable
  * @retval Status (0 if OK, -1 if error)
  */
int8_t Compressor_SetEnabled(uint8_t channelIndex, uint8_t enabled)
{
  /* Validate channel index */
  if (channelIndex >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("Compressor_SetEnabled: Invalid channel index %d\r\n", channelIndex);
    return -1;
  }
  
  /* Update parameter */
  compressorParams[channelIndex].enabled = enabled ? 1 : 0;
  compressorState[channelIndex].isEnabled = enabled ? 1 : 0;
  
  DEBUG_PRINT("Compressor[%d]: %s\r\n", channelIndex, enabled ? "Enabled" : "Disabled");
  return 0;
}

/**
  * @brief  Get current compressor parameters for a channel
  * @param  channelIndex: Channel to get parameters for
  * @param  params: Pointer to store parameters
  * @retval Status (0 if OK, -1 if error)
  */
int8_t Compressor_GetParams(uint8_t channelIndex, Compressor_Params_TypeDef* params)
{
  /* Validate parameters */
  if (channelIndex >= AUDIO_OUTPUT_CHANNELS || params == NULL) {
    DEBUG_PRINT("Compressor_GetParams: Invalid parameters\r\n");
    return -1;
  }
  
  /* Copy parameters */
  memcpy(params, &compressorParams[channelIndex], sizeof(Compressor_Params_TypeDef));
  
  return 0;
}

/**
  * @brief  Get compressor state for a channel
  * @param  channelIndex: Channel to get state for
  * @param  state: Pointer to store state
  * @retval Status (0 if OK, -1 if error)
  */
int8_t Compressor_GetState(uint8_t channelIndex, Compressor_State_TypeDef* state)
{
  /* Validate parameters */
  if (channelIndex >= AUDIO_OUTPUT_CHANNELS || state == NULL) {
    DEBUG_PRINT("Compressor_GetState: Invalid parameters\r\n");
    return -1;
  }
  
  /* Copy state */
  memcpy(state, &compressorState[channelIndex], sizeof(Compressor_State_TypeDef));
  
  return 0;
}

/**
  * @brief  Set all compressor parameters at once for a channel
  * @param  channelIndex: Channel to set parameters for
  * @param  params: Pointer to parameters
  * @retval Status (0 if OK, -1 if error)
  */
int8_t Compressor_SetParams(uint8_t channelIndex, const Compressor_Params_TypeDef* params)
{
  /* Validate parameters */
  if (channelIndex >= AUDIO_OUTPUT_CHANNELS || params == NULL) {
    DEBUG_PRINT("Compressor_SetParams: Invalid parameters\r\n");
    return -1;
  }
  
  /* Copy and clamp parameters */
  compressorParams[channelIndex].thresholdDb = CLAMP(params->thresholdDb, 
                                                    COMPRESSOR_MIN_THRESHOLD, 
                                                    COMPRESSOR_MAX_THRESHOLD);
  
  compressorParams[channelIndex].ratio = CLAMP(params->ratio, 
                                             COMPRESSOR_MIN_RATIO, 
                                             COMPRESSOR_MAX_RATIO);
  
  compressorParams[channelIndex].attackTimeMs = CLAMP(params->attackTimeMs, 
                                                     COMPRESSOR_MIN_ATTACK, 
                                                     COMPRESSOR_MAX_ATTACK);
  
  compressorParams[channelIndex].releaseTimeMs = CLAMP(params->releaseTimeMs, 
                                                      COMPRESSOR_MIN_RELEASE, 
                                                      COMPRESSOR_MAX_RELEASE);
  
  compressorParams[channelIndex].kneeWidthDb = CLAMP(params->kneeWidthDb, 
                                                    COMPRESSOR_MIN_KNEE, 
                                                    COMPRESSOR_MAX_KNEE);
  
  compressorParams[channelIndex].makeupGainDb = CLAMP(params->makeupGainDb, 
                                                     COMPRESSOR_MIN_MAKEUP, 
                                                     COMPRESSOR_MAX_MAKEUP);
  
  compressorParams[channelIndex].enabled = params->enabled ? 1 : 0;
  compressorParams[channelIndex].sampleRate = params->sampleRate > 0 ? params->sampleRate : AUDIO_SAMPLE_RATE;
  
  /* Recalculate coefficients */
  Compressor_CalculateCoefficients(channelIndex);
  
  DEBUG_PRINT("Compressor[%d]: Parameters updated\r\n", channelIndex);
  return 0;
}