/**
  ******************************************************************************
  * @file           : limiter_init.c
  * @brief          : Limiter module initialization implementation
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Limiter initialization module for STM32F411 Audio DSP Crossover system
  * Part of the DSP audio processing chain
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "limiter.h"
#include "limiter_types.h"
#include "dsp_common.h"
#include "math_utils.h"
#include "audio_config.h"
#include "debug.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define LIMITER_ENVELOPE_BUFFER_SIZE      512  /* Size of envelope follower buffer */
#define LIMITER_DEFAULT_THRESHOLD         -6.0f /* Default threshold in dB */
#define LIMITER_DEFAULT_RELEASE           50.0f /* Default release time in ms */
#define LIMITER_DEFAULT_ATTACK            0.1f  /* Default attack time in ms */
#define LIMITER_DEFAULT_LOOKAHEAD         2.0f  /* Default lookahead time in ms */
#define LIMITER_DEFAULT_CEILING           -0.3f /* Default output ceiling in dB */
#define LIMITER_MAX_THRESHOLD             0.0f  /* Maximum threshold value (dB) */
#define LIMITER_MIN_THRESHOLD             -60.0f /* Minimum threshold value (dB) */
#define LIMITER_MAX_RELEASE               1000.0f /* Maximum release time (ms) */
#define LIMITER_MIN_RELEASE               10.0f  /* Minimum release time (ms) */
#define LIMITER_MAX_ATTACK                10.0f  /* Maximum attack time (ms) */
#define LIMITER_MIN_ATTACK                0.05f  /* Minimum attack time (ms) */
#define LIMITER_MAX_LOOKAHEAD             10.0f  /* Maximum lookahead time (ms) */
#define LIMITER_MIN_LOOKAHEAD             0.0f   /* Minimum lookahead time (ms) */
#define LIMITER_MAX_CEILING               0.0f   /* Maximum ceiling value (dB) */
#define LIMITER_MIN_CEILING               -12.0f /* Minimum ceiling value (dB) */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static LimiterInstance_TypeDef limiterInstances[AUDIO_OUTPUT_CHANNELS];
static float limiterEnvelopeBuffers[AUDIO_OUTPUT_CHANNELS][LIMITER_ENVELOPE_BUFFER_SIZE];
static float limiterLookaheadBuffers[AUDIO_OUTPUT_CHANNELS][LIMITER_MAX_LOOKAHEAD_SAMPLES];

/* Private function prototypes -----------------------------------------------*/
static void Limiter_CalculateAttackCoefficient(LimiterInstance_TypeDef *instance);
static void Limiter_CalculateReleaseCoefficient(LimiterInstance_TypeDef *instance);
static void Limiter_CalculateLookaheadBufferSize(LimiterInstance_TypeDef *instance);

/**
  * @brief  Initialize the limiter module
  * @note   This function initializes the limiter module with default settings
  * @retval None
  */
void Limiter_Init(void)
{
  uint8_t i;
  
  DEBUG_PRINT("Initializing limiter module...\r\n");
  
  /* Initialize each limiter instance for all output channels */
  for (i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    Limiter_InitInstance(&limiterInstances[i], i);
  }
  
  DEBUG_PRINT("Limiter module initialized successfully\r\n");
}

/**
  * @brief  Initialize a single limiter instance
  * @param  instance: Pointer to the limiter instance to initialize
  * @param  channel: Audio channel number (0-based index)
  * @retval None
  */
void Limiter_InitInstance(LimiterInstance_TypeDef *instance, uint8_t channel)
{
  if (instance == NULL || channel >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("Error: Invalid limiter instance or channel number\r\n");
    return;
  }
  
  /* Initialize parameters with default values */
  instance->threshold = LIMITER_DEFAULT_THRESHOLD;
  instance->thresholdLin = dBToLinear(LIMITER_DEFAULT_THRESHOLD);
  instance->release = LIMITER_DEFAULT_RELEASE;
  instance->attack = LIMITER_DEFAULT_ATTACK;
  instance->lookahead = LIMITER_DEFAULT_LOOKAHEAD;
  instance->ceiling = LIMITER_DEFAULT_CEILING;
  instance->ceilingLin = dBToLinear(LIMITER_DEFAULT_CEILING);
  instance->bypass = 0;
  instance->channel = channel;
  
  /* Initialize state variables */
  instance->envelopeLevel = 0.0f;
  instance->currentGainReduction = 1.0f;
  instance->sampleRate = AUDIO_SAMPLE_RATE;
  
  /* Set up envelope buffer */
  instance->envelopeBuffer = limiterEnvelopeBuffers[channel];
  instance->envelopeBufferSize = LIMITER_ENVELOPE_BUFFER_SIZE;
  instance->envelopeBufferIndex = 0;
  
  /* Set up lookahead buffer */
  instance->lookaheadBuffer = limiterLookaheadBuffers[channel];
  instance->lookaheadBufferSize = 0;  /* Will be calculated based on lookahead time */
  instance->lookaheadBufferIndex = 0;
  
  /* Calculate coefficients based on time constants */
  Limiter_CalculateAttackCoefficient(instance);
  Limiter_CalculateReleaseCoefficient(instance);
  Limiter_CalculateLookaheadBufferSize(instance);
  
  /* Initialize buffers */
  uint16_t j;
  for (j = 0; j < instance->envelopeBufferSize; j++) {
    instance->envelopeBuffer[j] = 0.0f;
  }
  
  for (j = 0; j < LIMITER_MAX_LOOKAHEAD_SAMPLES; j++) {
    instance->lookaheadBuffer[j] = 0.0f;
  }
  
  DEBUG_PRINT("Limiter initialized for channel %d\r\n", channel);
}

