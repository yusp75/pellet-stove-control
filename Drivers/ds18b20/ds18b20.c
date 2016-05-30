/*
*    FILENAME:    ds1820.c
*    DATE:        2015.12.03
*    AUTHOR:      Christian Stadler
*/

#include "ds18b20.h"
#include "stm32f1xx_hal.h"

extern TIM_HandleTypeDef htim3;

/*
* 微秒延迟
*/
void therm_delay(uint16_t nCount)
{
  __IO uint16_t TIMCounter = nCount;
  
  uint16_t start = TIM_MST->CNT;
  uint16_t end = start + TIMCounter;
  __IO uint16_t read;
  
  while (1) {
    read = TIM_MST->CNT;
    if(read < start)
      read = 0xffff - start + read;
    if (read >= end)
      break;
  }
  
}

//复位
uint8_t therm_reset()
{
  uint8_t r;
  
  THERM_LOW();
  THERM_OUTPUT_MODE();
  
  therm_delay(480);
  THERM_HIGH();
  
  THERM_INPUT_MODE();
  therm_delay(60);
  
  r = !DQ_READ();
  therm_delay(420);
  
  return r;
}

// 写位
void therm_write_bit(uint8_t bit)
{
  
  THERM_LOW();
  THERM_OUTPUT_MODE();
  therm_delay(1);
  
  if (bit)
    THERM_INPUT_MODE();
  
  therm_delay(60);
  THERM_INPUT_MODE();
  
}

//读位
uint8_t therm_read_bit(void)
{
  uint8_t r;
  
  THERM_LOW();
  THERM_OUTPUT_MODE();    
  therm_delay(1);
  
  THERM_INPUT_MODE(); 
  therm_delay(14);
  
  r = DQ_READ();
  
  therm_delay(45);
  
  return r;
}

//读字节
uint8_t therm_read_byte(void)
{
  uint8_t i = 8, n = 0;
  
  while(i--) {
    
    n >>= 1;
    n |= (therm_read_bit() << 7);
  }
  return n;
} 

//写字节
void therm_write_byte(uint8_t byte)
{
  uint8_t i = 8;
  
  while (i--) {
    therm_write_bit(byte&1);
    byte >>= 1;
  }
}

//设置分辨率
void therm_setResolution(uint8_t res) 
{    
  uint8_t data[9];  
  // keep resolution within limits
  if(res > 12)
    res = 12;
  if(res < 9)
    res = 9;      
  
  if (therm_reset()) {
    therm_write_byte(THERM_CMD_SKIPROM);
    therm_write_byte(THERM_CMD_RSCRATCHPAD);            // to read Scratchpad
    for(uint8_t i = 0; i < 9; i++)  // read Scratchpad bytes
      data[i] = DQ_READ(); 
    
    data[4] |= (res - 9) << 5;      // update configuration byte (set resolution)  
  }
  
  if(therm_reset()) {
    therm_write_byte(THERM_CMD_SKIPROM);
    therm_write_byte(THERM_CMD_WSCRATCHPAD);  // 0x4e to write into Scratchpad
    for(uint8_t i = 2; i < 5; i++)  //write three bytes (2nd, 3rd, 4th) into Scratchpad
      therm_write_byte(data[i]);
  }
}

// 转换到浮点
float toFloat(uint16_t word) {
  if(word & 0x8000)
    return -1 * ((uint16_t)(~word + 1) / 256.0f);
  else
    return (word / 256.0f);
}

//开始转换
void therm_start_conversion(void)
{
  //Reset, skip ROM and start temperature conversion
  if(therm_reset()) {
    therm_write_byte(THERM_CMD_SKIPROM);
    therm_write_byte(THERM_CMD_CONVERTTEMP);
  }  
}

//读温度
ThermValue therm_read_temperature(void)
{
  uint8_t data[9];
  ThermValue t;
  
  if(therm_reset()) {
    therm_write_byte(THERM_CMD_SKIPROM);
    therm_write_byte(THERM_CMD_RSCRATCHPAD);            // to read Scratchpad
    for(uint8_t i = 0; i < 9; i++)  // read Scratchpad bytes
      data[i] = therm_read_byte();   
    //crc 校验
    uint8_t crc = crc8(data, 8);
    
    // 转换原始数据到16位无符号值
    
    if (crc == data[8]) {
      uint16_t* p_word = (uint16_t*) (&data[0]);       
      uint8_t cfg = (data[4] & 0x60); // default 12-bit resolution
      
      // at lower resolution, the low bits are undefined, so let's clear them
      if(cfg == 0x00)
        *p_word = *p_word &~7;      //  9-bit resolution
      else
        if(cfg == 0x20)
          *p_word = *p_word &~3;      // 10-bit resolution
        else
          if(cfg == 0x40)
            *p_word = *p_word &~1;      // 11-bit resolution
        
        
        // Convert the raw bytes to a 16-bit signed fixed point value :
        // 1 sign bit, 7 integer bits, 8 fractional bits (two’s compliment
        // and the LSB of the 16-bit binary number represents 1/256th of a unit).
        *p_word = *p_word << 4;
        
        // Convert to floating point value
        t.v = (int8_t)toFloat(*p_word);
        t.error = 0;
    }  // crc is ok
    else
      t.error = 1;
  }  // not lost
  else {
    t.error = 2;  // not probe
  }
  // 返回值
  return t;
}

// Compute a Dallas Semiconductor 8 bit CRC directly.
//
uint8_t crc8(const uint8_t *addr, uint8_t len)
{
  uint8_t crc = 0;
  
  while (len--) {
    uint8_t inbyte = *addr++;
    
    for (uint8_t i = 8; i; i--) {
      uint8_t mix = (crc ^ inbyte) & 0x01;
      
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      inbyte >>= 1;
    }
  }
  return crc;
}