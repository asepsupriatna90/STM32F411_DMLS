/**
  ******************************************************************************
  * @file           : codec_pcm1808.h
  * @brief          : Header file for PCM1808 ADC codec driver
  * @author         : asepsupriatna90
  ******************************************************************************
  * @attention
  *
  * Driver untuk ADC PCM1808 sebagai input audio untuk sistem DSP
  * Mendukung 2-channel input dengan resolusi hingga 24-bit
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CODEC_PCM1808_H
#define __CODEC_PCM1808_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "audio_config.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  PCM1808 Format configuration structure
  */
typedef struct {
  uint8_t DataFormat;      /* Format data (I2S, Left-Justified, DSP) */
  uint8_t MasterSlave;     /* Master atau Slave mode */
  uint8_t AudioWidth;      /* Resolusi audio (16/24/32 bit) */
  uint32_t SampleRate;     /* Sample rate (44.1kHz, 48kHz, etc.) */
} PCM1808_FormatTypeDef;

/**
  * @brief  PCM1808 Configuration structure
  */
typedef struct {
  PCM1808_FormatTypeDef Format;    /* Format konfigurasi */
  uint8_t DigitalFilter;           /* FIR/IIR filter selection */
  uint8_t DemphasisFilter;         /* De-emphasis filter (OFF, 44.1kHz, 48kHz) */
  uint8_t HighPassFilter;          /* High pass filter (ON/OFF) */
  int8_t  DigitalGain;             /* Gain digital in dB (-12dB to +12dB) */
} PCM1808_ConfigTypeDef;

/* Exported constants --------------------------------------------------------*/
/* Mode Operasi PCM1808 */
#define PCM1808_MODE_SLAVE                 0x00
#define PCM1808_MODE_MASTER                0x01

/* Format Data PCM1808 */
#define PCM1808_FORMAT_I2S                 0x00
#define PCM1808_FORMAT_LEFT_JUSTIFIED      0x01
#define PCM1808_FORMAT_RIGHT_JUSTIFIED     0x02
#define PCM1808_FORMAT_DSP                 0x03

/* Resolusi Audio PCM1808 */
#define PCM1808_WIDTH_16BIT                0x00
#define PCM1808_WIDTH_20BIT                0x01
#define PCM1808_WIDTH_24BIT                0x02
#define PCM1808_WIDTH_32BIT                0x03

/* PCM1808 Digital Filter */
#define PCM1808_FILTER_FIR                 0x00
#define PCM1808_FILTER_IIR                 0x01

/* PCM1808 De-emphasis Filter */
#define PCM1808_DEMPHASIS_NONE             0x00
#define PCM1808_DEMPHASIS_44_1KHZ          0x01
#define PCM1808_DEMPHASIS_48KHZ            0x02

/* PCM1808 High Pass Filter */
#define PCM1808_HPF_OFF                    0x00
#define PCM1808_HPF_ON                     0x01

/* PCM1808 Hardware GPIO Controls */
#define PCM1808_PIN_FMT_LOW                0x00  /* Format selection LOW: I2S/Left Justified */
#define PCM1808_PIN_FMT_HIGH               0x01  /* Format selection HIGH: Right Justified/DSP */

#define PCM1808_PIN_SF0_LOW                0x00  /* Sample format bit 0 */
#define PCM1808_PIN_SF0_HIGH               0x01

#define PCM1808_PIN_SF1_LOW                0x00  /* Sample format bit 1 */
#define PCM1808_PIN_SF1_HIGH               0x01

#define PCM1808_PIN_SCKI_LOW               0x00  /* System clock input */
#define PCM1808_PIN_SCKI_HIGH              0x01

#define PCM1808_PIN_MD0_LOW                0x00  /* Mode control bit 0 */
#define PCM1808_PIN_MD0_HIGH               0x01  /* HIGH: Audio, LOW: Test mode */

#define PCM1808_PIN_MD1_LOW                0x00  /* Mode control bit 1 */
#define PCM1808_PIN_MD1_HIGH               0x01  /* (MD1,MD0) = (H,H): Audio mode */

/* PCM1808 Status bits */
#define PCM1808_STATUS_INITIALIZED         0x01
#define PCM1808_STATUS_ERROR               0x02
#define PCM1808_STATUS_MUTE                0x04

/* Exported macros -----------------------------------------------------------*/
/* Hardware pin control macros - akan diimplementasikan sesuai layout hardware */
#define PCM1808_SET_FMT(state)    (state == PCM1808_PIN_FMT_HIGH) ? \
                                  HAL_GPIO_WritePin(PCM1808_FMT_GPIO_Port, PCM1808_FMT_Pin, GPIO_PIN_SET) : \
                                  HAL_GPIO_WritePin(PCM1808_FMT_GPIO_Port, PCM1808_FMT_Pin, GPIO_PIN_RESET)

#define PCM1808_SET_SF0(state)    (state == PCM1808_PIN_SF0_HIGH) ? \
                                  HAL_GPIO_WritePin(PCM1808_SF0_GPIO_Port, PCM1808_SF0_Pin, GPIO_PIN_SET) : \
                                  HAL_GPIO_WritePin(PCM1808_SF0_GPIO_Port, PCM1808_SF0_Pin, GPIO_PIN_RESET)

#define PCM1808_SET_SF1(state)    (state == PCM1808_PIN_SF1_HIGH) ? \
                                  HAL_GPIO_WritePin(PCM1808_SF1_GPIO_Port, PCM1808_SF1_Pin, GPIO_PIN_SET) : \
                                  HAL_GPIO_WritePin(PCM1808_SF1_GPIO_Port, PCM1808_SF1_Pin, GPIO_PIN_RESET)

