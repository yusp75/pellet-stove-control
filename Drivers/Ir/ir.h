/*
* HS0038A2
* ir.h
*
*/

#ifndef _IR_H
#define _IR_H

#include "tim.h"
#include "cmsis_os.h"

#define DELTA 300
#define HEADER_MIN 13500 - DELTA
#define HEADER_MAX 13500 + DELTA
#define DATA1_MIN 2250 - DELTA
#define DATA1_MAX 2250 + DELTA
#define DATA0_MIN 1120 - DELTA
#define DATA0_MAX 1120 + DELTA

#define IR_ADDR 2

typedef enum { 
  NECSTATE_IDLE = 0,
  NECSTATE_START = 1,
  NECSTATE_RECEIVE  = 2,
} NEC_State;

#endif