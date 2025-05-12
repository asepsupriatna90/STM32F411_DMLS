/**
  ******************************************************************************
  * @file           : crossover_config.c
  * @brief          : Implementation of crossover configuration functions
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Configuration and parameter management for audio crossover DSP processing
  * Handles the configuration of crossover filters for each output channel
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "crossover_types.h"
#include "crossover.h"
#include "filter_design.h"
#include "butterworth.h"
#include "linkwitz_riley.h"
#include "bessel.h"
#include "eeprom_driver.h"
#include "math_utils.h"
#include "debug.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define CROSSOVER_CONFIG_ADDR_BASE    0x1000
#define CROSSOVER_CONFIG_SIZE         (sizeof(CrossoverConfig_TypeDef) * AUDIO_OUTPUT_CHANNELS)

#define MIN_CROSSOVER_FREQ            20      /* Minimum crossover frequency in Hz */
#define MAX_CROSSOVER_FREQ            20000   /* Maximum crossover frequency in Hz */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static CrossoverConfig_TypeDef crossoverConfig[AUDIO_OUTPUT_CHANNELS];

/* Default crossover configurations for different setups */
static const CrossoverConfig_TypeDef defaultCrossoverConfig[AUDIO_OUTPUT_CHANNELS] = {
    /* Channel 0: Default High pass (2-way setup) */
    {
        .isEnabled = 1,
        .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
        .filterOrder = CROSSOVER_ORDER_24DB,
        .highPassFreq = 80.0f,
        .lowPassFreq = 20000.0f,
        .bandPassEnabled = 0
    },
    /* Channel 1: Default High pass (2-way setup) */
    {
        .isEnabled = 1,
        .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
        .filterOrder = CROSSOVER_ORDER_24DB,
        .highPassFreq = 80.0f,
        .lowPassFreq = 20000.0f,
        .bandPassEnabled = 0
    },
    /* Channel 2: Default Low pass (2-way setup) */
    {
        .isEnabled = 1,
        .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
        .filterOrder = CROSSOVER_ORDER_24DB,
        .highPassFreq = 20.0f,
        .lowPassFreq = 80.0f,
        .bandPassEnabled = 0
    },
    /* Channel 3: Default subwoofer (low pass) */
    {
        .isEnabled = 1,
        .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
        .filterOrder = CROSSOVER_ORDER_24DB,
        .highPassFreq = 20.0f,
        .lowPassFreq = 80.0f,
        .bandPassEnabled = 0
    }
};

/* Mapping tables for filter types and orders */
static const char* crossoverTypeNames[] = {
    "BUT", /* Butterworth */
    "L-R", /* Linkwitz-Riley */
    "BES"  /* Bessel */
};

static const char* crossoverOrderNames[] = {
    "6dB",   /* 1st order */
    "12dB",  /* 2nd order */
    "18dB",  /* 3rd order */
    "24dB",  /* 4th order */
    "36dB",  /* 6th order */
    "48dB"   /* 8th order */
};

/* Private function prototypes -----------------------------------------------*/
static void Crossover_UpdateFilters(uint8_t channel);
static float Crossover_ClampFrequency(float freq);
static uint8_t Crossover_ValidateConfig(CrossoverConfig_TypeDef* config);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize crossover configuration with default values
  * @param  None
  * @retval None
  */
void Crossover_Config_Init(void)
{
    /* Try to load configuration from EEPROM */
    if (EEPROM_ReadData(CROSSOVER_CONFIG_ADDR_BASE, (uint8_t*)crossoverConfig, CROSSOVER_CONFIG_SIZE) != HAL_OK) {
        DEBUG_PRINT("Failed to load crossover config from EEPROM, using defaults\r\n");
        
        /* If failed, copy default configuration */
        for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
            memcpy(&crossoverConfig[i], &defaultCrossoverConfig[i], sizeof(CrossoverConfig_TypeDef));
        }
    }
    
    /* Validate and update all channels */
    for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
        if (!Crossover_ValidateConfig(&crossoverConfig[i])) {
            /* If invalid config, reset to default */
            memcpy(&crossoverConfig[i], &defaultCrossoverConfig[i], sizeof(CrossoverConfig_TypeDef));
        }
        
        /* Update filters with current configuration */
        Crossover_UpdateFilters(i);
    }
    
    DEBUG_PRINT("Crossover configuration initialized\r\n");
}

