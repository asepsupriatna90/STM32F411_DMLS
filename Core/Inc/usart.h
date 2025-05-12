/**
  ******************************************************************************
  * @file           : usart.h
  * @brief          : USART configuration for debug interface
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
  USART_OK      = 0,
  USART_ERROR   = 1,
  USART_BUSY    = 2,
  USART_TIMEOUT = 3
} USART_Status_TypeDef;

/* Exported constants --------------------------------------------------------*/
#define USART_BAUDRATE            115200
#define USART_TX_BUFFER_SIZE      256
#define USART_RX_BUFFER_SIZE      256
#define USART_TIMEOUT_MS          1000

/* Command identifiers for debug interface */
#define USART_CMD_QUERY_STATUS    'S'  /* Query system status */
#define USART_CMD_DUMP_CONFIG     'C'  /* Dump current configuration */
#define USART_CMD_RESET           'R'  /* Reset system */
#define USART_CMD_SET_PARAM       'P'  /* Set parameter */
#define USART_CMD_GET_PARAM       'G'  /* Get parameter */
#define USART_CMD_LOAD_PRESET     'L'  /* Load preset */
#define USART_CMD_SAVE_PRESET     'V'  /* Save preset */

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart2;
extern uint8_t USART_RxBuffer[USART_RX_BUFFER_SIZE];
extern uint8_t USART_TxBuffer[USART_TX_BUFFER_SIZE];
extern volatile uint16_t USART_RxCount;

/* Exported functions prototypes ---------------------------------------------*/
void MX_USART2_UART_Init(void);
USART_Status_TypeDef USART_Transmit(uint8_t *pData, uint16_t Size);
USART_Status_TypeDef USART_Receive(uint8_t *pData, uint16_t Size);
USART_Status_TypeDef USART_TransmitString(const char *str);
USART_Status_TypeDef USART_Printf(const char *format, ...);
void USART_ProcessCommand(void);
void USART_RxCpltCallback(UART_HandleTypeDef *huart);
void USART_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
void USART_ErrorCallback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif
#endif /* __USART_H__ */