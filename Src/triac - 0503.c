/*
* 2016.01.23
* triac.c
* ��բ������
*/

#include "stm32f1xx_hal.h"
#include "tim.h"
#include "pinname.h"
#include "cmsis_os.h"
#include "user.h"

#define uS_DELAY_OFF 100  // 10us
#define HALF_T1 1000  // 10us

// ��200us��
#define T1_MIN 19900
#define T1_MAX 20100

// ����
#define ON_SMOKE PORT_SCR->BSRR = (uint32_t)SCR0_smoke << 16
#define OFF_SMOKE PORT_SCR->BSRR = SCR0_smoke

// ѭ��
#define ON_EXCHANGE PORT_SCR->BSRR = (uint32_t)SCR1_exchange << 16
#define OFF_EXCHANGE PORT_SCR->BSRR = SCR1_exchange
// ����
#define ON_FEED PORT_SCR->BSRR = (uint32_t)SCR2_feed << 16
#define OFF_FEED PORT_SCR->BSRR = SCR2_feed

static __IO uint16_t counter = 0;

static __IO  Triac triac_cur = {false, 2, false, 2, false, false, false, 3};  // ���ȫ���ر�
static __IO  uint16_t on_count = 250;
static __IO  uint16_t off_count = 250;
static __IO  uint16_t count_feed = 0;

static __IO bool feed_run = true;  //�����

static __IO bool is_lower_blow = false;
static __IO bool is_lower_exchange = false;
static __IO bool is_lower_feed = false;

/* t ���� */
static __IO uint32_t t1 = 0;
static __IO uint32_t t2 = 0;
static __IO uint32_t t3 = 0;
static __IO uint32_t t4 = 0;
static __IO uint32_t t5 = 0;
static __IO uint32_t t6 = 0;
static __IO int32_t t7 = 0;
static __IO uint32_t T1 = 0;

// ��ʱ�� 10us
const uint16_t table_delay[6] = {115, 110, 105,100, 95, 90 }; 

extern osPoolId  pool_TriacHandle;
extern osMessageQId Q_TriacHandle;
extern osPoolId pool_ZeroHandle;
extern osMessageQId Q_ErrorHandle;
extern osMessageQId Q_BeepHandle;

// debug
void debug(uint32_t v)
{
  Zero *z;
  z = (Zero*)osPoolAlloc(pool_ZeroHandle);
  if (z!=NULL) {
    z->T1 = v;
    osMessagePut(Q_ErrorHandle, (uint32_t)z, 0);
  }  
  
}


// ȷ��һ���޸��ŵ����
static __IO bool point_start = false;
static __IO uint8_t zero_right_cnt = 0;


/* ����㴥���ж� �ص����� */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  
  if (GPIO_Pin != ZERO) 
    return;
  
  bool rising_edge;
  // ��Pin���ж��������ػ����½���    
  if (HAL_GPIO_ReadPin(PORT_ZERO, ZERO) == GPIO_PIN_SET) {
    rising_edge = true;
    t6 = __HAL_TIM_GET_COUNTER(&htim3);
  }
  else {
    rising_edge = false;
    t4 = __HAL_TIM_GET_COUNTER(&htim3);
  }
  
  // t ˳�� 1 3 4 6
  
 
  if (!rising_edge)  // ֻ�ڲ��ȼ��㣬�����˳�
    return;
  
/*----------------------------------------------------------------------------*/
    
  is_lower_blow = true;  // ���ò���flag
  is_lower_exchange = true;
  is_lower_feed = true;
  
  /* �½���ʱ������һ����� */ 
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
    t7 = t5 + T1/4;
    
    // ��һ�����    
  if (t7>t6) {
    t7 -= t6;
  }
  else {
    t7 = t7 + 0xffff - t6;
  }  
  //
  if (!point_start) {    
    if (T1>=T1_MIN && T1<=T1_MAX) {
      zero_right_cnt++;      
      
      if(zero_right_cnt >= 9) {
        point_start = true;
        printf("ok\r\n");
      }
    }
    else {
      zero_right_cnt = 0;
    }
    
    // ������ֵ
    t1 = t4;
    t3 = t6;
    
    return;
  }
  
  if (point_start) {
    if (T1<=T1_MIN || T1>=T1_MAX) {
      debug(1);
      return;
    }
  }
  //debug
  //counter++;
//  if (T1<19000) {
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
//    //counter = 0;
//  }  
  
  
  // ������ֵ
  t1 = t4;
  t3 = t6;
    
  /*--------------------------------------------------------------------------*/
  
  // ���°벨ʱ����
  t7 /= 10;
  
  if (t7 <= 0) {
    t7 = 20;
  }
  
  is_lower_blow = true;  // ���ò���flag
  is_lower_exchange = true;
  is_lower_feed = true;
  
  //uint32_t compare = 0;
  
  // ���Q_Triac���У���������Ŀ���״̬
  if (Q_TriacHandle != NULL) {    
    // ��ȡֵ
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
      
      //triac_cur = *p;
      // ��������duty
      if (triac_cur.feed_duty != 0) {        
        on_count = 5 * triac_cur.feed_duty;  // 500 * triac_cur.feed_duty / 100
        off_count = 500 - on_count;  // 10s = 10000ms 10000 / 20 = 500
      }
       
      // �����ڴ�
      osStatus status = osPoolFree(pool_TriacHandle, evt.value.p);
      if (status != osOK)
        printf("Free Triac memory error:%x\r\n", status);
    }
  }
  
