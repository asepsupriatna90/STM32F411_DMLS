/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body for STM32F411 Audio DSP Crossover
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Panel Kontrol DSP STM32F411 untuk Sistem Audio Crossover Aktif
  * 2-input, 4-output DSP processing system
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Audio processing includes */
#include "audio_config.h"
#include "audio_driver.h"
#include "audio_routing.h"
#include "audio_processing.h"

/* UI includes */
#include "ui_config.h"
#include "oled_driver.h"
#include "rotary_encoder.h"
#include "button_handler.h"
#include "led_handler.h"
#include "menu_system.h"

/* Storage includes */
#include "eeprom_driver.h"
#include "preset_manager.h"

/* Utility includes */
#include "debug.h"
#include "system_monitor.h"

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void System_Init(void);
void Error_Handler(void);
static void Audio_Pipeline_Process(void);

/* Private variables ---------------------------------------------------------*/
static volatile uint32_t systemTicks = 0;
static volatile uint8_t audioProcessFlag = 0;
static volatile uint8_t uiUpdateFlag = 0;
static volatile uint32_t lastUserInteraction = 0;

AudioBuffer_TypeDef audioInputBuffer;
AudioBuffer_TypeDef audioOutputBuffer;

/* Global variables ---------------------------------------------------------*/
SystemState_TypeDef SystemState;

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all system components */
  System_Init();
  
  /* Display welcome message */
  OLED_Clear();
  OLED_DrawLogo();
  OLED_DisplayText(1, 0, "DSP Audio Crossover");
  OLED_DisplayText(3, 0, "v1.0.0");
  HAL_Delay(2000);
  
  /* Load last used preset */
  Preset_LoadLast();
  
  /* Initialize menu system */
  Menu_Init();
  
  /* Start audio processing */
  Audio_Start();
  
  DEBUG_PRINT("System initialized successfully\r\n");
  
  /* Infinite loop */
  while (1)
  {
    /* Audio DSP processing */
    if (audioProcessFlag) {
      audioProcessFlag = 0;
      Audio_Pipeline_Process();
    }
    
    /* UI update at lower frequency */
    if (uiUpdateFlag) {
      uiUpdateFlag = 0;
      Button_ProcessEvents();
      RotaryEncoder_ProcessEvents();
      Menu_Update();
      LED_UpdateVUMeter(SystemState.vuMeterLevels);
    }
    
    /* Check if we need to enter low power mode after no interaction */
    if ((HAL_GetTick() - lastUserInteraction) > UI_SCREEN_TIMEOUT_MS) {
      OLED_SetDim(1); /* Dim the display */
      
      /* If even longer inactivity, turn off display */
      if ((HAL_GetTick() - lastUserInteraction) > UI_SCREEN_OFF_TIMEOUT_MS) {
        OLED_DisplayOff();
      }
    }
    
    /* Monitor system performance if debug enabled */
    #ifdef DEBUG_ENABLE
    if ((HAL_GetTick() % 1000) == 0) {
      SystemMonitor_PrintStats();
    }
    #endif
    
    /* Process any pending commands from UART */
    DEBUG_ProcessCommands();
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /* Configure the main internal regulator output voltage */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Initializes the RCC Oscillators according to the specified parameters */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* Initializes the CPU, AHB and APB buses clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                             |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
  
  /* Configure I2S PLL for audio clock */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Audio Processing Pipeline
  * @note This function is called at audio sample rate frequency
  * @retval None
  */
static void Audio_Pipeline_Process(void)
{
  uint32_t startTime = DWT->CYCCNT;  // For performance measurement
  
  /* Get samples from ADC */
  Audio_GetInputSamples(&audioInputBuffer);
  
  /* Apply routing matrix */
  AudioRouting_Process(&audioInputBuffer, &audioOutputBuffer);
  
  /* Process each output channel through DSP chain */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    /* Apply crossover filters */
    DSP_Crossover_Process(i, &audioOutputBuffer);
    
    /* Apply parametric EQ */
    DSP_EQ_Process(i, &audioOutputBuffer);
    
    /* Apply dynamics processing (compressor) */
    DSP_Compressor_Process(i, &audioOutputBuffer);
    
    /* Apply limiter for protection */
    DSP_Limiter_Process(i, &audioOutputBuffer);
    
    /* Apply delay */
    DSP_Delay_Process(i, &audioOutputBuffer);
    
    /* Apply final gain */
    DSP_Gain_Process(i, &audioOutputBuffer);
  }
  
  /* Send processed samples to DAC */
  Audio_SendOutputSamples(&audioOutputBuffer);
  
  /* Update VU meter levels */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    SystemState.vuMeterLevels[i] = Audio_CalculateRMS(i, &audioOutputBuffer);
  }
  
  /* Performance monitoring */
  SystemState.dspLoadPercent = ((DWT->CYCCNT - startTime) * 100) / SystemState.dspCyclesPerFrame;
}

/**
  * @brief TIM6 Period elapsed callback - Used for UI timing
  * @param htim TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6) {
    systemTicks++;
    
    /* Update UI approximately at 20Hz (every 50ms) */
    if ((systemTicks % 50) == 0) {
      uiUpdateFlag = 1;
    }
  }
}

/**
  * @brief Audio processing callback - Called when DMA half/complete callback is received
  * @retval None
  */
void Audio_ProcessCallback(void)
{
  audioProcessFlag = 1;
}

/**
  * @brief User interaction callback - Called on any button press or encoder movement
  * @retval None
  */
void User_InteractionCallback(void)
{
  lastUserInteraction = HAL_GetTick();
  
  /* Turn display back on if it was off/dim */
  OLED_SetDim(0);
  OLED_DisplayOn();
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* Display error on OLED */
  OLED_Clear();
  OLED_DisplayText(0, 0, "ERROR!");
  OLED_DisplayText(2, 0, "System Halted");
  
  /* Turn on all LEDs to indicate error */
  LED_SetAll();
  
  DEBUG_PRINT("System Error! Halted.\r\n");
  
  /* Infinite error loop */
  while (1)
  {
    /* Toggle LEDs to indicate error state */
    LED_ToggleAll();
    HAL_Delay(500);
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  
  DEBUG_PRINT("Assert failed: file %s on line %d\r\n", file, line);
  
  /* Call the error handler */
  Error_Handler();
}
#endif /* USE_FULL_ASSERT */