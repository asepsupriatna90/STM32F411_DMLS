/**
  ******************************************************************************
  * @file           : limiter_proc.c
  * @brief          : Implementation of audio limiter processing
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Implementasi pemrosesan limiter untuk DSP Audio Crossover Aktif
  * Limiter berfungsi sebagai pelindung perangkat audio dengan mencegah
  * terjadinya clipping dan distorsi berlebih
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "limiter.h"
#include "limiter_types.h"
#include "dsp_common.h"
#include "math_utils.h"
#include "debug.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct {
  float currentGain;          /* Gain reduction saat ini */
  float peakLevel;            /* Level puncak yang terdeteksi */
  float releaseTime;          /* Waktu release dalam sampel */
  float attackTime;           /* Waktu attack dalam sampel */
  uint32_t holdCounter;       /* Counter untuk hold time */
  float targetGain;           /* Target gain reduction */
  float envelope;             /* Amplop sinyal terdeteksi */
  float lookaheadBuffer[LIMITER_MAX_LOOKAHEAD]; /* Buffer untuk lookahead */
  uint16_t lookaheadIndex;    /* Indeks untuk buffer lookahead */
  float prevSample;           /* Sampel sebelumnya untuk smoothing */
} LimiterState_Internal;

/* Private variables ---------------------------------------------------------*/
static LimiterState_Internal limiterState[AUDIO_OUTPUT_CHANNELS];
static uint8_t limiterInitialized = 0;

/* Private constants ---------------------------------------------------------*/
static const float MIN_GAIN_DB = -24.0f;    /* Batas gain reduction minimum */
static const float ENVELOPE_SMOOTHING = 0.9f; /* Faktor smoothing envelope */
static const uint32_t DEFAULT_HOLD_SAMPLES = 50; /* Hold time default dalam sampel */

/* Private function prototypes -----------------------------------------------*/
static float Limiter_ProcessSample(uint8_t channel, float inputSample);
static float Limiter_CalculateGainReduction(uint8_t channel, float inputLevel);
static void Limiter_UpdateReleaseEnvelope(uint8_t channel, float gainReduction);
static float Limiter_ApplyInterSampleProtection(uint8_t channel, float inputSample);
static float Limiter_ProcessLookahead(uint8_t channel, float inputSample);
static void Limiter_CalculateTimingParameters(uint8_t channel);

/**
  * @brief  Initialize limiter dengan setting default
  * @param  channel: Channel yang diinisialisasi
  * @param  config: Pointer ke struktur konfigurasi limiter
  * @retval Status hasil inisialisasi
  */
LimiterStatus_TypeDef Limiter_Init(uint8_t channel, Limiter_TypeDef *config) 
{
  if (channel >= AUDIO_OUTPUT_CHANNELS) {
    DEBUG_PRINT("Limiter init failed: Invalid channel %d\r\n", channel);
    return LIMITER_ERROR;
  }

  /* Reset state internal */
  limiterState[channel].currentGain = 1.0f;
  limiterState[channel].peakLevel = 0.0f;
  limiterState[channel].targetGain = 1.0f;
  limiterState[channel].envelope = 0.0f;
  limiterState[channel].holdCounter = 0;
  limiterState[channel].prevSample = 0.0f;
  limiterState[channel].lookaheadIndex = 0;

  /* Clear lookahead buffer */
  for (uint16_t i = 0; i < LIMITER_MAX_LOOKAHEAD; i++) {
    limiterState[channel].lookaheadBuffer[i] = 0.0f;
  }

  /* Hitung parameter timing */
  Limiter_CalculateTimingParameters(channel);

  limiterInitialized = 1;
  DEBUG_PRINT("Limiter initialized for channel %d\r\n", channel);
  
  return LIMITER_OK;
}

/**
  * @brief  Reset limiter state ke kondisi default
  * @param  channel: Channel yang akan direset
  * @retval Status hasil reset
  */
