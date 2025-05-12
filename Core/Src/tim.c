/**
  ******************************************************************************
  * @file           : tim.c
  * @brief          : Timer configuration for STM32F411 Audio DSP Crossover
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Timer implementations for Panel Kontrol DSP STM32F411
  * - TIM6: Used for UI update timing (20Hz)
  * - TIM2: Used for rotary encoder input
  * - TIM3: Used for system monitor timing
  * - TIM10: Used for LED blinking and VU meter effects
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include "gpio.h"

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;  /* Rotary encoder timer */
TIM_HandleTypeDef htim3;  /* System monitoring timer */
TIM_HandleTypeDef htim6;  /* UI update timer */
TIM_HandleTypeDef htim10; /* LED effects timer */

/* Private function prototypes -----------------------------------------------*/
static void MX_TIM6_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM10_Init(void);

/**
  * @brief Initialize all timers
  * @retval None
  */
void TIM_Init(void)
{
  /* Initialize timers */
  MX_TIM6_Init();   /* UI update timer */
  MX_TIM2_Init();   /* Rotary encoder timer */
  MX_TIM3_Init();   /* System monitoring timer */
  MX_TIM10_Init();  /* LED effects timer */
  
  /* Start timers */
  HAL_TIM_Base_Start_IT(&htim6);   /* UI timer with interrupt */
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL); /* Encoder timer */
  HAL_TIM_Base_Start_IT(&htim3);   /* System monitor timer */
  HAL_TIM_Base_Start_IT(&htim10);  /* LED effects timer */
  
  /* Enable interrupt for TIM6 */
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

/**
  * @brief TIM6 Initialization Function - UI Update Timer
  * @note  TIM6 configured to interrupt at 20Hz for UI updates
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* TIM6 runs at APB1 clock (e.g., 42 MHz)
   * With prescaler 4200-1, counter increments at 10 kHz
   * With period 500-1, interrupt occurs at 20 Hz (50ms) */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 4200 - 1;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 500 - 1;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function - Rotary Encoder Interface
  * @note  TIM2 configured as encoder interface for rotary input
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  TIM_Encoder_InitTypeDef sEncoderConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* TIM2 encoder mode - counting on both edges */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;  /* 16-bit counter */
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  
  sEncoderConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sEncoderConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sEncoderConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sEncoderConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sEncoderConfig.IC1Filter = 10;  /* Filtering to avoid glitches */
  sEncoderConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sEncoderConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sEncoderConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sEncoderConfig.IC2Filter = 10;  /* Filtering to avoid glitches */
  
  if (HAL_TIM_Encoder_Init(&htim2, &sEncoderConfig) != HAL_OK)
  {
    Error_Handler();
  }
  
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function - System Monitor Timer
  * @note  TIM3 configured to generate interrupts at 1Hz for system monitoring
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* TIM3 at 1Hz for system monitoring */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 8400 - 1;    /* Divide by 8400 */
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 10000 - 1;      /* 1 Hz interrupt */
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  
  /* Enable TIM3 interrupt */
  HAL_NVIC_SetPriority(TIM3_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

/**
  * @brief TIM10 Initialization Function - LED Effects Timer
  * @note  TIM10 configured for VU meter animation/effects (50Hz)
  * @param None
  * @retval None
  */
static void MX_TIM10_Init(void)
{
  /* TIM10 at 50Hz for LED animations */
  htim10.Instance = TIM10;
  htim10.Init.Prescaler = 8400 - 1;   /* Divide by 8400 */
  htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim10.Init.Period = 200 - 1;       /* 50 Hz interrupt */
  htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  
  if (HAL_TIM_Base_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  
  /* Enable TIM10 interrupt */
  HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
}

/**
  * @brief  Period elapsed callback for TIM6 - UI update timer
  * @note   This function is called from TIM6 IRQ handler
  * @param  htim TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* UI Update Timer */
  if (htim->Instance == TIM6)
  {
    /* This is handled in main.c */
  }
  
  /* System Monitoring Timer */
  else if (htim->Instance == TIM3)
  {
    /* Perform system status checks */
    SystemMonitor_Update();
  }
  
  /* LED Effects Timer */
  else if (htim->Instance == TIM10)
  {
    /* Update LED animations for VU meter */
    LED_UpdateEffects();
  }
}

/**
  * @brief  Gets the current value of the rotary encoder
  * @retval Current encoder value
  */
uint16_t TIM_GetEncoderValue(void)
{
  return (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
}

/**
  * @brief  Sets the value of the rotary encoder counter
  * @param  value: The value to set
  * @retval None
  */
void TIM_SetEncoderValue(uint16_t value)
{
  __HAL_TIM_SET_COUNTER(&htim2, value);
}

/**
  * @brief  TIM MSP Initialization
  * @param  htim: TIM handle pointer
  * @retval None
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  if (htim->Instance == TIM2)
  {
    /* TIM2 clock enable */
    __HAL_RCC_TIM2_CLK_ENABLE();
    
    /* TIM2 GPIO Configuration: Encoder inputs */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;  /* PA0, PA1 for encoder */
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
  else if (htim->Instance == TIM3)
  {
    /* TIM3 clock enable */
    __HAL_RCC_TIM3_CLK_ENABLE();
    
    /* TIM3 interrupt Init */
    HAL_NVIC_SetPriority(TIM3_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
  }
  else if (htim->Instance == TIM6)
  {
    /* TIM6 clock enable */
    __HAL_RCC_TIM6_CLK_ENABLE();
    
    /* TIM6 interrupt Init */
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
  }
  else if (htim->Instance == TIM10)
  {
    /* TIM10 clock enable */
    __HAL_RCC_TIM10_CLK_ENABLE();
    
    /* TIM10 interrupt Init */
    HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
  }
}

/**
  * @brief  TIM MSP De-Initialization
  * @param  htim: TIM handle pointer
  * @retval None
  */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim)
{
  if (htim->Instance == TIM2)
  {
    /* Peripheral clock disable */
    __HAL_RCC_TIM2_CLK_DISABLE();
    
    /* TIM2 GPIO Configuration: Encoder inputs */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0 | GPIO_PIN_1);
  }
  else if (htim->Instance == TIM3)
  {
    /* Peripheral clock disable */
    __HAL_RCC_TIM3_CLK_DISABLE();
    
    /* TIM3 interrupt DeInit */
    HAL_NVIC_DisableIRQ(TIM3_IRQn);
  }
  else if (htim->Instance == TIM6)
  {
    /* Peripheral clock disable */
    __HAL_RCC_TIM6_CLK_DISABLE();
    
    /* TIM6 interrupt DeInit */
    HAL_NVIC_DisableIRQ(TIM6_DAC_IRQn);
  }
  else if (htim->Instance == TIM10)
  {
    /* Peripheral clock disable */
    __HAL_RCC_TIM10_CLK_DISABLE();
    
    /* TIM10 interrupt DeInit */
    HAL_NVIC_DisableIRQ(TIM1_UP_TIM10_IRQn);
  }
}

/**
  * @brief TIM6 & DAC interrupt handler
  * @retval None
  */
void TIM6_DAC_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim6);
}

/**
  * @brief TIM3 interrupt handler
  * @retval None
  */
void TIM3_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim3);
}

/**
  * @brief TIM1 Update and TIM10 interrupt handler
  * @retval None
  */
void TIM1_UP_TIM10_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim10);
}