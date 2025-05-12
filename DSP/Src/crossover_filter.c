/**
  ******************************************************************************
  * @file           : crossover_filter.c
  * @brief          : Implementasi filter audio crossover
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Implementasi filter untuk sistem crossover audio
  * Mendukung:
  * - Filter Butterworth
  * - Filter Linkwitz-Riley
  * - Filter Bessel
  * Dengan berbagai slope: 6dB/oct, 12dB/oct, 18dB/oct, 24dB/oct, 36dB/oct, 48dB/oct
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "crossover.h"
#include "butterworth.h"
#include "linkwitz_riley.h"
#include "bessel.h"
#include "biquad.h"
#include "math_utils.h"
#include "debug.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* Private defines -----------------------------------------------------------*/
#define MAX_FILTER_STAGES       8      // Maximum biquad stages per filter
#define MAX_FILTER_ORDER        12     // Maximum filter order supported
#define CROSSOVER_MIN_FREQ      20.0f  // Minimum crossover frequency (Hz)
#define CROSSOVER_MAX_FREQ      20000.0f // Maximum crossover frequency (Hz)

/* Private typedefs ----------------------------------------------------------*/
typedef struct {
    BiquadFilter_TypeDef stages[MAX_FILTER_STAGES];
    uint8_t numStages;
    float gain;
} FilterChain_TypeDef;

/* Private variables ---------------------------------------------------------*/
static CrossoverConfig_TypeDef crossoverConfig[AUDIO_OUTPUT_CHANNELS];
static FilterChain_TypeDef highpassFilters[AUDIO_OUTPUT_CHANNELS];
static FilterChain_TypeDef lowpassFilters[AUDIO_OUTPUT_CHANNELS];
static FilterChain_TypeDef bandpassHighFilters[AUDIO_OUTPUT_CHANNELS];
static FilterChain_TypeDef bandpassLowFilters[AUDIO_OUTPUT_CHANNELS];

/* Static function prototypes ------------------------------------------------*/
static void CalculateFilterCoefficients(uint8_t outputChannel);
static uint8_t GetNumStagesForOrder(uint8_t order);
static void ClearFilterHistory(FilterChain_TypeDef* filter);
static float ProcessFilterChain(FilterChain_TypeDef* filter, float input);
static float ComputeGainCompensation(CrossoverFilterType_TypeDef filterType, uint8_t order);

/**
  * @brief  Inisialisasi filter crossover
  * @param  outputChannel: Channel output yang akan diinisialisasi (0-3)
  * @retval None
  */
void Crossover_Filter_Init(uint8_t outputChannel)
{
    if (outputChannel >= AUDIO_OUTPUT_CHANNELS) {
        DEBUG_PRINT("Crossover_Filter_Init: Invalid output channel\r\n");
        return;
    }

    /* Default initialization for crossover parameters */
    crossoverConfig[outputChannel].type = CROSSOVER_TYPE_BUTTERWORTH;
    crossoverConfig[outputChannel].filterMode = CROSSOVER_MODE_LOWPASS;
    crossoverConfig[outputChannel].lowFrequency = 80.0f;
    crossoverConfig[outputChannel].highFrequency = 2500.0f;
    crossoverConfig[outputChannel].order = 4; // 24dB/oct
    
    /* Initialize filter chains */
    highpassFilters[outputChannel].numStages = 0;
    lowpassFilters[outputChannel].numStages = 0;
    bandpassHighFilters[outputChannel].numStages = 0;
    bandpassLowFilters[outputChannel].numStages = 0;
    
    /* Setup default gains */
    highpassFilters[outputChannel].gain = 1.0f;
    lowpassFilters[outputChannel].gain = 1.0f;
    bandpassHighFilters[outputChannel].gain = 1.0f;
    bandpassLowFilters[outputChannel].gain = 1.0f;
    
    /* Calculate initial coefficients */
    CalculateFilterCoefficients(outputChannel);
    
    DEBUG_PRINT("Crossover filter initialized for channel %d\r\n", outputChannel);
}

/**
  * @brief  Set filter mode untuk crossover
  * @param  outputChannel: Channel output yang akan dikonfigurasi (0-3)
  * @param  mode: Mode filter (lowpass, highpass, bandpass, full range)
  * @retval Status operasi (0: sukses, 1: gagal)
  */
