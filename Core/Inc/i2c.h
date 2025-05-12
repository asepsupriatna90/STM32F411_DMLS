/**
  ******************************************************************************
  * @file           : i2c.h
  * @brief          : I2C configuration for OLED display and EEPROM communication
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
  I2C_OK       = 0,
  I2C_ERROR    = 1,
  I2C_BUSY     = 2,
  I2C_TIMEOUT  = 3
} I2C_Status_TypeDef;

/* Exported constants --------------------------------------------------------*/
/* I2C address for devices */
#define OLED_I2C_ADDRESS              0x3C    /* OLED SSD1306 address */
#define EEPROM_I2C_ADDRESS            0x50    /* 24LC256 EEPROM address */

/* Timeout for I2C operations in milliseconds */
#define I2C_TIMEOUT_MS                1000

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern I2C_HandleTypeDef hi2c1;

/* Exported functions prototypes ---------------------------------------------*/
void MX_I2C1_Init(void);
I2C_Status_TypeDef I2C_WriteData(uint8_t DevAddress, uint8_t *pData, uint16_t Size);
I2C_Status_TypeDef I2C_ReadData(uint8_t DevAddress, uint8_t *pData, uint16_t Size);
I2C_Status_TypeDef I2C_WriteRegister(uint8_t DevAddress, uint8_t RegAddress, uint8_t *pData, uint16_t Size);
I2C_Status_TypeDef I2C_ReadRegister(uint8_t DevAddress, uint8_t RegAddress, uint8_t *pData, uint16_t Size);
I2C_Status_TypeDef I2C_WriteMemory(uint8_t DevAddress, uint16_t MemAddress, uint8_t *pData, uint16_t Size);
I2C_Status_TypeDef I2C_ReadMemory(uint8_t DevAddress, uint16_t MemAddress, uint8_t *pData, uint16_t Size);
void I2C_ScanBus(void);

#ifdef __cplusplus
}
#endif
#endif /* __I2C_H__ */