/**
  * @brief  Save current crossover configuration to EEPROM
  * @param  None
  * @retval HAL status
  */
HAL_StatusTypeDef Crossover_Config_Save(void)
{
    HAL_StatusTypeDef status;
    
    status = EEPROM_WriteData(CROSSOVER_CONFIG_ADDR_BASE, (uint8_t*)crossoverConfig, CROSSOVER_CONFIG_SIZE);
    
    if (status == HAL_OK) {
        DEBUG_PRINT("Crossover configuration saved to EEPROM\r\n");
    } else {
        DEBUG_PRINT("Failed to save crossover configuration to EEPROM\r\n");
    }
    
    return status;
}

/**
  * @brief  Get current crossover configuration for a channel
  * @param  channel: Channel index (0-3)
  * @param  config: Pointer to configuration structure to fill
  * @retval None
  */
void Crossover_Config_Get(uint8_t channel, CrossoverConfig_TypeDef* config)
{
    if (channel < AUDIO_OUTPUT_CHANNELS && config != NULL) {
        memcpy(config, &crossoverConfig[channel], sizeof(CrossoverConfig_TypeDef));
    }
}

/**
  * @brief  Set crossover configuration for a channel
  * @param  channel: Channel index (0-3)
  * @param  config: Pointer to configuration structure
  * @retval HAL_OK if successful, HAL_ERROR otherwise
  */
HAL_StatusTypeDef Crossover_Config_Set(uint8_t channel, CrossoverConfig_TypeDef* config)
{
    if (channel >= AUDIO_OUTPUT_CHANNELS || config == NULL) {
        return HAL_ERROR;
    }
    
    /* Validate configuration */
    if (!Crossover_ValidateConfig(config)) {
        DEBUG_PRINT("Invalid crossover configuration for channel %d\r\n", channel);
        return HAL_ERROR;
    }
    
    /* Copy configuration */
    memcpy(&crossoverConfig[channel], config, sizeof(CrossoverConfig_TypeDef));
    
    /* Update filters */
    Crossover_UpdateFilters(channel);
    
    DEBUG_PRINT("Crossover config updated for channel %d - HP: %.1fHz, LP: %.1fHz, Type: %s, Order: %s\r\n",
               channel, 
               config->highPassFreq, 
               config->lowPassFreq,
               crossoverTypeNames[config->filterType],
               crossoverOrderNames[config->filterOrder]);
    
    return HAL_OK;
}

/**
  * @brief  Enable or disable crossover for a channel
  * @param  channel: Channel index (0-3)
  * @param  enable: 1 to enable, 0 to disable
  * @retval HAL_OK if successful, HAL_ERROR otherwise
  */
HAL_StatusTypeDef Crossover_Config_SetEnable(uint8_t channel, uint8_t enable)
{
    if (channel >= AUDIO_OUTPUT_CHANNELS) {
        return HAL_ERROR;
    }
    
    crossoverConfig[channel].isEnabled = enable ? 1 : 0;
    
    DEBUG_PRINT("Crossover %s for channel %d\r\n", 
               enable ? "enabled" : "disabled", 
               channel);
    
    return HAL_OK;
}

/**
  * @brief  Set high-pass frequency for a channel
  * @param  channel: Channel index (0-3)
  * @param  freq: High-pass frequency in Hz
  * @retval HAL_OK if successful, HAL_ERROR otherwise
  */