uint8_t Crossover_SetFilterMode(uint8_t outputChannel, CrossoverFilterMode_TypeDef mode)
{
    if (outputChannel >= AUDIO_OUTPUT_CHANNELS) {
        return 1;
    }
    
    if (mode > CROSSOVER_MODE_FULLRANGE) {
        return 1;
    }
    
    crossoverConfig[outputChannel].filterMode = mode;
    CalculateFilterCoefficients(outputChannel); // Recalculate coefficients
    
    DEBUG_PRINT("Crossover channel %d mode set to %d\r\n", outputChannel, mode);
    return 0;
}

/**
  * @brief  Set tipe filter crossover
  * @param  outputChannel: Channel output yang akan dikonfigurasi (0-3)
  * @param  type: Tipe filter (Butterworth, Linkwitz-Riley, Bessel)
  * @retval Status operasi (0: sukses, 1: gagal)
  */
uint8_t Crossover_SetFilterType(uint8_t outputChannel, CrossoverFilterType_TypeDef type)
{
    if (outputChannel >= AUDIO_OUTPUT_CHANNELS) {
        return 1;
    }
    
    if (type > CROSSOVER_TYPE_BESSEL) {
        return 1;
    }
    
    crossoverConfig[outputChannel].type = type;
    CalculateFilterCoefficients(outputChannel); // Recalculate coefficients
    
    DEBUG_PRINT("Crossover channel %d type set to %d\r\n", outputChannel, type);
    return 0;
}

/**
  * @brief  Set frekuensi crossover
  * @param  outputChannel: Channel output yang akan dikonfigurasi (0-3)
  * @param  frequencyType: Tipe frekuensi (low/high untuk bandpass)
  * @param  frequency: Nilai frekuensi (Hz)
  * @retval Status operasi (0: sukses, 1: gagal)
  */
uint8_t Crossover_SetFrequency(uint8_t outputChannel, CrossoverFrequencyType_TypeDef frequencyType, float frequency)
{
    if (outputChannel >= AUDIO_OUTPUT_CHANNELS) {
        return 1;
    }
    
    /* Validate frequency range */
    if (frequency < CROSSOVER_MIN_FREQ || frequency > CROSSOVER_MAX_FREQ) {
        return 1;
    }
    
    /* Update appropriate frequency based on type */
    if (frequencyType == CROSSOVER_FREQ_LOW) {
        crossoverConfig[outputChannel].lowFrequency = frequency;
        
        /* Ensure lowFreq is not higher than highFreq in bandpass mode */
        if (crossoverConfig[outputChannel].filterMode == CROSSOVER_MODE_BANDPASS && 
            crossoverConfig[outputChannel].lowFrequency >= crossoverConfig[outputChannel].highFrequency) {
            crossoverConfig[outputChannel].lowFrequency = crossoverConfig[outputChannel].highFrequency - 10.0f;
        }
    } else {
        crossoverConfig[outputChannel].highFrequency = frequency;
        
        /* Ensure highFreq is not lower than lowFreq in bandpass mode */
        if (crossoverConfig[outputChannel].filterMode == CROSSOVER_MODE_BANDPASS && 
            crossoverConfig[outputChannel].highFrequency <= crossoverConfig[outputChannel].lowFrequency) {
            crossoverConfig[outputChannel].highFrequency = crossoverConfig[outputChannel].lowFrequency + 10.0f;
        }
    }
    
    CalculateFilterCoefficients(outputChannel); // Recalculate coefficients
    
    DEBUG_PRINT("Crossover channel %d %s frequency set to %.1f Hz\r\n", 
                outputChannel, 
                (frequencyType == CROSSOVER_FREQ_LOW) ? "low" : "high", 
                (frequencyType == CROSSOVER_FREQ_LOW) ? 
                    crossoverConfig[outputChannel].lowFrequency : 
                    crossoverConfig[outputChannel].highFrequency);
    
    return 0;
}

/**
  * @brief  Set order (slope) dari filter crossover
  * @param  outputChannel: Channel output yang akan dikonfigurasi (0-3)
  * @param  order: Urutan filter (1=6dB/oct, 2=12dB/oct, 3=18dB/oct, dst)
  * @retval Status operasi (0: sukses, 1: gagal)
  */
