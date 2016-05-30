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

#define uS_DELAY_OFF 999
#define TOTAL_TIME 9999
#define TIM_MST TIM3

static __IO uint16_t counter = 0;

static Triac triac_cur = {false, 999, false, 999, false, 3};  // ���ȫ���ر�
static uint16_t on_count=500, off_count=500;
static uint16_t on_cnt = 0, off_cnt = 0;

static __IO bool running_feed = true;  //�����
static __IO bool waiting_feed = false; //ֹͣ��

/* t ���� */
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

// �����ء��½���ǰֵ
static __IO uint32_t t4_prev = 0;
static __IO uint32_t t6_prev = 0;

static __IO bool is_lower_blow = false;
static __IO bool is_lower_exchange = false;
static __IO bool is_lower_feed = false;

extern osPoolId  pool_TriacHandle;
extern osMessageQId Q_TriacHandle;

extern TIM_HandleTypeDef htim3;


// ����Ƚ� �ص�����
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
  uint16_t delay_new = 0;
  // TIM4
  if (htim->Instance == TIM4) {
    // 1
    // ���̷�� ͨ��
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
      // ���̷�� delay_on�� �����
      // �����On
      HAL_GPIO_WritePin(PORT_SCR, SCR0_smoke, GPIO_PIN_RESET);
      HAL_TIM_OC_Stop_IT( &htim4, TIM_CHANNEL_1);

      // ��������
      if (is_lower_blow) {
        is_lower_blow = false;
        delay_new = __HAL_TIM_GET_COMPARE(&htim4, TIM_CHANNEL_1);
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, delay_new + 10000 - 1);
        HAL_TIM_OC_Start_IT( &htim4, TIM_CHANNEL_1); 
      }
      
      //����ֹͣʱ��
      delay_new = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_1);
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, delay_new + uS_DELAY_OFF);
      
      // ������ʱ��
      HAL_TIM_OC_Start_IT( &htim1, TIM_CHANNEL_1); 
    }
    
    // 2
    // ѭ�����ͨ��
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {      
      // ѭ����� delay_on�� �����
      // �����On
      HAL_GPIO_WritePin(PORT_SCR, SCR1_exchange, GPIO_PIN_RESET);
      HAL_TIM_OC_Stop_IT( &htim4, TIM_CHANNEL_2);
      
      // ��������
      if (is_lower_exchange) {
        is_lower_exchange = false;
        delay_new = __HAL_TIM_GET_COMPARE(&htim4, TIM_CHANNEL_2);
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, delay_new + 10000 - 1);
        HAL_TIM_OC_Start_IT( &htim4, TIM_CHANNEL_2); 
        
      }
      //����ֹͣʱ��
      uint16_t delay_new = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_2);
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, delay_new + uS_DELAY_OFF);
      // ������ʱ��
      HAL_TIM_OC_Start_IT( &htim1, TIM_CHANNEL_2);      
    }
    
    // 3
    // ���ϵ�� ͨ��
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
      HAL_GPIO_WritePin(PORT_SCR, SCR2_feed, GPIO_PIN_RESET);      
      HAL_TIM_OC_Stop_IT( &htim4, TIM_CHANNEL_3);
      
      // ��������
      if (is_lower_feed) {
        is_lower_feed = false;
        uint16_t delay_new = __HAL_TIM_GET_COMPARE(&htim4, TIM_CHANNEL_3);
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, delay_new + 10000 - 1);
        HAL_TIM_OC_Start_IT( &htim4, TIM_CHANNEL_3); 
        
      }
      //����ֹͣʱ��
      uint16_t delay_new = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_3);
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, delay_new + uS_DELAY_OFF);
      
    }
    
  }  
  // TIM1  
  // ��ʱ1ms�رմ���
  if (htim->Instance == TIM1) {
    // 1
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
      HAL_GPIO_WritePin(PORT_SCR, SCR0_smoke, GPIO_PIN_SET);
      HAL_TIM_OC_Stop_IT( &htim1, TIM_CHANNEL_1);
    }
    
    // 2
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
      // ��� ѭ����Off
      HAL_GPIO_WritePin(PORT_SCR, SCR1_exchange, GPIO_PIN_SET);
      HAL_TIM_OC_Stop_IT( &htim1, TIM_CHANNEL_2);
    }
    
    // 3
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
      // ��� ���ϣ� Off
      HAL_GPIO_WritePin(PORT_SCR, SCR2_feed, GPIO_PIN_SET);
      HAL_TIM_OC_Stop_IT( &htim1, TIM_CHANNEL_3);
    }
    
  }  // TIM1
 //
  
}