HAL_StatusTypeDef Crossover_Config_SetHighPassFreq(uint8_t channel, float freq)
{
    if (channel >= AUDIO_OUTPUT_CHANNELS) {
        return HAL_ERROR;
    }
    
    /* Clamp frequency to valid range */
    freq = Crossover_ClampFrequency(freq);
    
    /* Check if frequency is valid relative to low-pass */
    if (crossoverConfig[channel].bandPassEnabled && 
        freq >= crossoverConfig[channel].lowPassFreq) {
        DEBUG_PRINT("Invalid HP freq: must be < LP freq (%.1f)\r\n", 
                   crossoverConfig[channel].lowPassFreq);
        return HAL_ERROR;
    }
    
    crossoverConfig[channel].highPassFreq = freq;
    
    /* Update filters */
    Crossover_UpdateFilters(channel);
    
    DEBUG_PRINT("Channel %d HP freq set to %.1f Hz\r\n", channel, freq);
    
    return HAL_OK;
}

/**
  * @brief  Set low-pass frequency for a channel
  * @param  channel: Channel index (0-3)
  * @param  freq: Low-pass frequency in Hz
  * @retval HAL_OK if successful, HAL_ERROR otherwise
  */
HAL_StatusTypeDef Crossover_Config_SetLowPassFreq(uint8_t channel, float freq)
{
    if (channel >= AUDIO_OUTPUT_CHANNELS) {
        return HAL_ERROR;
    }
    
    /* Clamp frequency to valid range */
    freq = Crossover_ClampFrequency(freq);
    
    /* Check if frequency is valid relative to high-pass */
    if (crossoverConfig[channel].bandPassEnabled && 
        freq <= crossoverConfig[channel].highPassFreq) {
        DEBUG_PRINT("Invalid LP freq: must be > HP freq (%.1f)\r\n", 
                   crossoverConfig[channel].highPassFreq);
        return HAL_ERROR;
    }
    
    crossoverConfig[channel].lowPassFreq = freq;
    
    /* Update filters */
    Crossover_UpdateFilters(channel);
    
    DEBUG_PRINT("Channel %d LP freq set to %.1f Hz\r\n", channel, freq);
    
    return HAL_OK;
}

/**
  * @brief  Set filter type for a channel
  * @param  channel: Channel index (0-3)
  * @param  type: Filter type (CROSSOVER_TYPE_xxx)
  * @retval HAL_OK if successful, HAL_ERROR otherwise
  */
HAL_StatusTypeDef Crossover_Config_SetFilterType(uint8_t channel, CrossoverType_TypeDef type)
{
    if (channel >= AUDIO_OUTPUT_CHANNELS || 
        type >= CROSSOVER_TYPE_COUNT) {
        return HAL_ERROR;
    }
    
    crossoverConfig[channel].filterType = type;
    
    /* Update filters */
    Crossover_UpdateFilters(channel);
    
    DEBUG_PRINT("Channel %d filter type set to %s\r\n", 
               channel, 
               crossoverTypeNames[type]);
    
    return HAL_OK;
}

/**
  * @brief  Set filter order for a channel
  * @param  channel: Channel index (0-3)
  * @param  order: Filter order (CROSSOVER_ORDER_xxx)
  * @retval HAL_OK if successful, HAL_ERROR otherwise
  */
HAL_StatusTypeDef Crossover_Config_SetFilterOrder(uint8_t channel, CrossoverOrder_TypeDef order)
{
    if (channel >= AUDIO_OUTPUT_CHANNELS || 
        order >= CROSSOVER_ORDER_COUNT) {
        return HAL_ERROR;
    }
    
    crossoverConfig[channel].filterOrder = order;
    
    /* Update filters */
    Crossover_UpdateFilters(channel);
    
    DEBUG_PRINT("Channel %d filter order set to %s\r\n", 
               channel, 
               crossoverOrderNames[order]);
    
    return HAL_OK;
}