uint8_t Crossover_SetOrder(uint8_t outputChannel, uint8_t order)
{
    if (outputChannel >= AUDIO_OUTPUT_CHANNELS) {
        return 1;
    }
    
    /* Validate order range */
    if (order < 1 || order > MAX_FILTER_ORDER) {
        return 1;
    }
    
    /* For Linkwitz-Riley, order must be even */
    if (crossoverConfig[outputChannel].type == CROSSOVER_TYPE_LINKWITZ_RILEY && (order % 2 != 0)) {
        order = (order + 1) & ~1; // Round up to next even number
    }
    
    crossoverConfig[outputChannel].order = order;
    CalculateFilterCoefficients(outputChannel); // Recalculate coefficients
    
    DEBUG_PRINT("Crossover channel %d order set to %d (%d dB/oct)\r\n", 
                outputChannel, order, order * 6);
    
    return 0;
}

/**
  * @brief  Get current crossover config
  * @param  outputChannel: Channel output (0-3)
  * @param  config: Pointer ke struct konfigurasi
  * @retval Status operasi (0: sukses, 1: gagal)
  */
uint8_t Crossover_GetConfig(uint8_t outputChannel, CrossoverConfig_TypeDef* config)
{
    if (outputChannel >= AUDIO_OUTPUT_CHANNELS || config == NULL) {
        return 1;
    }
    
    memcpy(config, &crossoverConfig[outputChannel], sizeof(CrossoverConfig_TypeDef));
    return 0;
}

/**
  * @brief  Proses sinyal audio melalui filter crossover
  * @param  outputChannel: Channel output (0-3)
  * @param  input: Nilai input sampel audio
  * @retval Nilai output yang telah diproses
  */
float Crossover_ProcessSample(uint8_t outputChannel, float input)
{
    float output = 0.0f;
    
    if (outputChannel >= AUDIO_OUTPUT_CHANNELS) {
        return input; // Return unprocessed sample on invalid channel
    }

    /* Process based on filter mode */
    switch (crossoverConfig[outputChannel].filterMode) {
        case CROSSOVER_MODE_LOWPASS:
            output = ProcessFilterChain(&lowpassFilters[outputChannel], input);
            break;
            
        case CROSSOVER_MODE_HIGHPASS:
            output = ProcessFilterChain(&highpassFilters[outputChannel], input);
            break;
            
        case CROSSOVER_MODE_BANDPASS:
            /* Bandpass is implemented as cascaded high-pass and low-pass */
            output = ProcessFilterChain(&bandpassHighFilters[outputChannel], input);
            output = ProcessFilterChain(&bandpassLowFilters[outputChannel], output);
            break;
            
        case CROSSOVER_MODE_FULLRANGE:
        default:
            output = input; // Bypass processing
            break;
    }
    
    return output;
}

/**
  * @brief  Reset filter state (clear history)
  * @param  outputChannel: Channel output (0-3)
  * @retval None
  */
void Crossover_ResetFilterState(uint8_t outputChannel)
{
    if (outputChannel >= AUDIO_OUTPUT_CHANNELS) {
        return;
    }
    
    ClearFilterHistory(&highpassFilters[outputChannel]);
    ClearFilterHistory(&lowpassFilters[outputChannel]);
    ClearFilterHistory(&bandpassHighFilters[outputChannel]);
    ClearFilterHistory(&bandpassLowFilters[outputChannel]);
    
    DEBUG_PRINT("Crossover filter state reset for channel %d\r\n", outputChannel);
}

/**
  * @brief  Hitung koefisien filter berdasarkan parameter saat ini
  * @param  outputChannel: Channel output (0-3)
  * @retval None
  */
