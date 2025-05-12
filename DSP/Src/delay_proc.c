/**
  ******************************************************************************
  * @file           : delay_proc.c
  * @brief          : Delay processing implementation for audio DSP
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Implements delay processing for time alignment in audio system
  * Supports linear and spline interpolation for high-quality delay lines
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "delay_types.h"
#include "delay.h"
#include "audio_config.h"
#include "math_utils.h"
#include "debug.h"
#include <string.h>
#include <math.h>

/* Private defines -----------------------------------------------------------*/
#define INTERPOLATION_MODE      DELAY_INTERPOLATION_LINEAR  /* Default interpolation mode */
#define LOW_PASS_COEFF_DEFAULT  0.7f                        /* Default smoothing coefficient */

/* Private variables ---------------------------------------------------------*/
static DelayInterpolation_TypeDef interpolationMode = INTERPOLATION_MODE;

/* Private function prototypes -----------------------------------------------*/
static float ProcessSampleWithDelay(uint8_t channel, float input);
static float InterpolateLinear(float *buffer, uint32_t bufferSize, uint32_t readIndex, float fraction);
static float InterpolateCubic(float *buffer, uint32_t bufferSize, uint32_t readIndex, float fraction);
static inline uint32_t ModuloBufferSize(int32_t index, uint32_t bufferSize);

/**
  * @brief  Process audio data through delay line
  * @param  channel: Output channel index (0-3)
  * @param  pData: Pointer to buffer containing samples
  * @param  size: Number of samples to process
  * @retval HAL status
  */
HAL_StatusTypeDef Delay_Process(uint8_t channel, float *pData, uint32_t size)
{
  /* Validate parameters */
  if (channel >= MAX_DELAY_CHANNELS || pData == NULL || size == 0) {
    DEBUG_PRINT("Delay_Process: Invalid parameters\r\n");
    return HAL_ERROR;
  }
  
  DelayInstance_TypeDef *instance = &delayInstances[channel];
  
  /* Check if delay is enabled and active */
  if (!instance->isActive || !instance->enabled) {
    /* Delay is disabled, pass-through audio data */
    return HAL_OK;
  }
  
  /* Process each sample through delay line */
  for (uint32_t i = 0; i < size; i++) {
    pData[i] = ProcessSampleWithDelay(channel, pData[i]);
  }
  
  return HAL_OK;
}

/**
  * @brief  Process one frame of audio data through delay line
  * @param  channel: Output channel index (0-3)
  * @param  inputBuffer: Input audio buffer
  * @param  outputBuffer: Output audio buffer
  * @retval HAL status
  */
HAL_StatusTypeDef Delay_ProcessFrame(uint8_t channel, 
                                    AudioBuffer_TypeDef *inputBuffer, 
                                    AudioBuffer_TypeDef *outputBuffer)
{
  /* Validate parameters */
  if (channel >= MAX_DELAY_CHANNELS || inputBuffer == NULL || outputBuffer == NULL) {
    DEBUG_PRINT("Delay_ProcessFrame: Invalid parameters\r\n");
    return HAL_ERROR;
  }
  
  DelayInstance_TypeDef *instance = &delayInstances[channel];
  
  /* Check if delay is enabled */
  if (!instance->isActive || !instance->enabled) {
    /* Delay is disabled, pass-through audio data */
    /* Copy input to output */
    memcpy(outputBuffer->data[channel], inputBuffer->data[channel], 
           sizeof(float) * inputBuffer->size);
    return HAL_OK;
  }
  
  /* Process each sample in the frame */
  for (uint32_t i = 0; i < inputBuffer->size; i++) {
    outputBuffer->data[channel][i] = ProcessSampleWithDelay(
      channel, inputBuffer->data[channel][i]
    );
  }
  
  return HAL_OK;
}

/**
  * @brief  Set delay time in milliseconds
  * @param  channel: Output channel index (0-3)
  * @param  delayMs: Delay time in milliseconds
  * @retval HAL status
  */
