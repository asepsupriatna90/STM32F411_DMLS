/**
  ******************************************************************************
  * @file           : crossover_init.c
  * @brief          : Crossover initialization implementation
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Implementation of crossover filter initialization functions for
  * Panel Kontrol DSP STM32F411 untuk Sistem Audio Crossover Aktif
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "crossover.h"
#include "crossover_types.h"
#include "filter_design.h"
#include "filter_types.h"
#include "butterworth.h"
#include "linkwitz_riley.h"
#include "bessel.h"
#include "math_utils.h"
#include "debug.h"

/* Private variables ---------------------------------------------------------*/
static CrossoverFilter_TypeDef crossoverFilters[AUDIO_OUTPUT_CHANNELS];
static uint8_t isInitialized = 0;

/* Private function prototypes -----------------------------------------------*/
static void Crossover_AllocateFilterMemory(uint8_t channel);
static void Crossover_FreeFilterMemory(uint8_t channel);
static CrossoverCoefficients_TypeDef* Crossover_CalculateCoefficients(CrossoverType_TypeDef type, 
                                                                    uint8_t order, 
                                                                    float frequency, 
                                                                    float sampleRate);

/**
  * @brief  Initialize crossover module
  * @param  sampleRate: Audio sample rate in Hz
  * @retval Status code (0: success, other: error)
  */
uint8_t Crossover_Init(float sampleRate)
{
  DEBUG_PRINT("Initializing crossover filters at sample rate %.1f Hz\r\n", sampleRate);
  
  if (isInitialized) {
    DEBUG_PRINT("Warning: Crossover already initialized. Reinitializing...\r\n");
    Crossover_DeInit();
  }
  
  for (uint8_t channel = 0; channel < AUDIO_OUTPUT_CHANNELS; channel++) {
    /* Initialize default filter settings */
    crossoverFilters[channel].isEnabled = 1;
    crossoverFilters[channel].type = CROSSOVER_TYPE_LINKWITZ_RILEY;
    crossoverFilters[channel].order = 4; /* 24dB/oct */
    crossoverFilters[channel].frequency = 80.0f; /* Default 80Hz */
    crossoverFilters[channel].sampleRate = sampleRate;
    crossoverFilters[channel].filterMode = CROSSOVER_MODE_HIGHPASS; /* Default to highpass */
    
    /* Allocate memory for filter states */
    Crossover_AllocateFilterMemory(channel);
    
    /* Reset filter states */
    Crossover_Reset(channel);
  }
  
  /* Set appropriate filter modes for standard configuration */
  crossoverFilters[0].filterMode = CROSSOVER_MODE_HIGHPASS; /* OUT1: Left High */
  crossoverFilters[1].filterMode = CROSSOVER_MODE_HIGHPASS; /* OUT2: Right High */
  crossoverFilters[2].filterMode = CROSSOVER_MODE_BANDPASS; /* OUT3: Left Low */
  crossoverFilters[3].filterMode = CROSSOVER_MODE_LOWPASS;  /* OUT4: Right Low/Sub */
  
  /* Initialize filter coefficients */
  for (uint8_t channel = 0; channel < AUDIO_OUTPUT_CHANNELS; channel++) {
    Crossover_UpdateCoefficients(channel);
  }
  
  isInitialized = 1;
  DEBUG_PRINT("Crossover initialization completed successfully\r\n");
  
  return 0;
}

/**
  * @brief  De-initialize crossover module and free resources
  * @retval None
  */
void Crossover_DeInit(void)
{
  if (!isInitialized) {
    return;
  }
  
  for (uint8_t channel = 0; channel < AUDIO_OUTPUT_CHANNELS; channel++) {
    Crossover_FreeFilterMemory(channel);
  }
  
  isInitialized = 0;
  DEBUG_PRINT("Crossover module deinitialized\r\n");
}

/**
  * @brief  Reset filter states for a channel
  * @param  channel: Channel index (0-3)
  * @retval None
  */
