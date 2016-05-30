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

#define uS_DELAY_OFF 100  // 10us
#define HALF_T1 1000  // 10us

// ±200us内
#define T1_MIN 19000
#define T1_MAX 21000

// 排烟
#define ON_SMOKE PORT_SCR->BSRR = (uint32_t)SCR0_smoke << 16
#define OFF_SMOKE PORT_SCR->BSRR = SCR0_smoke

// 循环
#define ON_EXCHANGE PORT_SCR->BSRR = (uint32_t)SCR1_exchange << 16
#define OFF_EXCHANGE PORT_SCR->BSRR = SCR1_exchange
// 送料
#define ON_FEED PORT_SCR->BSRR = (uint32_t)SCR2_feed << 16
#define OFF_FEED PORT_SCR->BSRR = SCR2_feed

/* 变量定义 */

static __IO  Triac triac_cur = {false, 2, false, 2, false, false, false, 3};  // 风机全部关闭
static __IO  uint16_t on_count = 250;
static __IO  uint16_t off_count = 250;
static __IO  uint16_t count_feed = 0;

static __IO bool feed_run = true;  //输出中

static __IO bool is_lower_blow = false;
static __IO bool is_lower_exchange = false;
static __IO bool is_lower_feed = false;

static __IO uint16_t i = 0;
static __IO bool debug_busy = false;

// 延时表 10us
const uint16_t table_delay[6] = {115, 110, 105,100, 95, 90 }; 

/* 外部变量引用 */
extern osPoolId  pool_TriacHandle;
extern osMessageQId Q_TriacHandle;
extern osPoolId pool_ZeroHandle;
extern osMessageQId Q_ErrorHandle;
extern osMessageQId Q_BeepHandle;

// debug
#ifdef DEBUG
void debug(uint32_t t1, uint32_t t2, uint8_t cr, uint8_t i, uint32_t v)
{
  Zero *z;
  z = (Zero*)osPoolAlloc(pool_ZeroHandle);
  if (z!=NULL) {
    z->t1 = t1;
    z->t3 = t2;
    z->cr = cr;
    z->i = i;
    z->T1 = v;
    osMessagePut(Q_ErrorHandle, (uint32_t)z, 0);
  }  
  
}
#endif

// 确定一段无干扰的零点
static __IO bool point_start = false;
static __IO uint8_t zero_right_cnt = 0;

static __IO uint32_t t_prev = 0;
static __IO uint32_t t_cur = 0;
static __IO uint32_t T_1 = 0;

/* 过零点触发中断 回调函数 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{  
  // 记录时间
  t_cur = __HAL_TIM_GET_COUNTER(&htim3);
  if (t_cur > t_prev) {
    T_1 = t_cur - t_prev;
  }
  else {
    T_1 = t_cur + 0xffff - t_prev;
  }
  
  //
  if (!point_start) {    
    if (T_1>=T1_MIN && T_1<=T1_MAX) {
      zero_right_cnt++;      
      
      if(zero_right_cnt >= 9) {
        point_start = true;
#ifdef DEBUG
        printf("ok\r\n");
#endif
      }
    }
    else {
      zero_right_cnt = 0;
    }
    
    // 更新数值
    t_prev = t_cur;
    
    return;
  }
  
#ifdef DEBUG
  if (!debug_busy) {
    if (T_1<=T1_MIN || T_1>=T1_MAX) {
      debug_busy = true; 
      i=0;
    }
  }
  
  //    if (T_1<=T1_MIN || T_1>=T1_MAX) {
  //      debug(t_prev, t_cur,T_1);
  //      //osMessagePut(Q_BeepHandle, BEEP_BUTTON, 0);
  //      //return;
  //    }  
  
  if (debug_busy) {
    if (i==2) {
      debug(t_prev, t_cur,1, i, T_1);
    }
    else {
      debug(t_prev, t_cur,0, i, T_1);
    }
    
    i++;
    
    if (i>4) {
      debug_busy = false;
    }
  }
  
#endif
  
  if (T_1<=T1_MIN || T_1>=T1_MAX) {
    return;
  }
  
  // 更新数值
  t_prev = t_cur;
    
  /*--------------------------------------------------------------------------*/
  
  // 在下半波时处理

  is_lower_blow = true;  // 设置波谷flag
  is_lower_exchange = true;
  is_lower_feed = true;
  
  //uint32_t compare = 0;
  
  // 检查Q_Triac队列，更新输出的控制状态
  
  // 读取值
  osEvent evt = osMessageGet(Q_TriacHandle, 0);
  if (evt.status == osEventMessage) {  
    Triac *p = (Triac *)evt.value.p;     
    triac_cur = *p;
    
    // 更新送料duty
    if (triac_cur.feed_duty != 0) {        
      on_count = 5 * triac_cur.feed_duty;  // 500 * triac_cur.feed_duty / 100
      off_count = 500 - on_count;  // 10s = 10000ms 10000 / 20 = 500
    }
    
    // 返还内存
    osStatus status = osPoolFree(pool_TriacHandle, p);
    if (status != osOK)
      printf("Free Triac memory error:%x\r\n", status);
  }
  
