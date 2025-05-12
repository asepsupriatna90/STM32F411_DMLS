/**
  ******************************************************************************
  * @file           : delay_init.c
  * @brief          : Delay initialization module for time alignment in audio DSP
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Provides initialization functions for audio delay components
  * Used for time alignment in multi-speaker configurations
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "delay_types.h"
#include "delay.h"
#include "math_utils.h"
#include "debug.h"
#include <string.h>
#include <stdlib.h>

/* Private defines -----------------------------------------------------------*/
#define MAX_TEMP_COMP_ADJUSTMENT_US  50     /* Maximum temperature compensation in microseconds */
#define DELAY_BUFFER_ALIGNMENT       8      /* Memory alignment for delay buffer (bytes) */
#define MINIMUM_BUFFER_PADDING       16     /* Safety padding for buffer boundaries */

/* Private variables --------------------------------------------------------*/
static DelayInstance_TypeDef delayInstances[MAX_DELAY_CHANNELS];
static uint8_t isDelaySystemInitialized = 0;
static float tempCompensationFactor = 1.0f;  /* Temperature compensation factor */

/* Private function prototypes -----------------------------------------------*/
static uint32_t CalculateBufferSizeBytes(uint32_t maxDelayMs, uint32_t sampleRate);
static void ApplyDelaySettings(uint8_t channel);
static float ConvertDistanceToTime(float distance, DelayUnit_TypeDef unit);
static HAL_StatusTypeDef AllocateDelayBuffer(uint8_t channel);

/**
  * @brief  Initialize delay system for all channels
  * @param  config: Pointer to delay system configuration
  * @retval HAL status
  */
HAL_StatusTypeDef Delay_Init(DelayConfig_TypeDef *config)
{
  if (config == NULL) {
    DEBUG_PRINT("Delay_Init: Invalid configuration\r\n");
    return HAL_ERROR;
  }

  /* Save configuration */
  uint32_t sampleRate = config->sampleRate;
  uint32_t maxDelayMs = config->maxDelayMs;
  
  DEBUG_PRINT("Delay_Init: Initializing delay system. Sample rate: %lu Hz, Max delay: %lu ms\r\n", 
              sampleRate, maxDelayMs);

  /* Initialize all delay instances */
  for (uint8_t i = 0; i < MAX_DELAY_CHANNELS; i++) {
    /* Initialize delay instance structure */
    delayInstances[i].isActive = 0;
    delayInstances[i].bufferSize = 0;
    delayInstances[i].writeIndex = 0;
    delayInstances[i].sampleRate = sampleRate;
    delayInstances[i].maxDelayMs = maxDelayMs;
    delayInstances[i].currentDelayMs = 0.0f;
    delayInstances[i].currentDelayDistance = 0.0f;
    delayInstances[i].delayUnit = DELAY_UNIT_MS;
    delayInstances[i].phaseInvert = 0;
    delayInstances[i].enabled = 0;
    delayInstances[i].filterCoeff = 0.7f;  /* Default low pass filter coefficient for interpolation */
    delayInstances[i].prevSample = 0.0f;
    
    /* Allocate delay buffer for this channel */
    if (AllocateDelayBuffer(i) != HAL_OK) {
      DEBUG_PRINT("Delay_Init: Failed to allocate buffer for channel %d\r\n", i);
      Delay_DeInit();  /* Clean up already allocated resources */
      return HAL_ERROR;
    }
    
    /* Clear delay buffer */
    memset(delayInstances[i].buffer, 0, delayInstances[i].bufferSize * sizeof(float));
  }
  
  /* Set temperature compensation factor based on ambient temperature */
  /* Default to normal room temperature conditions */
  tempCompensationFactor = 1.0f;
  
  isDelaySystemInitialized = 1;
  
  DEBUG_PRINT("Delay_Init: Delay system initialized successfully\r\n");
  return HAL_OK;
}

/**
  * @brief  De-initialize delay system and free resources
  * @retval HAL status
  */