LimiterStatus_TypeDef Limiter_Reset(uint8_t channel)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS) {
    return LIMITER_ERROR;
  }

  limiterState[channel].currentGain = 1.0f;
  limiterState[channel].peakLevel = 0.0f;
  limiterState[channel].targetGain = 1.0f;
  limiterState[channel].envelope = 0.0f;
  limiterState[channel].holdCounter = 0;
  limiterState[channel].prevSample = 0.0f;
  
  /* Clear lookahead buffer */
  for (uint16_t i = 0; i < LIMITER_MAX_LOOKAHEAD; i++) {
    limiterState[channel].lookaheadBuffer[i] = 0.0f;
  }
  limiterState[channel].lookaheadIndex = 0;

  return LIMITER_OK;
}

/**
  * @brief  Proses block data audio dengan limiter
  * @param  channel: Channel yang akan diproses
  * @param  pData: Blok data input yang akan dibatasi
  * @param  blockSize: Ukuran blok dalam sampel
  * @retval Status hasil pemrosesan
  */
LimiterStatus_TypeDef Limiter_ProcessBlock(uint8_t channel, float *pData, uint16_t blockSize)
{
  if (!limiterInitialized || channel >= AUDIO_OUTPUT_CHANNELS || pData == NULL) {
    return LIMITER_ERROR;
  }

  /* Process each sample in the block */
  for (uint16_t i = 0; i < blockSize; i++) {
    pData[i] = Limiter_ProcessSample(channel, pData[i]);
  }

  return LIMITER_OK;
}

/**
  * @brief  Memproses satu sampel audio dengan limiter
  * @param  channel: Channel yang akan diproses
  * @param  inputSample: Sampel input yang akan dibatasi
  * @retval Sampel output yang telah dibatasi
  */
static float Limiter_ProcessSample(uint8_t channel, float inputSample)
{
  float outputSample;
  
  /* Check for inter-sample peaks */
  float processedSample = Limiter_ApplyInterSampleProtection(channel, inputSample);
  
  /* Process through lookahead if enabled */
  processedSample = Limiter_ProcessLookahead(channel, processedSample);
  
  /* Get the absolute sample value */
  float sampleAbs = fabsf(processedSample);
  
  /* Update peak level using envelope follower */
  limiterState[channel].peakLevel = fmaxf(sampleAbs, 
      ENVELOPE_SMOOTHING * limiterState[channel].peakLevel + 
      (1.0f - ENVELOPE_SMOOTHING) * sampleAbs);
  
  /* Calculate gain reduction */
  float gainReduction = Limiter_CalculateGainReduction(channel, limiterState[channel].peakLevel);
  
  /* Apply gain reduction to produce output sample */
  outputSample = processedSample * limiterState[channel].currentGain;
  
  /* Update release envelope */
  Limiter_UpdateReleaseEnvelope(channel, gainReduction);
  
  /* Hard clip to prevent any overflows (safety measure) */
  if (outputSample > 1.0f) outputSample = 1.0f;
  if (outputSample < -1.0f) outputSample = -1.0f;
  
  /* Store previous sample for inter-sample detection */
  limiterState[channel].prevSample = inputSample;
  
  return outputSample;
}

/**
  * @brief  Menghitung nilai gain reduction berdasarkan level input
  * @param  channel: Channel yang akan dihitung
  * @param  inputLevel: Level input saat ini
  * @retval Nilai gain reduction yang dihitung
  */
