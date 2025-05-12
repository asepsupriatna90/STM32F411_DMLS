/**
  ******************************************************************************
  * @file           : codec_pcm1808.c
  * @brief          : PCM1808 ADC Driver Implementation
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * This file contains the implementation of the PCM1808 codec driver for 
  * the STM32F411 Audio DSP Crossover project.
  * The PCM1808 is a 2-channel, high-performance ADC used for audio input.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "codec_pcm1808.h"
#include "i2s.h"
#include "gpio.h"
#include "debug.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define PCM1808_FMT_PIN_PORT    GPIOB
#define PCM1808_FMT_PIN         GPIO_PIN_12
#define PCM1808_MD0_PIN_PORT    GPIOB
#define PCM1808_MD0_PIN         GPIO_PIN_13
#define PCM1808_MD1_PIN_PORT    GPIOB
#define PCM1808_MD1_PIN         GPIO_PIN_14
#define PCM1808_SCKI_PIN_PORT   GPIOB
#define PCM1808_SCKI_PIN        GPIO_PIN_15

/* PCM1808 Operating Mode Settings */
#define PCM1808_MODE_SLAVE      0x00  // MD1=0, MD0=0
#define PCM1808_MODE_MASTER     0x01  // MD1=0, MD0=1
#define PCM1808_MODE_RESERVED   0x02  // MD1=1, MD0=0
#define PCM1808_MODE_TEST       0x03  // MD1=1, MD0=1

/* PCM1808 Format Settings */
#define PCM1808_FMT_24BIT       0x00  // FMT=0: 24-bit 2's complement
#define PCM1808_FMT_20BIT       0x01  // FMT=1: 20-bit justified

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static PCM1808_Config_TypeDef PCM1808_Config = {
  .mode = PCM1808_MODE_SLAVE,      // Default to Slave mode
  .format = PCM1808_FMT_24BIT,     // Default to 24-bit format
  .sampleRate = AUDIO_SAMPLE_RATE, // Using project-wide sample rate
  .initialized = 0
};

/* Private function prototypes -----------------------------------------------*/
static void PCM1808_ConfigureGPIO(void);
static void PCM1808_SetMode(uint8_t mode);
static void PCM1808_SetFormat(uint8_t format);
static HAL_StatusTypeDef PCM1808_WaitForSyncI2S(I2S_HandleTypeDef *hi2s);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize the PCM1808 ADC codec
  * @param  config: PCM1808 configuration structure
  * @retval HAL status
  */
HAL_StatusTypeDef PCM1808_Init(PCM1808_Config_TypeDef *config)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  DEBUG_PRINT("PCM1808 Init: Starting initialization...\r\n");
  
  /* Configure GPIOs first */
  PCM1808_ConfigureGPIO();
  
  /* Apply configuration if provided */
  if (config != NULL) {
    memcpy(&PCM1808_Config, config, sizeof(PCM1808_Config_TypeDef));
  }
  
  /* Set codec operating mode */
  PCM1808_SetMode(PCM1808_Config.mode);
  
  /* Set codec data format */
  PCM1808_SetFormat(PCM1808_Config.format);
  
  /* Configure and start I2S peripheral for RX (ADC input) */
  hi2s2.Init.AudioFreq = PCM1808_Config.sampleRate;
  hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s2.Init.DataFormat = I2S_DATAFORMAT_24B;
  hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;  // Enable MCLK for PCM1808
  hi2s2.Init.Mode = I2S_MODE_MASTER_RX;
  hi2s2.Init.CPOL = I2S_CPOL_LOW;
  
  /* DeInit and reinit I2S with config */
  if (HAL_I2S_DeInit(&hi2s2) != HAL_OK) {
    DEBUG_PRINT("PCM1808 Init: I2S DeInit failed\r\n");
    return HAL_ERROR;
  }
  
  if (HAL_I2S_Init(&hi2s2) != HAL_OK) {
    DEBUG_PRINT("PCM1808 Init: I2S Init failed\r\n");
    return HAL_ERROR;
  }
  
  /* Wait for I2S to be ready */
  status = PCM1808_WaitForSyncI2S(&hi2s2);
  if (status != HAL_OK) {
    DEBUG_PRINT("PCM1808 Init: I2S Sync failed\r\n");
    return status;
  }
  
  PCM1808_Config.initialized = 1;
  DEBUG_PRINT("PCM1808 Init: Initialization completed successfully\r\n");
  
  return HAL_OK;
}

