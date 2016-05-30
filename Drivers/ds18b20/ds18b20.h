/*
 * DS18B20 使用PB8作为DQ
 * DQ数据线上有2K上拉电阻
*/
#ifndef _DS1820_H
#define _DS1820_H

#include "stdint.h"

#define THERM_CMD_CONVERTTEMP 0x44
#define THERM_CMD_RSCRATCHPAD 0xbe
#define THERM_CMD_WSCRATCHPAD 0x4e
#define THERM_CMD_CPYSCRATCHPAD 0x48
#define THERM_CMD_RECEEPROM 0xb8
#define THERM_CMD_RPWRSUPPLY 0xb4
#define THERM_CMD_SEARCHROM 0xf0
#define THERM_CMD_READROM 0x33
#define THERM_CMD_MATCHROM 0x55
#define THERM_CMD_SKIPROM 0xcc
#define THERM_CMD_ALARMSEARCH 0xec

#define RESOLUTION 10
#define TIM_MST TIM3

#define THERM_PORT GPIOB
#define THERM_PIN GPIO_PIN_8

#define THERM_LOW() GPIOB->BSRR = (uint32_t)GPIO_PIN_8 << 16
#define THERM_HIGH() GPIOB->BSRR = (uint32_t)GPIO_PIN_8

//PB8: IO方向设置
#define THERM_INPUT_MODE()  { GPIOB->CRH &= 0xFFFFFFF0; GPIOB->CRH |= 0x4; }
#define THERM_OUTPUT_MODE() { GPIOB->CRH &= 0xFFFFFFF0; GPIOB->CRH |= 0x5; }

#define DQ_READ() ((GPIOB->IDR & GPIO_PIN_8) >> 8)

// 读取结果
typedef struct {
  int8_t v;
  int8_t error;
} ThermValue;


void therm_delay(uint16_t nCount);
uint8_t therm_reset();
void therm_write_bit(uint8_t bit);
uint8_t therm_read_bit(void);
void therm_write_byte(uint8_t byte);
uint8_t therm_read_byte(void);
void therm_setResolution(uint8_t res);
void therm_start_conversion(void);
ThermValue therm_read_temperature(void);
uint8_t crc8(const uint8_t *addr, uint8_t len);

#endif 