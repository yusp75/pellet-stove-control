/*
* menu.h
*
*/

#ifndef _MENU_H
#define _MENU_H

#include "u8g.h"
#include "keypad.h"

#define MENU_ITEMS 4

typedef struct{
  uint8_t menu_current;
  uint8_t menu_redraw_required;
}Menu;

void draw_menu(void);
void update_menu(KeyPressed key);

#endif