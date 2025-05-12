/**
  ******************************************************************************
  * @file           : gpio.h
  * @brief          : GPIO configuration for STM32F411 Audio DSP Crossover
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
  LED_POWER = 0,
  LED_VU_1,
  LED_VU_2,
  LED_VU_3,
  LED_VU_4, 
  LED_VU_5,
  LED_VU_6,
  LED_VU_7,
  LED_VU_8,
  LED_COUNT
} LED_TypeDef;

typedef enum {
  BUTTON_MENU = 0,
  BUTTON_BACK,
  BUTTON_MUTE,
  BUTTON_PRESET,
  BUTTON_COUNT
} Button_TypeDef;

/* Exported constants --------------------------------------------------------*/
#define LED_POWER_PORT                GPIOC
#define LED_POWER_PIN                 GPIO_PIN_13

#define LED_VU_1_PORT                 GPIOB
#define LED_VU_1_PIN                  GPIO_PIN_12
#define LED_VU_2_PORT                 GPIOB
#define LED_VU_2_PIN                  GPIO_PIN_13
#define LED_VU_3_PORT                 GPIOB
#define LED_VU_3_PIN                  GPIO_PIN_14
#define LED_VU_4_PORT                 GPIOB
#define LED_VU_4_PIN                  GPIO_PIN_15
#define LED_VU_5_PORT                 GPIOA
#define LED_VU_5_PIN                  GPIO_PIN_8
#define LED_VU_6_PORT                 GPIOA
#define LED_VU_6_PIN                  GPIO_PIN_9
#define LED_VU_7_PORT                 GPIOA
#define LED_VU_7_PIN                  GPIO_PIN_10
#define LED_VU_8_PORT                 GPIOA
#define LED_VU_8_PIN                  GPIO_PIN_11

#define BUTTON_MENU_PORT              GPIOA
#define BUTTON_MENU_PIN               GPIO_PIN_0
#define BUTTON_BACK_PORT              GPIOA
#define BUTTON_BACK_PIN               GPIO_PIN_1
#define BUTTON_MUTE_PORT              GPIOA
#define BUTTON_MUTE_PIN               GPIO_PIN_2
#define BUTTON_PRESET_PORT            GPIOA
#define BUTTON_PRESET_PIN             GPIO_PIN_3

#define ROTARY_CLK_PORT               GPIOA
#define ROTARY_CLK_PIN                GPIO_PIN_6
#define ROTARY_DT_PORT                GPIOA
#define ROTARY_DT_PIN                 GPIO_PIN_7
#define ROTARY_SW_PORT                GPIOB
#define ROTARY_SW_PIN                 GPIO_PIN_0

/* I2C for OLED and EEPROM */
#define OLED_I2C_SCL_PORT             GPIOB
#define OLED_I2C_SCL_PIN              GPIO_PIN_8
#define OLED_I2C_SDA_PORT             GPIOB
#define OLED_I2C_SDA_PIN              GPIO_PIN_9

/* I2S for Audio Codec */
#define I2S_MCLK_PORT                 GPIOC
#define I2S_MCLK_PIN                  GPIO_PIN_7
#define I2S_SCLK_PORT                 GPIOC
#define I2S_SCLK_PIN                  GPIO_PIN_10
#define I2S_SD_PORT                   GPIOC
#define I2S_SD_PIN                    GPIO_PIN_12
#define I2S_WS_PORT                   GPIOA
#define I2S_WS_PIN                    GPIO_PIN_4

/* ADC PCM1808 specific pins */
#define PCM1808_FMT_PORT              GPIOB
#define PCM1808_FMT_PIN               GPIO_PIN_1
#define PCM1808_MD_PORT               GPIOB
#define PCM1808_MD_PIN                GPIO_PIN_2

/* DAC PCM5102A specific pins */
#define PCM5102A_XSMT_PORT            GPIOB
#define PCM5102A_XSMT_PIN             GPIO_PIN_3
#define PCM5102A_FLT_PORT             GPIOB
#define PCM5102A_FLT_PIN              GPIO_PIN_4
#define PCM5102A_DEMP_PORT            GPIOB
#define PCM5102A_DEMP_PIN             GPIO_PIN_5

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void MX_GPIO_Init(void);
void GPIO_LED_On(LED_TypeDef Led);
void GPIO_LED_Off(LED_TypeDef Led);
void GPIO_LED_Toggle(LED_TypeDef Led);
uint8_t GPIO_GetButtonState(Button_TypeDef Button);
uint8_t GPIO_GetRotaryEncoderClkState(void);
uint8_t GPIO_GetRotaryEncoderDtState(void);
uint8_t GPIO_GetRotaryEncoderSwState(void);
void GPIO_PCM1808_Configure(uint8_t format, uint8_t mode);
void GPIO_PCM5102A_Configure(uint8_t mute, uint8_t filter, uint8_t deemphasis);

#ifdef __cplusplus
}
#endif
#endif /* __GPIO_H__ */