HAL_StatusTypeDef Delay_SetTime(uint8_t channel, float delayMs)
{
  /* Validate parameters */
  if (channel >= MAX_DELAY_CHANNELS || !delayInstances[channel].isActive) {
    DEBUG_PRINT("Delay_SetTime: Invalid channel %d\r\n", channel);
    return HAL_ERROR;
  }
  
  if (delayMs < 0.0f || delayMs > delayInstances[channel].maxDelayMs) {
    DEBUG_PRINT("Delay_SetTime: Invalid delay time %.2f ms (max: %.2f)\r\n", 
                delayMs, delayInstances[channel].maxDelayMs);
    return HAL_ERROR;
  }
  
  /* Update delay settings */
  delayInstances[channel].currentDelayMs = delayMs;
  delayInstances[channel].delayUnit = DELAY_UNIT_MS;
  
  /* Calculate equivalent distance for reference */
  delayInstances[channel].currentDelayDistance = 
    delayMs * SPEED_OF_SOUND_M_PER_SEC / 1000.0f * 100.0f; /* Convert to cm */
  
  /* Apply the new delay settings */
  ApplyDelaySettings(channel);
  
  return HAL_OK;
}

/**
  * @brief  Set delay as distance
  * @param  channel: Output channel index (0-3)
  * @param  distance: Distance value
  * @param  unit: DELAY_UNIT_CM or DELAY_UNIT_INCH
  * @retval HAL status
  */
HAL_StatusTypeDef Delay_SetDistance(uint8_t channel, float distance, DelayUnit_TypeDef unit)
{
  /* Validate parameters */
  if (channel >= MAX_DELAY_CHANNELS || !delayInstances[channel].isActive) {
    DEBUG_PRINT("Delay_SetDistance: Invalid channel %d\r\n", channel);
    return HAL_ERROR;
  }
  
  if (unit != DELAY_UNIT_CM && unit != DELAY_UNIT_INCH) {
    DEBUG_PRINT("Delay_SetDistance: Invalid unit %d\r\n", unit);
    return HAL_ERROR;
  }
  
  /* Convert distance to time */
  float delayMs = ConvertDistanceToTime(distance, unit);
  
  /* Check if resulting delay is within limits */
  if (delayMs < 0.0f || delayMs > delayInstances[channel].maxDelayMs) {
    DEBUG_PRINT("Delay_SetDistance: Resulting delay %.2f ms exceeds limit (max: %.2f)\r\n", 
                delayMs, delayInstances[channel].maxDelayMs);
    return HAL_ERROR;
  }
  
  /* Update delay settings */
  delayInstances[channel].currentDelayMs = delayMs;
  delayInstances[channel].currentDelayDistance = distance;
  delayInstances[channel].delayUnit = unit;
  
  /* Apply the new delay settings */
  ApplyDelaySettings(channel);
  
  return HAL_OK;
}

/**
  * @brief  Toggle phase inversion (0° / 180°)
  * @param  channel: Output channel index (0-3)
  * @param  invert: 0 = normal phase, 1 = inverted phase (180°)
  * @retval HAL status
  */
HAL_StatusTypeDef Delay_SetPhaseInvert(uint8_t channel, uint8_t invert)
{
  /* Validate parameters */
  if (channel >= MAX_DELAY_CHANNELS || !delayInstances[channel].isActive) {
    DEBUG_PRINT("Delay_SetPhaseInvert: Invalid channel %d\r\n", channel);
    return HAL_ERROR;
  }
  
  /* Update phase inversion setting */
  delayInstances[channel].phaseInvert = invert ? 1 : 0;
  
  DEBUG_PRINT("Delay_SetPhaseInvert: Channel %d phase %s\r\n", 
              channel, invert ? "inverted" : "normal");
  
  return HAL_OK;
}

/**
  * @brief  Enable or disable delay processing for a channel
  * @param  channel: Output channel index (0-3)
  * @param  enable: 0 = disable, 1 = enable
  * @retval HAL status
  */