/**
  * @brief  Set bandpass mode for a channel
  * @param  channel: Channel index (0-3)
  * @param  enable: 1 to enable bandpass, 0 for high/low pass only
  * @retval HAL_OK if successful, HAL_ERROR otherwise
  */
HAL_StatusTypeDef Crossover_Config_SetBandPassMode(uint8_t channel, uint8_t enable)
{
    if (channel >= AUDIO_OUTPUT_CHANNELS) {
        return HAL_ERROR;
    }
    
    /* Check if frequencies are valid for bandpass */
    if (enable && 
        crossoverConfig[channel].highPassFreq >= crossoverConfig[channel].lowPassFreq) {
        DEBUG_PRINT("Invalid freq range for bandpass: HP (%.1f) must be < LP (%.1f)\r\n",
                   crossoverConfig[channel].highPassFreq,
                   crossoverConfig[channel].lowPassFreq);
        return HAL_ERROR;
    }
    
    crossoverConfig[channel].bandPassEnabled = enable ? 1 : 0;
    
    /* Update filters */
    Crossover_UpdateFilters(channel);
    
    DEBUG_PRINT("Channel %d set to %s mode\r\n", 
               channel,
               enable ? "bandpass" : "highpass/lowpass");
    
    return HAL_OK;
}

/**
  * @brief  Get string representation of current filter type
  * @param  channel: Channel index (0-3)
  * @retval Pointer to filter type string or NULL if invalid
  */
const char* Crossover_Config_GetFilterTypeString(uint8_t channel)
{
    if (channel >= AUDIO_OUTPUT_CHANNELS ||
        crossoverConfig[channel].filterType >= CROSSOVER_TYPE_COUNT) {
        return NULL;
    }
    
    return crossoverTypeNames[crossoverConfig[channel].filterType];
}

/**
  * @brief  Get string representation of current filter order
  * @param  channel: Channel index (0-3)
  * @retval Pointer to filter order string or NULL if invalid
  */
const char* Crossover_Config_GetFilterOrderString(uint8_t channel)
{
    if (channel >= AUDIO_OUTPUT_CHANNELS ||
        crossoverConfig[channel].filterOrder >= CROSSOVER_ORDER_COUNT) {
        return NULL;
    }
    
    return crossoverOrderNames[crossoverConfig[channel].filterOrder];
}

/**
  * @brief  Reset crossover configuration for a channel to default
  * @param  channel: Channel index (0-3)
  * @retval HAL_OK if successful, HAL_ERROR otherwise
  */
HAL_StatusTypeDef Crossover_Config_ResetToDefault(uint8_t channel)
{
    if (channel >= AUDIO_OUTPUT_CHANNELS) {
        return HAL_ERROR;
    }
    
    /* Copy default configuration */
    memcpy(&crossoverConfig[channel], &defaultCrossoverConfig[channel], sizeof(CrossoverConfig_TypeDef));
    
    /* Update filters */
    Crossover_UpdateFilters(channel);
    
    DEBUG_PRINT("Channel %d crossover reset to default\r\n", channel);
    
    return HAL_OK;
}

/**
  * @brief  Apply a preset configuration to all channels
  * @param  preset: Preset index (0-4)
  * @retval HAL_OK if successful, HAL_ERROR otherwise
  */
