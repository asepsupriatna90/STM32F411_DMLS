/**
  ******************************************************************************
  * @file           : filter_design.h
  * @brief          : Interface untuk desain filter DSP
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Modul desain filter untuk aplikasi Crossover Audio
  * Target: STM32F411 (Black Pill)
  *
  ******************************************************************************
  */

#ifndef __FILTER_DESIGN_H
#define __FILTER_DESIGN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "filter_types.h"

/**
  * @brief  Desain filter IIR berdasarkan parameter yang diberikan
  * @param  filter: Pointer ke struktur filter IIR yang akan didesain
  * @param  params: Parameter desain filter
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_CreateIIR(IIRFilter_t *filter, FilterDesignParams_t *params);

/**
  * @brief  Desain filter butterworth low pass
  * @param  filter: Pointer ke struktur filter IIR yang akan didesain
  * @param  frequency: Frekuensi cut-off dalam Hz
  * @param  order: Order filter (1 sampai 8)
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_ButterworthLowPass(IIRFilter_t *filter, float frequency, FilterOrder_t order, float sampleRate);

/**
  * @brief  Desain filter butterworth high pass
  * @param  filter: Pointer ke struktur filter IIR yang akan didesain
  * @param  frequency: Frekuensi cut-off dalam Hz
  * @param  order: Order filter (1 sampai 8)
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_ButterworthHighPass(IIRFilter_t *filter, float frequency, FilterOrder_t order, float sampleRate);

/**
  * @brief  Desain filter Linkwitz-Riley low pass
  * @param  filter: Pointer ke struktur filter IIR yang akan didesain
  * @param  frequency: Frekuensi cut-off dalam Hz
  * @param  order: Order filter (harus genap: 2, 4, 6, 8)
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_LinkwitzRileyLowPass(IIRFilter_t *filter, float frequency, FilterOrder_t order, float sampleRate);

/**
  * @brief  Desain filter Linkwitz-Riley high pass
  * @param  filter: Pointer ke struktur filter IIR yang akan didesain
  * @param  frequency: Frekuensi cut-off dalam Hz
  * @param  order: Order filter (harus genap: 2, 4, 6, 8)
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_LinkwitzRileyHighPass(IIRFilter_t *filter, float frequency, FilterOrder_t order, float sampleRate);

/**
  * @brief  Desain filter Bessel low pass
  * @param  filter: Pointer ke struktur filter IIR yang akan didesain
  * @param  frequency: Frekuensi cut-off dalam Hz
  * @param  order: Order filter (1 sampai 8)
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_BesselLowPass(IIRFilter_t *filter, float frequency, FilterOrder_t order, float sampleRate);

/**
  * @brief  Desain filter Bessel high pass
  * @param  filter: Pointer ke struktur filter IIR yang akan didesain
  * @param  frequency: Frekuensi cut-off dalam Hz
  * @param  order: Order filter (1 sampai 8)
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_BesselHighPass(IIRFilter_t *filter, float frequency, FilterOrder_t order, float sampleRate);

/**
  * @brief  Desain filter parametric EQ (Bell/Peaking)
  * @param  filter: Pointer ke struktur filter IIR yang akan didesain
  * @param  frequency: Frekuensi tengah dalam Hz
  * @param  Q: Q-factor (bandwidth)
  * @param  gainDB: Gain dalam dB
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_ParametricEQ(IIRFilter_t *filter, float frequency, float Q, float gainDB, float sampleRate);

/**
  * @brief  Desain filter low shelf
  * @param  filter: Pointer ke struktur filter IIR yang akan didesain
  * @param  frequency: Frekuensi cut-off dalam Hz
  * @param  Q: Q-factor (bandwidth)
  * @param  gainDB: Gain dalam dB
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_LowShelf(IIRFilter_t *filter, float frequency, float Q, float gainDB, float sampleRate);

/**
  * @brief  Desain filter high shelf
  * @param  filter: Pointer ke struktur filter IIR yang akan didesain
  * @param  frequency: Frekuensi cut-off dalam Hz
  * @param  Q: Q-factor (bandwidth)
  * @param  gainDB: Gain dalam dB
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_HighShelf(IIRFilter_t *filter, float frequency, float Q, float gainDB, float sampleRate);

/**
  * @brief  Desain filter all-pass
  * @param  filter: Pointer ke struktur filter IIR yang akan didesain
  * @param  frequency: Frekuensi tengah dalam Hz
  * @param  Q: Q-factor (bandwidth)
  * @param  order: Order filter (1 atau 2)
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_AllPass(IIRFilter_t *filter, float frequency, float Q, FilterOrder_t order, float sampleRate);

/**
  * @brief  Desain filter band-pass
  * @param  filter: Pointer ke struktur filter IIR yang akan didesain
  * @param  frequency: Frekuensi tengah dalam Hz
  * @param  Q: Q-factor (bandwidth)
  * @param  order: Order filter (1 sampai 8)
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_BandPass(IIRFilter_t *filter, float frequency, float Q, FilterOrder_t order, float sampleRate);

/**
  * @brief  Desain filter band-stop (notch)
  * @param  filter: Pointer ke struktur filter IIR yang akan didesain
  * @param  frequency: Frekuensi tengah dalam Hz
  * @param  Q: Q-factor (bandwidth)
  * @param  order: Order filter (1 sampai 8)
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_BandStop(IIRFilter_t *filter, float frequency, float Q, FilterOrder_t order, float sampleRate);

/**
  * @brief  Menghitung kembali koefisien filter dengan parameter baru tanpa reset state
  * @param  filter: Pointer ke struktur filter IIR yang akan diperbarui
  * @param  params: Parameter desain filter baru
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_UpdateFilter(IIRFilter_t *filter, FilterDesignParams_t *params);

/**
  * @brief  Reset state filter (menghapus semua riwayat sample)
  * @param  filter: Pointer ke struktur filter IIR yang akan di-reset
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_ResetFilter(IIRFilter_t *filter);

/**
  * @brief  Mengaktifkan atau menonaktifkan filter
  * @param  filter: Pointer ke struktur filter IIR
  * @param  enabled: Status enabled (true = aktif, false = bypass)
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_SetEnabled(IIRFilter_t *filter, bool enabled);

/**
  * @brief  Mendapatkan respons filter pada frekuensi tertentu
  * @param  filter: Pointer ke struktur filter IIR
  * @param  frequency: Frekuensi dalam Hz
  * @param  sampleRate: Sample rate dalam Hz
  * @param  magnitude: Pointer untuk menyimpan magnitude respons (dalam dB)
  * @param  phase: Pointer untuk menyimpan fase respons (dalam derajat)
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_GetResponse(IIRFilter_t *filter, float frequency, float sampleRate, float *magnitude, float *phase);

/**
  * @brief  Mendapatkan nilai koefisien filter
  * @param  filter: Pointer ke struktur filter IIR
  * @param  section: Indeks section (0-3)
  * @param  coeff: Pointer untuk menyimpan koefisien
  * @retval Status operasi (true jika berhasil)
  */
bool FilterDesign_GetCoefficients(IIRFilter_t *filter, uint8_t section, BiquadCoeff_t *coeff);

#ifdef __cplusplus
}
#endif

#endif /* __FILTER_DESIGN_H */