/**
  * @brief  Start receiving data from PCM1808 ADC using DMA
  * @param  buffer: Pointer to the buffer where data will be stored
  * @param  size: Size of the buffer
  * @retval HAL status
  */
HAL_StatusTypeDef PCM1808_Start_DMA(uint16_t *buffer, uint16_t size)
{
  if (!PCM1808_Config.initialized) {
    DEBUG_PRINT("PCM1808 Start DMA: Codec not initialized\r\n");
    return HAL_ERROR;
  }
  
  DEBUG_PRINT("PCM1808 Start DMA: Starting DMA transfer\r\n");
  
  return HAL_I2S_Receive_DMA(&hi2s2, buffer, size);
}

/**
  * @brief  Stop PCM1808 DMA transfer
  * @retval HAL status
  */
HAL_StatusTypeDef PCM1808_Stop_DMA(void)
{
  if (!PCM1808_Config.initialized) {
    return HAL_ERROR;
  }
  
  DEBUG_PRINT("PCM1808 Stop DMA: Stopping DMA transfer\r\n");
  
  return HAL_I2S_DMAStop(&hi2s2);
}

/**
  * @brief  Get current PCM1808 configuration
  * @param  config: Pointer to store configuration
  * @retval None
  */
void PCM1808_GetConfig(PCM1808_Config_TypeDef *config)
{
  if (config != NULL) {
    memcpy(config, &PCM1808_Config, sizeof(PCM1808_Config_TypeDef));
  }
}

/**
  * @brief  Update PCM1808 configuration
  * @param  config: New configuration to apply
  * @retval HAL status
  */
HAL_StatusTypeDef PCM1808_UpdateConfig(PCM1808_Config_TypeDef *config)
{
  if (config == NULL) {
    return HAL_ERROR;
  }
  
  /* Store current state */
  uint8_t was_initialized = PCM1808_Config.initialized;
  
  /* If running, stop first */
  if (was_initialized) {
    PCM1808_Stop_DMA();
  }
  
  /* Re-initialize with new config */
  HAL_StatusTypeDef status = PCM1808_Init(config);
  
  return status;
}

/**
  * @brief  Check if PCM1808 is correctly detecting audio signal
  * @retval 1 if audio detected, 0 otherwise
  */