/*----------------------------------------------------------------------------*/
    
  // 设置period
  if (triac_cur.fan_smoke_power == 0) { 
    triac_cur.fan_smoke_power = 2;
  }  
  
  // 最低延时2.5ms，最高延时6ms
  uint16_t delay = (uint16_t)triac_cur.fan_smoke_power;  
  if (triac_cur.fan_smoke_power<=10) { // *50
    delay = (uint16_t)(5*table_delay[triac_cur.fan_smoke_power]);
  }
  if (delay>800 || delay<10) {
    delay = 450;    
  }
  
  // 计数器复位
  __HAL_TIM_SET_COUNTER (&htim2, 0);
  __HAL_TIM_SET_COUNTER (&htim4, 0);
  
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, delay); 
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, 450);
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 450);
  
  
  /* 输出: 条件判断  */  

  /* - 排烟通道 */
  if (triac_cur.fan_smoke_is_on) {
     __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC1);
     //HAL_TIM_OC_Start_IT(&htim4, TIM_CHANNEL_1);
  }
  else {
    OFF_SMOKE;
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
    __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC1);
    
  }  
  
  /* - 循环通道 */
  if (triac_cur.fan_exchange_is_on) {
    __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC2);
  }
  else {
    OFF_EXCHANGE;
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC2);
    __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC2);
    
  }
    
  /* - 送料通道 */
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
  // 关闭 
  if ((!triac_cur.feed_is_on) && (!triac_cur.feed_by_manual) &&(!triac_cur.feed_full)) { 
    OFF_FEED;
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC3);
    __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC3);   
  }
  //end if
}

/*----------------------------------------------------------------------------*/

/* TIM4 打开触发脉冲 */
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
  uint16_t compare = 0;
  /* TIM4 */
  if (htim->Instance == TIM4) {
    // 1
    // 排烟风机 通道
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
      // 排烟风机 delay_on， 打开输出
      if (!is_lower_blow) {
        __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC1);
      }
      // 接通
      ON_SMOKE;

      // 延时关断
      compare = (uint16_t)(__HAL_TIM_GET_COUNTER(&htim2) + uS_DELAY_OFF);
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, compare);
      __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);
      
      // 下一个半波
      if (is_lower_blow) {
        is_lower_blow = false;
        
        compare = (uint16_t)(__HAL_TIM_GET_COUNTER(&htim4) + HALF_T1);        
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, compare);
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
      compare = (uint16_t)(__HAL_TIM_GET_COUNTER(&htim2) + uS_DELAY_OFF);
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, compare);      
      __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC2); 
      
      // 下一个半波
      if (is_lower_exchange) {
        is_lower_exchange = false;
        
        compare =  (uint16_t)(__HAL_TIM_GET_COUNTER(&htim4) + HALF_T1);        
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, compare);
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
      compare = (uint16_t)(__HAL_TIM_GET_COUNTER(&htim2) + uS_DELAY_OFF);
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, compare);
      __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC3); 
      
      // 下一个半波
      if (is_lower_feed){
        is_lower_feed = false;
        
        compare =  (uint16_t)(__HAL_TIM_GET_COUNTER(&htim4) + HALF_T1);        
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, compare);
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

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  
}