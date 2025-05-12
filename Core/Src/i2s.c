/**
  ******************************************************************************
  * @file           : i2s.c
  * @brief          : I2S initialization and functions for audio interface
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * I2S configuration for PCM1808 ADC and PCM5102A DAC
  * Handles audio data transfer between codecs and MCU
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "i2s.h"
#include "gpio.h"
#include "dma.h"
#include "audio_config.h"
#include "debug.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define I2S_STANDARD                   I2S_STANDARD_PHILIPS
#define I2S_MODE_MASTER_TX             1
#define I2S_MODE_MASTER_RX             2
#define I2S_MODE_SLAVE_TX              3
#define I2S_MODE_SLAVE_RX              4

/* Private variables ---------------------------------------------------------*/
I2S_HandleTypeDef hi2s2;  /* I2S2 for input - ADC PCM1808 */
I2S_HandleTypeDef hi2s3;  /* I2S3 for output - DAC PCM5102A channels 1-2 */
I2S_HandleTypeDef hi2s4;  /* I2S4 for output - DAC PCM5102A channels 3-4 */

DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi3_tx;
DMA_HandleTypeDef hdma_spi4_tx;

/* Private function prototypes -----------------------------------------------*/
static void I2S2_Init(uint32_t AudioFreq);
static void I2S3_Init(uint32_t AudioFreq);
static void I2S4_Init(uint32_t AudioFreq);
static uint32_t I2S_GetPrescaler(uint32_t AudioFreq);

/* External function callbacks -----------------------------------------------*/
extern void Audio_ProcessCallback(void);

/**
  * @brief Initialize all I2S interfaces
  * @param AudioFreq: Audio sampling frequency in Hz (44100, 48000, etc.)
  * @retval None
  */
void MX_I2S_Init(uint32_t AudioFreq)
{
  DEBUG_PRINT("Initializing I2S interfaces at %luHz\r\n", AudioFreq);
  
  /* Initialize I2S2 for ADC input */
  I2S2_Init(AudioFreq);
  
  /* Initialize I2S3 for first DAC output */
  I2S3_Init(AudioFreq);
  
  /* Initialize I2S4 for second DAC output */
  I2S4_Init(AudioFreq);
  
  DEBUG_PRINT("I2S interfaces initialized successfully\r\n");
}

/**
  * @brief Initialize I2S2 for ADC input (PCM1808)
  * @param AudioFreq: Audio sampling frequency in Hz
  * @retval None
  */