/* ����㴥���ж� �ص����� */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  
  if (GPIO_Pin != ZERO) 
    return;
  
  bool wave_is_rise;
  // ��Pin���ж��������ػ����½���    
  if (HAL_GPIO_ReadPin(PORT_ZERO, ZERO) == GPIO_PIN_RESET) {
    wave_is_rise = true;
  }
  else {
    wave_is_rise = false;
  }
  
  
  // t ˳�� 1 3 4 6
  
  // ������
  if (wave_is_rise) {
    t4 = __HAL_TIM_GET_COUNTER(&htim3);
    // �������Ƿ�valid?
    //      if (!it_is_valid(t4_prev, t4)) {
    //          t4_prev = t4;
    //          r++;
    //          return;
    //      }
    //r++;
  }
  else {
    // �½���
    t6 = __HAL_TIM_GET_COUNTER(&htim3);
    // �½����Ƿ�valid?
    //      if (!it_is_valid(t6_prev, t6)) {
    //          t6_prev = t6;
    //          f++;
    //          return;
    //      }
    //f++;
  }
  
  if (wave_is_rise)  // ֻ�ڲ��ȼ��㣬�����˳�
    return;
  
  is_lower_blow = true;  // ���ò���flag
  is_lower_exchange = true;
  is_lower_feed = true;
  
  /* �½���ʱ������һ����� */ 
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
  
  // ������Դ����
  if (t5 < t2)
    T1 = 0xffff + t5 - t2;
  else
    T1 = t5 - t2;
  // ��һ�����
  if (t5 < t4)
    t7 = t5 + 0xffff - t4 + (T1 / 4);  /*  ���㲻�� �� */
  else
    t7 = t5 - t4 + (T1 / 4);
  
//  counter++;
//  if (counter >=299) {
//    //printf("t4:%d,t5:%d,t6:%d,t7:%d,T1:%d\r\n", t4,t5,t6,t7,T1);
//    printf("delay:%d\r\n", t7 + TOTAL_TIME - triac_cur.fan_smoke_delay * 100);
//    counter = 0;
//  }      
  
  // ������ֵ
  t1 = t4;
  t2 = t5;
  t3 = t6;
  
  // ��������λ
  __HAL_TIM_SET_COUNTER (&htim1, 0);
  __HAL_TIM_SET_COUNTER (&htim4, 0);
  
  // ���Q_Triac���У���������Ŀ���״̬
  if (Q_TriacHandle != NULL) {
    
    // ��ȡֵ
    osEvent evt = osMessageGet(Q_TriacHandle, 0);
    if (evt.status == osEventMessage) {        
      triac_cur = *((Triac *)evt.value.p);   
      
      // ��������duty
      on_count = triac_cur.feed_duty * 100;
      off_count = 1000 - on_count;  // 10s = 10000ms 10000 / 10 = 1000
      
      // �����ڴ�
      osStatus status = osPoolFree(pool_TriacHandle, evt.value.p);
      if (status != osOK)
        printf("Free Triac memory error:%x\r\n", status);
    }
  }
  
  // ����period
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, t7 + TOTAL_TIME - triac_cur.fan_smoke_delay * 100);  // 80��100��10��1000 �� 80��100
  
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, t7 + TOTAL_TIME - triac_cur.fan_exchange_delay * 100);
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, t7);
  
  /* ���: �����ж�  */
  
  HAL_StatusTypeDef status;
  
  //����ͨ��
  if (triac_cur.fan_smoke_is_on) {
    status = HAL_TIM_OC_Start_IT(&htim4, TIM_CHANNEL_1);
    if (status != HAL_OK)
      printf("time4 c1 failed at zero:%x\r\n", status);
  }
  
  //ѭ��ͨ��
  if (triac_cur.fan_exchange_is_on) {
    status = HAL_TIM_OC_Start_IT(&htim4, TIM_CHANNEL_2);
    if (status != HAL_OK)
      printf("time4 c2 failed:%x\r\n", status);
  }
  
  //����ͨ�� ON���� OFF����
  if (triac_cur.feed_is_on) {
    // ����
    if (running_feed) {
      if (on_cnt < on_count) {
        // ��� ���ϣ� On        
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
    // ������
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


/* ��ʱ������ص� */
void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM4)
    printf("timer4 occour error.\r\n");
  
}