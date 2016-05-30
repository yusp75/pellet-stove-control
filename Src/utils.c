/*
* utils.c
*
*/

#include "utils.h"

/*
* Î¢ÃëÑÓ³Ù
*/
void delay_us(uint16_t us)
{
  __IO uint16_t TIMCounter = us;
  
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


char *trim(char *str)
{
  char *ibuf = str, *obuf = str;
  int i = 0, cnt = 0;
  
  /*
  **  Trap NULL
  */
  
  if (str)
  {
    /*
    **  Remove leading spaces (from RMLEAD.C)
    */
    
    for (ibuf = str; *ibuf && isspace(*ibuf); ++ibuf)
      ;
    if (str != ibuf)
      memmove(str, ibuf, ibuf - str);
    
    /*
    **  Collapse embedded spaces (from LV1WS.C)
    */
    
    while (*ibuf)
    {
      if (isspace(*ibuf) && cnt)
        ibuf++;
      else
      {
        if (!isspace(*ibuf))
          cnt = 0;
        else
        {
          *ibuf = ' ';
          cnt = 1;
        }
        obuf[i++] = *ibuf++;
      }
    }
    obuf[i] = NUL;
    
    /*
    **  Remove trailing spaces (from RMTRAIL.C)
    */
    
    while (--i >= 0)
    {
      if (!isspace(obuf[i]))
        break;
    }
    obuf[++i] = NUL;
  }
  return str;
}