static void I2S2_Init(uint32_t AudioFreq)
{
  hi2s2.Instance = SPI2;
  hi2s2.Init.Mode = I2S_MODE_MASTER_RX;
  hi2s2.Init.Standard = I2S_STANDARD;
  hi2s2.Init.DataFormat = I2S_DATAFORMAT_24B;
  hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s2.Init.AudioFreq = AudioFreq;
  hi2s2.Init.CPOL = I2S_CPOL_LOW;
  hi2s2.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Initialize I2S3 for DAC output (PCM5102A channels 1-2)
  * @param AudioFreq: Audio sampling frequency in Hz
  * @retval None
  */
static void I2S3_Init(uint32_t AudioFreq)
{
  hi2s3.Instance = SPI3;
  hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s3.Init.Standard = I2S_STANDARD;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_24B;
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s3.Init.AudioFreq = AudioFreq;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  
  if (HAL_I2S_Init(&hi2s3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Initialize I2S4 for DAC output (PCM5102A channels 3-4)
  * @param AudioFreq: Audio sampling frequency in Hz
  * @retval None
  */
static void I2S4_Init(uint32_t AudioFreq)
{
  hi2s4.Instance = SPI4;
  hi2s4.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s4.Init.Standard = I2S_STANDARD;
  hi2s4.Init.DataFormat = I2S_DATAFORMAT_24B;
  hi2s4.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s4.Init.AudioFreq = AudioFreq;
  hi2s4.Init.CPOL = I2S_CPOL_LOW;
  hi2s4.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s4.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  
  if (HAL_I2S_Init(&hi2s4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Start I2S with DMA for audio processing
  * @param input_buffer: Pointer to the input buffer for ADC
  * @param output_buffer1: Pointer to the first output buffer for DAC
  * @param output_buffer2: Pointer to the second output buffer for DAC
  * @param size: Buffer size in samples (each sample is 32 bits)
  * @retval HAL status
  */
HAL_StatusTypeDef I2S_StartAudio(int32_t *input_buffer, int32_t *output_buffer1, 
                                int32_t *output_buffer2, uint16_t size)
{
  HAL_StatusTypeDef status;
  
  /* Start I2S2 reception with DMA (PCM1808 ADC) */
  status = HAL_I2S_Receive_DMA(&hi2s2, (uint16_t*)input_buffer, size * 2);
  if (status != HAL_OK) {
    DEBUG_PRINT("Error: Could not start I2S2 reception\r\n");
    return status;
  }
  
  /* Start I2S3 transmission with DMA (PCM5102A channels 1-2) */
  status = HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)output_buffer1, size * 2);
  if (status != HAL_OK) {
    DEBUG_PRINT("Error: Could not start I2S3 transmission\r\n");
    return status;
  }
  
  /* Start I2S4 transmission with DMA (PCM5102A channels 3-4) */
  status = HAL_I2S_Transmit_DMA(&hi2s4, (uint16_t*)output_buffer2, size * 2);
  if (status != HAL_OK) {
    DEBUG_PRINT("Error: Could not start I2S4 transmission\r\n");
    return status;
  }
  
  DEBUG_PRINT("I2S interfaces started successfully with DMA\r\n");
  return HAL_OK;
}

/**
  * @brief Stop I2S interfaces
  * @retval HAL status
  */
HAL_StatusTypeDef I2S_StopAudio(void)
{
  HAL_StatusTypeDef status;
  
  /* Stop I2S2 reception */
  status = HAL_I2S_DMAStop(&hi2s2);
  if (status != HAL_OK) {
    DEBUG_PRINT("Error: Could not stop I2S2 reception\r\n");
    return status;
  }
  
  /* Stop I2S3 transmission */
  status = HAL_I2S_DMAStop(&hi2s3);
  if (status != HAL_OK) {
    DEBUG_PRINT("Error: Could not stop I2S3 transmission\r\n");
    return status;
  }
  
  /* Stop I2S4 transmission */
  status = HAL_I2S_DMAStop(&hi2s4);
  if (status != HAL_OK) {
    DEBUG_PRINT("Error: Could not stop I2S4 transmission\r\n");
    return status;
  }
  
  DEBUG_PRINT("I2S interfaces stopped\r\n");
  return HAL_OK;
}

/**
  * @brief  Tx Transfer half completed callback
  * @param  hi2s: I2S handle
  * @retval None
  */
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  /* Call DSP processing function for first half of buffer */
  if (hi2s->Instance == SPI3) {
    Audio_ProcessCallback();
  }
}

/**
  * @brief  Tx Transfer completed callback
  * @param  hi2s: I2S handle
  * @retval None
  */
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  /* Call DSP processing function for second half of buffer */
  if (hi2s->Instance == SPI3) {
    Audio_ProcessCallback();
  }
}

/**
  * @brief  Rx Transfer half completed callback
  * @param  hi2s: I2S handle
  * @retval None
  */
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  /* Input data has been received - processing is initiated in TxHalfCpltCallback */
}

/**
  * @brief  Rx Transfer completed callback
  * @param  hi2s: I2S handle
  * @retval None
  */
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  /* Input data has been received - processing is initiated in TxCpltCallback */
}

/**
  * @brief  I2S error callback
  * @param  hi2s: I2S handle
  * @retval None
  */
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
  DEBUG_PRINT("I2S Error on interface %d\r\n", 
              (hi2s->Instance == SPI2) ? 2 : 
              (hi2s->Instance == SPI3) ? 3 : 
              (hi2s->Instance == SPI4) ? 4 : 0);
  
  /* Stop and restart I2S on error */
  if (hi2s->Instance == SPI2) {
    HAL_I2S_DMAStop(&hi2s2);
    HAL_I2S_Receive_DMA(&hi2s2, (uint16_t*)hi2s->pRxBuffPtr, hi2s->RxXferSize);
  } else if (hi2s->Instance == SPI3) {
    HAL_I2S_DMAStop(&hi2s3);
    HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)hi2s->pTxBuffPtr, hi2s->TxXferSize);
  } else if (hi2s->Instance == SPI4) {
    HAL_I2S_DMAStop(&hi2s4);
    HAL_I2S_Transmit_DMA(&hi2s4, (uint16_t*)hi2s->pTxBuffPtr, hi2s->TxXferSize);
  }
}

/**
  * @brief I2S MSP Initialization
  * @param hi2s: I2S handle pointer
  * @retval None
  */
