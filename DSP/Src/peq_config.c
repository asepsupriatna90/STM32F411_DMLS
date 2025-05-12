/**
  ******************************************************************************
  * @file           : peq_config.c
  * @brief          : Parametric EQ configuration implementation
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Configuration and parameter management for parametric equalizer in
  * DSP STM32F411 Audio Crossover system
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "peq.h"
#include "peq_types.h"
#include "math_utils.h"
#include "storage_types.h"
#include "debug.h"
#include <string.h>
#include <math.h>

/* Private defines -----------------------------------------------------------*/
#define PEQ_MIN_FREQ           20.0f      /* Minimum frequency (Hz) */
#define PEQ_MAX_FREQ           20000.0f   /* Maximum frequency (Hz) */
#define PEQ_MIN_GAIN           -12.0f     /* Minimum gain (dB) */
#define PEQ_MAX_GAIN           12.0f      /* Maximum gain (dB) */
#define PEQ_MIN_Q              0.1f       /* Minimum Q factor */
#define PEQ_MAX_Q              10.0f      /* Maximum Q factor */
#define PEQ_DEFAULT_FREQ       1000.0f    /* Default frequency (Hz) */
#define PEQ_DEFAULT_GAIN       0.0f       /* Default gain (dB) */
#define PEQ_DEFAULT_Q          0.707f     /* Default Q factor (Butterworth) */

/* Log frequency scale divisions for frequency adjustment */
#define PEQ_FREQ_STEPS_BELOW_100     5.0f
#define PEQ_FREQ_STEPS_BELOW_1000    20.0f
#define PEQ_FREQ_STEPS_BELOW_10000   100.0f
#define PEQ_FREQ_STEPS_ABOVE_10000   500.0f

/* Log Q scale divisions for Q adjustment */
#define PEQ_Q_STEPS_BELOW_1          0.05f
#define PEQ_Q_STEPS_BELOW_3          0.1f
#define PEQ_Q_STEPS_ABOVE_3          0.5f

/* Private variables ---------------------------------------------------------*/
/* Global EQ configuration for all channels */
static PEQ_Config_TypeDef peqConfig[AUDIO_OUTPUT_CHANNELS][PEQ_BANDS_PER_CHANNEL];

/* Filter type description strings */
static const char* peqTypeNames[PEQ_TYPE_MAX] = {
    "Bell",
    "Low Shelf",
    "High Shelf",
    "Low Pass",
    "High Pass"
};

/* Function prototypes -------------------------------------------------------*/
static float PEQ_AdjustFrequency(float freq, int8_t direction);
static float PEQ_AdjustGain(float gain, int8_t direction);
static float PEQ_AdjustQ(float q, int8_t direction);
static uint8_t PEQ_ValidateBandIndex(uint8_t band);
static uint8_t PEQ_ValidateChannelIndex(uint8_t channel);

/**
  * @brief  Initialize all EQ bands with default values
  * @param  None
  * @retval None
  */
void PEQ_Config_Init(void)
{
    /* Initialize all channels and bands with default values */
    for (uint8_t channel = 0; channel < AUDIO_OUTPUT_CHANNELS; channel++) {
        for (uint8_t band = 0; band < PEQ_BANDS_PER_CHANNEL; band++) {
            PEQ_Config_TypeDef *bandConfig = &peqConfig[channel][band];
            
            /* Set default values */
            bandConfig->enabled = (band == 0) ? 1 : 0;  /* Enable only first band by default */
            bandConfig->type = PEQ_TYPE_BELL;           /* Bell filter by default */
            bandConfig->frequency = PEQ_DEFAULT_FREQ;   /* Default frequency */
            bandConfig->gain = PEQ_DEFAULT_GAIN;        /* Default gain */
            bandConfig->q = PEQ_DEFAULT_Q;              /* Default Q */
            bandConfig->dirty = 1;                      /* Mark for initialization */
        }
    }
    
    /* Configure default EQ settings for each output (basic starting point) */
    /* Output 1 - High frequencies emphasis */
    peqConfig[0][0].enabled = 1;
    peqConfig[0][0].type = PEQ_TYPE_HIGH_SHELF;
    peqConfig[0][0].frequency = 5000.0f;
    peqConfig[0][0].gain = 2.0f;
    peqConfig[0][0].q = 0.7f;
    
    /* Output 2 - Midrange presence */
    peqConfig[1][0].enabled = 1;
    peqConfig[1][0].type = PEQ_TYPE_BELL;
    peqConfig[1][0].frequency = 2500.0f;
    peqConfig[1][0].gain = 1.5f;
    peqConfig[1][0].q = 1.0f;
    
    /* Output 3 - Low-mid emphasis */
    peqConfig[2][0].enabled = 1;
    peqConfig[2][0].type = PEQ_TYPE_BELL;
    peqConfig[2][0].frequency = 250.0f;
    peqConfig[2][0].gain = 1.0f;
    peqConfig[2][0].q = 1.2f;
    
    /* Output 4 - Bass boost */
    peqConfig[3][0].enabled = 1;
    peqConfig[3][0].type = PEQ_TYPE_LOW_SHELF;
    peqConfig[3][0].frequency = 100.0f;
    peqConfig[3][0].gain = 3.0f;
    peqConfig[3][0].q = 0.7f;
    
    DEBUG_PRINT("PEQ configuration initialized\r\n");
}

