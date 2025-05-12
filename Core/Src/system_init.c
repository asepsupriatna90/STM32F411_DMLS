/**
  ******************************************************************************
  * @file           : system_init.c
  * @brief          : System initialization for STM32F411 Audio DSP Crossover
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * System initialization sequence for the Panel Kontrol DSP STM32F411
  * Sets up all hardware and software components
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
#include "codec_pcm1808.h"
#include "codec_pcm5102a.h"

/* UI includes */
#include "ui_config.h"
#include "oled_driver.h"
#include "oled_graphics.h"
#include "rotary_encoder.h"
#include "button_handler.h"
#include "led_handler.h"
#include "menu_system.h"

/* Storage includes */
#include "eeprom_driver.h"
#include "preset_manager.h"
#include "factory_presets.h"

/* Utility includes */
#include "debug.h"
#include "system_monitor.h"
#include "buffer_manager.h"

/* Private function prototypes -----------------------------------------------*/
static void SystemPeripherals_Init(void);
static void SystemDWT_Init(void);
static void AudioSubsystem_Init(void);
static void UISubsystem_Init(void);
static void StorageSubsystem_Init(void);
static void DSP_Init(void);

/**
  * @brief  Initialize all subsystems
  * @note   Called from main.c after SystemClock_Config
  * @retval None
  */
void System_Init(void)
{
  /* Initialize DWT for cycle counting (performance monitoring) */
  SystemDWT_Init();
  
  /* Initialize all configured peripherals */
  SystemPeripherals_Init();
  
  /* Initialize debug subsystem */
  DEBUG_Init();
  DEBUG_PRINT("\r\n\r\n--- STM32F411 Audio DSP Crossover ---\r\n");
  DEBUG_PRINT("System initialization started...\r\n");
  
  /* Initialize memory buffers */
  BufferManager_Init();
  
  /* Initialize UI subsystem */
  UISubsystem_Init();
  
  /* Initialize storage subsystem */
  StorageSubsystem_Init();
  
  /* Initialize audio subsystem */
  AudioSubsystem_Init();
  
  /* Initialize DSP modules */
  DSP_Init();
  
  /* Initialize system monitor */
  SystemMonitor_Init();
  
  /* Reset system state */
  memset(&SystemState, 0, sizeof(SystemState_TypeDef));
  SystemState.activePresetIndex = 0xFF; /* Invalid index to force loading */
  SystemState.dspSampleRate = AUDIO_SAMPLE_RATE;
  SystemState.dspCyclesPerFrame = (SystemCoreClock / AUDIO_SAMPLE_RATE);
  
  DEBUG_PRINT("System initialization completed\r\n");
}

/**
  * @brief  Initialize MCU peripherals
  * @retval None
  */
static void SystemPeripherals_Init(void)
{
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();  /* For OLED display and EEPROM */
  MX_I2S2_Init();  /* For audio codec PCM1808 (ADC) */
  MX_I2S3_Init();  /* For audio codec PCM5102A (DAC 1-2) */
  MX_SPI1_Init();  /* For audio codec PCM5102A (DAC 3-4) */
  MX_TIM6_Init();  /* For system timing */
  MX_TIM3_Init();  /* For rotary encoder */
  MX_USART1_UART_Init(); /* For debug */
  
  /* Start timers */
  HAL_TIM_Base_Start_IT(&htim6);
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
}

/**
  * @brief  Initialize DWT Cycle Counter for performance monitoring
  * @retval None
  */
