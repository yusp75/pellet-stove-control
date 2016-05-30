/* ÈË»ú½»»¥ hmi
* hmi.h
*/

#ifndef _HMI_H
#define _HMI_H 
//#include "ugui.h"
//#include "sh1106.h"
#include "u8g.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "utils.h"

#define OLED_DC_Port  (GPIOC)
#define OLED_DC_Pin  (GPIO_PIN_15)
#define OLED_CS_Port  (GPIOA)
#define OLED_CS_Pin  (GPIO_PIN_8)
#define OLED_RES_Port  (GPIOC)
#define OLED_RES_Pin  (GPIO_PIN_14)

#define OLED_RES(a) if (a) \
                      HAL_GPIO_WritePin(OLED_RES_Port,OLED_RES_Pin, GPIO_PIN_SET);\
                      else \
                      HAL_GPIO_WritePin(OLED_RES_Port,OLED_RES_Pin, GPIO_PIN_RESET)

#define OLED_DC(a) if (a) \
                      HAL_GPIO_WritePin(OLED_DC_Port,OLED_DC_Pin, GPIO_PIN_SET);\
                      else \
                      HAL_GPIO_WritePin(OLED_DC_Port,OLED_DC_Pin, GPIO_PIN_RESET)
                                          
#define OLED_CS(a) if (a) \
                      HAL_GPIO_WritePin(OLED_CS_Port,OLED_CS_Pin, GPIO_PIN_SET);\
                      else \
                      HAL_GPIO_WritePin(OLED_CS_Port,OLED_CS_Pin, GPIO_PIN_RESET)


extern u8g_t u8g;

void hmi(void);
uint8_t u8g_com_hw_spi_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr);
void draw(uint8_t pos);

#endif