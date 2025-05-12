/**
  ******************************************************************************
  * @file           : compressor_config.c
  * @brief          : Compressor configuration implementation
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Configuration implementation for compressor DSP module of the
  * STM32F411 Audio DSP Crossover system
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "compressor.h"
#include "compressor_types.h"
#include "dsp_common.h"
#include "math_utils.h"
#include "debug.h"
#include "system_monitor.h"

/* Private function prototypes -----------------------------------------------*/
static float ComputeLogRatio(float ratio);
static float ComputeGainReduction(CompressorState_TypeDef *state, float inputLevel);
static void RecalculateCompressorParameters(CompressorConfig_TypeDef *config);

/* Global variables ---------------------------------------------------------*/
extern SystemState_TypeDef SystemState;

/**
  * @brief  Initialize a compressor configuration with default values
  * @param  config: Pointer to CompressorConfig_TypeDef structure
  * @retval None
  */
void Compressor_InitConfig(CompressorConfig_TypeDef *config)
{
  if (config == NULL) {
    DEBUG_PRINT("Compressor_InitConfig: NULL pointer\r\n");
    return;
  }
  
  /* Default settings - moderate compression */
  config->enabled = 0;                        /* Disabled by default */
  config->threshold = -20.0f;                 /* Threshold in dB */
  config->ratio = 4.0f;                       /* Ratio 4:1 */
  config->kneeWidth = 10.0f;                  /* Soft knee (10 dB) */
  config->attackTime = 20.0f;                 /* 20 ms attack */
  config->releaseTime = 200.0f;               /* 200 ms release */
  config->makeupGain = 0.0f;                  /* No makeup gain by default */
  
  /* Calculate derived parameters */
  RecalculateCompressorParameters(config);
  
  DEBUG_PRINT("Compressor config initialized with defaults\r\n");
}

/**
  * @brief  Apply new compressor configuration and update state parameters
  * @param  config: Pointer to new configuration
  * @param  state: Pointer to compressor state to update
  * @param  sampleRate: Current audio sample rate
  * @retval None
  */
void Compressor_ApplyConfig(CompressorConfig_TypeDef *config, CompressorState_TypeDef *state, uint32_t sampleRate)
{
  if (config == NULL || state == NULL) {
    DEBUG_PRINT("Compressor_ApplyConfig: NULL pointer\r\n");
    return;
  }
  
  /* First, make sure parameters are properly calculated */
  RecalculateCompressorParameters(config);
  
  /* Copy configuration values */
  state->enabled = config->enabled;
  state->threshold = config->threshold;
  state->ratio = config->ratio;
  state->kneeWidth = config->kneeWidth;
  state->makeupGain = config->makeupGain;
  
  /* Compute time constants from attack and release times */
  if (sampleRate > 0) {
    /* Formula: coefficient = exp(-1 / (time_ms * sampleRate / 1000)) */
    state->attackCoeff = MathUtils_Exp(-1000.0f / (config->attackTime * (float)sampleRate));
    state->releaseCoeff = MathUtils_Exp(-1000.0f / (config->releaseTime * (float)sampleRate));
  } else {
    /* Fallback to reasonable values if sampleRate is invalid */
    state->attackCoeff = 0.9f;
    state->releaseCoeff = 0.995f;
  }
  
  /* Set up internal parameters */
  state->thresholdLinear = MathUtils_DB2Linear(state->threshold);
  state->makeupGainLinear = MathUtils_DB2Linear(state->makeupGain);
  
  /* Compute knee parameters */
  state->kneeStartDb = state->threshold - (state->kneeWidth / 2.0f);
  state->kneeEndDb = state->threshold + (state->kneeWidth / 2.0f);
  state->kneeStartLinear = MathUtils_DB2Linear(state->kneeStartDb);
  state->kneeEndLinear = MathUtils_DB2Linear(state->kneeEndDb);
  
  /* For soft knee calculations */
  state->inverseKneeWidth = 1.0f / state->kneeWidth;
  state->kneeSlope = (1.0f - (1.0f / state->ratio)) * 0.5f;
  
  /* Compute the log ratio for faster calculations */
  state->logRatio = ComputeLogRatio(state->ratio);
  
  DEBUG_PRINT("Compressor config applied: thr=%.1f ratio=%.1f:1 attack=%.1f rel=%.1f\r\n", 
             state->threshold, state->ratio, config->attackTime, config->releaseTime);
}

