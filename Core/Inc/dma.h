/**
  ******************************************************************************
  * @file           : dma.h
  * @brief          : DMA configuration for audio data streaming
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DMA_H__
#define __DMA_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
  DMA_OK      = 0,
  DMA_ERROR   = 1,
  DMA_BUSY    = 2,
  DMA_TIMEOUT = 3
} DMA_Status_TypeDef;

typedef enum {
  DMA_PRIORITY_LOW      = DMA_PRIORITY_LOW,
  DMA_PRIORITY_MEDIUM   = DMA_PRIORITY_MEDIUM,
  DMA_PRIORITY_HIGH     = DMA_PRIORITY_HIGH,
  DMA_PRIORITY_VERYHIGH = DMA_PRIORITY_VERY_HIGH
} DMA_Priority_TypeDef;

typedef enum {
  DMA_STREAM_I2S2_RX = 0,  /* Stream for I2S2 Rx (PCM1808 ADC) */
  DMA_STREAM_I2S3_TX = 1,  /* Stream for I2S3 Tx (PCM5102A DAC) */
  DMA_STREAM_USART_TX = 2, /* Stream for USART Tx */
  DMA_STREAM_USART_RX = 3  /* Stream for USART Rx */
} DMA_Stream_TypeDef;

/* Exported constants --------------------------------------------------------*/
#define DMA_TIMEOUT_MS           1000  /* Timeout for DMA operations in milliseconds */

/* DMA stream configuration for I2S - audio transfer */
#define DMA_I2S2_RX_STREAM       DMA1_Stream3
#define DMA_I2S2_RX_CHANNEL      DMA_CHANNEL_0
#define DMA_I2S3_TX_STREAM       DMA1_Stream5
#define DMA_I2S3_TX_CHANNEL      DMA_CHANNEL_0

/* DMA stream configuration for USART - debug interface */
#define DMA_USART_TX_STREAM      DMA1_Stream6
#define DMA_USART_TX_CHANNEL     DMA_CHANNEL_4
#define DMA_USART_RX_STREAM      DMA1_Stream5
#define DMA_USART_RX_CHANNEL     DMA_CHANNEL_4

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_i2s2_rx;    /* DMA handle for I2S2 reception */
extern DMA_HandleTypeDef hdma_i2s3_tx;    /* DMA handle for I2S3 transmission */
extern DMA_HandleTypeDef hdma_usart2_tx;  /* DMA handle for USART2 transmission */
extern DMA_HandleTypeDef hdma_usart2_rx;  /* DMA handle for USART2 reception */

/* Exported functions prototypes ---------------------------------------------*/
void MX_DMA_Init(void);
DMA_Status_TypeDef DMA_SetPriority(DMA_Stream_TypeDef Stream, DMA_Priority_TypeDef Priority);
DMA_Status_TypeDef DMA_Start(DMA_Stream_TypeDef Stream, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);
DMA_Status_TypeDef DMA_Stop(DMA_Stream_TypeDef Stream);
DMA_Status_TypeDef DMA_PollForTransfer(DMA_Stream_TypeDef Stream, uint32_t CompleteLevel, uint32_t Timeout);
uint8_t DMA_GetTransferStatus(DMA_Stream_TypeDef Stream);
void DMA_IRQHandler(DMA_Stream_TypeDef Stream);

#ifdef __cplusplus
}
#endif
#endif /* __DMA_H__ */