HAL_StatusTypeDef Delay_SetEnable(uint8_t channel, uint8_t enable)
{
  /* Validate parameters */
  if (channel >= MAX_DELAY_CHANNELS || !delayInstances[channel].isActive) {
    DEBUG_PRINT("Delay_SetEnable: Invalid channel %d\r\n", channel);
    return HAL_ERROR;
  }
  
  /* Update enabled state */
  delayInstances[channel].enabled = enable ? 1 : 0;
  
  DEBUG_PRINT("Delay_SetEnable: Channel %d delay %s\r\n", 
              channel, enable ? "enabled" : "disabled");
  
  return HAL_OK;
}

/**
  * @brief  Set interpolation mode for delay lines
  * @param  mode: Interpolation mode (linear or cubic)
  * @retval HAL status
  */
HAL_StatusTypeDef Delay_SetInterpolationMode(DelayInterpolation_TypeDef mode)
{
  if (mode != DELAY_INTERPOLATION_LINEAR && mode != DELAY_INTERPOLATION_CUBIC) {
    DEBUG_PRINT("Delay_SetInterpolationMode: Invalid mode %d\r\n", mode);
    return HAL_ERROR;
  }
  
  interpolationMode = mode;
  
  DEBUG_PRINT("Delay_SetInterpolationMode: Set to %s\r\n", 
              mode == DELAY_INTERPOLATION_LINEAR ? "linear" : "cubic");
  
  return HAL_OK;
}

/**
  * @brief  Process a single sample through the delay line
  * @param  channel: Channel index
  * @param  input: Input sample
  * @retval Processed output sample
  */
static float ProcessSampleWithDelay(uint8_t channel, float input)
{
  DelayInstance_TypeDef *instance = &delayInstances[channel];
  float output;
  
  /* Store input sample in circular buffer */
  instance->buffer[instance->writeIndex] = input;
  
  /* Calculate read position with fractional delay */
  float readPos = (float)instance->writeIndex - instance->delaySamples;
  
  /* Handle buffer wrap-around */
  while (readPos < 0) {
    readPos += (float)instance->bufferSize;
  }
  
  /* Split into integer and fractional parts */
  int32_t readIndex = (int32_t)readPos;
  float fraction = readPos - (float)readIndex;
  
  /* Ensure read index is within buffer bounds */
  readIndex = ModuloBufferSize(readIndex, instance->bufferSize);
  
  /* Perform interpolation based on mode */
  if (interpolationMode == DELAY_INTERPOLATION_LINEAR) {
    output = InterpolateLinear(instance->buffer, instance->bufferSize, readIndex, fraction);
  } else {
    output = InterpolateCubic(instance->buffer, instance->bufferSize, readIndex, fraction);
  }
  
  /* Apply phase inversion if needed */
  if (instance->phaseInvert) {
    output = -output;
  }
  
  /* Apply low-pass filter for smoothing if needed */
  output = output * (1.0f - instance->filterCoeff) + instance->prevSample * instance->filterCoeff;
  instance->prevSample = output;
  
  /* Update write index for next sample */
  instance->writeIndex = ModuloBufferSize(instance->writeIndex + 1, instance->bufferSize);
  
  return output;
}

/**
  * @brief  Linear interpolation between samples
  * @param  buffer: Sample buffer
  * @param  bufferSize: Buffer size
  * @param  readIndex: Integer read index
  * @param  fraction: Fractional part of read position
  * @retval Interpolated sample
  */
static float InterpolateLinear(float *buffer, uint32_t bufferSize, uint32_t readIndex, float fraction)
{
  /* Get two samples for linear interpolation */
  float sample1 = buffer[readIndex];
  float sample2 = buffer[ModuloBufferSize(readIndex + 1, bufferSize)];
  
  /* Perform linear interpolation: y = y1 + fraction * (y2 - y1) */
  return sample1 + fraction * (sample2 - sample1);
}

/**
  * @brief  Cubic interpolation between samples (higher quality)
  * @param  buffer: Sample buffer
  * @param  bufferSize: Buffer size
  * @param  readIndex: Integer read index
  * @param  fraction: Fractional part of read position
  * @retval Interpolated sample
  */
