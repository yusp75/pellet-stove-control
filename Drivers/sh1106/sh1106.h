#ifndef _OLED_H
#define _OLED_H


#include "stm32f1xx_hal.h"
#include "cmsis_os.h"

#define isCMD  0
#define isDATA 1

#define OLED_SCLK_Port  (GPIOA)
#define OLED_SCLK_Pin  (GPIO_PIN_5)
#define OLED_MOSI_Port  (GPIOA)
#define OLED_MOSI_Pin  (GPIO_PIN_7)
#define OLED_RES_Port  (GPIOC)
#define OLED_RES_Pin  (GPIO_PIN_14)
#define OLED_DC_Port  (GPIOC)
#define OLED_DC_Pin  (GPIO_PIN_15)
#define OLED_CS_Port  (GPIOA)
#define OLED_CS_Pin  (GPIO_PIN_8)

#define SIZE 16
#define XLevelL		0x02
#define XLevelH		0x10
#define Max_Column	128
#define Max_Row		64
#define	Brightness	0xA5
#define X_WIDTH 	128
#define Y_WIDTH 	64

//-----------------OLED¶Ë¿Ú¶¨Òå----------------  					   

void delay_ms(uint32_t uCount);
		     
void OLED_WriteByte(uint8_t dat,uint8_t cmd);	       							   		    
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr);
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t size2);
void OLED_ShowString(uint8_t x,uint8_t y, uint8_t *p);	 
void OLED_SetPos(uint8_t x, uint8_t y);
void OLED_ShowChinese(uint8_t x,uint8_t y,uint8_t no);
void OLED_DrawBMP(uint8_t x0, uint8_t y0,uint8_t x1, uint8_t y1,uint8_t BMP[]);


#define OLED_SCLK(a) if (a) \
                      HAL_GPIO_WritePin(OLED_SCLK_Port,OLED_SCLK_Pin, GPIO_PIN_SET);\
                      else \
                      HAL_GPIO_WritePin(OLED_SCLK_Port,OLED_SCLK_Pin, GPIO_PIN_RESET)
                        
#define OLED_MOSI(a) if (a) \
                      HAL_GPIO_WritePin(OLED_MOSI_Port,OLED_MOSI_Pin, GPIO_PIN_SET);\
                      else \
                      HAL_GPIO_WritePin(OLED_MOSI_Port,OLED_MOSI_Pin, GPIO_PIN_RESET)
					
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

#define delay_ms(x)     osDelay(x)
                              
#endif