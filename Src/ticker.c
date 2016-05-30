#include "ticker.h"

uint16_t us_ticker_read(void)
{
  return TIM_MST->CNT;

}

void wait_us(int us) {
    uint32_t start = us_ticker_read();
    while ((us_ticker_read() - start) < (uint32_t)us);
}