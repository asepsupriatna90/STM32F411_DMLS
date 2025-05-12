/**
  ******************************************************************************
  * @file           : usart.c
  * @brief          : USART configuration and handling for STM32F411 Audio DSP
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  * @attention
  *
  * Implementation of UART functions for debug and command interface
  * Used for debugging audio processing and receiving commands
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include "gpio.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* Buffer for UART reception */
#define UART_RX_BUFFER_SIZE    256
#define UART_TX_BUFFER_SIZE    256

static uint8_t uartRxBuffer[UART_RX_BUFFER_SIZE];
static uint8_t uartTxBuffer[UART_TX_BUFFER_SIZE];
static volatile uint16_t uartRxIndex = 0;
static volatile uint8_t uartRxComplete = 0;

/* Command buffer for processing received commands */
#define MAX_CMD_LENGTH          64
static char cmdBuffer[MAX_CMD_LENGTH];
static uint8_t cmdBufferIndex = 0;

/* Private function prototypes -----------------------------------------------*/
static void UART_ProcessCommand(char *cmd);

/**
  * @brief USART2 Initialization Function
  * @retval None
  */
void MX_USART2_UART_Init(void)
{
  /* Configure the UART peripheral */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }

  /* Enable UART IDLE line detection */
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
  
  /* Start UART reception in DMA mode */
  HAL_UART_Receive_DMA(&huart2, uartRxBuffer, UART_RX_BUFFER_SIZE);
}

/**
  * @brief USART2 De-Initialization Function
  * @retval None
  */
void MX_USART2_UART_DeInit(void)
{
  HAL_UART_DeInit(&huart2);
}

/**
  * @brief  Send string over UART
  * @param  str: String to send
  * @retval None
  */
void UART_SendString(const char *str)
{
  uint16_t len = strlen(str);
  if (len > UART_TX_BUFFER_SIZE) {
    len = UART_TX_BUFFER_SIZE;
  }
  
  /* Copy string to transmit buffer */
  memcpy(uartTxBuffer, str, len);
  
  /* Transmit data over UART */
  HAL_UART_Transmit(&huart2, uartTxBuffer, len, 100);
}

/**
  * @brief  Printf-style function for UART output
  * @param  format: Format string
  * @param  ...: Variable arguments
  * @retval None
  */
void UART_Printf(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  
  /* Format string into transmit buffer */
  vsnprintf((char *)uartTxBuffer, UART_TX_BUFFER_SIZE, format, args);
  
  /* Send formatted string */
  HAL_UART_Transmit(&huart2, uartTxBuffer, strlen((char *)uartTxBuffer), 100);
  
  va_end(args);
}

/**
  * @brief  Process UART reception
  * @note   Should be called in main loop
  * @retval None
  */
void DEBUG_ProcessCommands(void)
{
  if (uartRxComplete) {
    uartRxComplete = 0;
    
    /* Process each byte in the RX buffer */
    while (uartRxIndex > 0) {
      char c = uartRxBuffer[--uartRxIndex];
      
      /* Check for command terminator */
      if (c == '\r' || c == '\n') {
        if (cmdBufferIndex > 0) {
          cmdBuffer[cmdBufferIndex] = '\0';
          UART_ProcessCommand(cmdBuffer);
          cmdBufferIndex = 0;
        }
      } else if (cmdBufferIndex < MAX_CMD_LENGTH - 1) {
        /* Add character to command buffer */
        cmdBuffer[cmdBufferIndex++] = c;
      }
    }
  }
}

/**
  * @brief  Process UART command
  * @param  cmd: Command string
  * @retval None
  */