/**
  * @brief  Get EQ configuration for specified channel and band
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @param  config: Pointer to configuration structure to fill
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_GetBand(uint8_t channel, uint8_t band, PEQ_Config_TypeDef *config)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || 
        !PEQ_ValidateBandIndex(band) || 
        config == NULL) {
        return HAL_ERROR;
    }
    
    /* Copy configuration */
    memcpy(config, &peqConfig[channel][band], sizeof(PEQ_Config_TypeDef));
    
    return HAL_OK;
}

/**
  * @brief  Set EQ configuration for specified channel and band
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @param  config: Pointer to configuration structure with new parameters
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_SetBand(uint8_t channel, uint8_t band, const PEQ_Config_TypeDef *config)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || 
        !PEQ_ValidateBandIndex(band) || 
        config == NULL) {
        return HAL_ERROR;
    }
    
    /* Copy configuration */
    memcpy(&peqConfig[channel][band], config, sizeof(PEQ_Config_TypeDef));
    
    /* Mark as dirty to trigger filter coefficient recalculation */
    peqConfig[channel][band].dirty = 1;
    
    /* Log the change */
    DEBUG_PRINT("PEQ[%d][%d]: Type=%s, Freq=%.1f Hz, Gain=%.1f dB, Q=%.2f\r\n", 
               channel, band, 
               peqTypeNames[peqConfig[channel][band].type],
               peqConfig[channel][band].frequency,
               peqConfig[channel][band].gain,
               peqConfig[channel][band].q);
    
    return HAL_OK;
}

/**
  * @brief  Toggle EQ band enabled state
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_ToggleBandEnabled(uint8_t channel, uint8_t band)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || !PEQ_ValidateBandIndex(band)) {
        return HAL_ERROR;
    }
    
    /* Toggle enabled state */
    peqConfig[channel][band].enabled = !peqConfig[channel][band].enabled;
    
    /* Mark as dirty to trigger update */
    peqConfig[channel][band].dirty = 1;
    
    DEBUG_PRINT("PEQ[%d][%d]: %s\r\n", channel, band, 
               peqConfig[channel][band].enabled ? "Enabled" : "Disabled");
    
    return HAL_OK;
}

/**
  * @brief  Change EQ band filter type
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @param  type: New PEQ filter type 
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_SetBandType(uint8_t channel, uint8_t band, PEQ_FilterType_TypeDef type)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || 
        !PEQ_ValidateBandIndex(band) ||
        type >= PEQ_TYPE_MAX) {
        return HAL_ERROR;
    }
    
    /* Set new type */
    peqConfig[channel][band].type = type;
    
    /* Mark as dirty to trigger update */
    peqConfig[channel][band].dirty = 1;
    
    DEBUG_PRINT("PEQ[%d][%d]: Type changed to %s\r\n", 
               channel, band, peqTypeNames[type]);
    
    return HAL_OK;
}

