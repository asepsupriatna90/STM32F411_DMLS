/**
  ******************************************************************************
  * @file           : dma.c
  * @brief          : DMA implementation for STM32F411 Audio DSP Crossover
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * DMA configuration for I2S audio streams
  * Handles efficient data transfer between memory and audio peripherals
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "dma.h"
#include "i2s.h"
#include "audio_config.h"
#include "debug.h"

/* Private variables ---------------------------------------------------------*/
static DMA_HandleTypeDef hdma_i2s2_ext_rx;   /* DMA untuk PCM1808 (ADC) */
static DMA_HandleTypeDef hdma_spi2_tx;       /* DMA untuk PCM5102A #1 (DAC) */
static DMA_HandleTypeDef hdma_spi3_tx;       /* DMA untuk PCM5102A #2 (DAC) */

/* DMA transfer complete callback prototypes */
static void DMA_RxCpltCallback(DMA_HandleTypeDef *hdma);
static void DMA_RxHalfCpltCallback(DMA_HandleTypeDef *hdma);
static void DMA_TxCpltCallback(DMA_HandleTypeDef *hdma);
static void DMA_TxHalfCpltCallback(DMA_HandleTypeDef *hdma);
static void DMA_ErrorCallback(DMA_HandleTypeDef *hdma);

/**
  * @brief DMA initialization function
  * @retval None
  */
void MX_DMA_Init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream3_IRQn interrupt configuration for SPI2_TX */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  
  /* DMA1_Stream3_IRQn interrupt configuration for I2S2_EXT_RX */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  
  /* DMA1_Stream5_IRQn interrupt configuration for SPI3_TX */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  
  DEBUG_PRINT("DMA initialized\r\n");
}

/**
  * @brief Configure the DMA for I2S2_EXT_RX (Audio Input)
  * @param hi2s: I2S handle
  * @retval HAL status
  */
HAL_StatusTypeDef DMA_ConfigureRx(I2S_HandleTypeDef *hi2s)
{
  /* Configure DMA for I2S2_EXT_RX (Audio Input) */
  hdma_i2s2_ext_rx.Instance = DMA1_Stream3;
  hdma_i2s2_ext_rx.Init.Channel = DMA_CHANNEL_3;
  hdma_i2s2_ext_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_i2s2_ext_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_i2s2_ext_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_i2s2_ext_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_i2s2_ext_rx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_i2s2_ext_rx.Init.Mode = DMA_CIRCULAR;
  hdma_i2s2_ext_rx.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_i2s2_ext_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  
  /* Initialize the DMA stream */
  if (HAL_DMA_Init(&hdma_i2s2_ext_rx) != HAL_OK)
  {
    DEBUG_PRINT("DMA Rx init failed\r\n");
    return HAL_ERROR;
  }
  
  /* Associate the DMA handle with the I2S handle */
  __HAL_LINKDMA(hi2s, hdmarx, hdma_i2s2_ext_rx);
  
  /* Set DMA callbacks */
  hdma_i2s2_ext_rx.XferCpltCallback = DMA_RxCpltCallback;
  hdma_i2s2_ext_rx.XferHalfCpltCallback = DMA_RxHalfCpltCallback;
  hdma_i2s2_ext_rx.XferErrorCallback = DMA_ErrorCallback;
  
  DEBUG_PRINT("DMA Rx configured\r\n");
  return HAL_OK;
}

/**
  * @brief Configure the DMA for SPI2_TX (Audio Output 1-2)
  * @param hi2s: I2S handle
  * @retval HAL status
  */
HAL_StatusTypeDef DMA_ConfigureTx1(I2S_HandleTypeDef *hi2s)
{
  /* Configure DMA for SPI2_TX (Audio Output 1-2) */
  hdma_spi2_tx.Instance = DMA1_Stream4;
  hdma_spi2_tx.Init.Channel = DMA_CHANNEL_0;
  hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_spi2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_spi2_tx.Init.Mode = DMA_CIRCULAR;
  hdma_spi2_tx.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_spi2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  
  /* Initialize the DMA stream */
  if (HAL_DMA_Init(&hdma_spi2_tx) != HAL_OK)
  {
    DEBUG_PRINT("DMA Tx1 init failed\r\n");
    return HAL_ERROR;
  }
  
  /* Associate the DMA handle with the I2S handle */
  __HAL_LINKDMA(hi2s, hdmatx, hdma_spi2_tx);
  
  /* Set DMA callbacks */
  hdma_spi2_tx.XferCpltCallback = DMA_TxCpltCallback;
  hdma_spi2_tx.XferHalfCpltCallback = DMA_TxHalfCpltCallback;
  hdma_spi2_tx.XferErrorCallback = DMA_ErrorCallback;
  
  DEBUG_PRINT("DMA Tx1 configured\r\n");
  return HAL_OK;
}

/**
  * @brief Configure the DMA for SPI3_TX (Audio Output 3-4)
  * @param hi2s: I2S handle
  * @retval HAL status
  */
