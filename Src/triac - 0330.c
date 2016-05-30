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
#define TOTAL_T1 T1
#define HALF_T1 T1/2


static __IO uint16_t counter = 0;

static __IO  Triac triac_cur = {false, 90, false, 90, false, 3};  // ���ȫ���ر�
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
static __IO uint32_t t7 = 0;
static __IO uint32_t T1 = 0;

extern osPoolId  pool_TriacHandle;
extern osMessageQId Q_TriacHandle;


/* ����㴥���ж� �ص����� */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  
  if (GPIO_Pin != ZERO) 
    return;
  
  bool rising_edge;
  // ��Pin���ж��������ػ����½���    
  if (HAL_GPIO_ReadPin(PORT_ZERO, ZERO) == GPIO_PIN_SET) {
    rising_edge = true;
  }
  else {
    rising_edge = false;
  }
  
  // t ˳�� 1 3 4 6
  
  // ������
  if (rising_edge) {
    t6 = __HAL_TIM_GET_COUNTER(&htim3);
  }
  else {
    // �½���
    t4 = __HAL_TIM_GET_COUNTER(&htim3);
  }
  
  if (!rising_edge)  // ֻ�ڲ��ȼ��㣬�����˳�
    return;
  
  is_lower_blow = true;  // ���ò���flag
  is_lower_exchange = true;
  is_lower_feed = true;
  
  /* �½���ʱ������һ����� */ 
  // t2
  if (t1<t3 & t3<t4 & t4<t6) {    
    
    t2 = (t1+t3) / 2;
    
    // t5    
    t5 = (t4+t6) / 2;
    
    // ������Դ����    
    T1 = t5 - t2;
    
    // ��һ�����    
    t7 = t5 + T1/4 - t6;
  }  
 
  
  // ������ֵ
  t1 = t4;
  t2 = t5;
  t3 = t6;
    
/*----------------------------------------------------------------------------*/
  
  // ֻ�����°벨
  if (!rising_edge) {
    return;
  }
  
  if (t7 == 0) 
    return;
  
  is_lower_blow = true;  // ���ò���flag
  is_lower_exchange = true;
  is_lower_feed = true;
  
  //uint32_t compare = 0;
  
  // ���Q_Triac���У���������Ŀ���״̬
  if (Q_TriacHandle != NULL) {    
    // ��ȡֵ
    osEvent evt = osMessageGet(Q_TriacHandle, 0);
    if (evt.status == osEventMessage) {        
      triac_cur = *((Triac *)evt.value.p);   
      
      // ��������duty
      if (triac_cur.feed_duty != 0) {
        on_count = triac_cur.feed_duty * 100;
        off_count = 1000 - on_count;  // 10s = 10000ms 10000 / 10 = 1000
      }
      
      // �����ڴ�
      osStatus status = osPoolFree(pool_TriacHandle, evt.value.p);
      if (status != osOK)
        printf("Free Triac memory error:%x\r\n", status);
    }
  }
  