/**
  * @brief  Set the limiter threshold
  * @param  instance: Pointer to the limiter instance
  * @param  thresholdDB: Threshold value in dB
  * @retval None
  */
void Limiter_SetThreshold(LimiterInstance_TypeDef *instance, float thresholdDB)
{
  if (instance == NULL) {
    return;
  }
  
  /* Clamp threshold within valid range */
  if (thresholdDB > LIMITER_MAX_THRESHOLD) {
    thresholdDB = LIMITER_MAX_THRESHOLD;
  } else if (thresholdDB < LIMITER_MIN_THRESHOLD) {
    thresholdDB = LIMITER_MIN_THRESHOLD;
  }
  
  instance->threshold = thresholdDB;
  instance->thresholdLin = dBToLinear(thresholdDB);
  
  DEBUG_PRINT("Limiter threshold set to %.1f dB\r\n", thresholdDB);
}

/**
  * @brief  Set the limiter release time
  * @param  instance: Pointer to the limiter instance
  * @param  releaseMS: Release time in milliseconds
  * @retval None
  */
void Limiter_SetRelease(LimiterInstance_TypeDef *instance, float releaseMS)
{
  if (instance == NULL) {
    return;
  }
  
  /* Clamp release time within valid range */
  if (releaseMS > LIMITER_MAX_RELEASE) {
    releaseMS = LIMITER_MAX_RELEASE;
  } else if (releaseMS < LIMITER_MIN_RELEASE) {
    releaseMS = LIMITER_MIN_RELEASE;
  }
  
  instance->release = releaseMS;
  Limiter_CalculateReleaseCoefficient(instance);
  
  DEBUG_PRINT("Limiter release time set to %.1f ms\r\n", releaseMS);
}

/**
  * @brief  Set the limiter attack time
  * @param  instance: Pointer to the limiter instance
  * @param  attackMS: Attack time in milliseconds
  * @retval None
  */
void Limiter_SetAttack(LimiterInstance_TypeDef *instance, float attackMS)
{
  if (instance == NULL) {
    return;
  }
  
  /* Clamp attack time within valid range */
  if (attackMS > LIMITER_MAX_ATTACK) {
    attackMS = LIMITER_MAX_ATTACK;
  } else if (attackMS < LIMITER_MIN_ATTACK) {
    attackMS = LIMITER_MIN_ATTACK;
  }
  
  instance->attack = attackMS;
  Limiter_CalculateAttackCoefficient(instance);
  
  DEBUG_PRINT("Limiter attack time set to %.2f ms\r\n", attackMS);
}

/**
  * @brief  Set the limiter lookahead time
  * @param  instance: Pointer to the limiter instance
  * @param  lookaheadMS: Lookahead time in milliseconds
  * @retval None
  */
void Limiter_SetLookahead(LimiterInstance_TypeDef *instance, float lookaheadMS)
{
  if (instance == NULL) {
    return;
  }
  
  /* Clamp lookahead time within valid range */
  if (lookaheadMS > LIMITER_MAX_LOOKAHEAD) {
    lookaheadMS = LIMITER_MAX_LOOKAHEAD;
  } else if (lookaheadMS < LIMITER_MIN_LOOKAHEAD) {
    lookaheadMS = LIMITER_MIN_LOOKAHEAD;
  }
  
  instance->lookahead = lookaheadMS;
  Limiter_CalculateLookaheadBufferSize(instance);
  
  DEBUG_PRINT("Limiter lookahead time set to %.2f ms\r\n", lookaheadMS);
}

