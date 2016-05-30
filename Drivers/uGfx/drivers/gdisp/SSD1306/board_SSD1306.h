/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 * file: board_SSD1306.h
 * author: yu
 * date: 2015.12.14
 * site: www.mazclub.com
*/

/**
 * @file    boards/addons/gdisp/board_SSD1306_spi.h
 * @brief   GDISP Graphic Driver subsystem board interface for the SSD1306 display.
 *
 * @note	This file contains a mix of hardware specific and operating system specific
 *			code. You will need to change it for your CPU and/or operating system.
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
// The command byte to put on the front of each page line
//#define SSD1306_PAGE_PREFIX		0x40			 		// Co = 0, D/C = 1

// For a multiple display configuration we would put all this in a structure and then
//	set g->board to that structure.

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

#define SET_RST HAL_GPIO_WritePin(OLED_RES_Port, OLED_RES_Pin, GPIO_PIN_SET);
#define CLR_RST HAL_GPIO_WritePin(OLED_RES_Port, OLED_RES_Pin, GPIO_PIN_RESET);

extern SPI_HandleTypeDef hspi1;

static void init_board(GDisplay *g) {
	// As we are not using multiple displays we set g->board to NULL as we don't use it.
	g->board = 0;
}

static void post_init_board(GDisplay *g) {
	(void) g;
}

static void setpin_reset(GDisplay *g, bool_t state) {
	(void) g;
	if(state)
		CLR_RST
	else
		SET_RST
}

static void acquire_bus(GDisplay *g) {
	(void) g;
}

static void release_bus(GDisplay *g) {
	(void) g;
}

static void write_cmd(GDisplay *g, uint8_t cmd) {
	(void)	g;
        OLED_DC(0);
        OLED_CS(0);
        
        HAL_SPI_Transmit(&hspi1, &cmd, 1, 1);  

        OLED_CS(1);
}

static void write_data(GDisplay *g, uint8_t* data, uint16_t length) {
	(void) g;
        OLED_DC(1);
        OLED_CS(0);
        
        HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, data, length, 20);
        
        if (status != HAL_OK) {
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
          HAL_Delay(200);
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
          
        }
        OLED_CS(1);

}


//systemticks_t gfxSystemTicks(void)
//{
//	return HAL_GetTick();
//}
// 
//systemticks_t gfxMillisecondsToTicks(delaytime_t ms)
//{
//	return ms;
//}

#endif /* _GDISP_LLD_BOARD_H */

