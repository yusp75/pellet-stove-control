/*
* 2016.01.23
* triac.c
* 晶闸管驱动
*/

#include "stm32f1xx_hal.h"
#include "tim.h"
#include "pinname.h"
#include "cmsis_os.h"
#include "user.h"

#define uS_DELAY_OFF 999
#define HALF_T1 9999

// 排烟
#define ON_SMOKE PORT_SCR->BSRR = (uint32_t)SCR0_smoke << 16
#define OFF_SMOKE PORT_SCR->BSRR = SCR0_smoke

// 循环
#define ON_EXCHANGE PORT_SCR->BSRR = (uint32_t)SCR1_exchange << 16
#define OFF_EXCHANGE PORT_SCR->BSRR = SCR1_exchange
// 送料
#define ON_FEED PORT_SCR->BSRR = (uint32_t)SCR2_feed << 16
#define OFF_FEED PORT_SCR->BSRR = SCR2_feed

static __IO uint16_t counter = 0;

static __IO  Triac triac_cur = {false, 1000, false, 1000, false, false, false, 3};  // 风机全部关闭
static __IO  uint16_t on_count = 250;
static __IO  uint16_t off_count = 250;
static __IO  uint16_t count_feed = 0;

static __IO bool feed_run = true;  //输出中

static __IO bool is_lower_blow = false;
static __IO bool is_lower_exchange = false;
static __IO bool is_lower_feed = false;

static __IO uint32_t last_delay = 4500;

/* t 参数 */
static __IO uint32_t t1 = 0;
static __IO uint32_t t2 = 0;
static __IO uint32_t t3 = 0;
static __IO uint32_t t4 = 0;
static __IO uint32_t t5 = 0;
static __IO uint32_t t6 = 0;
static __IO uint32_t t7 = 0;
static __IO uint32_t T1 = 0;

// 延时表
const uint16_t table_delay[7] = {115, 110, 105,100, 95, 90, 80 };

extern osPoolId  pool_TriacHandle;
extern osMessageQId Q_TriacHandle;
extern osPoolId pool_ZeroHandle;
extern osMessageQId Q_ErrorHandle;

/* 过零点触发中断 回调函数 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  
  if (GPIO_Pin != ZERO) 
    return;
  
  bool rising_edge;
  // 读Pin，判断是上升沿还是下降沿    
  if (HAL_GPIO_ReadPin(PORT_ZERO, ZERO) == GPIO_PIN_SET) {
    rising_edge = true;
  }
  else {
    rising_edge = false;
  }
  
  // t 顺序： 1 3 4 6
  
  // 上升沿
  if (rising_edge) {
    t6 = __HAL_TIM_GET_COUNTER(&htim3);
  }
  else {
    // 下降沿
    t4 = __HAL_TIM_GET_COUNTER(&htim3);
  }

  if (!rising_edge)  // 只在波谷计算，否则退出
    return;
  
/*----------------------------------------------------------------------------*/
    
  is_lower_blow = true;  // 设置波谷flag
  is_lower_exchange = true;
  is_lower_feed = true;
  
  /* 下降沿时计算下一个零点 */ 
  // t1 t3 t4 t6
  if ((t1<t3) && (t3<t4) && (t4<t6)) {  
    t2 = (t1+t3) / 2; 
    t5 = (t4+t6) / 2;
  }
  else if(t3<t1) {
    t2 = (t1+t3+0xffff) / 2;
    t5 = (t4+t6)/2 + 0xffff;
  }
  else if(t4<t3) {
    t2 = (t1+t3) / 2;
    t5 = (t4+t6)/2 + 0xffff;
  }
  else if(t6<t4) {
    t2 = (t1+t3) / 2;
    t5 = (t4+t6+0xffff) / 2;
  }
  
    T1 = t5 - t2;
    
    // 下一个零点    
  if (t6<t5) {
    t7 = t5 + T1/4 - t6 -0xffff;
  }
  else {
    t7 = t5 + T1/4 - t6;
  }  

  //  debug
//  counter++;
//  if (counter >= 149) {
//    Zero *z;
//    z = (Zero*)osPoolAlloc(pool_ZeroHandle);
//    if (z!=NULL) {
//      z->t1 = t1;
//      z->t3 = t3;
//      z->t4 = t4;
//      z->t6 = t6;
//      z->t7 = t7;
//      z->T1 = T1;
//      osMessagePut(Q_ErrorHandle, (uint32_t)z, 0);
//    }
//    
//    counter = 0;
//  }  
//  
  
  // 更新数值
  t1 = t4;
  t3 = t6;
    
  /*--------------------------------------------------------------------------*/
  
  // 只处理下半波
  
  if (t7 == 0) {
    t7 = 220;
  }
  
  is_lower_blow = true;  // 设置波谷flag
  is_lower_exchange = true;
  is_lower_feed = true;
  
  //uint32_t compare = 0;
  
  // 检查Q_Triac队列，更新输出的控制状态
  if (Q_TriacHandle != NULL) {    
    // 读取值
    osEvent evt = osMessageGet(Q_TriacHandle, 0);
    if (evt.status == osEventMessage) {  
      Triac *p = (Triac *)evt.value.p;  
      
      triac_cur.fan_exchange_delay = p->fan_exchange_delay; 
      triac_cur.fan_exchange_is_on = p->fan_exchange_is_on;
      triac_cur.fan_smoke_is_on = p->fan_smoke_is_on;
      triac_cur.fan_smoke_power = p->fan_smoke_power;
      triac_cur.feed_by_manual = p->feed_by_manual;
      triac_cur.feed_duty = p->feed_duty;
      triac_cur.feed_full = p->feed_full;
      triac_cur.feed_is_on = p->feed_is_on;
      
      // 更新送料duty
      if (triac_cur.feed_duty != 0) {
        // 500×triac_cur.feed_duty÷100
        on_count = 5 * triac_cur.feed_duty;  
        off_count = 500 - on_count;  // 10s = 10000ms 10000 / 20 = 500
      }
       
      // 返还内存
      osStatus status = osPoolFree(pool_TriacHandle, evt.value.p);
      if (status != osOK)
        printf("Free Triac memory error:%x\r\n", status);
    }
  }
  