void HAL_I2S_MspInit(I2S_HandleTypeDef* hi2s)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  if(hi2s->Instance == SPI2) /* I2S2 for PCM1808 ADC input */
  {
    /* Peripheral clock enable */
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    /* I2S2 GPIO Configuration
    PB12     ------> I2S2_WS
    PB13     ------> I2S2_CK
    PB14     ------> I2S2_ext_SD
    PB15     ------> I2S2_SD
    PC6      ------> I2S2_MCK */
    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    /* I2S2 DMA Init */
    hdma_spi2_rx.Instance = DMA1_Stream3;
    hdma_spi2_rx.Init.Channel = DMA_CHANNEL_0;
    hdma_spi2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_spi2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_spi2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_spi2_rx.Init.Mode = DMA_CIRCULAR;
    hdma_spi2_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_spi2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_spi2_rx) != HAL_OK)
    {
      Error_Handler();
    }
    
    __HAL_LINKDMA(hi2s, hdmarx, hdma_spi2_rx);
  }
  else if(hi2s->Instance == SPI3) /* I2S3 for PCM5102A DAC channels 1-2 */
  {
    /* Peripheral clock enable */
    __HAL_RCC_SPI3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    /* I2S3 GPIO Configuration
    PA4      ------> I2S3_WS
    PC7      ------> I2S3_MCK
    PC10     ------> I2S3_CK
    PC12     ------> I2S3_SD */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_10|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    /* I2S3 DMA Init */
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
    if (HAL_DMA_Init(&hdma_spi3_tx) != HAL_OK)
    {
      Error_Handler();
    }
    
    __HAL_LINKDMA(hi2s, hdmatx, hdma_spi3_tx);
  }
  else if(hi2s->Instance == SPI4) /* I2S4 for PCM5102A DAC channels 3-4 */
  {
    /* Peripheral clock enable */
    __HAL_RCC_SPI4_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* I2S4 GPIO Configuration
    PA1     ------> I2S4_SD
    PB0     ------> I2S4_WS
    PB1     ------> I2S4_CK */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /* I2S4 DMA Init */
    hdma_spi4_tx.Instance = DMA2_Stream1;
    hdma_spi4_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_spi4_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi4_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi4_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi4_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_spi4_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_spi4_tx.Init.Mode = DMA_CIRCULAR;
    hdma_spi4_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_spi4_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_spi4_tx) != HAL_OK)
    {
      Error_Handler();
    }
    
    __HAL_LINKDMA(hi2s, hdmatx, hdma_spi4_tx);
  }
}

/**
  * @brief I2S MSP De-Initialization
  * @param hi2s: I2S handle pointer
  * @retval None
  */
void HAL_I2S_MspDeInit(I2S_HandleTypeDef* hi2s)
{
  if(hi2s->Instance == SPI2) /* I2S2 for ADC input */
  {
    /* Peripheral clock disable */
    __HAL_RCC_SPI2_CLK_DISABLE();
    
    /* I2S2 GPIO Configuration */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6);
    
    /* I2S2 DMA DeInit */
    HAL_DMA_DeInit(hi2s->hdmarx);
  }
  else if(hi2s->Instance == SPI3) /* I2S3 for DAC output 1-2 */
  {
    /* Peripheral clock disable */
    __HAL_RCC_SPI3_CLK_DISABLE();
    
    /* I2S3 GPIO Configuration */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_7|GPIO_PIN_10|GPIO_PIN_12);
    
    /* I2S3 DMA DeInit */
    HAL_DMA_DeInit(hi2s->hdmatx);
  }
  else if(hi2s->Instance == SPI4) /* I2S4 for DAC output 3-4 */
  {
    /* Peripheral clock disable */
    __HAL_RCC_SPI4_CLK_DISABLE();
    
    /* I2S4 GPIO Configuration */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0|GPIO_PIN_1);
    
    /* I2S4 DMA DeInit */
    HAL_DMA_DeInit(hi2s->hdmatx);
  }
}

/**
  * @brief  Calculate and return the correct PLLI2S prescaler value
  * @param  AudioFreq: Audio frequency
  * @retval I2S prescaler value
  */
static uint32_t I2S_GetPrescaler(uint32_t AudioFreq)
{
  uint32_t plli2sn = 192;
  uint32_t plli2sr = 2;
  uint32_t pllm = 25;
  
  uint32_t tmpreg = 0;
  uint32_t i2sdiv = 2;
  uint32_t i2sodd = 0;
  
  /* PLLI2S_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLI2SN */
  uint32_t plli2svco = (HSE_VALUE / pllm) * plli2sn;
  
  /* I2S_CLK = PLLI2S_VCO / PLLI2SR */
  uint32_t i2sclk = plli2svco / plli2sr;
  
  /* Adjust for desired audio frequency */
  uint32_t tmp = i2sclk / (AudioFreq * 256);
  
  /* Check if odd division required */
  if ((i2sclk % (AudioFreq * 256)) > (AudioFreq * 128))
  {
    i2sodd = 1;
  }
  
  /* Get the optimal value for i2sdiv */
  if (tmp > 63)
  {
    i2sdiv = 63;
  }
  else if (tmp < 2)
  {
    i2sdiv = 2;
  }
  else
  {
    i2sdiv = tmp;
  }
  
  /* Calculate the prescaler value */
  tmpreg = (i2sdiv << 0) | (i2sodd << 8);
  
  return tmpreg;
}