HAL_StatusTypeDef Crossover_Config_ApplyPreset(uint8_t preset)
{
    /* Preset configurations for common setups */
    static const CrossoverConfig_TypeDef presetConfigs[][AUDIO_OUTPUT_CHANNELS] = {
        /* Preset 0: Standard 2-way */
        {
            /* Channel 0: Left High (>80Hz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 80.0f,
                .lowPassFreq = 20000.0f,
                .bandPassEnabled = 0
            },
            /* Channel 1: Right High (>80Hz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 80.0f,
                .lowPassFreq = 20000.0f,
                .bandPassEnabled = 0
            },
            /* Channel 2: Left Low (<80Hz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 20.0f,
                .lowPassFreq = 80.0f,
                .bandPassEnabled = 0
            },
            /* Channel 3: Right Low (<80Hz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 20.0f,
                .lowPassFreq = 80.0f,
                .bandPassEnabled = 0
            }
        },
        
        /* Preset 1: Standard 3-way */
        {
            /* Channel 0: Left High (>2.5kHz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 2500.0f,
                .lowPassFreq = 20000.0f,
                .bandPassEnabled = 0
            },
            /* Channel 1: Right High (>2.5kHz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 2500.0f,
                .lowPassFreq = 20000.0f,
                .bandPassEnabled = 0
            },
            /* Channel 2: Left Mid (250Hz-2.5kHz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 250.0f,
                .lowPassFreq = 2500.0f,
                .bandPassEnabled = 1
            },
            /* Channel 3: Right Mid (250Hz-2.5kHz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 250.0f,
                .lowPassFreq = 2500.0f,
                .bandPassEnabled = 1
            }
        },
        
        /* Preset 2: Subwoofer + Full range */
        {
            /* Channel 0: Left Full Range (>80Hz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_BUTTERWORTH,
                .filterOrder = CROSSOVER_ORDER_12DB,
                .highPassFreq = 80.0f,
                .lowPassFreq = 20000.0f,
                .bandPassEnabled = 0
            },
            /* Channel 1: Right Full Range (>80Hz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_BUTTERWORTH,
                .filterOrder = CROSSOVER_ORDER_12DB,
                .highPassFreq = 80.0f,
                .lowPassFreq = 20000.0f,
                .bandPassEnabled = 0
            },
            /* Channel 2: Mono Subwoofer (<80Hz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_BUTTERWORTH,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 20.0f,
                .lowPassFreq = 80.0f,
                .bandPassEnabled = 0
            },
            /* Channel 3: Duplicate Subwoofer (<80Hz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_BUTTERWORTH,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 20.0f,
                .lowPassFreq = 80.0f,
                .bandPassEnabled = 0
            }
        },
        
        /* Preset 3: Bi-amp monitor */
        {
            /* Channel 0: Left High (>1.2kHz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 1200.0f,
                .lowPassFreq = 20000.0f,
                .bandPassEnabled = 0
            },
            /* Channel 1: Right High (>1.2kHz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 1200.0f,
                .lowPassFreq = 20000.0f,
                .bandPassEnabled = 0
            },
            /* Channel 2: Left Low (<1.2kHz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 40.0f,
                .lowPassFreq = 1200.0f,
                .bandPassEnabled = 0
            },
            /* Channel 3: Right Low (<1.2kHz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 40.0f,
                .lowPassFreq = 1200.0f,
                .bandPassEnabled = 0
            }
        },
        
        /* Preset 4: Tri-amp system */
        {
            /* Channel 0: High (>3kHz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 3000.0f,
                .lowPassFreq = 20000.0f,
                .bandPassEnabled = 0
            },
            /* Channel 1: Mid (500Hz-3kHz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 500.0f,
                .lowPassFreq = 3000.0f,
                .bandPassEnabled = 1
            },
            /* Channel 2: Low (80Hz-500Hz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 80.0f,
                .lowPassFreq = 500.0f,
                .bandPassEnabled = 1
            },
            /* Channel 3: Subwoofer (<80Hz) */
            {
                .isEnabled = 1,
                .filterType = CROSSOVER_TYPE_LINKWITZ_RILEY,
                .filterOrder = CROSSOVER_ORDER_24DB,
                .highPassFreq = 20.0f,
                .lowPassFreq = 80.0f,
                .bandPassEnabled = 0
            }
        }
    };
    
    if (preset >= 5) {  /* We have 5 presets (0-4) */
        return HAL_ERROR;
    }
    
    /* Copy preset configuration to working config */
    for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
        memcpy(&crossoverConfig[i], &presetConfigs[preset][i], sizeof(CrossoverConfig_TypeDef));
        
        /* Update filters */
        Crossover_UpdateFilters(i);
    }
    
    DEBUG_PRINT("Applied crossover preset %d\r\n", preset);
    
    return HAL_OK;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Update filter coefficients based on current configuration
  * @param  channel: Channel index to update
  * @retval None
  */
static void Crossover_UpdateFilters(uint8_t channel)
{
    if (channel >= AUDIO_OUTPUT_CHANNELS) {
        return;
    }
    
    CrossoverConfig_TypeDef* config = &crossoverConfig[channel];
    
    /* Update high-pass filter if enabled */
    if (config->isEnabled && (config->highPassFreq > MIN_CROSSOVER_FREQ || config->bandPassEnabled)) {
        switch (config->filterType) {
            case CROSSOVER_TYPE_BUTTERWORTH:
                Butterworth_HighPassInit(channel, config->highPassFreq, config->filterOrder);
                break;
                
            case CROSSOVER_TYPE_LINKWITZ_RILEY:
                LinkwitzRiley_HighPassInit(channel, config->highPassFreq, config->filterOrder);
                break;
                
            case CROSSOVER_TYPE_BESSEL:
                Bessel_HighPassInit(channel, config->highPassFreq, config->filterOrder);
                break;
                
            default:
                /* Should not reach here */
                break;
        }
    } else {
        /* Disable high-pass filter */
        Crossover_DisableHighPass(channel);
    }
    
    /* Update low-pass filter if enabled */
    if (config->isEnabled && (config->lowPassFreq < MAX_CROSSOVER_FREQ || config->bandPassEnabled)) {
        switch (config->filterType) {
            case CROSSOVER_TYPE_BUTTERWORTH:
                Butterworth_LowPassInit(channel, config->lowPassFreq, config->filterOrder);
                break;
                
            case CROSSOVER_TYPE_LINKWITZ_RILEY:
                LinkwitzRiley_LowPassInit(channel, config->lowPassFreq, config->filterOrder);
                break;
                
            case CROSSOVER_TYPE_BESSEL:
                Bessel_LowPassInit(channel, config->lowPassFreq, config->filterOrder);
                break;
                
            default:
                /* Should not reach here */
                break;
        }
    } else {
        /* Disable low-pass filter */
        Crossover_DisableLowPass(channel);
    }
}

/**
  * @brief  Clamp frequency to valid range
  * @param  freq: Frequency in Hz
  * @retval Clamped frequency
  */
static float Crossover_ClampFrequency(float freq)
{
    if (freq < MIN_CROSSOVER_FREQ) {
        return MIN_CROSSOVER_FREQ;
    }
    
    if (freq > MAX_CROSSOVER_FREQ) {
        return MAX_CROSSOVER_FREQ;
    }
    
    return freq;
}

/**
  * @brief  Validate crossover configuration
  * @param  config: Pointer to configuration structure
  * @retval 1 if valid, 0 if invalid
  */
static uint8_t Crossover_ValidateConfig(CrossoverConfig_TypeDef* config)
{
    if (config == NULL) {
        return 0;
    }
    
    /* Check filter type */
    if (config->filterType >= CROSSOVER_TYPE_COUNT) {
        return 0;
    }
    
    /* Check filter order */
    if (config->filterOrder >= CROSSOVER_ORDER_COUNT) {
        return 0;
    }
    
    /* Check frequency values */
    if (config->highPassFreq < MIN_CROSSOVER_FREQ || 
        config->highPassFreq > MAX_CROSSOVER_FREQ ||
        config->lowPassFreq < MIN_CROSSOVER_FREQ || 
        config->lowPassFreq > MAX_CROSSOVER_FREQ) {
        return 0;
    }
    
    /* If bandpass enabled, check HP < LP */
    if (config->bandPassEnabled && 
        config->highPassFreq >= config->lowPassFreq) {
        return 0;
    }
    
    return 1;
}

/* EOF */