static void UART_ProcessCommand(char *cmd)
{
  /* Echo command */
  UART_Printf("Received command: %s\r\n", cmd);
  
  /* Command: HELP */
  if (strcmp(cmd, "HELP") == 0) {
    UART_SendString("Available commands:\r\n");
    UART_SendString(" HELP - Display this help\r\n");
    UART_SendString(" VERSION - Show firmware version\r\n");
    UART_SendString(" STATUS - Show system status\r\n");
    UART_SendString(" RESET - Reset system\r\n");
    UART_SendString(" PRESET x - Load preset x (1-10)\r\n");
    UART_SendString(" PRESET SAVE x - Save to preset x (1-10)\r\n");
    UART_SendString(" MUTE x - Mute channel x (1-4)\r\n");
    UART_SendString(" UNMUTE x - Unmute channel x (1-4)\r\n");
    UART_SendString(" XOVER x y - Set crossover frequency for channel x to y Hz\r\n");
    UART_SendString(" GAIN x y - Set gain for channel x to y dB\r\n");
  }
  /* Command: VERSION */
  else if (strcmp(cmd, "VERSION") == 0) {
    UART_SendString("DSP Audio Crossover v1.0.0\r\n");
    UART_SendString("Built: " __DATE__ " " __TIME__ "\r\n");
  }
  /* Command: STATUS */
  else if (strcmp(cmd, "STATUS") == 0) {
    extern SystemState_TypeDef SystemState;
    
    UART_SendString("System Status:\r\n");
    UART_Printf(" DSP load: %d%%\r\n", SystemState.dspLoadPercent);
    UART_Printf(" Sample rate: %d Hz\r\n", SystemState.currentSampleRate);
    
    for (int i = 0; i < AUDIO_OUTPUT_CHANNELS; i++) {
      UART_Printf(" Channel %d: %s, %d Hz, %+.1f dB\r\n", 
                 i+1, 
                 SystemState.channelMute[i] ? "MUTED" : "ACTIVE",
                 SystemState.crossoverFreq[i],
                 SystemState.channelGain[i]);
    }
  }
  /* Command: RESET */
  else if (strcmp(cmd, "RESET") == 0) {
    UART_SendString("Resetting system...\r\n");
    HAL_Delay(100);
    NVIC_SystemReset();
  }
  /* Command pattern: PRESET x */
  else if (strncmp(cmd, "PRESET ", 7) == 0) {
    int presetNum = atoi(&cmd[7]);
    
    if (presetNum >= 1 && presetNum <= 10) {
      UART_Printf("Loading preset %d...\r\n", presetNum);
      /* Call preset loading function */
      extern void Preset_Load(uint8_t presetNum);
      Preset_Load(presetNum);
      UART_SendString("Preset loaded\r\n");
    } else {
      UART_SendString("Invalid preset number. Use 1-10\r\n");
    }
  }
  /* Command pattern: MUTE x */
  else if (strncmp(cmd, "MUTE ", 5) == 0) {
    int channelNum = atoi(&cmd[5]);
    
    if (channelNum >= 1 && channelNum <= AUDIO_OUTPUT_CHANNELS) {
      extern void Audio_MuteChannel(uint8_t channel);
      Audio_MuteChannel(channelNum - 1);
      UART_Printf("Channel %d muted\r\n", channelNum);
    } else {
      UART_SendString("Invalid channel number\r\n");
    }
  }
  /* Command pattern: UNMUTE x */
  else if (strncmp(cmd, "UNMUTE ", 7) == 0) {
    int channelNum = atoi(&cmd[7]);
    
    if (channelNum >= 1 && channelNum <= AUDIO_OUTPUT_CHANNELS) {
      extern void Audio_UnmuteChannel(uint8_t channel);
      Audio_UnmuteChannel(channelNum - 1);
      UART_Printf("Channel %d unmuted\r\n", channelNum);
    } else {
      UART_SendString("Invalid channel number\r\n");
    }
  }
  /* Unknown command */
  else {
    UART_SendString("Unknown command. Type 'HELP' for available commands\r\n");
  }
}

/**
  * @brief  UART MSP Initialization
  * @param  huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  if(huart->Instance == USART2)
  {
    /* Enable peripheral clocks */
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    
    /* USART2 GPIO Configuration
       PA2     ------> USART2_TX
       PA3     ------> USART2_RX */
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 DMA Init */
    /* USART2_RX Init */
    hdma_usart2_rx.Instance = DMA1_Stream5;
    hdma_usart2_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(huart, hdmarx, hdma_usart2_rx);

    /* USART2_TX Init */
    hdma_usart2_tx.Instance = DMA1_Stream6;
    hdma_usart2_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(huart, hdmatx, hdma_usart2_tx);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  }
}

/**
  * @brief  UART MSP De-Initialization
  * @param  huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if(huart->Instance == USART2)
  {
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();
  
    /* USART2 GPIO Configuration
       PA2     ------> USART2_TX
       PA3     ------> USART2_RX */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(huart->hdmarx);
    HAL_DMA_DeInit(huart->hdmatx);

    /* USART2 interrupt DeInit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  }
}

/**
  * @brief  UART RX complete callback
  * @param  huart: UART handle pointer
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2) {
    uartRxComplete = 1;
  }
}

/**
  * @brief  USART2 IRQ Handler
  * @note   This function handles IDLE line detection for DMA-based reception
  * @retval None
  */
void USART2_IRQHandler(void)
{
  /* Check for IDLE line detection */
  if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_IDLE)) {
    /* Clear IDLE flag */
    __HAL_UART_CLEAR_IDLEFLAG(&huart2);
    
    /* Calculate number of received bytes */
    uint16_t dmaCounter = __HAL_DMA_GET_COUNTER(huart2.hdmarx);
    uartRxIndex = UART_RX_BUFFER_SIZE - dmaCounter;
    
    /* Set reception complete flag */
    uartRxComplete = 1;
  }
  
  /* Call HAL UART IRQ handler */
  HAL_UART_IRQHandler(&huart2);
}

/**
  * @brief  DMA1 Stream5 IRQ handler (USART2 RX)
  * @retval None
  */
void DMA1_Stream5_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_usart2_rx);
}

/**
  * @brief  DMA1 Stream6 IRQ handler (USART2 TX)
  * @retval None
  */
void DMA1_Stream6_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_usart2_tx);
}