static void SystemDWT_Init(void)
{
  /* Enable trace and debug clocks */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  
  /* Reset cycle counter */
  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
  DWT->CYCCNT = 0;
  
  /* Enable cycle counter */
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
  * @brief  Initialize UI subsystem (OLED, buttons, encoder, LEDs)
  * @retval None
  */
static void UISubsystem_Init(void)
{
  DEBUG_PRINT("Initializing UI subsystem...\r\n");
  
  /* Initialize OLED display */
  if (OLED_Init() != HAL_OK) {
    DEBUG_PRINT("OLED initialization failed!\r\n");
    Error_Handler();
  }
  
  /* Initialize rotary encoder */
  RotaryEncoder_Init();
  
  /* Initialize buttons */
  Button_Init();
  
  /* Initialize LEDs */
  LED_Init();
  
  /* Flash LEDs to indicate initialization */
  for (int i = 0; i < 3; i++) {
    LED_SetAll();
    HAL_Delay(100);
    LED_ClearAll();
    HAL_Delay(100);
  }
  
  DEBUG_PRINT("UI subsystem initialized successfully\r\n");
}

/**
  * @brief  Initialize storage subsystem (EEPROM for presets)
  * @retval None
  */
static void StorageSubsystem_Init(void)
{
  uint8_t eepromStatus;
  
  DEBUG_PRINT("Initializing storage subsystem...\r\n");
  
  /* Initialize EEPROM driver */
  eepromStatus = EEPROM_Init();
  
  if (eepromStatus != HAL_OK) {
    DEBUG_PRINT("EEPROM initialization failed! Status: %d\r\n", eepromStatus);
    
    /* Continue with default factory presets */
    DEBUG_PRINT("Using factory presets only\r\n");
    SystemState.storageAvailable = 0;
  } else {
    SystemState.storageAvailable = 1;
    
    /* Initialize preset manager */
    Preset_Init();
    
    /* Check if we need to load factory presets */
    if (EEPROM_ReadByte(EEPROM_INIT_SIGNATURE_ADDR) != EEPROM_INIT_SIGNATURE) {
      DEBUG_PRINT("First run detected, loading factory presets\r\n");
      
      /* Load factory presets */
      Factory_LoadDefaultPresets();
      
      /* Write initialization signature */
      EEPROM_WriteByte(EEPROM_INIT_SIGNATURE_ADDR, EEPROM_INIT_SIGNATURE);
    }
  }
  
  DEBUG_PRINT("Storage subsystem initialized successfully\r\n");
}

/**
  * @brief  Initialize audio subsystem (codecs, DMA, buffers)
  * @retval None
  */
static void AudioSubsystem_Init(void)
{
  DEBUG_PRINT("Initializing audio subsystem...\r\n");
  
  /* Initialize audio configuration */
  Audio_Config_Init();
  
  /* Initialize audio routing matrix */
  AudioRouting_Init();
  
  /* Initialize codec drivers */
  if (PCM1808_Init() != HAL_OK) {
    DEBUG_PRINT("PCM1808 ADC initialization failed!\r\n");
    Error_Handler();
  }
  
  if (PCM5102A_Init() != HAL_OK) {
    DEBUG_PRINT("PCM5102A DAC initialization failed!\r\n");
    Error_Handler();
  }
  
  /* Initialize audio driver */
  if (Audio_Init() != HAL_OK) {
    DEBUG_PRINT("Audio driver initialization failed!\r\n");
    Error_Handler();
  }
  
  DEBUG_PRINT("Audio subsystem initialized successfully\r\n");
}

/**
  * @brief  Initialize DSP modules
  * @retval None
  */
static void DSP_Init(void)
{
  DEBUG_PRINT("Initializing DSP modules...\r\n");
  
  /* Initialize crossover filters */
  if (DSP_Crossover_Init() != HAL_OK) {
    DEBUG_PRINT("Crossover initialization failed!\r\n");
    Error_Handler();
  }
  
  /* Initialize parametric EQ */
  if (DSP_EQ_Init() != HAL_OK) {
    DEBUG_PRINT("EQ initialization failed!\r\n");
    Error_Handler();
  }
  
  /* Initialize compressor */
  if (DSP_Compressor_Init() != HAL_OK) {
    DEBUG_PRINT("Compressor initialization failed!\r\n");
    Error_Handler();
  }
  
  /* Initialize limiter */
  if (DSP_Limiter_Init() != HAL_OK) {
    DEBUG_PRINT("Limiter initialization failed!\r\n");
    Error_Handler();
  }
  
  /* Initialize delay */
  if (DSP_Delay_Init() != HAL_OK) {
    DEBUG_PRINT("Delay initialization failed!\r\n");
    Error_Handler();
  }
  
  /* Set default DSP configuration */
  DSP_SetDefaultConfiguration();
  
  DEBUG_PRINT("DSP modules initialized successfully\r\n");
}

/**
  * @brief  Apply default DSP configuration
  * @note   Called when no preset is available
  * @retval None
  */
void DSP_SetDefaultConfiguration(void)
{
  DEBUG_PRINT("Setting default DSP configuration...\r\n");
  
  /* Set default routing - Stereo 2-way */
  AudioRouting_Config routingConfig;
  routingConfig.output[0].inputSource = INPUT_CH1;  /* Left High */
  routingConfig.output[1].inputSource = INPUT_CH2;  /* Right High */
  routingConfig.output[2].inputSource = INPUT_CH1;  /* Left Low */
  routingConfig.output[3].inputSource = INPUT_CH2;  /* Right Low */
  AudioRouting_SetConfig(&routingConfig);
  
  /* Set default crossover configuration */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    CrossoverConfig_TypeDef xoverConfig;
    
    if (i < 2) {
      /* Outputs 0,1: Highpass 2.5kHz LR24 */
      xoverConfig.type = XOVER_TYPE_HIGHPASS;
      xoverConfig.frequency = 2500;
      xoverConfig.filterType = FILTER_LINKWITZ_RILEY;
      xoverConfig.filterOrder = FILTER_ORDER_24DB;
    } else {
      /* Outputs 2,3: Lowpass 2.5kHz LR24 */
      xoverConfig.type = XOVER_TYPE_LOWPASS;
      xoverConfig.frequency = 2500;
      xoverConfig.filterType = FILTER_LINKWITZ_RILEY;
      xoverConfig.filterOrder = FILTER_ORDER_24DB;
    }
    
    DSP_Crossover_SetConfig(i, &xoverConfig);
  }
  
  /* Set default EQ configuration - flat response */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    for (uint8_t band = 0; band < MAX_PEQ_BANDS; band++) {
      PEQConfig_TypeDef eqConfig;
      
      eqConfig.enabled = (band == 0) ? 1 : 0;  /* Enable only first band */
      eqConfig.frequency = 1000;  /* 1kHz */
      eqConfig.gain = 0.0f;       /* 0dB - flat */
      eqConfig.q = 1.0f;          /* Q = 1 */
      eqConfig.filterType = EQ_FILTER_PEAK;
      
      DSP_EQ_SetBandConfig(i, band, &eqConfig);
    }
  }
  
  /* Set default compressor configuration - disabled */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    CompressorConfig_TypeDef compConfig;
    
    compConfig.enabled = 0;
    compConfig.threshold = -20.0f;  /* -20dB */
    compConfig.ratio = 2.0f;        /* 2:1 ratio */
    compConfig.attack = 20.0f;      /* 20ms */
    compConfig.release = 200.0f;    /* 200ms */
    compConfig.makeupGain = 0.0f;   /* 0dB */
    
    DSP_Compressor_SetConfig(i, &compConfig);
  }
  
  /* Set default limiter configuration - enabled for safety */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    LimiterConfig_TypeDef limConfig;
    
    limConfig.enabled = 1;
    limConfig.threshold = -1.0f;    /* -1dB */
    limConfig.release = 50.0f;      /* 50ms */
    
    DSP_Limiter_SetConfig(i, &limConfig);
  }
  
  /* Set default delay configuration - no delay */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    DelayConfig_TypeDef delayConfig;
    
    delayConfig.enabled = 0;
    delayConfig.delayMs = 0.0f;     /* 0ms */
    delayConfig.phase = PHASE_NORMAL;
    
    DSP_Delay_SetConfig(i, &delayConfig);
  }
  
  /* Set default gain configuration - unity gain */
  for (uint8_t i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
    GainConfig_TypeDef gainConfig;
    
    gainConfig.enabled = 1;
    gainConfig.gainDb = 0.0f;      /* 0dB */
    gainConfig.mute = 0;           /* Not muted */
    
    DSP_Gain_SetConfig(i, &gainConfig);
  }
  
  DEBUG_PRINT("Default DSP configuration set\r\n");
}