static float InterpolateCubic(float *buffer, uint32_t bufferSize, uint32_t readIndex, float fraction)
{
  /* Get four samples for cubic interpolation */
  float y0 = buffer[ModuloBufferSize(readIndex - 1, bufferSize)];
  float y1 = buffer[readIndex];
  float y2 = buffer[ModuloBufferSize(readIndex + 1, bufferSize)];
  float y3 = buffer[ModuloBufferSize(readIndex + 2, bufferSize)];
  
  /* Cubic interpolation coefficients */
  float a0 = y3 - y2 - y0 + y1;
  float a1 = y0 - y1 - a0;
  float a2 = y2 - y0;
  float a3 = y1;
  
  /* Calculate cubic polynomial: a0*x^3 + a1*x^2 + a2*x + a3 */
  float frac2 = fraction * fraction;
  float result = a0 * fraction * frac2 + a1 * frac2 + a2 * fraction + a3;
  
  return result;
}

/**
  * @brief  Calculate modulo for circular buffer operations
  * @param  index: Buffer index (can be negative)
  * @param  bufferSize: Size of the buffer
  * @retval Index wrapped to buffer bounds
  */
static inline uint32_t ModuloBufferSize(int32_t index, uint32_t bufferSize)
{
  /* Handle negative indices for circular buffer */
  if (index < 0) {
    index += bufferSize;
  }
  
  return index % bufferSize;
}

/**
  * @brief  Get current delay settings for display or configuration
  * @param  channel: Output channel index (0-3)
  * @param  config: Pointer to store current configuration
  * @retval HAL status
  */
HAL_StatusTypeDef Delay_GetSettings(uint8_t channel, DelayChannelConfig_TypeDef *config)
{
  /* Validate parameters */
  if (channel >= MAX_DELAY_CHANNELS || !delayInstances[channel].isActive || config == NULL) {
    return HAL_ERROR;
  }
  
  /* Fill configuration structure with current settings */
  config->enabled = delayInstances[channel].enabled;
  config->delayUnit = delayInstances[channel].delayUnit;
  config->phaseInvert = delayInstances[channel].phaseInvert;
  
  /* Return the value in proper units */
  if (config->delayUnit == DELAY_UNIT_MS) {
    config->delayValue = delayInstances[channel].currentDelayMs;
  } else {
    config->delayValue = delayInstances[channel].currentDelayDistance;
  }
  
  return HAL_OK;
}

/**
  * @brief  Flush delay buffer for a channel (clear to zeros)
  * @param  channel: Output channel index (0-3)
  * @retval HAL status
  */
HAL_StatusTypeDef Delay_FlushBuffer(uint8_t channel)
{
  /* Validate parameters */
  if (channel >= MAX_DELAY_CHANNELS || !delayInstances[channel].isActive) {
    DEBUG_PRINT("Delay_FlushBuffer: Invalid channel %d\r\n", channel);
    return HAL_ERROR;
  }
  
  /* Clear delay buffer to zeros */
  memset(delayInstances[channel].buffer, 0, 
         delayInstances[channel].bufferSize * sizeof(float));
  
  /* Reset state variables */
  delayInstances[channel].writeIndex = 0;
  delayInstances[channel].prevSample = 0.0f;
  
  DEBUG_PRINT("Delay_FlushBuffer: Channel %d buffer cleared\r\n", channel);
  
  return HAL_OK;
}

/**
  * @brief  Reset all delay buffers and state variables
  * @retval HAL status
  */
HAL_StatusTypeDef Delay_ResetAll(void)
{
  if (!isDelaySystemInitialized) {
    DEBUG_PRINT("Delay_ResetAll: Delay system not initialized\r\n");
    return HAL_ERROR;
  }
  
  /* Reset all channels */
  for (uint8_t i = 0; i < MAX_DELAY_CHANNELS; i++) {
    if (delayInstances[i].isActive) {
      Delay_FlushBuffer(i);
    }
  }
  
  DEBUG_PRINT("Delay_ResetAll: All delay buffers reset\r\n");
  
  return HAL_OK;
}