/**
  * @brief  Cycle through EQ band filter types
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @param  direction: 1 for next type, -1 for previous type
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_CycleBandType(uint8_t channel, uint8_t band, int8_t direction)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || !PEQ_ValidateBandIndex(band)) {
        return HAL_ERROR;
    }
    
    /* Get current type */
    int8_t type = (int8_t)peqConfig[channel][band].type;
    
    /* Calculate new type with wrapping */
    type = (type + direction) % PEQ_TYPE_MAX;
    if (type < 0) {
        type += PEQ_TYPE_MAX;
    }
    
    /* Set new type */
    peqConfig[channel][band].type = (PEQ_FilterType_TypeDef)type;
    
    /* Mark as dirty to trigger update */
    peqConfig[channel][band].dirty = 1;
    
    DEBUG_PRINT("PEQ[%d][%d]: Type changed to %s\r\n", 
               channel, band, peqTypeNames[type]);
    
    return HAL_OK;
}

/**
  * @brief  Set EQ band frequency
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @param  frequency: New frequency value in Hz (20-20000)
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_SetBandFrequency(uint8_t channel, uint8_t band, float frequency)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || 
        !PEQ_ValidateBandIndex(band) ||
        frequency < PEQ_MIN_FREQ || 
        frequency > PEQ_MAX_FREQ) {
        return HAL_ERROR;
    }
    
    /* Set new frequency, clamped to allowed range */
    peqConfig[channel][band].frequency = MATH_Clamp(frequency, PEQ_MIN_FREQ, PEQ_MAX_FREQ);
    
    /* Mark as dirty to trigger update */
    peqConfig[channel][band].dirty = 1;
    
    DEBUG_PRINT("PEQ[%d][%d]: Frequency set to %.1f Hz\r\n", 
               channel, band, peqConfig[channel][band].frequency);
    
    return HAL_OK;
}

/**
  * @brief  Adjust EQ band frequency using stepped increments
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @param  direction: 1 for increase, -1 for decrease
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_AdjustBandFrequency(uint8_t channel, uint8_t band, int8_t direction)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || !PEQ_ValidateBandIndex(band)) {
        return HAL_ERROR;
    }
    
    /* Get current frequency */
    float freq = peqConfig[channel][band].frequency;
    
    /* Apply logarithmic stepped adjustment */
    freq = PEQ_AdjustFrequency(freq, direction);
    
    /* Set the new frequency */
    peqConfig[channel][band].frequency = freq;
    
    /* Mark as dirty to trigger update */
    peqConfig[channel][band].dirty = 1;
    
    DEBUG_PRINT("PEQ[%d][%d]: Frequency adjusted to %.1f Hz\r\n", 
               channel, band, freq);
    
    return HAL_OK;
}

/**
  * @brief  Set EQ band gain
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @param  gain: New gain value in dB (-12 to +12)
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_SetBandGain(uint8_t channel, uint8_t band, float gain)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || 
        !PEQ_ValidateBandIndex(band)) {
        return HAL_ERROR;
    }
    
    /* Set new gain, clamped to allowed range */
    peqConfig[channel][band].gain = MATH_Clamp(gain, PEQ_MIN_GAIN, PEQ_MAX_GAIN);
    
    /* Mark as dirty to trigger update */
    peqConfig[channel][band].dirty = 1;
    
    DEBUG_PRINT("PEQ[%d][%d]: Gain set to %.1f dB\r\n", 
               channel, band, peqConfig[channel][band].gain);
    
    return HAL_OK;
}

/**
  * @brief  Adjust EQ band gain in 0.5dB steps
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @param  direction: 1 for increase, -1 for decrease
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_AdjustBandGain(uint8_t channel, uint8_t band, int8_t direction)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || !PEQ_ValidateBandIndex(band)) {
        return HAL_ERROR;
    }
    
    /* Get current gain */
    float gain = peqConfig[channel][band].gain;
    
    /* Apply step adjustment */
    gain = PEQ_AdjustGain(gain, direction);
    
    /* Set the new gain */
    peqConfig[channel][band].gain = gain;
    
    /* Mark as dirty to trigger update */
    peqConfig[channel][band].dirty = 1;
    
    DEBUG_PRINT("PEQ[%d][%d]: Gain adjusted to %.1f dB\r\n", 
               channel, band, gain);
    
    return HAL_OK;
}