void Crossover_Reset(uint8_t channel)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("Error: Invalid channel index %d for crossover reset\r\n", channel);
    return;
  }
  
  CrossoverFilter_TypeDef* filter = &crossoverFilters[channel];
  
  /* Reset high-pass filter states */
  if (filter->highpass.state != NULL) {
    for (uint8_t i = 0; i < filter->highpass.sections; i++) {
      filter->highpass.state[i].x1 = 0.0f;
      filter->highpass.state[i].x2 = 0.0f;
      filter->highpass.state[i].y1 = 0.0f;
      filter->highpass.state[i].y2 = 0.0f;
    }
  }
  
  /* Reset low-pass filter states */
  if (filter->lowpass.state != NULL) {
    for (uint8_t i = 0; i < filter->lowpass.sections; i++) {
      filter->lowpass.state[i].x1 = 0.0f;
      filter->lowpass.state[i].x2 = 0.0f;
      filter->lowpass.state[i].y1 = 0.0f;
      filter->lowpass.state[i].y2 = 0.0f;
    }
  }
  
  DEBUG_PRINT("Reset crossover filter states for channel %d\r\n", channel);
}

/**
  * @brief  Allocate memory for filter states
  * @param  channel: Channel index (0-3)
  * @retval None
  */
static void Crossover_AllocateFilterMemory(uint8_t channel)
{
  CrossoverFilter_TypeDef* filter = &crossoverFilters[channel];
  uint8_t sections = (filter->order + 1) / 2; /* Calculate number of biquad sections needed */
  
  /* Allocate memory for high-pass filter */
  filter->highpass.sections = sections;
  filter->highpass.coef = (BiquadCoefficients_TypeDef*)malloc(sizeof(BiquadCoefficients_TypeDef) * sections);
  filter->highpass.state = (BiquadState_TypeDef*)malloc(sizeof(BiquadState_TypeDef) * sections);
  
  if (filter->highpass.coef == NULL || filter->highpass.state == NULL) {
    DEBUG_PRINT("Error: Failed to allocate memory for high-pass filter, channel %d\r\n", channel);
    /* Handle allocation failure */
    Crossover_FreeFilterMemory(channel);
    return;
  }
  
  /* Allocate memory for low-pass filter */
  filter->lowpass.sections = sections;
  filter->lowpass.coef = (BiquadCoefficients_TypeDef*)malloc(sizeof(BiquadCoefficients_TypeDef) * sections);
  filter->lowpass.state = (BiquadState_TypeDef*)malloc(sizeof(BiquadState_TypeDef) * sections);
  
  if (filter->lowpass.coef == NULL || filter->lowpass.state == NULL) {
    DEBUG_PRINT("Error: Failed to allocate memory for low-pass filter, channel %d\r\n", channel);
    /* Handle allocation failure */
    Crossover_FreeFilterMemory(channel);
    return;
  }
  
  DEBUG_PRINT("Allocated memory for crossover filters, channel %d (%d sections)\r\n", 
              channel, sections);
}

/**
  * @brief  Free memory for filter states
  * @param  channel: Channel index (0-3)
  * @retval None
  */
static void Crossover_FreeFilterMemory(uint8_t channel)
{
  CrossoverFilter_TypeDef* filter = &crossoverFilters[channel];
  
  /* Free high-pass filter memory */
  if (filter->highpass.coef != NULL) {
    free(filter->highpass.coef);
    filter->highpass.coef = NULL;
  }
  
  if (filter->highpass.state != NULL) {
    free(filter->highpass.state);
    filter->highpass.state = NULL;
  }
  
  /* Free low-pass filter memory */
  if (filter->lowpass.coef != NULL) {
    free(filter->lowpass.coef);
    filter->lowpass.coef = NULL;
  }
  
  if (filter->lowpass.state != NULL) {
    free(filter->lowpass.state);
    filter->lowpass.state = NULL;
  }
  
  filter->highpass.sections = 0;
  filter->lowpass.sections = 0;
  
  DEBUG_PRINT("Freed memory for crossover filters, channel %d\r\n", channel);
}

/**
  * @brief  Update filter coefficients for a channel
  * @param  channel: Channel index (0-3)
  * @retval Status code (0: success, other: error)
  */