/*----------------------------------------------------------------------------*/
  
  
  // ����period
  if (triac_cur.fan_smoke_power == 0) { 
    triac_cur.fan_smoke_power = 2;
  }
  
  if (t7 < 0)
    t7 = 10;
  
  // �����ʱ2.5ms�������ʱ6ms
  uint16_t delay = (uint16_t)(t7 + triac_cur.fan_smoke_power);  
  if (triac_cur.fan_smoke_power<=10) { // *50
    delay = (uint16_t)(t7 + 5*table_delay[triac_cur.fan_smoke_power]);
  }
  if (delay>800 || delay<10) {
    delay = 450;    
  }
  
  // ��������λ
  __HAL_TIM_SET_COUNTER (&htim2, 0);
  __HAL_TIM_SET_COUNTER (&htim4, 0);
  
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, delay); 
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, 450);
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 450);
  
  
  /* ���: �����ж�  */  

  //����ͨ��
  if (triac_cur.fan_smoke_is_on) {
     __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC1);
     //HAL_TIM_OC_Start_IT(&htim4, TIM_CHANNEL_1);
  }
  else {
    OFF_SMOKE;
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
    __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC1);
    
  }  
  //ѭ��ͨ��
  if (triac_cur.fan_exchange_is_on) {
    __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC2);
  }
  else {
    OFF_EXCHANGE;
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC2);
    __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC2);
    
  }
    
  //����ͨ��
  if ((triac_cur.feed_by_manual || triac_cur.feed_full) && (!triac_cur.feed_is_on)) {
    __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC3);
  }
  
  //����ͨ�� ON���� OFF����
  if (triac_cur.feed_is_on && (!triac_cur.feed_by_manual)) {  // 0
    // ����
    if (feed_run) {
      __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_CC3);      
      count_feed ++;
      if (count_feed >= on_count) {        
        count_feed = 0;
        feed_run = false;
      }
      
    } 
    else {
      // ������
      count_feed ++;
      if (count_feed >= off_count) {
        count_feed = 0;
        feed_run = true;        
      }
    }  
  } // if 0
  else {
    OFF_FEED;
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC3);
    __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC3);
    
  }
  
}


/* TIM4 �򿪴������� */
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
  uint16_t compare = 0;
  /* TIM4 */
  if (htim->Instance == TIM4) {
    // 1
    // ���̷�� ͨ��
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
      // ���̷�� delay_on�� �����
      if (!is_lower_blow) {
        __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC1);
      }
      // ��ͨ
      ON_SMOKE;

      // ��ʱ�ض�
      compare = (uint16_t)(__HAL_TIM_GET_COUNTER(&htim2) + uS_DELAY_OFF);
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, compare);
      __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);
      
      // ��һ���벨
      if (is_lower_blow) {
        is_lower_blow = false;
        
        compare = (uint16_t)(__HAL_TIM_GET_COUNTER(&htim4) + HALF_T1);        
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, compare);
      }      
    }
    
    // 2
    // ѭ�����ͨ��
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {      
      // ѭ����� delay_on�� �����
      if (!is_lower_exchange){
        __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC2);    
      }
      
      ON_EXCHANGE;
      
      // ��ʱ�ض�
      compare = __HAL_TIM_GET_COUNTER(&htim2) + uS_DELAY_OFF;
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, compare);      
      __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC2); 
      
      // ��һ���벨
      if (is_lower_exchange) {
        is_lower_exchange = false;
        
        compare =  __HAL_TIM_GET_COUNTER(&htim4) + HALF_T1;        
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, compare);
      }          
    }
    
    // 3
    // ���ϵ�� ͨ��
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
      // ����
      if (!is_lower_feed) {
        __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_CC3);
      }
      
      ON_FEED; 

      // ��ʱ�ض�
      compare = __HAL_TIM_GET_COUNTER(&htim2) + uS_DELAY_OFF;
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, compare);
      __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC3); 
      
      // ��һ���벨
      if (is_lower_feed){
        is_lower_feed = false;
        
        compare =  __HAL_TIM_GET_COUNTER(&htim4) + HALF_T1;        
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, compare);
      }
    }
    //  
  } 
  
  /* TIM2 �رմ������� */
  // ��ʱx us�رմ���
  if (htim->Instance == TIM2) {
    // 1
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
      OFF_SMOKE;
      __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
    }
    
    // 2
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
      // ��� ѭ����Off
      OFF_EXCHANGE;
      __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC2);        
    }
    
    // 3
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
      // ��� ���ϣ� Off
      OFF_FEED;
      __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC3);
    }
    
  }  // TIM2
  //
  
}


/* ��ʱ������ص� */
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