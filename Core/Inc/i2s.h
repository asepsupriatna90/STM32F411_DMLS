/**
  ******************************************************************************
  * @file           : i2s.h
  * @brief          : I2S configuration for audio data transfer with PCM1808 and PCM5102A
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __I2S_H__
#define __I2S_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
  I2S_OK       = 0,
  I2S_ERROR    = 1,
  I2S_BUSY     = 2,
  I2S_TIMEOUT  = 3
} I2S_Status_TypeDef;

typedef enum {
  I2S_SAMPLERATE_44K1  = 44100,
  I2S_SAMPLERATE_48K   = 48000,
  I2S_SAMPLERATE_96K   = 96000
} I2S_SampleRate_TypeDef;

typedef enum {
  I2S_DATAWIDTH_16 = 16,
  I2S_DATAWIDTH_24 = 24,
  I2S_DATAWIDTH_32 = 32
} I2S_DataWidth_TypeDef;

/* Exported constants --------------------------------------------------------*/
#define I2S_BUFFER_SIZE              256 /* Buffer size for audio processing (samples) */
#define I2S_DEFAULT_SAMPLERATE       I2S_SAMPLERATE_48K
#define I2S_DEFAULT_DATAWIDTH        I2S_DATAWIDTH_24

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern I2S_HandleTypeDef hi2s2;  /* For PCM1808 (ADC) */
extern I2S_HandleTypeDef hi2s3;  /* For PCM5102A (DAC) */

/* Exported functions prototypes ---------------------------------------------*/
void MX_I2S2_Init(void); /* Initialize I2S2 for PCM1808 (ADC) */
void MX_I2S3_Init(void); /* Initialize I2S3 for PCM5102A (DAC) */
I2S_Status_TypeDef I2S_SetSampleRate(I2S_SampleRate_TypeDef SampleRate);
I2S_Status_TypeDef I2S_SetDataWidth(I2S_DataWidth_TypeDef DataWidth);
I2S_Status_TypeDef I2S_StartAudioInterface(void);
I2S_Status_TypeDef I2S_StopAudioInterface(void);
I2S_Status_TypeDef I2S_TransmitData(uint16_t *pData, uint16_t Size);
I2S_Status_TypeDef I2S_ReceiveData(uint16_t *pData, uint16_t Size);
void I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s);
void I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s);
void I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
void I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
void I2S_ErrorCallback(I2S_HandleTypeDef *hi2s);

#ifdef __cplusplus
}
#endif
#endif /* __I2S_H__ */