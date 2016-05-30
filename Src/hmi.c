#include "hmi.h"
#include "sh1106.h"

#define OLD_CODE

u8g_t u8g;
extern SPI_HandleTypeDef hspi1;

void hmi(void)
{
  u8g.mode = U8G_MODE_BW;
  u8g_InitComFn(&u8g, &u8g_dev_sh1106_128x64_hw_spi, u8g_com_hw_spi_fn);
  u8g_SetDefaultForegroundColor(&u8g);  
  u8g_FirstPage(&u8g);
  u8g_EnableCursor(&u8g);
  do
  {
    draw(0);
  } while ( u8g_NextPage(&u8g) );
}

//Í¨Ñ¶º¯Êý
uint8_t u8g_com_hw_spi_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr)
{
  switch(msg)
  {
    case U8G_COM_MSG_STOP:
      break;
    
    case U8G_COM_MSG_INIT:           
      break;
    
    case U8G_COM_MSG_ADDRESS:                     /* define cmd (arg_val = 0) or data mode (arg_val = 1) */
      HAL_GPIO_WritePin(OLED_DC_Port, OLED_DC_Pin, (GPIO_PinState)arg_val);
     break;

    case U8G_COM_MSG_CHIP_SELECT:
      if ( arg_val == 0 )
      {
        /* disable */
	/* this delay is required to avoid that the display is switched off too early */
        HAL_GPIO_WritePin(OLED_CS_Port, OLED_CS_Pin, GPIO_PIN_SET);
      }
      else
      {
        /* enable */
	HAL_GPIO_WritePin(OLED_CS_Port, OLED_CS_Pin, GPIO_PIN_RESET);
      }
      break;
      
    case U8G_COM_MSG_RESET:
      OLED_CS(0);
      HAL_GPIO_WritePin(OLED_RES_Port, OLED_RES_Pin, (GPIO_PinState)arg_val);
      //delay_us(10);
      HAL_Delay(20);
      break;
      
    case U8G_COM_MSG_WRITE_BYTE:
        OLED_CS(0);
        HAL_SPI_Transmit(&hspi1, &arg_val, 1, 1);
        OLED_CS(1);
      break;
    
    case U8G_COM_MSG_WRITE_SEQ:
    case U8G_COM_MSG_WRITE_SEQ_P:
      {
        register uint8_t *ptr = arg_ptr;
        OLED_CS(0);
        while( arg_val > 0 )
        {
          HAL_SPI_Transmit(&hspi1, ptr, 1, 1);
          ptr++;
          arg_val--;
        }
        OLED_CS(1);
      }
      break;
  }
  return 1;
}

// draw
void draw(uint8_t pos)
{
  //u8g_SetFont(&u8g, u8g_font_unifont);
  u8g_DrawStr(&u8g,  0, 12, "Hello World!");
  u8g_DrawStr(&u8g,  0, 30, "Hello World!");
  u8g_DrawLine(&u8g, 0, 10, 60, 60);
  //u8g_draw_circle(&u8g, 0, 40, 10, 1);
  u8g_SetCursorPos(&u8g, 0, 12);
  u8g_DrawCursor(&u8g);
}