HAL_StatusTypeDef Delay_DeInit(void)
{
  if (!isDelaySystemInitialized) {
    return HAL_OK;  /* Already de-initialized */
  }
  
  DEBUG_PRINT("Delay_DeInit: Freeing delay resources\r\n");
  
  /* Free all allocated delay buffers */
  for (uint8_t i = 0; i < MAX_DELAY_CHANNELS; i++) {
    if (delayInstances[i].buffer != NULL) {
      free(delayInstances[i].buffer);
      delayInstances[i].buffer = NULL;
    }
  }
  
  isDelaySystemInitialized = 0;
  DEBUG_PRINT("Delay_DeInit: Delay resources freed successfully\r\n");
  
  return HAL_OK;
}

/**
  * @brief  Configure delay for a specific channel
  * @param  channel: Output channel index (0-3)
  * @param  config: Delay configuration
  * @retval HAL status
  */
HAL_StatusTypeDef Delay_ConfigChannel(uint8_t channel, DelayChannelConfig_TypeDef *config)
{
  /* Validate parameters */
  if (channel >= MAX_DELAY_CHANNELS) {
    DEBUG_PRINT("Delay_ConfigChannel: Invalid channel %d\r\n", channel);
    return HAL_ERROR;
  }
  
  if (!isDelaySystemInitialized) {
    DEBUG_PRINT("Delay_ConfigChannel: Delay system not initialized\r\n");
    return HAL_ERROR; 
  }
  
  if (config == NULL) {
    DEBUG_PRINT("Delay_ConfigChannel: Invalid configuration\r\n");
    return HAL_ERROR;
  }
  
  DEBUG_PRINT("Delay_ConfigChannel: Configuring channel %d, Delay: %.2f, Unit: %d, Phase: %d\r\n", 
               channel, config->delayValue, config->delayUnit, config->phaseInvert);
  
  /* Save configuration to delay instance */
  if (config->delayUnit == DELAY_UNIT_CM || config->delayUnit == DELAY_UNIT_INCH) {
    /* Convert distance to time */
    delayInstances[channel].currentDelayDistance = config->delayValue;
    delayInstances[channel].currentDelayMs = ConvertDistanceToTime(config->delayValue, config->delayUnit);
  } else {
    /* Direct time value */
    delayInstances[channel].currentDelayMs = config->delayValue;
    /* Calculate equivalent distance for display purposes */
    delayInstances[channel].currentDelayDistance = delayInstances[channel].currentDelayMs * SPEED_OF_SOUND_M_PER_SEC / 1000.0f * 100.0f; /* Convert to cm */
  }
  
  delayInstances[channel].delayUnit = config->delayUnit;
  delayInstances[channel].phaseInvert = config->phaseInvert;
  delayInstances[channel].enabled = config->enabled;
  
  /* Apply settings to update read indices */
  ApplyDelaySettings(channel);
  
  /* Mark channel as active */
  delayInstances[channel].isActive = 1;
  
  return HAL_OK;
}

/**
  * @brief  Update temperature compensation for delay calculation
  * @param  temperatureC: Ambient temperature in Celsius
  * @retval None
  */
void Delay_UpdateTemperatureCompensation(float temperatureC)
{
  /* Speed of sound varies with temperature: approximately 0.6 m/s per degree Celsius */
  /* Base reference is 343 m/s at 20°C */
  
  const float BASE_TEMP_C = 20.0f;
  const float SOUND_SPEED_CHANGE_PER_C = 0.6f;  /* m/s per degree C */
  
  /* Calculate new compensation factor */
  float tempDiff = temperatureC - BASE_TEMP_C;
  float speedOfSound = SPEED_OF_SOUND_M_PER_SEC + (tempDiff * SOUND_SPEED_CHANGE_PER_C);
  tempCompensationFactor = speedOfSound / SPEED_OF_SOUND_M_PER_SEC;
  
  DEBUG_PRINT("Delay_UpdateTemperatureCompensation: Temp %.1f C, Factor %.3f\r\n", 
              temperatureC, tempCompensationFactor);
  
  /* Re-apply settings for all active channels to update timing */
  for (uint8_t i = 0; i < MAX_DELAY_CHANNELS; i++) {
    if (delayInstances[i].isActive && delayInstances[i].enabled) {
      ApplyDelaySettings(i);
    }
  }
}