/*----------------------------------------------------------------------------*/
  
  // debug
  counter++;
  if (counter >=299) {
    //printf("t1:%d,t3:%d,t4:%d,t6:%d,T1:%d\r\n", t1,t3,t4,t6,T1);
    printf("t7:%d,smoke:%d,delay:%d\r\n", t7, triac_cur.fan_smoke_delay, t7 + HALF_T1 - triac_cur.fan_smoke_delay * 100);
    counter = 0;
  } 
  
  
  // ��������λ
  __HAL_TIM_SET_COUNTER (&htim2, 0);
  __HAL_TIM_SET_COUNTER (&htim4, 0);
  
  // ����period
  if (triac_cur.fan_smoke_delay == 0) { 
    triac_cur.fan_smoke_delay = 60;
    printf("p is zero \r\n");
  }
  
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, t7 + HALF_T1 - triac_cur.fan_smoke_delay * 100);  // 80��100��10��1000 �� 80��100  
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, t7);
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
  if (triac_cur.feed_is_on) {  // 0
    // ����
    if (feed_run) {
      HAL_TIM_OC_Start_IT( &htim4, TIM_CHANNEL_3);      
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
  

}


/* ����Ƚ� �ص����� */
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
  uint32_t compare = 0;
  /* TIM4 */
  if (htim->Instance == TIM4) {
    // 1
    // ���̷�� ͨ��
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
      // ���̷�� delay_on�� �����
      HAL_TIM_OC_Stop_IT( &htim4, TIM_CHANNEL_1);
      //HAL_GPIO_WritePin(PORT_SCR, SCR0_smoke, GPIO_PIN_RESET);
      GPIOE->BSRR = (uint32_t)GPIO_PIN_7 << 16;
      // printf("0 ccr:%d,cnt:%d\r\n", __HAL_TIM_GET_COMPARE(&htim4, TIM_CHANNEL_1), __HAL_TIM_GET_COUNTER(&htim4));
      // ��ʱ�ض�
      compare = __HAL_TIM_GET_COUNTER(&htim2) + uS_DELAY_OFF;
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, compare);
      HAL_TIM_OC_Start_IT( &htim2, TIM_CHANNEL_1);
      
      // ��һ���벨
      if (is_lower_blow) {
        is_lower_blow = false;
        compare = __HAL_TIM_GET_COUNTER(&htim4) + HALF_T1;
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, compare);
        HAL_TIM_OC_Start_IT( &htim4, TIM_CHANNEL_1); 
      }      
    }
    
    // 2
    // ѭ�����ͨ��
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {      
      // ѭ����� delay_on�� �����
      HAL_TIM_OC_Stop_IT( &htim4, TIM_CHANNEL_2);
      HAL_GPIO_WritePin(PORT_SCR, SCR1_exchange, GPIO_PIN_RESET);
      
      // ��ʱ�ض�
      compare = __HAL_TIM_GET_COUNTER(&htim2) + uS_DELAY_OFF;
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, compare);      
      HAL_TIM_OC_Start_IT( &htim2, TIM_CHANNEL_2);
      
      // ��һ���벨
      if (is_lower_exchange) {
        is_lower_exchange = false;
        compare =  __HAL_TIM_GET_COUNTER(&htim4) + HALF_T1;
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, compare);
        HAL_TIM_OC_Start_IT( &htim4, TIM_CHANNEL_2);         
      }          
    }
    
    // 3
    // ���ϵ�� ͨ��
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
      // ����
      HAL_TIM_OC_Stop_IT( &htim4, TIM_CHANNEL_3);
      HAL_GPIO_WritePin(PORT_SCR, SCR2_feed, GPIO_PIN_RESET); 
      // printf("2 ccr:%d,cnt:%d\r\n", __HAL_TIM_GET_COMPARE(&htim4, TIM_CHANNEL_3), __HAL_TIM_GET_COUNTER(&htim4));
      // ��ʱ�ض�
      compare = __HAL_TIM_GET_COUNTER(&htim2) + uS_DELAY_OFF;
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, compare);
      HAL_TIM_OC_Start_IT( &htim2, TIM_CHANNEL_3); 
      
      if (is_lower_feed){
        is_lower_feed = false;
        compare =  __HAL_TIM_GET_COUNTER(&htim4) + HALF_T1;
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, compare);
        HAL_TIM_OC_Start_IT( &htim4, TIM_CHANNEL_3); 
      }
    }
    //  
  } 
  
  /* TIM2 */
  // ��ʱx ms�رմ���
  if (htim->Instance == TIM2) {
    // 1
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
      //HAL_GPIO_WritePin(PORT_SCR, SCR0_smoke, GPIO_PIN_SET);
      GPIOE->BSRR = GPIO_PIN_7;
      HAL_TIM_OC_Stop_IT( &htim2, TIM_CHANNEL_1);
      //printf("cnt:%d,compare:%d\r\n", __HAL_TIM_GET_COUNTER(&htim2), __HAL_TIM_GET_COMPARE(&htim2,TIM_CHANNEL_1) );
    }
    
    // 2
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
      // ��� ѭ����Off
      HAL_GPIO_WritePin(PORT_SCR, SCR1_exchange, GPIO_PIN_SET);
      HAL_TIM_OC_Stop_IT( &htim2, TIM_CHANNEL_2);         
    }
    
    // 3
    if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
      // ��� ���ϣ� Off
      HAL_GPIO_WritePin(PORT_SCR, SCR2_feed, GPIO_PIN_SET);
      HAL_TIM_OC_Stop_IT( &htim2, TIM_CHANNEL_3);
    }
    
  }  // TIM2
  //
  
}


/* ��ʱ������ص� */
void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM4)
    printf("timer4 occour error.\r\n");
  
}