/**
  * @brief  Set compressor threshold
  * @param  config: Pointer to compressor configuration
  * @param  thresholdDb: New threshold in dB
  * @retval 0 if successful, non-zero otherwise
  */
int Compressor_SetThreshold(CompressorConfig_TypeDef *config, float thresholdDb)
{
  if (config == NULL) {
    DEBUG_PRINT("Compressor_SetThreshold: NULL pointer\r\n");
    return -1;
  }
  
  /* Limit threshold to a reasonable range */
  if (thresholdDb < -60.0f) thresholdDb = -60.0f;
  if (thresholdDb > 0.0f) thresholdDb = 0.0f;
  
  config->threshold = thresholdDb;
  RecalculateCompressorParameters(config);
  
  DEBUG_PRINT("Compressor threshold set to %.1f dB\r\n", thresholdDb);
  return 0;
}

/**
  * @brief  Set compressor ratio
  * @param  config: Pointer to compressor configuration
  * @param  ratio: New ratio (e.g., 4.0 for 4:1 compression)
  * @retval 0 if successful, non-zero otherwise
  */
int Compressor_SetRatio(CompressorConfig_TypeDef *config, float ratio)
{
  if (config == NULL) {
    DEBUG_PRINT("Compressor_SetRatio: NULL pointer\r\n");
    return -1;
  }
  
  /* Validate ratio range: 1.0 (no compression) to 20.0 (limiting) */
  if (ratio < 1.0f) ratio = 1.0f;
  if (ratio > 20.0f) ratio = 20.0f;
  
  config->ratio = ratio;
  RecalculateCompressorParameters(config);
  
  DEBUG_PRINT("Compressor ratio set to %.1f:1\r\n", ratio);
  return 0;
}

/**
  * @brief  Set compressor attack time
  * @param  config: Pointer to compressor configuration
  * @param  attackTimeMs: New attack time in milliseconds
  * @retval 0 if successful, non-zero otherwise
  */
int Compressor_SetAttackTime(CompressorConfig_TypeDef *config, float attackTimeMs)
{
  if (config == NULL) {
    DEBUG_PRINT("Compressor_SetAttackTime: NULL pointer\r\n");
    return -1;
  }
  
  /* Validate attack time range */
  if (attackTimeMs < 0.1f) attackTimeMs = 0.1f;
  if (attackTimeMs > 100.0f) attackTimeMs = 100.0f;
  
  config->attackTime = attackTimeMs;
  RecalculateCompressorParameters(config);
  
  DEBUG_PRINT("Compressor attack time set to %.1f ms\r\n", attackTimeMs);
  return 0;
}

/**
  * @brief  Set compressor release time
  * @param  config: Pointer to compressor configuration
  * @param  releaseTimeMs: New release time in milliseconds
  * @retval 0 if successful, non-zero otherwise
  */
int Compressor_SetReleaseTime(CompressorConfig_TypeDef *config, float releaseTimeMs)
{
  if (config == NULL) {
    DEBUG_PRINT("Compressor_SetReleaseTime: NULL pointer\r\n");
    return -1;
  }
  
  /* Validate release time range */
  if (releaseTimeMs < 10.0f) releaseTimeMs = 10.0f;
  if (releaseTimeMs > 1000.0f) releaseTimeMs = 1000.0f;
  
  config->releaseTime = releaseTimeMs;
  RecalculateCompressorParameters(config);
  
  DEBUG_PRINT("Compressor release time set to %.1f ms\r\n", releaseTimeMs);
  return 0;
}

/**
  * @brief  Set compressor knee width (for soft knee compression)
  * @param  config: Pointer to compressor configuration
  * @param  kneeWidthDb: New knee width in dB (0 = hard knee)
  * @retval 0 if successful, non-zero otherwise
  */
int Compressor_SetKneeWidth(CompressorConfig_TypeDef *config, float kneeWidthDb)
{
  if (config == NULL) {
    DEBUG_PRINT("Compressor_SetKneeWidth: NULL pointer\r\n");
    return -1;
  }
  
  /* Validate knee width range */
  if (kneeWidthDb < 0.0f) kneeWidthDb = 0.0f;  /* Hard knee */
  if (kneeWidthDb > 24.0f) kneeWidthDb = 24.0f;
  
  config->kneeWidth = kneeWidthDb;
  RecalculateCompressorParameters(config);
  
  DEBUG_PRINT("Compressor knee width set to %.1f dB\r\n", kneeWidthDb);
  return 0;
}

