/**
  ******************************************************************************
  * @file           : gpio.c
  * @brief          : GPIO configuration for STM32F411 Audio DSP Crossover
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * GPIO configuration for buttons, LED indicators, and peripheral control pins
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "button_handler.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void GPIO_ConfigButtons(void);
static void GPIO_ConfigLEDs(void);
static void GPIO_ConfigCodecControl(void);
static void GPIO_ConfigRotaryEncoder(void);

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* Configure GPIO pin Output Level for LEDs */
  HAL_GPIO_WritePin(GPIOC, LED_VU1_Pin|LED_VU2_Pin|LED_VU3_Pin|LED_VU4_Pin
                         |LED_VU5_Pin|LED_VU6_Pin|LED_VU7_Pin|LED_VU8_Pin, GPIO_PIN_RESET);

  /* Configure GPIO pin Output Level for codec control */
  HAL_GPIO_WritePin(GPIOB, CODEC_RESET_Pin|CODEC_FORMAT_Pin, GPIO_PIN_RESET);

  /* Configure GPIO sub-modules */
  GPIO_ConfigButtons();
  GPIO_ConfigLEDs();
  GPIO_ConfigCodecControl();
  GPIO_ConfigRotaryEncoder();

  /* Configure EXTI for buttons and encoder */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
  HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
  HAL_NVIC_SetPriority(EXTI2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
  HAL_NVIC_SetPriority(EXTI3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
  HAL_NVIC_SetPriority(EXTI4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/**
  * @brief Configure GPIO pins for pushbuttons
  * @param None
  * @retval None
  */
static void GPIO_ConfigButtons(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  /* Configure GPIO pins for buttons with interrupts (EXTI) */
  GPIO_InitStruct.Pin = BUTTON_MENU_Pin|BUTTON_BACK_Pin|BUTTON_MUTE_Pin|BUTTON_PRESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
  * @brief Configure GPIO pins for LED indicators
  * @param None
  * @retval None
  */
static void GPIO_ConfigLEDs(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  /* Configure GPIO pins for LEDs */
  GPIO_InitStruct.Pin = LED_VU1_Pin|LED_VU2_Pin|LED_VU3_Pin|LED_VU4_Pin
                     |LED_VU5_Pin|LED_VU6_Pin|LED_VU7_Pin|LED_VU8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/**
  * @brief Configure GPIO pins for codec control
  * @param None
  * @retval None
  */
static void GPIO_ConfigCodecControl(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  /* Configure GPIO pins for codec control */
  GPIO_InitStruct.Pin = CODEC_RESET_Pin|CODEC_FORMAT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
  * @brief Configure GPIO pins for rotary encoder
  * @param None
  * @retval None
  */
static void GPIO_ConfigRotaryEncoder(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  /* Configure rotary encoder pins A and B with interrupts */
  GPIO_InitStruct.Pin = ENCODER_A_Pin|ENCODER_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /* Configure rotary encoder button with interrupt */
  GPIO_InitStruct.Pin = ENCODER_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
  * @brief  EXTI line detection callback
  * @param  GPIO_Pin Specifies the pin connected to the EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* Handle button presses */
  if (GPIO_Pin == BUTTON_MENU_Pin) {
    Button_HandlePress(BUTTON_MENU);
  } else if (GPIO_Pin == BUTTON_BACK_Pin) {
    Button_HandlePress(BUTTON_BACK);
  } else if (GPIO_Pin == BUTTON_MUTE_Pin) {
    Button_HandlePress(BUTTON_MUTE);
  } else if (GPIO_Pin == BUTTON_PRESET_Pin) {
    Button_HandlePress(BUTTON_PRESET);
  } else if (GPIO_Pin == ENCODER_BTN_Pin) {
    Button_HandlePress(BUTTON_ENCODER);
  }
  
  /* Handle rotary encoder */
  if (GPIO_Pin == ENCODER_A_Pin || GPIO_Pin == ENCODER_B_Pin) {
    RotaryEncoder_UpdateState(
      HAL_GPIO_ReadPin(ENCODER_A_GPIO_Port, ENCODER_A_Pin),
      HAL_GPIO_ReadPin(ENCODER_B_GPIO_Port, ENCODER_B_Pin)
    );
  }
  
  /* Callback to main system for user interaction timing */
  extern void User_InteractionCallback(void);
  User_InteractionCallback();
}