static float Limiter_CalculateGainReduction(uint8_t channel, float inputLevel)
{
  float gainReduction = 1.0f;
  Limiter_TypeDef *config = Limiter_GetConfig(channel);
  
  /* Jika level melebihi threshold, hitung gain reduction */
  if (inputLevel > config->threshold) {
    /* Hitung gain reduction dalam linear */
    gainReduction = config->threshold / inputLevel;
    
    /* Terapkan knee */
    if (config->knee > 0.0f) {
      float kneeStart = config->threshold - (config->knee / 2.0f);
      float kneeEnd = config->threshold + (config->knee / 2.0f);
      
      if (inputLevel < kneeEnd) {
        /* Dalam knee region, terapkan transisi halus */
        float kneeRatio = (inputLevel - kneeStart) / config->knee;
        gainReduction = 1.0f - kneeRatio * (1.0f - gainReduction);
      }
    }
    
    /* Pastikan gain reduction tidak kurang dari batas minimum */
    float gainReductionDB = 20.0f * log10f(gainReduction);
    if (gainReductionDB < MIN_GAIN_DB) {
      gainReduction = DB_TO_LINEAR(MIN_GAIN_DB);
    }
    
    limiterState[channel].targetGain = gainReduction;
    limiterState[channel].holdCounter = DEFAULT_HOLD_SAMPLES;
  } else {
    /* Jika di bawah threshold, target gain adalah 1.0 (no reduction) */
    limiterState[channel].targetGain = 1.0f;
    
    /* Decrement hold counter jika masih dalam periode hold */
    if (limiterState[channel].holdCounter > 0) {
      limiterState[channel].holdCounter--;
    }
  }
  
  return gainReduction;
}

/**
  * @brief  Update envelope release
  * @param  channel: Channel yang akan diupdate
  * @param  gainReduction: Nilai gain reduction saat ini
  * @retval None
  */
static void Limiter_UpdateReleaseEnvelope(uint8_t channel, float gainReduction)
{
  Limiter_TypeDef *config = Limiter_GetConfig(channel);
  
  /* Jika dalam hold period, pertahankan current gain */
  if (limiterState[channel].holdCounter > 0) {
    limiterState[channel].currentGain = limiterState[channel].targetGain;
    return;
  }

  /* Attack phase - gain reduction semakin besar */
  if (gainReduction < limiterState[channel].currentGain) {
    /* Cepat attack untuk hasil yang responsif */
    float attackCoeff = 1.0f / limiterState[channel].attackTime;
    limiterState[channel].currentGain = limiterState[channel].currentGain * (1.0f - attackCoeff) + gainReduction * attackCoeff;
  } 
  /* Release phase - gain reduction semakin kecil */
  else if (gainReduction > limiterState[channel].currentGain) {
    /* Release lebih lambat untuk mencegah distorsi */
    float releaseCoeff = 1.0f / limiterState[channel].releaseTime;
    
    /* Gunakan karakteristik release yang eksponen */
    limiterState[channel].currentGain = limiterState[channel].currentGain * (1.0f - releaseCoeff) + gainReduction * releaseCoeff;
    
    /* Adaptif release - semakin besar gain reduction, semakin lambat release */
    if (config->adaptiveRelease) {
      /* Skala koefisien release berdasarkan gain reduction saat ini */
      float adaptiveScale = 1.0f + 5.0f * (1.0f - limiterState[channel].currentGain);
      limiterState[channel].currentGain = limiterState[channel].currentGain * (1.0f - releaseCoeff/adaptiveScale) + 
                                          gainReduction * releaseCoeff/adaptiveScale;
    }
  }
}

/**
  * @brief  Menerapkan proteksi inter-sample peak
  * @param  channel: Channel yang akan diproses
  * @param  inputSample: Sampel input
  * @retval Sampel yang telah diproses
  */