HAL_StatusTypeDef DMA_ConfigureTx2(I2S_HandleTypeDef *hi2s)
{
  /* Configure DMA for SPI3_TX (Audio Output 3-4) */
  hdma_spi3_tx.Instance = DMA1_Stream5;
  hdma_spi3_tx.Init.Channel = DMA_CHANNEL_0;
  hdma_spi3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_spi3_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi3_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_spi3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_spi3_tx.Init.Mode = DMA_CIRCULAR;
  hdma_spi3_tx.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_spi3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  
  /* Initialize the DMA stream */
  if (HAL_DMA_Init(&hdma_spi3_tx) != HAL_OK)
  {
    DEBUG_PRINT("DMA Tx2 init failed\r\n");
    return HAL_ERROR;
  }
  
  /* Associate the DMA handle with the I2S handle */
  __HAL_LINKDMA(hi2s, hdmatx, hdma_spi3_tx);
  
  /* Set DMA callbacks */
  hdma_spi3_tx.XferCpltCallback = DMA_TxCpltCallback;
  hdma_spi3_tx.XferHalfCpltCallback = DMA_TxHalfCpltCallback;
  hdma_spi3_tx.XferErrorCallback = DMA_ErrorCallback;
  
  DEBUG_PRINT("DMA Tx2 configured\r\n");
  return HAL_OK;
}

/**
  * @brief Start DMA transfers for audio streaming
  * @param inputBuffer: Pointer to input buffer
  * @param outputBuffer1: Pointer to output buffer for DAC 1
  * @param outputBuffer2: Pointer to output buffer for DAC 2
  * @param bufferSize: Size of audio buffer in samples
  * @retval HAL status
  */
HAL_StatusTypeDef DMA_StartAudioTransfers(uint16_t *inputBuffer, uint16_t *outputBuffer1, 
                                         uint16_t *outputBuffer2, uint16_t bufferSize)
{
  HAL_StatusTypeDef status;
  
  /* Start DMA for I2S receive (input audio) */
  status = HAL_I2SEx_ReceiveDMA(&hi2s2, inputBuffer, bufferSize);
  if (status != HAL_OK)
  {
    DEBUG_PRINT("DMA Rx start failed: %d\r\n", status);
    return status;
  }
  
  /* Start DMA for I2S transmit (output audio) */
  status = HAL_I2S_Transmit_DMA(&hi2s2, outputBuffer1, bufferSize);
  if (status != HAL_OK)
  {
    DEBUG_PRINT("DMA Tx1 start failed: %d\r\n", status);
    return status;
  }
  
  status = HAL_I2S_Transmit_DMA(&hi2s3, outputBuffer2, bufferSize);
  if (status != HAL_OK)
  {
    DEBUG_PRINT("DMA Tx2 start failed: %d\r\n", status);
    return status;
  }
  
  DEBUG_PRINT("DMA audio transfers started\r\n");
  return HAL_OK;
}

/**
  * @brief Stop DMA transfers for audio streaming
  * @retval HAL status
  */
HAL_StatusTypeDef DMA_StopAudioTransfers(void)
{
  HAL_StatusTypeDef status;
  
  /* Stop DMA for I2S receive */
  status = HAL_I2S_DMAStop(&hi2s2);
  if (status != HAL_OK)
  {
    DEBUG_PRINT("DMA I2S2 stop failed: %d\r\n", status);
    return status;
  }
  
  /* Stop DMA for I2S transmit */
  status = HAL_I2S_DMAStop(&hi2s3);
  if (status != HAL_OK)
  {
    DEBUG_PRINT("DMA I2S3 stop failed: %d\r\n", status);
    return status;
  }
  
  DEBUG_PRINT("DMA audio transfers stopped\r\n");
  return HAL_OK;
}

/**
  * @brief DMA receive complete callback
  * @param hdma: DMA handle
  * @retval None
  */
static void DMA_RxCpltCallback(DMA_HandleTypeDef *hdma)
{
  /* Call external audio processing callback to notify completion */
  /* Process second half of buffer */
  Audio_ProcessCallback();
}

/**
  * @brief DMA receive half complete callback
  * @param hdma: DMA handle
  * @retval None
  */
static void DMA_RxHalfCpltCallback(DMA_HandleTypeDef *hdma)
{
  /* Call external audio processing callback to notify half completion */
  /* Process first half of buffer */
  Audio_ProcessCallback();
}

/**
  * @brief DMA transmit complete callback
  * @param hdma: DMA handle
  * @retval None
  */
static void DMA_TxCpltCallback(DMA_HandleTypeDef *hdma)
{
  /* For synchronization with audio processing */
  /* Could be used for synchronizing multiple DACs */
}

/**
  * @brief DMA transmit half complete callback
  * @param hdma: DMA handle
  * @retval None
  */
static void DMA_TxHalfCpltCallback(DMA_HandleTypeDef *hdma)
{
  /* For synchronization with audio processing */
  /* Could be used for synchronizing multiple DACs */
}

/**
  * @brief DMA error callback
  * @param hdma: DMA handle
  * @retval None
  */
static void DMA_ErrorCallback(DMA_HandleTypeDef *hdma)
{
  /* Log DMA error */
  DEBUG_PRINT("DMA error detected\r\n");
  
  /* Restart audio streams to recover */
  DMA_StopAudioTransfers();
  
  /* Could implement a delayed restart or notify main system */
}

/**
  * @brief DMA IRQ handler for DMA1_Stream3
  * @retval None
  */
void DMA1_Stream3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_i2s2_ext_rx);
}

/**
  * @brief DMA IRQ handler for DMA1_Stream4
  * @retval None
  */
void DMA1_Stream4_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_spi2_tx);
}

/**
  * @brief DMA IRQ handler for DMA1_Stream5
  * @retval None
  */
void DMA1_Stream5_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_spi3_tx);
}

/* External function declarations required by dma.c */
__weak void Audio_ProcessCallback(void)
{
  /* NOTE: This function should be implemented in audio_processing.c */
  /* This is just a placeholder */
}