static void CalculateFilterCoefficients(uint8_t outputChannel)
{
    float sampleRate = Audio_GetSampleRate();
    uint8_t order = crossoverConfig[outputChannel].order;
    float lowFreq = crossoverConfig[outputChannel].lowFrequency;
    float highFreq = crossoverConfig[outputChannel].highFrequency;
    CrossoverFilterType_TypeDef filterType = crossoverConfig[outputChannel].type;
    
    uint8_t numStages = GetNumStagesForOrder(order);
    BiquadCoeff_TypeDef coeffs;
    
    /* Clear old filter configurations */
    highpassFilters[outputChannel].numStages = 0;
    lowpassFilters[outputChannel].numStages = 0;
    bandpassHighFilters[outputChannel].numStages = 0;
    bandpassLowFilters[outputChannel].numStages = 0;
    
    /* Calculate gain compensation for each filter type */
    float gainCompensation = ComputeGainCompensation(filterType, order);
    
    /* Configure filters based on the mode */
    switch (crossoverConfig[outputChannel].filterMode) {
        case CROSSOVER_MODE_LOWPASS:
            lowpassFilters[outputChannel].numStages = numStages;
            lowpassFilters[outputChannel].gain = gainCompensation;
            
            /* Calculate coefficients for each stage */
            for (uint8_t stage = 0; stage < numStages; stage++) {
                switch (filterType) {
                    case CROSSOVER_TYPE_BUTTERWORTH:
                        Butterworth_LowPass(&coeffs, stage, numStages, lowFreq, sampleRate);
                        break;
                    case CROSSOVER_TYPE_LINKWITZ_RILEY:
                        LinkwitzRiley_LowPass(&coeffs, stage, numStages, lowFreq, sampleRate);
                        break;
                    case CROSSOVER_TYPE_BESSEL:
                        Bessel_LowPass(&coeffs, stage, numStages, lowFreq, sampleRate);
                        break;
                }
                
                /* Initialize biquad filter with calculated coefficients */
                Biquad_Init(&lowpassFilters[outputChannel].stages[stage], &coeffs);
            }
            break;
            
        case CROSSOVER_MODE_HIGHPASS:
            highpassFilters[outputChannel].numStages = numStages;
            highpassFilters[outputChannel].gain = gainCompensation;
            
            /* Calculate coefficients for each stage */
            for (uint8_t stage = 0; stage < numStages; stage++) {
                switch (filterType) {
                    case CROSSOVER_TYPE_BUTTERWORTH:
                        Butterworth_HighPass(&coeffs, stage, numStages, highFreq, sampleRate);
                        break;
                    case CROSSOVER_TYPE_LINKWITZ_RILEY:
                        LinkwitzRiley_HighPass(&coeffs, stage, numStages, highFreq, sampleRate);
                        break;
                    case CROSSOVER_TYPE_BESSEL:
                        Bessel_HighPass(&coeffs, stage, numStages, highFreq, sampleRate);
                        break;
                }
                
                /* Initialize biquad filter with calculated coefficients */
                Biquad_Init(&highpassFilters[outputChannel].stages[stage], &coeffs);
            }
            break;
            
        case CROSSOVER_MODE_BANDPASS:
            /* Bandpass = HighPass + LowPass in cascade */
            bandpassHighFilters[outputChannel].numStages = numStages;
            bandpassLowFilters[outputChannel].numStages = numStages;
            bandpassHighFilters[outputChannel].gain = gainCompensation;
            bandpassLowFilters[outputChannel].gain = 1.0f; // Only apply gain once
            
            /* Calculate coefficients for high-pass part */
            for (uint8_t stage = 0; stage < numStages; stage++) {
                switch (filterType) {
                    case CROSSOVER_TYPE_BUTTERWORTH:
                        Butterworth_HighPass(&coeffs, stage, numStages, lowFreq, sampleRate);
                        break;
                    case CROSSOVER_TYPE_LINKWITZ_RILEY:
                        LinkwitzRiley_HighPass(&coeffs, stage, numStages, lowFreq, sampleRate);
                        break;
                    case CROSSOVER_TYPE_BESSEL:
                        Bessel_HighPass(&coeffs, stage, numStages, lowFreq, sampleRate);
                        break;
                }
                
                /* Initialize biquad filter with calculated coefficients */
                Biquad_Init(&bandpassHighFilters[outputChannel].stages[stage], &coeffs);
            }
            
            /* Calculate coefficients for low-pass part */
            for (uint8_t stage = 0; stage < numStages; stage++) {
                switch (filterType) {
                    case CROSSOVER_TYPE_BUTTERWORTH:
                        Butterworth_LowPass(&coeffs, stage, numStages, highFreq, sampleRate);
                        break;
                    case CROSSOVER_TYPE_LINKWITZ_RILEY:
                        LinkwitzRiley_LowPass(&coeffs, stage, numStages, highFreq, sampleRate);
                        break;
                    case CROSSOVER_TYPE_BESSEL:
                        Bessel_LowPass(&coeffs, stage, numStages, highFreq, sampleRate);
                        break;
                }
                
                /* Initialize biquad filter with calculated coefficients */
                Biquad_Init(&bandpassLowFilters[outputChannel].stages[stage], &coeffs);
            }
            break;
            
        case CROSSOVER_MODE_FULLRANGE:
        default:
            /* No filters needed for full range */
            break;
    }
    
    /* Reset filter states to avoid transient at coefficient change */
    Crossover_ResetFilterState(outputChannel);
}