#define PCM1808_SET_MD0(state)    (state == PCM1808_PIN_MD0_HIGH) ? \
                                  HAL_GPIO_WritePin(PCM1808_MD0_GPIO_Port, PCM1808_MD0_Pin, GPIO_PIN_SET) : \
                                  HAL_GPIO_WritePin(PCM1808_MD0_GPIO_Port, PCM1808_MD0_Pin, GPIO_PIN_RESET)

#define PCM1808_SET_MD1(state)    (state == PCM1808_PIN_MD1_HIGH) ? \
                                  HAL_GPIO_WritePin(PCM1808_MD1_GPIO_Port, PCM1808_MD1_Pin, GPIO_PIN_SET) : \
                                  HAL_GPIO_WritePin(PCM1808_MD1_GPIO_Port, PCM1808_MD1_Pin, GPIO_PIN_RESET)

/* Exported variables --------------------------------------------------------*/
extern PCM1808_ConfigTypeDef PCM1808_Config;
extern uint8_t PCM1808_Status;

/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  Inisialisasi codec PCM1808
  * @param  config: Pointer ke struct konfigurasi PCM1808_ConfigTypeDef
  * @retval Status inisialisasi (0: Sukses, 1: Error)
  */
uint8_t PCM1808_Init(PCM1808_ConfigTypeDef *config);

/**
  * @brief  Set mode operasi PCM1808 (master/slave)
  * @param  mode: PCM1808_MODE_SLAVE atau PCM1808_MODE_MASTER
  * @retval Status operasi (0: Sukses, 1: Error)
  */
uint8_t PCM1808_SetMode(uint8_t mode);

/**
  * @brief  Set format data PCM1808
  * @param  format: PCM1808_FORMAT_I2S, PCM1808_FORMAT_LEFT_JUSTIFIED,
  *                PCM1808_FORMAT_RIGHT_JUSTIFIED, PCM1808_FORMAT_DSP
  * @retval Status operasi (0: Sukses, 1: Error)
  */
uint8_t PCM1808_SetDataFormat(uint8_t format);

/**
  * @brief  Set audio width (resolusi bit)
  * @param  width: PCM1808_WIDTH_16BIT, PCM1808_WIDTH_20BIT,
  *               PCM1808_WIDTH_24BIT, PCM1808_WIDTH_32BIT
  * @retval Status operasi (0: Sukses, 1: Error)
  */
uint8_t PCM1808_SetAudioWidth(uint8_t width);

/**
  * @brief  Set sample rate codec
  * @param  sampleRate: Sample rate dalam Hz (misal: 44100, 48000)
  * @retval Status operasi (0: Sukses, 1: Error)
  */
uint8_t PCM1808_SetSampleRate(uint32_t sampleRate);

/**
  * @brief  Set filter digital
  * @param  filter: PCM1808_FILTER_FIR atau PCM1808_FILTER_IIR
  * @retval Status operasi (0: Sukses, 1: Error)
  */
uint8_t PCM1808_SetDigitalFilter(uint8_t filter);

/**
  * @brief  Set high-pass filter
  * @param  state: PCM1808_HPF_OFF atau PCM1808_HPF_ON
  * @retval Status operasi (0: Sukses, 1: Error)
  */
uint8_t PCM1808_SetHighPassFilter(uint8_t state);

/**
  * @brief  Set de-emphasis filter
  * @param  filter: PCM1808_DEMPHASIS_NONE, PCM1808_DEMPHASIS_44_1KHZ, PCM1808_DEMPHASIS_48KHZ
  * @retval Status operasi (0: Sukses, 1: Error)
  */
uint8_t PCM1808_SetDemphasisFilter(uint8_t filter);

/**
  * @brief  Mute atau unmute ADC
  * @param  state: 0 - Unmute, 1 - Mute
  * @retval Status operasi (0: Sukses, 1: Error)
  */
uint8_t PCM1808_SetMute(uint8_t state);

/**
  * @brief  Atur gain digital
  * @param  gain: Nilai gain dalam dB (-12 sampai +12)
  * @retval Status operasi (0: Sukses, 1: Error)
  */
uint8_t PCM1808_SetDigitalGain(int8_t gain);

/**
  * @brief  Reset codec PCM1808
  * @retval Status operasi (0: Sukses, 1: Error)
  */
uint8_t PCM1808_Reset(void);

/**
  * @brief  Dapatkan status codec PCM1808
  * @retval Status codec
  */
uint8_t PCM1808_GetStatus(void);

/**
  * @brief  Atur seluruh konfigurasi dari struktur
  * @param  config: Pointer ke struct konfigurasi PCM1808_ConfigTypeDef
  * @retval Status operasi (0: Sukses, 1: Error)
  */
uint8_t PCM1808_SetConfig(PCM1808_ConfigTypeDef *config);

/**
  * @brief  Dapatkan konfigurasi codec saat ini
  * @param  config: Pointer ke struct konfigurasi PCM1808_ConfigTypeDef untuk diisi
  * @retval Status operasi (0: Sukses, 1: Error)
  */
uint8_t PCM1808_GetConfig(PCM1808_ConfigTypeDef *config);

/**
  * @brief  Cek apakah codec siap menerima data
  * @retval Status siap (1: Siap, 0: Tidak siap)
  */
uint8_t PCM1808_IsReady(void);

#ifdef __cplusplus
}
#endif

#endif /* __CODEC_PCM1808_H */