/**
  * @brief  Set compressor makeup gain
  * @param  config: Pointer to compressor configuration
  * @param  makeupGainDb: New makeup gain in dB
  * @retval 0 if successful, non-zero otherwise
  */
int Compressor_SetMakeupGain(CompressorConfig_TypeDef *config, float makeupGainDb)
{
  if (config == NULL) {
    DEBUG_PRINT("Compressor_SetMakeupGain: NULL pointer\r\n");
    return -1;
  }
  
  /* Validate makeup gain range */
  if (makeupGainDb < 0.0f) makeupGainDb = 0.0f;
  if (makeupGainDb > 24.0f) makeupGainDb = 24.0f;
  
  config->makeupGain = makeupGainDb;
  RecalculateCompressorParameters(config);
  
  DEBUG_PRINT("Compressor makeup gain set to %.1f dB\r\n", makeupGainDb);
  return 0;
}

/**
  * @brief  Enable or disable the compressor
  * @param  config: Pointer to compressor configuration
  * @param  enable: 1 to enable, 0 to disable
  * @retval 0 if successful, non-zero otherwise
  */
int Compressor_SetEnabled(CompressorConfig_TypeDef *config, uint8_t enable)
{
  if (config == NULL) {
    DEBUG_PRINT("Compressor_SetEnabled: NULL pointer\r\n");
    return -1;
  }
  
  config->enabled = enable ? 1 : 0;
  
  DEBUG_PRINT("Compressor %s\r\n", enable ? "enabled" : "disabled");
  return 0;
}

/**
  * @brief  Calculate auto makeup gain based on threshold and ratio
  * @param  config: Pointer to compressor configuration
  * @retval Recommended makeup gain in dB
  */
float Compressor_CalculateAutoMakeupGain(CompressorConfig_TypeDef *config)
{
  if (config == NULL) {
    DEBUG_PRINT("Compressor_CalculateAutoMakeupGain: NULL pointer\r\n");
    return 0.0f;
  }
  
  /* Simple formula: makeup = -threshold * (1 - 1/ratio) */
  float makeup = -config->threshold * (1.0f - (1.0f / config->ratio));
  
  /* Limit to reasonable range */
  if (makeup < 0.0f) makeup = 0.0f;
  if (makeup > 24.0f) makeup = 24.0f;
  
  DEBUG_PRINT("Auto makeup gain calculated: %.1f dB\r\n", makeup);
  return makeup;
}

/**
  * @brief  Apply auto makeup gain based on current settings
  * @param  config: Pointer to compressor configuration
  * @retval 0 if successful, non-zero otherwise
  */
int Compressor_ApplyAutoMakeupGain(CompressorConfig_TypeDef *config)
{
  if (config == NULL) {
    DEBUG_PRINT("Compressor_ApplyAutoMakeupGain: NULL pointer\r\n");
    return -1;
  }
  
  float autoGain = Compressor_CalculateAutoMakeupGain(config);
  return Compressor_SetMakeupGain(config, autoGain);
}

/**
  * @brief  Recalculate derived compressor parameters after configuration changes
  * @param  config: Pointer to compressor configuration
  * @retval None
  * @note   This function is called internally whenever a parameter changes
  */
static void RecalculateCompressorParameters(CompressorConfig_TypeDef *config)
{
  /* Nothing else needs to be computed at the config level - 
     The actual calculations are performed when applying the config to a state */
}

/**
  * @brief  Compute log ratio value for faster gain calculation
  * @param  ratio: Compressor ratio
  * @retval Precomputed log ratio value
  */
static float ComputeLogRatio(float ratio)
{
  /* This calculates the value (1 - 1/ratio) which is used 
     in the gain calculation formula */
  if (ratio <= 1.0f) {
    return 0.0f;  /* No compression */
  }
  
  return 1.0f - (1.0f / ratio);
}

/**
  * @brief  Compute gain reduction for a given input level based on settings
  * @param  state: Compressor state
  * @param  inputLevel: Input level in dB
  * @retval Gain reduction in dB (negative or zero)
  * @note   This is primarily a helper for UI display and level visualization
  */
