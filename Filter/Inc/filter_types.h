/**
  ******************************************************************************
  * @file           : filter_types.h
  * @brief          : Tipe data untuk implementasi filter DSP
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Tipe data untuk filter DSP untuk aplikasi Crossover Audio
  * Target: STM32F411 (Black Pill)
  *
  ******************************************************************************
  */

#ifndef __FILTER_TYPES_H
#define __FILTER_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "dsp_common.h"

/**
  * @brief  Tipe filter yang tersedia
  */
typedef enum {
  FILTER_TYPE_NONE = 0,
  FILTER_TYPE_LOWPASS,     /* Filter low pass */
  FILTER_TYPE_HIGHPASS,    /* Filter high pass */
  FILTER_TYPE_BANDPASS,    /* Filter band pass */
  FILTER_TYPE_BANDSTOP,    /* Filter band stop/notch */
  FILTER_TYPE_LOWSHELF,    /* Filter low shelf */
  FILTER_TYPE_HIGHSHELF,   /* Filter high shelf */
  FILTER_TYPE_PEAKING,     /* Filter peaking/bell */
  FILTER_TYPE_ALLPASS      /* Filter all pass */
} FilterType_t;

/**
  * @brief  Kategori filter yang tersedia
  */
typedef enum {
  FILTER_CATEGORY_NONE = 0,
  FILTER_CATEGORY_BUTTERWORTH,  /* Filter Butterworth */
  FILTER_CATEGORY_LINKWITZ_RILEY, /* Filter Linkwitz-Riley */
  FILTER_CATEGORY_BESSEL,       /* Filter Bessel */
  FILTER_CATEGORY_CUSTOM        /* Filter kustom dengan koefisien manual */
} FilterCategory_t;

/**
  * @brief  Order/slope filter yang tersedia
  */
typedef enum {
  FILTER_ORDER_NONE = 0,
  FILTER_ORDER_FIRST = 1,    /* 1st order (6dB/oct) */
  FILTER_ORDER_SECOND = 2,   /* 2nd order (12dB/oct) */
  FILTER_ORDER_THIRD = 3,    /* 3rd order (18dB/oct) */
  FILTER_ORDER_FOURTH = 4,   /* 4th order (24dB/oct) */
  FILTER_ORDER_SIXTH = 6,    /* 6th order (36dB/oct) */
  FILTER_ORDER_EIGHTH = 8    /* 8th order (48dB/oct) */
} FilterOrder_t;

/**
  * @brief  Struktur koefisien biquad
  * @note   Koefisien dalam bentuk normalized, dengan a0 = 1.0
  */
typedef struct {
  float b0;      /* Koefisien b0 */
  float b1;      /* Koefisien b1 */
  float b2;      /* Koefisien b2 */
  float a1;      /* Koefisien a1 (a0 = 1.0, normalized) */
  float a2;      /* Koefisien a2 */
} BiquadCoeff_t;

/**
  * @brief  Status biquad untuk menyimpan riwayat sample
  */
typedef struct {
  float x1;      /* Input sample[n-1] */
  float x2;      /* Input sample[n-2] */
  float y1;      /* Output sample[n-1] */
  float y2;      /* Output sample[n-2] */
} BiquadState_t;

/**
  * @brief  Struktur biquad filter (satu section)
  */
typedef struct {
  BiquadCoeff_t coeff;   /* Koefisien filter */
  BiquadState_t state;   /* State (riwayat) filter */
  bool enabled;          /* Status enabled/disabled */
} BiquadFilter_t;

/**
  * @brief  Struktur IIR filter cascade (dapat berisi multiple biquad)
  */
typedef struct {
  uint8_t numSections;            /* Jumlah section biquad */
  BiquadFilter_t sections[4];     /* Array biquad sections, max 4 (= orde 8) */
  FilterType_t type;              /* Tipe filter */
  FilterCategory_t category;      /* Kategori filter */
  FilterOrder_t order;            /* Order filter */
  float frequency;                /* Frekuensi cut-off dalam Hz */
  float q;                        /* Q-factor / bandwidth */
  float gain;                     /* Gain untuk EQ filter (dalam dB) */
  bool enabled;                   /* Status enabled/disabled */
} IIRFilter_t;

/**
  * @brief  Parameter desain filter
  */
typedef struct {
  float sampleRate;       /* Sample rate dalam Hz */
  float frequency;        /* Frekuensi cut-off dalam Hz */
  float q;                /* Q-factor / bandwidth */
  float gainDB;           /* Gain dalam dB (untuk filter shelf/peak) */
  FilterType_t type;      /* Tipe filter */
  FilterCategory_t category; /* Kategori filter */
  FilterOrder_t order;    /* Order filter */
} FilterDesignParams_t;

#ifdef __cplusplus
}
#endif

#endif /* __FILTER_TYPES_H */