static float Limiter_ApplyInterSampleProtection(uint8_t channel, float inputSample)
{
  Limiter_TypeDef *config = Limiter_GetConfig(channel);
  
  if (!config->enableISP) {
    return inputSample;
  }
  
  /* Deteksi kemungkinan inter-sample peak dengan oversampling prediktif */
  float prevSample = limiterState[channel].prevSample;
  float predictedPeak = 0.0f;
  
  /* Prediksi inter-sample peak menggunakan interpolasi kuadratik */
  if ((inputSample > 0 && prevSample > 0) || (inputSample < 0 && prevSample < 0)) {
    /* Jika keduanya memiliki tanda yang sama, cek kemungkinan puncak */
    if (fabsf(inputSample) > fabsf(prevSample)) {
      predictedPeak = inputSample * 1.05f; /* 5% margin */
    } else {
      predictedPeak = prevSample * 1.05f; /* 5% margin */
    }
  } else {
    /* Jika ada perubahan tanda, interpolasi untuk menemukan puncak */
    float t = fabsf(prevSample) / (fabsf(prevSample) + fabsf(inputSample));
    float maxPossibleValue = fabsf(prevSample) * (1.0f - t) + fabsf(inputSample) * t;
    predictedPeak = maxPossibleValue * 1.15f; /* 15% margin untuk perubahan tanda */
  }
  
  /* Jika predicted peak melebihi nilai sample, gunakan predicted peak */
  if (fabsf(predictedPeak) > fabsf(inputSample)) {
    if (inputSample >= 0) {
      return predictedPeak;
    } else {
      return -predictedPeak;
    }
  }
  
  return inputSample;
}

/**
  * @brief  Memproses sampel melalui sistem lookahead
  * @param  channel: Channel yang akan diproses
  * @param  inputSample: Sampel input untuk lookahead
  * @retval Sampel dari buffer lookahead
  */
static float Limiter_ProcessLookahead(uint8_t channel, float inputSample)
{
  Limiter_TypeDef *config = Limiter_GetConfig(channel);
  
  if (!config->enableLookahead || config->lookaheadTime <= 0) {
    return inputSample;
  }
  
  /* Simpan sampel saat ini ke buffer lookahead */
  limiterState[channel].lookaheadBuffer[limiterState[channel].lookaheadIndex] = inputSample;
  
  /* Hitung indeks output berdasarkan delay lookahead */
  uint16_t outputIndex = (limiterState[channel].lookaheadIndex + 
                          LIMITER_MAX_LOOKAHEAD - config->lookaheadTime) % 
                          LIMITER_MAX_LOOKAHEAD;
                          
  /* Ambil sampel dari buffer lookahead untuk output */
  float outputSample = limiterState[channel].lookaheadBuffer[outputIndex];
  
  /* Update indeks buffer lookahead */
  limiterState[channel].lookaheadIndex = (limiterState[channel].lookaheadIndex + 1) % LIMITER_MAX_LOOKAHEAD;
  
  return outputSample;
}

/**
  * @brief  Calculate timing parameters for attack and release
  * @param  channel: Channel to calculate parameters for
  * @retval None
  */
static void Limiter_CalculateTimingParameters(uint8_t channel)
{
  Limiter_TypeDef *config = Limiter_GetConfig(channel);
  
  /* Convert time values from ms to samples */
  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  
  /* Calculate attack time (in samples) */
  limiterState[channel].attackTime = (config->attackTime / 1000.0f) * sampleRate;
  if (limiterState[channel].attackTime < 1.0f) {
    limiterState[channel].attackTime = 1.0f;
  }
  
  /* Calculate release time (in samples) */
  limiterState[channel].releaseTime = (config->releaseTime / 1000.0f) * sampleRate;
  if (limiterState[channel].releaseTime < 1.0f) {
    limiterState[channel].releaseTime = 1.0f;
  }
}

/**
  * @brief  Update configuration for limiter
  * @param  channel: Channel to update
  * @param  config: Pointer to new configuration
  * @retval Status of the update operation
  */