/**
  * @brief  Set EQ band Q factor
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @param  q: New Q value (0.1 to 10.0)
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_SetBandQ(uint8_t channel, uint8_t band, float q)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || 
        !PEQ_ValidateBandIndex(band) ||
        q < PEQ_MIN_Q || 
        q > PEQ_MAX_Q) {
        return HAL_ERROR;
    }
    
    /* Set new Q, clamped to allowed range */
    peqConfig[channel][band].q = MATH_Clamp(q, PEQ_MIN_Q, PEQ_MAX_Q);
    
    /* Mark as dirty to trigger update */
    peqConfig[channel][band].dirty = 1;
    
    DEBUG_PRINT("PEQ[%d][%d]: Q set to %.2f\r\n", 
               channel, band, peqConfig[channel][band].q);
    
    return HAL_OK;
}

/**
  * @brief  Adjust EQ band Q factor with non-linear steps
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @param  direction: 1 for increase, -1 for decrease
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_AdjustBandQ(uint8_t channel, uint8_t band, int8_t direction)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || !PEQ_ValidateBandIndex(band)) {
        return HAL_ERROR;
    }
    
    /* Get current Q */
    float q = peqConfig[channel][band].q;
    
    /* Apply non-linear step adjustment */
    q = PEQ_AdjustQ(q, direction);
    
    /* Set the new Q */
    peqConfig[channel][band].q = q;
    
    /* Mark as dirty to trigger update */
    peqConfig[channel][band].dirty = 1;
    
    DEBUG_PRINT("PEQ[%d][%d]: Q adjusted to %.2f\r\n", 
               channel, band, q);
    
    return HAL_OK;
}

/**
  * @brief  Check if any EQ band needs coefficient recalculation
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @retval uint8_t: 1 if band is marked dirty, 0 otherwise
  */
uint8_t PEQ_Config_IsBandDirty(uint8_t channel, uint8_t band)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || !PEQ_ValidateBandIndex(band)) {
        return 0;
    }
    
    return peqConfig[channel][band].dirty;
}

/**
  * @brief  Clear dirty flag after coefficient recalculation
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @retval None
  */
void PEQ_Config_ClearBandDirty(uint8_t channel, uint8_t band)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || !PEQ_ValidateBandIndex(band)) {
        return;
    }
    
    peqConfig[channel][band].dirty = 0;
}

/**
  * @brief  Reset EQ band to default values
  * @param  channel: Audio output channel index (0-3)
  * @param  band: EQ band index (0-4)
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_ResetBand(uint8_t channel, uint8_t band)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(channel) || !PEQ_ValidateBandIndex(band)) {
        return HAL_ERROR;
    }
    
    /* Reset to default values */
    peqConfig[channel][band].type = PEQ_TYPE_BELL;
    peqConfig[channel][band].frequency = PEQ_DEFAULT_FREQ;
    peqConfig[channel][band].gain = PEQ_DEFAULT_GAIN;
    peqConfig[channel][band].q = PEQ_DEFAULT_Q;
    peqConfig[channel][band].dirty = 1;
    
    DEBUG_PRINT("PEQ[%d][%d]: Reset to defaults\r\n", channel, band);
    
    return HAL_OK;
}

/**
  * @brief  Get a pointer to the PEQ configuration for saving to preset
  * @param  None
  * @retval PEQ_Config_TypeDef*: Pointer to the PEQ configuration array
  */
PEQ_Config_TypeDef* PEQ_Config_GetConfigurationPointer(void)
{
    return &peqConfig[0][0];
}

/**
  * @brief  Set all PEQ configurations from loaded preset
  * @param  config: Pointer to the loaded PEQ configuration from preset
  * @param  size: Size of the configuration data in bytes
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_SetFromPreset(const PEQ_Config_TypeDef* config, uint32_t size)
{
    /* Check parameters */
    if (config == NULL || size != sizeof(peqConfig)) {
        return HAL_ERROR;
    }
    
    /* Copy configuration */
    memcpy(&peqConfig[0][0], config, size);
    
    /* Mark all bands as dirty to trigger recalculation */
    for (uint8_t channel = 0; channel < AUDIO_OUTPUT_CHANNELS; channel++) {
        for (uint8_t band = 0; band < PEQ_BANDS_PER_CHANNEL; band++) {
            peqConfig[channel][band].dirty = 1;
        }
    }
    
    DEBUG_PRINT("PEQ configuration loaded from preset\r\n");
    
    return HAL_OK;
}

/**
  * @brief  Get filter type name as string
  * @param  type: Filter type enumeration value
  * @retval const char*: String representation of filter type
  */