uint8_t Crossover_UpdateCoefficients(uint8_t channel)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS || !isInitialized) {
    DEBUG_PRINT("Error: Invalid channel or uninitialized crossover\r\n");
    return 1;
  }
  
  CrossoverFilter_TypeDef* filter = &crossoverFilters[channel];
  
  /* Calculate coefficients based on filter type and order */
  CrossoverCoefficients_TypeDef* coeffs = Crossover_CalculateCoefficients(
    filter->type,
    filter->order,
    filter->frequency,
    filter->sampleRate
  );
  
  if (coeffs == NULL) {
    DEBUG_PRINT("Error: Failed to calculate crossover coefficients for channel %d\r\n", channel);
    return 2;
  }
  
  /* Copy coefficients to filter structure */
  for (uint8_t i = 0; i < filter->highpass.sections; i++) {
    filter->highpass.coef[i] = coeffs->highpass[i];
    filter->lowpass.coef[i] = coeffs->lowpass[i];
  }
  
  /* Free temporary coefficient structure */
  free(coeffs);
  
  DEBUG_PRINT("Updated crossover coefficients for channel %d (f=%.1f Hz, type=%d, order=%d)\r\n",
              channel, filter->frequency, filter->type, filter->order);
  
  return 0;
}

/**
  * @brief  Calculate filter coefficients based on type and parameters
  * @param  type: Filter type (Butterworth, Linkwitz-Riley, Bessel)
  * @param  order: Filter order (determines slope in dB/oct)
  * @param  frequency: Crossover frequency in Hz
  * @param  sampleRate: Audio sample rate in Hz
  * @retval Pointer to calculated coefficients (must be freed by caller)
  */
static CrossoverCoefficients_TypeDef* Crossover_CalculateCoefficients(CrossoverType_TypeDef type, 
                                                                    uint8_t order, 
                                                                    float frequency, 
                                                                    float sampleRate)
{
  uint8_t sections = (order + 1) / 2; /* Number of biquad sections needed */
  
  /* Allocate memory for coefficients */
  CrossoverCoefficients_TypeDef* coeffs = (CrossoverCoefficients_TypeDef*)malloc(sizeof(CrossoverCoefficients_TypeDef));
  if (coeffs == NULL) {
    DEBUG_PRINT("Error: Failed to allocate memory for crossover coefficients\r\n");
    return NULL;
  }
  
  /* Allocate arrays for filter coefficients */
  coeffs->highpass = (BiquadCoefficients_TypeDef*)malloc(sizeof(BiquadCoefficients_TypeDef) * sections);
  coeffs->lowpass = (BiquadCoefficients_TypeDef*)malloc(sizeof(BiquadCoefficients_TypeDef) * sections);
  
  if (coeffs->highpass == NULL || coeffs->lowpass == NULL) {
    DEBUG_PRINT("Error: Failed to allocate memory for filter coefficient arrays\r\n");
    /* Free any allocated memory */
    if (coeffs->highpass != NULL) free(coeffs->highpass);
    if (coeffs->lowpass != NULL) free(coeffs->lowpass);
    free(coeffs);
    return NULL;
  }
  
  /* Normalize frequency to Nyquist */
  float normalizedFreq = frequency / (sampleRate / 2.0f);
  if (normalizedFreq >= 1.0f) {
    normalizedFreq = 0.999f; /* Ensure it's below Nyquist */
  }
  
  /* Calculate coefficients based on filter type */
  switch (type) {
    case CROSSOVER_TYPE_BUTTERWORTH:
      Butterworth_Calculate(normalizedFreq, order, coeffs->lowpass, coeffs->highpass);
      break;
      
    case CROSSOVER_TYPE_LINKWITZ_RILEY:
      LinkwitzRiley_Calculate(normalizedFreq, order, coeffs->lowpass, coeffs->highpass);
      break;
      
    case CROSSOVER_TYPE_BESSEL:
      Bessel_Calculate(normalizedFreq, order, coeffs->lowpass, coeffs->highpass);
      break;
      
    default:
      DEBUG_PRINT("Error: Unsupported crossover filter type %d\r\n", type);
      free(coeffs->highpass);
      free(coeffs->lowpass);
      free(coeffs);
      return NULL;
  }
  
  return coeffs;
}

/**
  * @brief  Get crossover filter configuration for a channel
  * @param  channel: Channel index (0-3)
  * @retval Pointer to filter configuration (NULL if error)
  */
CrossoverFilter_TypeDef* Crossover_GetConfig(uint8_t channel)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS || !isInitialized) {
    DEBUG_PRINT("Error: Invalid channel or uninitialized crossover\r\n");
    return NULL;
  }
  
  return &crossoverFilters[channel];
}