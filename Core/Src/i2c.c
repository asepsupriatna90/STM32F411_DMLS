/**
  ******************************************************************************
  * @file           : i2c.c
  * @brief          : I2C configuration for STM32F411 Audio DSP Crossover
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * I2C configuration for OLED display and EEPROM storage
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

/* Private function prototypes -----------------------------------------------*/

/**
  * @brief I2C1 Initialization Function - Used for OLED display
  * @param None
  * @retval None
  */
void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;                 /* 400 KHz Fast Mode */
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function - Used for EEPROM storage
  * @param None
  * @retval None
  */
void MX_I2C2_Init(void)
{
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;                 /* 100 KHz Standard Mode for EEPROM */
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C MSP Initialization
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  if(hi2c->Instance == I2C1)
  {
    /* Peripheral clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  
    /* I2C1 GPIO Configuration
     * PB6 ------> I2C1_SCL
     * PB7 ------> I2C1_SDA
     */
    GPIO_InitStruct.Pin = OLED_SCL_Pin|OLED_SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
  else if(hi2c->Instance == I2C2)
  {
    /* Peripheral clock enable */
    __HAL_RCC_I2C2_CLK_ENABLE();
  
    /* I2C2 GPIO Configuration
     * PB10 ------> I2C2_SCL
     * PB11 ------> I2C2_SDA
     */
    GPIO_InitStruct.Pin = EEPROM_SCL_Pin|EEPROM_SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}

/**
  * @brief I2C MSP De-Initialization
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
  if(hi2c->Instance == I2C1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();
  
    /* I2C1 GPIO Configuration
     * PB6 ------> I2C1_SCL
     * PB7 ------> I2C1_SDA
     */
    HAL_GPIO_DeInit(GPIOB, OLED_SCL_Pin|OLED_SDA_Pin);
  }
  else if(hi2c->Instance == I2C2)
  {
    /* Peripheral clock disable */
    __HAL_RCC_I2C2_CLK_DISABLE();
  
    /* I2C2 GPIO Configuration
     * PB10 ------> I2C2_SCL
     * PB11 ------> I2C2_SDA
     */
    HAL_GPIO_DeInit(GPIOB, EEPROM_SCL_Pin|EEPROM_SDA_Pin);
  }
}

/**
  * @brief  I2C error callback function
  * @param  hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
  /* Log the error */
  if (hi2c->Instance == I2C1) {
    DEBUG_PRINT("I2C1 Error: %d\r\n", hi2c->ErrorCode);
  } else if (hi2c->Instance == I2C2) {
    DEBUG_PRINT("I2C2 Error: %d\r\n", hi2c->ErrorCode);
  }
  
  /* Attempt to reset the I2C bus */
  if (hi2c->ErrorCode == HAL_I2C_ERROR_BERR || hi2c->ErrorCode == HAL_I2C_ERROR_ARLO) {
    HAL_I2C_DeInit(hi2c);
    if (hi2c->Instance == I2C1) {
      MX_I2C1_Init();
    } else if (hi2c->Instance == I2C2) {
      MX_I2C2_Init();
    }
    DEBUG_PRINT("I2C Bus Reset Complete\r\n");
  }
}

/**
  * @brief  Check if device is available on I2C bus
  * @param  hi2c: I2C handle pointer
  * @param  DevAddress: Target device address
  * @param  Trials: Number of trials
  * @param  Timeout: Timeout duration 
  * @retval HAL status
  */
HAL_StatusTypeDef I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout)
{
  return HAL_I2C_IsDeviceReady(hi2c, DevAddress, Trials, Timeout);
}