/*----------------------------------------------------------------------------*/
  
  
  // 设置period
  if (triac_cur.fan_smoke_power == 0) { 
    triac_cur.fan_smoke_power = 60;
  }
  
  // 最低延时2.5ms，最高延时6ms
  uint32_t delay = t7 + triac_cur.fan_smoke_power;  
  if (delay>8999) {
    delay = last_delay;    
  }
  else {
    last_delay = delay;
  }
  
  
    // 计数器复位
  __HAL_TIM_SET_COUNTER (&htim2, 0);
  __HAL_TIM_SET_COUNTER (&htim4, 0);
  
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, delay); 
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, t7+T1/4);
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, t7+T1/4);
  
  
  /* 输出: 条件判断  */  

  //排烟通道
  if (triac_cur.fan_smoke_is_on) {
     __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC1);
  }
    
  //循环通道
  if (triac_cur.fan_exchange_is_on) {
    __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC2);
  }
    
  //送料通道
  if ((triac_cur.feed_by_manual || triac_cur.feed_full) && (!triac_cur.feed_is_on)) {
    __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC3);
  }
  //送料通道 ON计数 OFF计数
  if (triac_cur.feed_is_on && (!triac_cur.feed_by_manual)) {  // 0
    // 送料
    if (feed_run) {
      __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC3);      
      count_feed ++;
      if (count_feed >= on_count) {        
        count_feed = 0;
        feed_run = false;
      }
      
    } 
    else {
      // 不送料
      count_feed ++;
      if (count_feed >= off_count) {
        count_feed = 0;
        feed_run = true;        
      }
    }  
  } // if 0
  
}


/* TIM4 打开触发脉冲 */
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
  uint32_t compare = 0;
  /* TIM4 */
  if (htim->Instance == TIM4) {
    // 1
    // 排烟风机 通道
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
      // 排烟风机 delay_on， 打开输出
      if (!is_lower_blow) {
        __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC1);
      }
      
      ON_SMOKE;

      // 延时关断
      compare = __HAL_TIM_GET_COUNTER(&htim2) + uS_DELAY_OFF;
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, compare);
      __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);
      
      // 下一个半波
      if (is_lower_blow) {
        is_lower_blow = false;
        
        compare = __HAL_TIM_GET_COUNTER(&htim4) + HALF_T1;        
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, compare);
        //__HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC1);
      }      
    }
    
    // 2
    // 循环风机通道
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {      
      // 循环风机 delay_on， 打开输出
      if (!is_lower_exchange){
        __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC2); 
      }
      
      ON_EXCHANGE;
      
      // 延时关断
      compare = __HAL_TIM_GET_COUNTER(&htim2) + uS_DELAY_OFF;
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, compare);      
      __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC2); 
      
      // 下一个半波
      if (is_lower_exchange) {
        is_lower_exchange = false;
        
        compare =  __HAL_TIM_GET_COUNTER(&htim4) + HALF_T1;        
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, compare);
        //__HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC2); 
      }          
    }
    
    // 3
    // 送料电机 通道
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
      // 送料
      if (!is_lower_feed) {
        __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC3);
      }
      
      ON_FEED; 

      // 延时关断
      compare = __HAL_TIM_GET_COUNTER(&htim2) + uS_DELAY_OFF;
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, compare);
      __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC3); 
      
      // 下一个半波
      if (is_lower_feed){
        is_lower_feed = false;
        
        compare =  __HAL_TIM_GET_COUNTER(&htim4) + HALF_T1;        
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, compare);
        //__HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC3);
      }
    }
    //  
  } 
  
  /* TIM2 关闭触发脉冲 */
  // 延时x us关闭触发
  if (htim->Instance == TIM2) {
    // 1
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
      OFF_SMOKE;
      __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
    }
    
    // 2
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
      // 输出 循环：Off
      OFF_EXCHANGE;
      __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC2);        
    }
    
    // 3
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
      // 输出 送料： Off
      OFF_FEED;
      __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC3);
    }
    
  }  // TIM2
  //
  
}


/* 定时器错误回调 */
void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM4)
    osMessagePut(Q_ErrorHandle, 13, 0);
  
}