uint8_t PCM1808_IsAudioDetected(void)
{
  /* This function would implement audio detection logic
     by examining the incoming PCM data for non-zero samples */
  /* For now, simply return that audio is detected if initialized */
  return PCM1808_Config.initialized;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Configure GPIO pins for PCM1808
  * @retval None
  */
static void PCM1808_ConfigureGPIO(void)
{
  GPIO_InitTypeDef  GPIO_InitStruct = {0};

  /* Enable GPIO clocks if not already done */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Configure FMT, MD0, MD1 pins as outputs */
  GPIO_InitStruct.Pin = PCM1808_FMT_PIN | PCM1808_MD0_PIN | PCM1808_MD1_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /* Set initial pin states to default state */
  HAL_GPIO_WritePin(PCM1808_FMT_PIN_PORT, PCM1808_FMT_PIN, GPIO_PIN_RESET); // 24-bit
  HAL_GPIO_WritePin(PCM1808_MD0_PIN_PORT, PCM1808_MD0_PIN, GPIO_PIN_RESET); // Slave mode
  HAL_GPIO_WritePin(PCM1808_MD1_PIN_PORT, PCM1808_MD1_PIN, GPIO_PIN_RESET); // Slave mode
  
  DEBUG_PRINT("PCM1808 GPIO: Pins configured\r\n");
}

/**
  * @brief  Set PCM1808 operating mode
  * @param  mode: Operating mode (slave/master)
  * @retval None
  */
static void PCM1808_SetMode(uint8_t mode)
{
  switch (mode) {
    case PCM1808_MODE_SLAVE:
      /* MD1=0, MD0=0 for Slave mode */
      HAL_GPIO_WritePin(PCM1808_MD1_PIN_PORT, PCM1808_MD1_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(PCM1808_MD0_PIN_PORT, PCM1808_MD0_PIN, GPIO_PIN_RESET);
      DEBUG_PRINT("PCM1808 Mode: Set to SLAVE\r\n");
      break;
      
    case PCM1808_MODE_MASTER:
      /* MD1=0, MD0=1 for Master mode */
      HAL_GPIO_WritePin(PCM1808_MD1_PIN_PORT, PCM1808_MD1_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(PCM1808_MD0_PIN_PORT, PCM1808_MD0_PIN, GPIO_PIN_SET);
      DEBUG_PRINT("PCM1808 Mode: Set to MASTER\r\n");
      break;
      
    case PCM1808_MODE_TEST:
      /* MD1=1, MD0=1 for Test mode */
      HAL_GPIO_WritePin(PCM1808_MD1_PIN_PORT, PCM1808_MD1_PIN, GPIO_PIN_SET);
      HAL_GPIO_WritePin(PCM1808_MD0_PIN_PORT, PCM1808_MD0_PIN, GPIO_PIN_SET);
      DEBUG_PRINT("PCM1808 Mode: Set to TEST\r\n");
      break;
      
    default:
      /* Default to safe mode (slave) */
      HAL_GPIO_WritePin(PCM1808_MD1_PIN_PORT, PCM1808_MD1_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(PCM1808_MD0_PIN_PORT, PCM1808_MD0_PIN, GPIO_PIN_RESET);
      DEBUG_PRINT("PCM1808 Mode: Invalid mode, defaulting to SLAVE\r\n");
      break;
  }
}

/**
  * @brief  Set PCM1808 data format
  * @param  format: Data format (24-bit/20-bit)
  * @retval None
  */
static void PCM1808_SetFormat(uint8_t format)
{
  if (format == PCM1808_FMT_24BIT) {
    /* FMT=0 for 24-bit 2's complement */
    HAL_GPIO_WritePin(PCM1808_FMT_PIN_PORT, PCM1808_FMT_PIN, GPIO_PIN_RESET);
    DEBUG_PRINT("PCM1808 Format: Set to 24-bit\r\n");
  } else {
    /* FMT=1 for 20-bit justified */
    HAL_GPIO_WritePin(PCM1808_FMT_PIN_PORT, PCM1808_FMT_PIN, GPIO_PIN_SET);
    DEBUG_PRINT("PCM1808 Format: Set to 20-bit\r\n");
  }
}

/**
  * @brief  Wait for I2S to be ready
  * @param  hi2s: I2S handle
  * @retval HAL status
  */
static HAL_StatusTypeDef PCM1808_WaitForSyncI2S(I2S_HandleTypeDef *hi2s)
{
  uint32_t timeout = 100;  // Timeout in ms
  uint32_t tickstart = HAL_GetTick();
  
  while ((hi2s->State != HAL_I2S_STATE_READY) && (hi2s->State != HAL_I2S_STATE_BUSY_TX) && (hi2s->State != HAL_I2S_STATE_BUSY_RX)) {
    if (((HAL_GetTick() - tickstart) > timeout)) {
      return HAL_TIMEOUT;
    }
    HAL_Delay(1);
  }
  
  return HAL_OK;
}

/**
  * @brief  I2S error callback implementation for PCM1808
  * @param  hi2s: I2S handle
  * @retval None
  */
void PCM1808_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
  /* Handle I2S errors here */
  DEBUG_PRINT("PCM1808 Error: I2S Error detected\r\n");
  
  /* Reset I2S DMA - Recovery attempt */
  HAL_I2S_DMAStop(hi2s);
  
  /* Restart I2S DMA if needed - Application should handle this */
}