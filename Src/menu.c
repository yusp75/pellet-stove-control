/*
* menu.c
*/
#include "menu.h"

char *menu_strings[MENU_ITEMS] = { "First Line", "Second Item", "3333333", "abcdefg" };
Menu menu = {0, 0};
extern u8g_t u8g;

void draw_menu(void) 
{
  uint8_t i, h;
  u8g_uint_t w, d;
  //u8g_SetFont(&u8g, u8g_font_6x13);
  u8g_SetFontRefHeightText(&u8g);
  u8g_SetFontPosTop(&u8g);
  h = u8g_GetFontAscent(&u8g)-u8g_GetFontDescent(&u8g);
  w = u8g_GetWidth(&u8g);
  for( i = 0; i < MENU_ITEMS; i++ ) {        // draw all menu items
    d = (w-u8g_GetStrWidth(&u8g, menu_strings[i]))/2;
    u8g_SetDefaultForegroundColor(&u8g);
    if ( i == menu.menu_current ) {               // current selected menu item
      u8g_DrawBox(&u8g, 0, i*h+1, w, h);     // draw cursor bar
      u8g_SetDefaultBackgroundColor(&u8g);
    }
    u8g_DrawStr(&u8g, d, i*h, menu_strings[i]);
  }
}

void update_menu(KeyPressed key)
{
 if ( key == Key_Up ) {
      menu.menu_current++;
      if ( menu.menu_current >= MENU_ITEMS )
        menu.menu_current = 0;
      menu.menu_redraw_required = 1;
 }
 else if ( key == Key_Down ) {
      if ( menu.menu_current == 0 )
        menu.menu_current = MENU_ITEMS;
      menu.menu_current--;
      menu.menu_redraw_required = 1;
 }
}
