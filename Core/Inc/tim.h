/**
  ******************************************************************************
  * @file           : tim.h
  * @brief          : Timer configuration for system timing and UI events
  * @author         : asepsupriatna90
  * @version        : 1.0.0
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
  TIM_OK      = 0,
  TIM_ERROR   = 1,
  TIM_BUSY    = 2,
  TIM_TIMEOUT = 3
} TIM_Status_TypeDef;

typedef enum {
  TIM_PRESCALER_1    = 0,
  TIM_PRESCALER_10   = 9,
  TIM_PRESCALER_100  = 99,
  TIM_PRESCALER_1000 = 999
} TIM_Prescaler_TypeDef;

/* Exported constants --------------------------------------------------------*/
/* Timer 6 is used for system tick timing */
#define TIM6_PERIOD_MS           1   /* 1ms timer period */

/* Timer 3 is used for rotary encoder debouncing */
#define TIM3_PERIOD_MS           5   /* 5ms debounce period */

/* Timer 4 is used for button debouncing */
#define TIM4_PERIOD_MS           10  /* 10ms debounce period */

/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim6;   /* System timing */
extern TIM_HandleTypeDef htim3;   /* Rotary encoder debouncing */
extern TIM_HandleTypeDef htim4;   /* Button debouncing */

/* Exported functions prototypes ---------------------------------------------*/
void MX_TIM6_Init(void);     /* Initialize TIM6 for system timing */
void MX_TIM3_Init(void);     /* Initialize TIM3 for rotary encoder */
void MX_TIM4_Init(void);     /* Initialize TIM4 for buttons */

TIM_Status_TypeDef TIM_StartTimer(TIM_HandleTypeDef *htim);
TIM_Status_TypeDef TIM_StopTimer(TIM_HandleTypeDef *htim);
TIM_Status_TypeDef TIM_SetPeriod(TIM_HandleTypeDef *htim, uint32_t Period);
TIM_Status_TypeDef TIM_EnableIT(TIM_HandleTypeDef *htim);
TIM_Status_TypeDef TIM_DisableIT(TIM_HandleTypeDef *htim);
uint32_t TIM_GetTicksElapsed(TIM_HandleTypeDef *htim);
void TIM_DelayMs(uint32_t Delay);

#ifdef __cplusplus
}
#endif
#endif /* __TIM_H__ */