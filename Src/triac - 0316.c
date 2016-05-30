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
#define TOTAL_TIME 9999
#define TIM_MST TIM3

static __IO uint16_t counter = 0;

static Triac triac_cur = {false, 999, false, 999, false, 3};  // 风机全部关闭
static uint16_t on_count=500, off_count=500;
static uint16_t on_cnt = 0, off_cnt = 0;

static __IO bool running_feed = true;  //输出中
static __IO bool waiting_feed = false; //停止中

/* t 参数 */
static __IO uint16_t t1 = 0;
static __IO uint16_t t2 = 0;
static __IO uint16_t t3 = 0;
static __IO uint16_t t4 = 0;
static __IO uint16_t t5 = 0;
static __IO uint16_t t6 = 0;
static __IO uint16_t t7 = 0;

static __IO uint16_t T1 = 0;

static __IO uint32_t r = 0;
static __IO uint32_t f = 0;

// 上升沿、下降沿前值
static __IO uint32_t t4_prev = 0;
static __IO uint32_t t6_prev = 0;

static __IO bool is_lower_blow = false;
static __IO bool is_lower_exchange = false;
static __IO bool is_lower_feed = false;

extern osPoolId  pool_TriacHandle;
extern osMessageQId Q_TriacHandle;

extern TIM_HandleTypeDef htim3;


// 输出比较 回调函数
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
  uint16_t delay_new = 0;
  // TIM4
  if (htim->Instance == TIM4) {
    // 1
    // 排烟风机 通道
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
      // 排烟风机 delay_on， 打开输出
      // 输出：On
      HAL_GPIO_WritePin(PORT_SCR, SCR0_smoke, GPIO_PIN_RESET);
      HAL_TIM_OC_Stop_IT( &htim4, TIM_CHANNEL_1);

      // 启动波峰
      if (is_lower_blow) {
        is_lower_blow = false;
        delay_new = __HAL_TIM_GET_COMPARE(&htim4, TIM_CHANNEL_1);
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, delay_new + 10000 - 1);
        HAL_TIM_OC_Start_IT( &htim4, TIM_CHANNEL_1); 
      }
      
      //设置停止时间
      delay_new = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_1);
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, delay_new + uS_DELAY_OFF);
      
      // 启动定时器
      HAL_TIM_OC_Start_IT( &htim1, TIM_CHANNEL_1); 
    }
    
    // 2
    // 循环风机通道
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {      
      // 循环风机 delay_on， 打开输出
      // 输出：On
      HAL_GPIO_WritePin(PORT_SCR, SCR1_exchange, GPIO_PIN_RESET);
      HAL_TIM_OC_Stop_IT( &htim4, TIM_CHANNEL_2);
      
      // 启动波峰
      if (is_lower_exchange) {
        is_lower_exchange = false;
        delay_new = __HAL_TIM_GET_COMPARE(&htim4, TIM_CHANNEL_2);
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, delay_new + 10000 - 1);
        HAL_TIM_OC_Start_IT( &htim4, TIM_CHANNEL_2); 
        
      }
      //设置停止时间
      uint16_t delay_new = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_2);
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, delay_new + uS_DELAY_OFF);
      // 启动定时器
      HAL_TIM_OC_Start_IT( &htim1, TIM_CHANNEL_2);      
    }
    
    // 3
    // 送料电机 通道
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
      HAL_GPIO_WritePin(PORT_SCR, SCR2_feed, GPIO_PIN_RESET);      
      HAL_TIM_OC_Stop_IT( &htim4, TIM_CHANNEL_3);
      
      // 启动波峰
      if (is_lower_feed) {
        is_lower_feed = false;
        uint16_t delay_new = __HAL_TIM_GET_COMPARE(&htim4, TIM_CHANNEL_3);
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, delay_new + 10000 - 1);
        HAL_TIM_OC_Start_IT( &htim4, TIM_CHANNEL_3); 
        
      }
      //设置停止时间
      uint16_t delay_new = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_3);
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, delay_new + uS_DELAY_OFF);
      
    }
    
  }  
  // TIM1  
  // 延时1ms关闭触发
  if (htim->Instance == TIM1) {
    // 1
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
      HAL_GPIO_WritePin(PORT_SCR, SCR0_smoke, GPIO_PIN_SET);
      HAL_TIM_OC_Stop_IT( &htim1, TIM_CHANNEL_1);
    }
    
    // 2
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
      // 输出 循环：Off
      HAL_GPIO_WritePin(PORT_SCR, SCR1_exchange, GPIO_PIN_SET);
      HAL_TIM_OC_Stop_IT( &htim1, TIM_CHANNEL_2);
    }
    
    // 3
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
      // 输出 送料： Off
      HAL_GPIO_WritePin(PORT_SCR, SCR2_feed, GPIO_PIN_SET);
      HAL_TIM_OC_Stop_IT( &htim1, TIM_CHANNEL_3);
    }
    
  }  // TIM1
 //
  
}


