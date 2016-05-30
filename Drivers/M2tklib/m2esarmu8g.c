/*

  m2esarmu8g.c
  
  event source handler for arm controller with u8glib
  
  m2tklib = Mini Interative Interface Toolkit Library
  
  Copyright (C) 2013  olikraus@gmail.com

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.


  This file includes "u8g_arm.h"  
  It is assumed, that the following procedures are available:

    void set_gpio_mode(uint16_t pin, uint8_t is_output, uint8_t is_pullup) U8G_NOINLINE;
    void set_gpio_level(uint16_t pin, uint8_t level) U8G_NOINLINE;
    uint8_t get_gpio_level(uint16_t pin) U8G_NOINLINE;

*/


//#include "u8g.h"
#include "u8g_arm.h"
#include "m2.h"
#include "cmsis_os.h"

static void m2_arm_u8g_setup_key(uint8_t key) M2_NOINLINE;
static void m2_arm_u8g_setup(void) M2_NOINLINE;
static uint8_t m2_arm_u8g_check_key(uint8_t key) M2_NOINLINE;
static uint8_t m2_arm_u8g_get_key(void) M2_NOINLINE;

extern osMessageQId Q_KeyHandle;
extern m2_key_to_pin_entry_t m2_key_to_pin_array[M2_KEY_CNT];  // 键索引

static void m2_arm_u8g_setup_key(uint8_t key)
{
  __IO uint8_t pin;
  if ( m2_IsPinAssigned(key) != 0 )
  {
    /* here, we assume that the user has assigned the internal u8g pin number: PN(port,bit) */
    pin = m2_GetPin(key);
    //set_gpio_mode(pin, 0, 1);	/* input with pullup */
    
  }
}

static void m2_arm_u8g_setup(void) 
{
  uint8_t i;
  for( i = M2_KEY_LOOP_START; i <= M2_KEY_LOOP_END; i++ )
    m2_arm_u8g_setup_key(i);
}

static uint8_t m2_arm_u8g_check_key(uint8_t key)
{
  // 不用
  return 0;
}

static uint8_t m2_arm_u8g_get_key(void)
{  
  __IO KeyPressed key_read;
    // 从message里取键
    osEvent evt;
    evt = osMessageGet(Q_KeyHandle, 0);
    
    if (evt.status == osEventMessage) {
      key_read = (KeyPressed)evt.value.v;
      // printf("pin:%d\r\n", key_read);       
    
      if ( key_read == m2_key_to_pin_array[M2_KEY_SELECT-1].pin ) 
        return M2_KEY_SELECT;
        
      if ( key_read == m2_key_to_pin_array[M2_KEY_SELECT2-1].pin ) 
        return M2_KEY_SELECT;
      
      /* map to normal SELECT */
      if ( key_read == m2_key_to_pin_array[M2_KEY_EXIT-1].pin ) 
        return M2_KEY_EXIT;
      
      if ( key_read == m2_key_to_pin_array[M2_KEY_NEXT-1].pin ) 
        return M2_KEY_NEXT;
      
      if ( key_read == m2_key_to_pin_array[M2_KEY_PREV-1].pin ) 
        return M2_KEY_PREV;
      
      if ( key_read == m2_key_to_pin_array[M2_KEY_DATA_UP-1].pin ) 
        return M2_KEY_DATA_UP;
      
      if ( key_read == m2_key_to_pin_array[M2_KEY_DATA_DOWN-1].pin ) 
        return M2_KEY_DATA_DOWN;
      
      if ( key_read == m2_key_to_pin_array[M2_KEY_HOME-1].pin ) 
        return M2_KEY_HOME;
    }   
    
  return M2_KEY_NONE;

}

uint8_t m2_es_arm_u8g(m2_p ep, uint8_t msg)
{
  switch(msg)
  {
    case M2_ES_MSG_GET_KEY:
      // 取键消息
      return m2_arm_u8g_get_key();
    case M2_ES_MSG_INIT:
      m2_arm_u8g_setup();
      return 0;
  }
  return 0;
}
