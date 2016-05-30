/*
* utils.h
*
*/
#ifndef _UTIL_H
#define _UTIL_H

#define NUL '\0'

#include "stm32f1xx_hal.h"
#include "stdint.h"
#include "stdbool.h"
#include <ctype.h>
#include <string.h>


#define TIM_MST TIM3

typedef struct _TM_DELAY_Timer_t {
    uint32_t ARR;                                        /*!< Auto reload value */
    uint32_t AR;                                         /*!< Set to 1 if timer should be auto reloaded when it reaches zero */
    uint32_t CNT;                                        /*!< Counter value, counter counts down */
    uint8_t CTRL;                                        /*!< Set to 1 when timer is enabled */
    void (*Callback)(struct _TM_DELAY_Timer_t*, void *); /*!< Callback which will be called when timer reaches zero */
    void* UserParameters;                                /*!< Pointer to user parameters used for callback function */
} TM_DELAY_Timer_t;

// FIFOÀàÐÍ
typedef struct {
  uint8_t *buf;
  uint8_t head;
  uint8_t tail;
  uint8_t size;
} fifo_t;


void delay_us(uint16_t us);

char *trim(char *str);


#endif
