/*
* ir.c
*/

#include "ir.h"

extern osMessageQId Q_IrHandle;

static __IO uint32_t ccr_prev = 0;
static __IO  uint32_t ccr_cur = 0;
static __IO  uint32_t timeout = 0;
static __IO  uint8_t pulse_cnt = 0;
static __IO  uint32_t ir_code;

NEC_State state = NECSTATE_IDLE;

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance==TIM3 && htim->Channel==HAL_TIM_ACTIVE_CHANNEL_4) {    
    
    ccr_cur = __HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_4);
    
    uint32_t delta_t;
    
    if (ccr_cur>ccr_prev) {
      delta_t = ccr_cur - ccr_prev;      
    }
    else {
      delta_t = 0xffff - ccr_prev + ccr_cur;  // 0xffff溢出
    }
    // 接收到信息头，就接收4个8字节
    switch (state) {
    case NECSTATE_IDLE:
      if (delta_t >= HEADER_MIN && delta_t <= HEADER_MAX) {
        state = NECSTATE_START;
        timeout = delta_t;
        pulse_cnt = 0;
        ir_code = 0;
        // printf("Ir head\r\n");
      }
      break;
      
    case NECSTATE_START:  
      if (delta_t >= DATA1_MIN && delta_t <= DATA1_MAX) {  
        ir_code |= (1<<pulse_cnt);
        pulse_cnt++;
        
        timeout += delta_t;
      }
      else if (delta_t >= DATA0_MIN && delta_t <= DATA0_MAX) {
        pulse_cnt++;
        timeout += delta_t;
      }
      else if (delta_t >= 30000) {  // 3ms 超时
        state = NECSTATE_IDLE;
        pulse_cnt = 0;
        ir_code = 0;
        timeout = 0;
        
      }
      
      break;
      
    }
    // 数据是0~31, 应判断32
    if (pulse_cnt >= 32) {    
      // 接收完毕，复位
      state = NECSTATE_IDLE;
      pulse_cnt = 0;      
      //printf("Ir:%x\r\n", ir_code);
      uint8_t code[4];
      code[0] = ir_code & 0xff;
      code[1] = (ir_code&0xff00) >> 8;
      code[2] = (ir_code&0xff0000) >> 16;
      code[3] = (ir_code&0xff000000) >> 24;
      
      //printf("%x,%x;%x,%x\r\n", code[0],code[1],code[2],code[3]);
      // printf("%x,%x;%x,%x\r\n", ~code[0],code[1],~code[2],code[3]);
      ir_code = 0;
      timeout = 0;
      
      // 校验
      if ((uint8_t)(~code[0])!=code[1] || (uint8_t)(~code[2])!=code[3])
        return;
      
      if (code[0] != IR_ADDR)
        return;
      
      // 放到队列    
      if (Q_IrHandle != NULL) {        
        osMessagePut(Q_IrHandle, (uint32_t)code[2], 0);
      }
    }
    
    // 超时 110ms
    if (timeout >= 110*1000) {   
      state = NECSTATE_IDLE;
      pulse_cnt = 0;
      ir_code = 0;
      timeout = 0;
    }
    
    ccr_prev = ccr_cur;
    
  }
  // 
}