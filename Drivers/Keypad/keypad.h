/*
*
* Name: keypad.h
*/
#ifndef KEYPAD_H
#define KEYPAD_H

#include "stm32f1xx_hal.h"
#include "pinname.h"
#include "cmsis_os.h"

#define PORT_KEY GPIOD
#define COLS (GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6)

// ¶Ápin
//#define In(GPIO_Pin) (PORT_KEY->IDR & GPIO_Pin)
#define In(GPIO_Pin) HAL_GPIO_ReadPin(PORT_KEY, GPIO_Pin)
// Ð´1µ½Pin
//#define High(GPIO_Pin) PORT_KEY->BSRR = GPIO_Pin
#define High(GPIO_Pin) HAL_GPIO_WritePin(PORT_KEY, GPIO_Pin, GPIO_PIN_SET)
// Ð´0µ½Pin
//#define Low(GPIO_Pin) PORT_KEY->BSRR = (uint32_t)GPIO_Pin << 16 
#define Low(GPIO_Pin) HAL_GPIO_WritePin(PORT_KEY, GPIO_Pin, GPIO_PIN_RESET)

/* 
*  0  1  2   3
*  4  5  6   7
*  8  9  10  11
*  12 13 14  15
*/

typedef enum {
    Key_Up = 0x02,
    Key_Left = 0x03,
    Key_Right = 0x04,
    Key_Down = 0x08,
    Key_Power =  0x09,
    Key_Mode = 0x0a,
    Key_Menu = 0x05,
    Key_None = 0xFF
} KeyPressed;
	
static const int row_count = 4;
static const int col_count = 3;

uint16_t bus_out(void);

void Keypad(void);
char AnyKey(void);
char SameKey(void);
char ScanKey(void);
void FindKey(void);
void ClearKey(void);
void Read(void);
/** Start the keypad interrupt routines */
void Start(void);
/** Stop the keypad interrupt routines */
void Stop(void);
void Cols_out(uint16_t v);
void Scan_Keyboard(void);
KeyPressed getKey(void);
KeyPressed getIrKey(void);
#endif // KEYPAD_H