const char* PEQ_Config_GetTypeString(PEQ_FilterType_TypeDef type)
{
    if (type >= PEQ_TYPE_MAX) {
        return "Unknown";
    }
    
    return peqTypeNames[type];
}

/**
  * @brief  Validate channel index
  * @param  channel: Channel index to validate
  * @retval uint8_t: 1 if valid, 0 otherwise
  */
static uint8_t PEQ_ValidateChannelIndex(uint8_t channel)
{
    return (channel < AUDIO_OUTPUT_CHANNELS);
}

/**
  * @brief  Validate band index
  * @param  band: Band index to validate
  * @retval uint8_t: 1 if valid, 0 otherwise
  */
static uint8_t PEQ_ValidateBandIndex(uint8_t band)
{
    return (band < PEQ_BANDS_PER_CHANNEL);
}

/**
  * @brief  Adjust frequency with logarithmic steps
  * @param  freq: Current frequency value
  * @param  direction: 1 for increase, -1 for decrease
  * @retval float: New frequency value
  */
static float PEQ_AdjustFrequency(float freq, int8_t direction)
{
    float step;
    
    /* Determine step size based on frequency range (logarithmic) */
    if (freq < 100.0f) {
        step = PEQ_FREQ_STEPS_BELOW_100;
    } else if (freq < 1000.0f) {
        step = PEQ_FREQ_STEPS_BELOW_1000;
    } else if (freq < 10000.0f) {
        step = PEQ_FREQ_STEPS_BELOW_10000;
    } else {
        step = PEQ_FREQ_STEPS_ABOVE_10000;
    }
    
    /* Apply step in the specified direction */
    freq += step * direction;
    
    /* Clamp to valid range */
    return MATH_Clamp(freq, PEQ_MIN_FREQ, PEQ_MAX_FREQ);
}

/**
  * @brief  Adjust gain with fixed 0.5dB steps
  * @param  gain: Current gain value
  * @param  direction: 1 for increase, -1 for decrease
  * @retval float: New gain value
  */
static float PEQ_AdjustGain(float gain, int8_t direction)
{
    /* Fixed 0.5dB steps */
    const float step = 0.5f;
    
    /* Apply step in the specified direction */
    gain += step * direction;
    
    /* Clamp to valid range */
    return MATH_Clamp(gain, PEQ_MIN_GAIN, PEQ_MAX_GAIN);
}

/**
  * @brief  Adjust Q factor with non-linear steps
  * @param  q: Current Q factor value
  * @param  direction: 1 for increase, -1 for decrease
  * @retval float: New Q factor value
  */
static float PEQ_AdjustQ(float q, int8_t direction)
{
    float step;
    
    /* Determine step size based on Q range (non-linear) */
    if (q < 1.0f) {
        step = PEQ_Q_STEPS_BELOW_1;
    } else if (q < 3.0f) {
        step = PEQ_Q_STEPS_BELOW_3;
    } else {
        step = PEQ_Q_STEPS_ABOVE_3;
    }
    
    /* Apply step in the specified direction */
    q += step * direction;
    
    /* Clamp to valid range */
    return MATH_Clamp(q, PEQ_MIN_Q, PEQ_MAX_Q);
}

/**
  * @brief  Link EQ parameters between channels for stereo operation
  * @param  sourceChannel: Source channel index
  * @param  targetChannel: Target channel index
  * @retval HAL_StatusTypeDef: HAL_OK if successful
  */
HAL_StatusTypeDef PEQ_Config_LinkChannels(uint8_t sourceChannel, uint8_t targetChannel)
{
    /* Validate parameters */
    if (!PEQ_ValidateChannelIndex(sourceChannel) || 
        !PEQ_ValidateChannelIndex(targetChannel) ||
        sourceChannel == targetChannel) {
        return HAL_ERROR;
    }
    
    /* Copy all band configurations from source to target */
    for (uint8_t band = 0; band < PEQ_BANDS_PER_CHANNEL; band++) {
        memcpy(&peqConfig[targetChannel][band], 
               &peqConfig[sourceChannel][band], 
               sizeof(PEQ_Config_TypeDef));
        
        /* Mark target as dirty to trigger recalculation */
        peqConfig[targetChannel][band].dirty = 1;
    }
    
    DEBUG_PRINT("PEQ: Channel %d linked to channel %d\r\n", 
               targetChannel, sourceChannel);
    
    return HAL_OK;
}