/**
  * @brief  Tentukan jumlah stage biquad berdasarkan orde filter
  * @param  order: Orde filter
  * @retval Jumlah stage biquad
  */
static uint8_t GetNumStagesForOrder(uint8_t order)
{
    /* Calculate number of biquad stages needed for specified order */
    uint8_t numStages = order / 2;
    
    /* If odd order, add one more stage */
    if (order % 2 != 0) {
        numStages++;
    }
    
    /* Ensure we don't exceed maximum */
    if (numStages > MAX_FILTER_STAGES) {
        numStages = MAX_FILTER_STAGES;
    }
    
    return numStages;
}

/**
  * @brief  Reset riwayat filter (clear internal state)
  * @param  filter: Pointer ke filter chain
  * @retval None
  */
static void ClearFilterHistory(FilterChain_TypeDef* filter)
{
    if (filter == NULL) {
        return;
    }
    
    for (uint8_t i = 0; i < filter->numStages; i++) {
        Biquad_Reset(&filter->stages[i]);
    }
}

/**
  * @brief  Memproses sampel audio dengan filter chain
  * @param  filter: Pointer ke filter chain
  * @param  input: Nilai sampel input
  * @retval Nilai sampel output
  */
static float ProcessFilterChain(FilterChain_TypeDef* filter, float input)
{
    float output = input;
    
    if (filter == NULL || filter->numStages == 0) {
        return input;
    }
    
    /* Process through each biquad stage */
    for (uint8_t i = 0; i < filter->numStages; i++) {
        output = Biquad_Process(&filter->stages[i], output);
    }
    
    /* Apply gain compensation */
    output *= filter->gain;
    
    return output;
}

/**
  * @brief  Hitung kompensasi gain untuk tipe filter tertentu
  * @param  filterType: Tipe filter
  * @param  order: Orde filter
  * @retval Nilai kompensasi gain
  */
static float ComputeGainCompensation(CrossoverFilterType_TypeDef filterType, uint8_t order)
{
    float gain = 1.0f;
    
    /* Apply gain compensation based on filter type and order */
    switch (filterType) {
        case CROSSOVER_TYPE_BUTTERWORTH:
            /* Butterworth has flat passband response, no compensation needed */
            gain = 1.0f;
            break;
            
        case CROSSOVER_TYPE_LINKWITZ_RILEY:
            /* LinkwitzRiley filters need gain compensation at crossover
               since summing two LR4 filters results in 0dB at crossover */
            if (order == 2) {      // LR4 (2nd order per filter)
                gain = 0.9f;       // Minor adjustment
            } else if (order == 4) { // LR8 (4th order per filter)
                gain = 0.95f;      // Minor adjustment
            } else if (order >= 6) { // Higher orders
                gain = 1.0f;       // No additional adjustment needed
            }
            break;
            
        case CROSSOVER_TYPE_BESSEL:
            /* Bessel filters may need slight gain boost in some cases */
            if (order >= 4) {
                gain = 1.05f;      // Small boost for higher orders
            }
            break;
            
        default:
            gain = 1.0f;
            break;
    }
    
    return gain;
}

/**
  * @brief  Fungsi helper untuk mendapatkan nama tipe filter dalam string
  * @param  type: Tipe filter
  * @retval Pointer ke string nama
  */
const char* Crossover_GetFilterTypeName(CrossoverFilterType_TypeDef type)
{
    switch (type) {
        case CROSSOVER_TYPE_BUTTERWORTH:  return "BUT";
        case CROSSOVER_TYPE_LINKWITZ_RILEY: return "LR";
        case CROSSOVER_TYPE_BESSEL:       return "BES";
        default:                          return "???";
    }
}

/**
  * @brief  Fungsi helper untuk mendapatkan nama mode filter dalam string
  * @param  mode: Mode filter
  * @retval Pointer ke string nama
  */
const char* Crossover_GetFilterModeName(CrossoverFilterMode_TypeDef mode)
{
    switch (mode) {
        case CROSSOVER_MODE_LOWPASS:    return "LPF";
        case CROSSOVER_MODE_HIGHPASS:   return "HPF";
        case CROSSOVER_MODE_BANDPASS:   return "BPF";
        case CROSSOVER_MODE_FULLRANGE:  return "FULL";
        default:                        return "???";
    }
}