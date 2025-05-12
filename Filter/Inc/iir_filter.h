/**
  ******************************************************************************
  * @file           : iir_filter.h
  * @brief          : Interface untuk implementasi IIR filter
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Modul IIR filter untuk aplikasi Crossover Audio
  * Target: STM32F411 (Black Pill)
  *
  ******************************************************************************
  */

#ifndef __IIR_FILTER_H
#define __IIR_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "filter_types.h"

/**
  * @brief  Inisialisasi filter IIR
  * @param  filter: Pointer ke struktur filter IIR
  * @retval Status operasi (true jika berhasil)
  */
bool IIR_Init(IIRFilter_t *filter);

/**
  * @brief  Reset state filter IIR (menghapus semua riwayat sample)
  * @param  filter: Pointer ke struktur filter IIR
  * @retval Status operasi (true jika berhasil)
  */
bool IIR_Reset(IIRFilter_t *filter);

/**
  * @brief  Memproses satu sample melalui filter IIR
  * @param  filter: Pointer ke struktur filter IIR
  * @param  input: Sample input
  * @retval Sample output setelah diproses
  */
float IIR_ProcessSample(IIRFilter_t *filter, float input);

/**
  * @brief  Memproses buffer sample melalui filter IIR
  * @param  filter: Pointer ke struktur filter IIR
  * @param  input: Pointer ke buffer input
  * @param  output: Pointer ke buffer output (bisa sama dengan input untuk in-place)
  * @param  len: Jumlah sample yang akan diproses
  * @retval Status operasi (true jika berhasil)
  */
bool IIR_ProcessBuffer(IIRFilter_t *filter, float *input, float *output, uint16_t len);

/**
  * @brief  Memproses buffer stereo frame melalui filter IIR (float interleaved L,R,L,R)
  * @param  filterLeft: Pointer ke struktur filter IIR untuk kanal kiri
  * @param  filterRight: Pointer ke struktur filter IIR untuk kanal kanan
  * @param  input: Pointer ke buffer input (interleaved L,R,L,R)
  * @param  output: Pointer ke buffer output (bisa sama dengan input untuk in-place)
  * @param  numFrames: Jumlah frame stereo yang akan diproses
  * @retval Status operasi (true jika berhasil)
  */
bool IIR_ProcessStereoBuffer(IIRFilter_t *filterLeft, IIRFilter_t *filterRight, 
                             float *input, float *output, uint16_t numFrames);

/**
  * @brief  Memproses buffer multi-kanal melalui filter IIR
  * @param  filters: Array pointer ke struktur filter IIR untuk setiap kanal
  * @param  input: Pointer ke buffer input (interleaved format)
  * @param  output: Pointer ke buffer output (bisa sama dengan input untuk in-place)
  * @param  numChannels: Jumlah kanal dalam buffer
  * @param  numFrames: Jumlah frame yang akan diproses
  * @retval Status operasi (true jika berhasil)
  */
bool IIR_ProcessMultiChannelBuffer(IIRFilter_t **filters, float *input, float *output, 
                                   uint8_t numChannels, uint16_t numFrames);

/**
  * @brief  Menghitung fase filter pada frekuensi tertentu
  * @param  filter: Pointer ke struktur filter IIR
  * @param  frequency: Frekuensi dalam Hz
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Fase filter dalam derajat
  */
float IIR_CalculatePhase(IIRFilter_t *filter, float frequency, float sampleRate);

/**
  * @brief  Menghitung magnitude filter pada frekuensi tertentu
  * @param  filter: Pointer ke struktur filter IIR
  * @param  frequency: Frekuensi dalam Hz
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Magnitude filter dalam dB
  */
float IIR_CalculateMagnitude(IIRFilter_t *filter, float frequency, float sampleRate);

/**
  * @brief  Mengaktifkan atau menonaktifkan satu section biquad dalam filter IIR
  * @param  filter: Pointer ke struktur filter IIR
  * @param  sectionIndex: Indeks section (dimulai dari 0)
  * @param  enabled: Status enabled (true = aktif, false = bypass)
  * @retval Status operasi (true jika berhasil)
  */
bool IIR_SetSectionEnabled(IIRFilter_t *filter, uint8_t sectionIndex, bool enabled);

/**
  * @brief  Mengaktifkan atau menonaktifkan seluruh filter IIR
  * @param  filter: Pointer ke struktur filter IIR
  * @param  enabled: Status enabled (true = aktif, false = bypass)
  * @retval Status operasi (true jika berhasil)
  */
bool IIR_SetEnabled(IIRFilter_t *filter, bool enabled);

/**
  * @brief  Memeriksa apakah filter sedang aktif
  * @param  filter: Pointer ke struktur filter IIR
  * @retval Status aktif (true = aktif, false = bypass)
  */
bool IIR_IsEnabled(IIRFilter_t *filter);

/**
  * @brief  Mendapatkan delay filter dalam sample
  * @param  filter: Pointer ke struktur filter IIR
  * @retval Delay dalam jumlah sample
  */
float IIR_GetDelay(IIRFilter_t *filter);

/**
  * @brief  Mendapatkan delay group filter pada frekuensi tertentu
  * @param  filter: Pointer ke struktur filter IIR
  * @param  frequency: Frekuensi dalam Hz
  * @param  sampleRate: Sample rate dalam Hz
  * @retval Group delay dalam ms
  */
float IIR_GetGroupDelay(IIRFilter_t *filter, float frequency, float sampleRate);

/**
  * @brief  Mendapatkan representasi teks dari tipe filter
  * @param  type: Tipe filter
  * @retval String representasi tipe filter
  */
const char* IIR_GetFilterTypeString(FilterType_t type);

/**
  * @brief  Mendapatkan representasi teks dari kategori filter
  * @param  category: Kategori filter
  * @retval String representasi kategori filter
  */
const char* IIR_GetFilterCategoryString(FilterCategory_t category);

/**
  * @brief  Mendapatkan representasi teks dari order filter
  * @param  order: Order filter
  * @retval String representasi order filter
  */
const char* IIR_GetFilterOrderString(FilterOrder_t order);

#ifdef __cplusplus
}
#endif

#endif /* __IIR_FILTER_H */