/* 过零点触发中断 回调函数 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  
  if (GPIO_Pin != ZERO) 
    return;
  
  bool wave_is_rise;
  // 读Pin，判断是上升沿还是下降沿    
  if (HAL_GPIO_ReadPin(PORT_ZERO, ZERO) == GPIO_PIN_RESET) {
    wave_is_rise = true;
  }
  else {
    wave_is_rise = false;
  }
  
  
  // t 顺序： 1 3 4 6
  
  // 上升沿
  if (wave_is_rise) {
    t4 = __HAL_TIM_GET_COUNTER(&htim3);
    // 上升沿是否valid?
    //      if (!it_is_valid(t4_prev, t4)) {
    //          t4_prev = t4;
    //          r++;
    //          return;
    //      }
    //r++;
  }
  else {
    // 下降沿
    t6 = __HAL_TIM_GET_COUNTER(&htim3);
    // 下降沿是否valid?
    //      if (!it_is_valid(t6_prev, t6)) {
    //          t6_prev = t6;
    //          f++;
    //          return;
    //      }
    //f++;
  }
  
  if (wave_is_rise)  // 只在波谷计算，否则退出
    return;
  
  is_lower_blow = true;  // 设置波谷flag
  is_lower_exchange = true;
  is_lower_feed = true;
  
  /* 下降沿时计算下一个零点 */ 
  // t2
  if (t3 < t1)
    t2 = (0xffff + t1 + t3) / 2;
  else
    t2 = (t1 + t3) / 2;
  
  // t5
  if (t6 < t4)
    t5 = (0xffff + t4 + t6) / 2;
  else
    t5 = (t4 + t6) / 2;
  
  // 交流电源周期
  if (t5 < t2)
    T1 = 0xffff + t5 - t2;
  else
    T1 = t5 - t2;
  // 下一个零点
  if (t5 < t4)
    t7 = t5 + 0xffff - t4 + (T1 / 4);  /*  计算不对 ！ */
  else
    t7 = t5 - t4 + (T1 / 4);
  
//  counter++;
//  if (counter >=299) {
//    //printf("t4:%d,t5:%d,t6:%d,t7:%d,T1:%d\r\n", t4,t5,t6,t7,T1);
//    printf("delay:%d\r\n", t7 + TOTAL_TIME - triac_cur.fan_smoke_delay * 100);
//    counter = 0;
//  }      
  
  // 更新数值
  t1 = t4;
  t2 = t5;
  t3 = t6;
  
  // 计数器复位
  __HAL_TIM_SET_COUNTER (&htim1, 0);
  __HAL_TIM_SET_COUNTER (&htim4, 0);
  
  // 检查Q_Triac队列，更新输出的控制状态
  if (Q_TriacHandle != NULL) {
    
    // 读取值
    osEvent evt = osMessageGet(Q_TriacHandle, 0);
    if (evt.status == osEventMessage) {        
      triac_cur = *((Triac *)evt.value.p);   
      
      // 更新送料duty
      on_count = triac_cur.feed_duty * 100;
      off_count = 1000 - on_count;  // 10s = 10000ms 10000 / 10 = 1000
      
      // 返还内存
      osStatus status = osPoolFree(pool_TriacHandle, evt.value.p);
      if (status != osOK)
        printf("Free Triac memory error:%x\r\n", status);
    }
  }
  
  // 设置period
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, t7 + TOTAL_TIME - triac_cur.fan_smoke_delay * 100);  // 80÷100×10×1000 ＝ 80×100
  
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, t7 + TOTAL_TIME - triac_cur.fan_exchange_delay * 100);
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, t7);
  
  /* 输出: 条件判断  */
  
  HAL_StatusTypeDef status;
  
  //排烟通道
  if (triac_cur.fan_smoke_is_on) {
    status = HAL_TIM_OC_Start_IT(&htim4, TIM_CHANNEL_1);
    if (status != HAL_OK)
      printf("time4 c1 failed at zero:%x\r\n", status);
  }
  
  //循环通道
  if (triac_cur.fan_exchange_is_on) {
    status = HAL_TIM_OC_Start_IT(&htim4, TIM_CHANNEL_2);
    if (status != HAL_OK)
      printf("time4 c2 failed:%x\r\n", status);
  }
  
  //送料通道 ON计数 OFF计数
  if (triac_cur.feed_is_on) {
    // 送料
    if (running_feed) {
      if (on_cnt < on_count) {
        // 输出 送料： On        
        HAL_TIM_OC_Start_IT(&htim4, TIM_CHANNEL_3);
        on_cnt++;
      }
      else {
        on_cnt = 0;
        off_cnt = 0;
        running_feed = false;
        waiting_feed = true;
      }
      
    }
    // 不送料
    if (waiting_feed) {
      off_cnt ++;
      if (off_cnt >= off_count) {
        on_cnt = 0;
        off_cnt = 0;
        running_feed = true;
        waiting_feed = false;
        
      }
      
    }
  }
// 
}


/* 定时器错误回调 */
void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM4)
    printf("timer4 occour error.\r\n");
  
}