/**
  * @brief  Set the limiter output ceiling
  * @param  instance: Pointer to the limiter instance
  * @param  ceilingDB: Ceiling value in dB
  * @retval None
  */
void Limiter_SetCeiling(LimiterInstance_TypeDef *instance, float ceilingDB)
{
  if (instance == NULL) {
    return;
  }
  
  /* Clamp ceiling within valid range */
  if (ceilingDB > LIMITER_MAX_CEILING) {
    ceilingDB = LIMITER_MAX_CEILING;
  } else if (ceilingDB < LIMITER_MIN_CEILING) {
    ceilingDB = LIMITER_MIN_CEILING;
  }
  
  instance->ceiling = ceilingDB;
  instance->ceilingLin = dBToLinear(ceilingDB);
  
  DEBUG_PRINT("Limiter ceiling set to %.1f dB\r\n", ceilingDB);
}

/**
  * @brief  Set the limiter bypass state
  * @param  instance: Pointer to the limiter instance
  * @param  bypass: Bypass state (0 = disabled, 1 = enabled)
  * @retval None
  */
void Limiter_SetBypass(LimiterInstance_TypeDef *instance, uint8_t bypass)
{
  if (instance == NULL) {
    return;
  }
  
  instance->bypass = bypass;
  
  DEBUG_PRINT("Limiter bypass %s\r\n", bypass ? "enabled" : "disabled");
}

/**
  * @brief  Get the limiter instance for a specific channel
  * @param  channel: Channel number (0-based index)
  * @retval Pointer to the limiter instance
  */
LimiterInstance_TypeDef* Limiter_GetInstance(uint8_t channel)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS) {
    return NULL;
  }
  
  return &limiterInstances[channel];
}

/**
  * @brief  Reset the limiter state (clear buffers and reset envelope)
  * @param  instance: Pointer to the limiter instance
  * @retval None
  */
void Limiter_Reset(LimiterInstance_TypeDef *instance)
{
  if (instance == NULL) {
    return;
  }
  
  uint16_t i;
  
  /* Reset state variables */
  instance->envelopeLevel = 0.0f;
  instance->currentGainReduction = 1.0f;
  instance->envelopeBufferIndex = 0;
  instance->lookaheadBufferIndex = 0;
  
  /* Clear buffers */
  for (i = 0; i < instance->envelopeBufferSize; i++) {
    instance->envelopeBuffer[i] = 0.0f;
  }
  
  for (i = 0; i < instance->lookaheadBufferSize; i++) {
    instance->lookaheadBuffer[i] = 0.0f;
  }
  
  DEBUG_PRINT("Limiter state reset for channel %d\r\n", instance->channel);
}

/**
  * @brief  Calculate attack coefficient from attack time
  * @param  instance: Pointer to the limiter instance
  * @retval None
  */
static void Limiter_CalculateAttackCoefficient(LimiterInstance_TypeDef *instance)
{
  if (instance == NULL) {
    return;
  }
  
  /* Calculate attack coefficient based on attack time and sample rate */
  float attackSamples = (instance->attack / 1000.0f) * instance->sampleRate;
  instance->attackCoeff = expf(-1.0f / attackSamples);
}

/**
  * @brief  Calculate release coefficient from release time
  * @param  instance: Pointer to the limiter instance
  * @retval None
  */
static void Limiter_CalculateReleaseCoefficient(LimiterInstance_TypeDef *instance)
{
  if (instance == NULL) {
    return;
  }
  
  /* Calculate release coefficient based on release time and sample rate */
  float releaseSamples = (instance->release / 1000.0f) * instance->sampleRate;
  instance->releaseCoeff = expf(-1.0f / releaseSamples);
}

/**
  * @brief  Calculate lookahead buffer size from lookahead time
  * @param  instance: Pointer to the limiter instance
  * @retval None
  */
static void Limiter_CalculateLookaheadBufferSize(LimiterInstance_TypeDef *instance)
{
  if (instance == NULL) {
    return;
  }
  
  /* Calculate lookahead buffer size based on lookahead time and sample rate */
  uint16_t lookaheadSamples = (uint16_t)((instance->lookahead / 1000.0f) * instance->sampleRate);
  
  /* Make sure lookahead buffer size is within valid range */
  if (lookaheadSamples > LIMITER_MAX_LOOKAHEAD_SAMPLES) {
    lookaheadSamples = LIMITER_MAX_LOOKAHEAD_SAMPLES;
  }
  
  instance->lookaheadBufferSize = lookaheadSamples;
}