static float ComputeGainReduction(CompressorState_TypeDef *state, float inputLevel)
{
  /* If compressor is disabled or input level is below threshold */
  if (!state->enabled || inputLevel <= state->kneeStartDb) {
    return 0.0f;  /* No gain reduction */
  }
  
  float gainReduction = 0.0f;
  
  if (state->kneeWidth > 0.0f && inputLevel < state->kneeEndDb) {
    /* In the knee region - gradual increase in compression */
    float kneePosition = (inputLevel - state->kneeStartDb) * state->inverseKneeWidth;
    gainReduction = state->kneeSlope * (inputLevel - state->kneeStartDb) * kneePosition;
  } else {
    /* Above knee - full compression */
    gainReduction = (inputLevel - state->threshold) * state->logRatio;
  }
  
  /* Gain reduction is a negative value */
  return -gainReduction;
}

/**
  * @brief  Get current compressor gain reduction for UI display
  * @param  state: Compressor state
  * @retval Current gain reduction in dB (negative or zero)
  */
float Compressor_GetGainReduction(CompressorState_TypeDef *state)
{
  if (state == NULL) {
    return 0.0f;
  }
  
  /* Return the last calculated gain reduction value */
  return MathUtils_Linear2DB(state->currentGainLinear);
}

/**
  * @brief  Create preset with specific compressor characteristics
  * @param  config: Pointer to compressor configuration
  * @param  presetType: Type of preset to create
  * @retval 0 if successful, non-zero otherwise
  */
int Compressor_CreatePreset(CompressorConfig_TypeDef *config, CompressorPreset_TypeDef presetType)
{
  if (config == NULL) {
    DEBUG_PRINT("Compressor_CreatePreset: NULL pointer\r\n");
    return -1;
  }
  
  /* Apply preset settings based on type */
  switch (presetType) {
    case COMPRESSOR_PRESET_GENTLE:
      config->enabled = 1;
      config->threshold = -20.0f;
      config->ratio = 2.0f;
      config->kneeWidth = 10.0f;
      config->attackTime = 25.0f;
      config->releaseTime = 150.0f;
      config->makeupGain = 3.0f;
      break;
      
    case COMPRESSOR_PRESET_MODERATE:
      config->enabled = 1;
      config->threshold = -25.0f;
      config->ratio = 4.0f;
      config->kneeWidth = 8.0f;
      config->attackTime = 15.0f;
      config->releaseTime = 120.0f;
      config->makeupGain = 6.0f;
      break;
      
    case COMPRESSOR_PRESET_AGGRESSIVE:
      config->enabled = 1;
      config->threshold = -30.0f;
      config->ratio = 8.0f;
      config->kneeWidth = 4.0f;
      config->attackTime = 5.0f;
      config->releaseTime = 80.0f;
      config->makeupGain = 10.0f;
      break;
      
    case COMPRESSOR_PRESET_LIMITING:
      config->enabled = 1;
      config->threshold = -3.0f;
      config->ratio = 20.0f;
      config->kneeWidth = 1.0f;
      config->attackTime = 0.5f;
      config->releaseTime = 50.0f;
      config->makeupGain = 0.0f;
      break;
      
    case COMPRESSOR_PRESET_VOCAL:
      config->enabled = 1;
      config->threshold = -18.0f;
      config->ratio = 3.0f;
      config->kneeWidth = 6.0f;
      config->attackTime = 8.0f;
      config->releaseTime = 100.0f;
      config->makeupGain = 4.0f;
      break;
      
    case COMPRESSOR_PRESET_BASS:
      config->enabled = 1;
      config->threshold = -25.0f;
      config->ratio = 5.0f;
      config->kneeWidth = 5.0f;
      config->attackTime = 10.0f;
      config->releaseTime = 150.0f;
      config->makeupGain = 5.0f;
      break;
      
    default:
      /* Unknown preset type, leave settings unchanged */
      DEBUG_PRINT("Unknown compressor preset type: %d\r\n", presetType);
      return -1;
  }
  
  /* Recalculate derived parameters */
  RecalculateCompressorParameters(config);
  
  DEBUG_PRINT("Applied compressor preset: %d\r\n", presetType);
  return 0;
}