/**
  * @brief  Get delay instance for direct access
  * @param  channel: Output channel index (0-3)
  * @retval Pointer to delay instance or NULL if invalid
  */
DelayInstance_TypeDef* Delay_GetInstance(uint8_t channel)
{
  if (channel >= MAX_DELAY_CHANNELS || !isDelaySystemInitialized) {
    return NULL;
  }
  
  return &delayInstances[channel];
}

/**
  * @brief  Calculate necessary buffer size in bytes
  * @param  maxDelayMs: Maximum delay in milliseconds
  * @param  sampleRate: Sample rate in Hz
  * @retval Buffer size in samples
  */
static uint32_t CalculateBufferSizeBytes(uint32_t maxDelayMs, uint32_t sampleRate)
{
  /* Calculate required samples for maximum delay time */
  uint32_t maxDelaySamples = (maxDelayMs * sampleRate) / 1000;
  
  /* Add safety margin to buffer */
  uint32_t bufferSize = maxDelaySamples + MINIMUM_BUFFER_PADDING;
  
  return bufferSize;
}

/**
  * @brief  Allocate delay buffer for a channel
  * @param  channel: Channel index
  * @retval HAL status
  */
static HAL_StatusTypeDef AllocateDelayBuffer(uint8_t channel)
{
  uint32_t bufferSize = CalculateBufferSizeBytes(
    delayInstances[channel].maxDelayMs, 
    delayInstances[channel].sampleRate
  );
  
  /* Allocate memory for delay buffer */
  delayInstances[channel].buffer = (float*)malloc(bufferSize * sizeof(float));
  
  if (delayInstances[channel].buffer == NULL) {
    DEBUG_PRINT("AllocateDelayBuffer: Memory allocation failed for channel %d\r\n", channel);
    return HAL_ERROR;
  }
  
  delayInstances[channel].bufferSize = bufferSize;
  return HAL_OK;
}

/**
  * @brief  Apply delay settings to channel
  * @param  channel: Channel index
  * @retval None
  */
static void ApplyDelaySettings(uint8_t channel)
{
  if (channel >= MAX_DELAY_CHANNELS || !isDelaySystemInitialized) {
    return;
  }
  
  /* Apply compensation factor for temperature */
  float compensatedDelayMs = delayInstances[channel].currentDelayMs / tempCompensationFactor;
  
  /* Calculate delay in samples */
  float delaySamplesFloat = (compensatedDelayMs * delayInstances[channel].sampleRate) / 1000.0f;
  
  /* Store delay samples as fractional for interpolation */
  delayInstances[channel].delaySamples = delaySamplesFloat;
  
  DEBUG_PRINT("ApplyDelaySettings: Channel %d delay set to %.2f ms (%.2f samples)\r\n", 
              channel, compensatedDelayMs, delaySamplesFloat);
}

/**
  * @brief  Convert distance to time based on speed of sound
  * @param  distance: Distance value
  * @param  unit: Distance unit (cm or inch)
  * @retval Time in milliseconds
  */
static float ConvertDistanceToTime(float distance, DelayUnit_TypeDef unit)
{
  float meters;
  
  /* Convert to meters first */
  if (unit == DELAY_UNIT_CM) {
    meters = distance / 100.0f;  /* cm to meters */
  } else if (unit == DELAY_UNIT_INCH) {
    meters = distance * 0.0254f;  /* inches to meters */
  } else {
    /* Invalid unit, return original value */
    return distance;
  }
  
  /* Calculate time using speed of sound */
  float timeSeconds = meters / SPEED_OF_SOUND_M_PER_SEC;
  
  /* Return time in milliseconds */
  return timeSeconds * 1000.0f;
}