LimiterStatus_TypeDef Limiter_UpdateConfig(uint8_t channel, Limiter_TypeDef *config)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS || config == NULL) {
    return LIMITER_ERROR;
  }
  
  /* Validate configuration parameters */
  if (config->threshold <= 0.0f || config->threshold > 1.0f) {
    DEBUG_PRINT("Invalid limiter threshold: %f\r\n", config->threshold);
    return LIMITER_ERROR;
  }
  
  if (config->attackTime < 0.01f) {
    config->attackTime = 0.01f;
    DEBUG_PRINT("Limiter attack time limited to 0.01ms\r\n");
  }
  
  if (config->releaseTime < 1.0f) {
    config->releaseTime = 1.0f;
    DEBUG_PRINT("Limiter release time limited to 1ms\r\n");
  }
  
  if (config->lookaheadTime > LIMITER_MAX_LOOKAHEAD) {
    config->lookaheadTime = LIMITER_MAX_LOOKAHEAD;
    DEBUG_PRINT("Limiter lookahead time limited to max value\r\n");
  }
  
  /* Save config to global configuration structure */
  Limiter_SetConfig(channel, config);
  
  /* Recalculate timing parameters */
  Limiter_CalculateTimingParameters(channel);
  
  DEBUG_PRINT("Limiter config updated for channel %d: threshold=%f, attack=%f, release=%f\r\n", 
              channel, config->threshold, config->attackTime, config->releaseTime);
  
  return LIMITER_OK;
}

/**
  * @brief  Get current gain reduction amount in dB
  * @param  channel: Channel to query
  * @retval Gain reduction in dB (negative value)
  */
float Limiter_GetGainReduction(uint8_t channel)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS) {
    return 0.0f;
  }
  
  /* Convert linear gain to dB (will be negative for reduction) */
  return 20.0f * log10f(limiterState[channel].currentGain);
}

/**
  * @brief  Check if limiter is currently active (reducing gain)
  * @param  channel: Channel to check
  * @retval 1 if active, 0 if inactive
  */
uint8_t Limiter_IsActive(uint8_t channel)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS) {
    return 0;
  }
  
  /* Consider the limiter active if gain reduction is more than 0.5dB */
  return (limiterState[channel].currentGain < 0.94f) ? 1 : 0;
}

/**
  * @brief  Get peak level detected by the limiter
  * @param  channel: Channel to query
  * @retval Peak level (0.0 to 1.0)
  */
float Limiter_GetPeakLevel(uint8_t channel)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS) {
    return 0.0f;
  }
  
  return limiterState[channel].peakLevel;
}

/**
  * @brief  Set lookahead time
  * @param  channel: Channel to update
  * @param  lookaheadTime: Lookahead time in samples
  * @retval Status of the operation
  */
LimiterStatus_TypeDef Limiter_SetLookahead(uint8_t channel, uint16_t lookaheadTime)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS) {
    return LIMITER_ERROR;
  }
  
  /* Check if lookahead time is within valid range */
  if (lookaheadTime > LIMITER_MAX_LOOKAHEAD) {
    lookaheadTime = LIMITER_MAX_LOOKAHEAD;
  }
  
  /* Update configuration */
  Limiter_TypeDef *config = Limiter_GetConfig(channel);
  config->lookaheadTime = lookaheadTime;
  config->enableLookahead = (lookaheadTime > 0) ? 1 : 0;
  
  /* Reset buffer if lookahead is disabled */
  if (lookaheadTime == 0) {
    for (uint16_t i = 0; i < LIMITER_MAX_LOOKAHEAD; i++) {
      limiterState[channel].lookaheadBuffer[i] = 0.0f;
    }
    limiterState[channel].lookaheadIndex = 0;
  }
  
  return LIMITER_OK;
}

/**
  * @brief  Enable/disable adaptive release
  * @param  channel: Channel to update
  * @param  enable: 1 to enable, 0 to disable
  * @retval Status of the operation
  */
LimiterStatus_TypeDef Limiter_SetAdaptiveRelease(uint8_t channel, uint8_t enable)
{
  if (channel >= AUDIO_OUTPUT_CHANNELS) {
    return LIMITER_ERROR;
  }
  
  /* Update configuration */
  Limiter_TypeDef *config = Limiter_GetConfig(channel);
  config->adaptiveRelease = enable ? 1 : 0;
  
  return LIMITER_OK;
}

/* --- End of File --- */