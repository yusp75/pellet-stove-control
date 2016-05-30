/*
*
* max667.h
*/
#ifndef _MAX6675_H
#define _MAX667_H

#include "stm32f1xx_hal.h"
#include "pinname.h"
#include "cmsis_os.h"

#define K_PORT GPIOC
 
typedef struct {
  uint16_t v;
  uint8_t error;
} K_Value;

void read_k(void);    
                          
#endif
