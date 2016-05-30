/*
*Î¢Ãëº¯Êý
*
*/
#ifndef TICKER_H
#define TICKER_H
#include "stm32f1xx_hal.h"

#define TIM_MST TIM3

uint16_t us_ticker_read(void);
void wait_us(int us);

#endif