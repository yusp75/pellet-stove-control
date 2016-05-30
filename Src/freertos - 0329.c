/**
******************************************************************************
* File Name          : freertos.c
* Description        : Code for freertos applications
******************************************************************************
*
* COPYRIGHT(c) 2016 STMicroelectronics
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation
*      and/or other materials provided with the distribution.
*   3. Neither the name of STMicroelectronics nor the names of its contributors
*      may be used to endorse or promote products derived from this software
*      without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */     
#include "stm32f1xx_hal.h"
#include "stdio.h"

#include "pinname.h"
#include "sd3088.h"

#include "ds18b20.h"
#include "max6675.h"
#include "sh1106.h"
#include "keypad.h"

#include "u8g.h"
#include "m2.h"
#include "u8g_arm.h"

#include "user.h"
#include "string.h"
/* USER CODE END Includes */

/* Variables -----------------------------------------------------------------*/
osThreadId defaultTaskHandle;
osThreadId TaskScanKeyHandle;
osMessageQId Q_KeyHandle;
osTimerId tmr_blowDustHandle;
osTimerId tmr_preFeedHandle;
osTimerId tmr_fireTimeoutHandle;
osTimerId tmr_beforeFeedHandle;
osTimerId tmr_delayRunningHandle;
osTimerId tmr_full_blowHandle;
osTimerId tmr_full_exchangeHandle;
osTimerId tmr_boil_timeoutHandle;
osTimerId tmr_switch_delayHandle;
osMutexId mutex_rtcHandle;

/* USER CODE BEGIN Variables */
#define N_KTEMP 4  // ƽ��ֵ

// �ⲿrtc����
extern RTC_Time Time_sd3088;

// �ⲿ iwdg����
//extern IWDG_HandleTypeDef hiwdg;
// ¯�Ӳ���
// Stove mystove;
/*�ɿع���Ϣ*/
osPoolDef(pool_triac, 1, Triac); // Define Triac �ڴ��
osPoolId  pool_TriacHandle;
osMessageQId Q_TriacHandle;

/*¯����Ϣ*/
osPoolDef(pool_data, 1, Data); // Define Stove �ڴ��
osPoolId  pool_DataHandle;
osMessageQId Q_DataHandle;

/* �ṹʵ�� */
M_Status m_status;
RTC_Time rtc_time = {0};

// stove
Stove stove;

/* Triac */
__IO uint8_t fan_smoke_delay = 60;
__IO uint8_t fan_exchange_delay = 80;
__IO uint8_t feed_duty = 5;
__IO bool flag_full_blow = false;  // ����ȫ��3��
__IO bool flag_full_exchange = false;  // ѭ��ȫ��3��

/* �䷽ */
int8_t recipe_index = 0;
int8_t recipe[RECIPE_INDEX] = {
  30, 45, 60,
  35, 50, 65,
  40, 55, 60,
  0
};

RECIPE recipe_cur;  // ��ǰ�䷽

/* ���̹��� */
uint8_t power_of_blow[3] = {9, 7, 6};  // 50 + x * 5

/* ʱ����ʾ */
char *p_rtc_display;
char rtc_display[20];

/* �ƻ� */
Schedule plan[3] = {
  {8, 0, 12, 0, 13, 0, 18, 0, 1},
  {8, 0, 12, 0, 13, 0, 18, 0, 1},
  {8, 0, 12, 0, 13, 0, 18, 0, 1}
};


char info_stage[8] = "\x88\x89";  // ��ʼ�� ֹͣ
char *p_info_stage;

// ״̬�ַ���
char info_message[8];
char *p_info_message;
/* �¶�cv, pv�ַ��� */
char info_cv_pv[20];
char *p_info_cv_pv;

/* ����״̬���� */
State run_state = {0x2d, 0, 0, 1};

/*��ʾ�� ԤԼ*/
char schedule_time[20] = "\x8c\x8d\x8e 08:00-12:00";
char *p_schedule_time;

/* USER CODE END Variables */

/* Function prototypes -------------------------------------------------------*/
void StartDefaultTask(void const * argument);
void StartTaskScanKey(void const * argument);
void callback_blowDust(void const * argument);
void callback_preFeed(void const * argument);
void callback_fireTimeout(void const * argument);
void callback_beforeFeed(void const * argument);
void callback_delayRunning(void const * argument);
void callback_full_blow(void const * argument);
void callback_full_exchange(void const * argument);
void callback_boil_timeout(void const * argument);
void callback_switch_delay(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* Hook prototypes */

/* Init FreeRTOS */

void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  
  /* USER CODE END Init */
  
  /* Create the mutex(es) */
  /* definition and creation of mutex_rtc */
  osMutexDef(mutex_rtc);
  mutex_rtcHandle = osMutexCreate(osMutex(mutex_rtc));
  
  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */
  
  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */
  
  /* Create the timer(s) */
  /* definition and creation of tmr_blowDust */
  osTimerDef(tmr_blowDust, callback_blowDust);
  tmr_blowDustHandle = osTimerCreate(osTimer(tmr_blowDust), osTimerOnce, NULL);
  
  /* definition and creation of tmr_preFeed */
  osTimerDef(tmr_preFeed, callback_preFeed);
  tmr_preFeedHandle = osTimerCreate(osTimer(tmr_preFeed), osTimerOnce, NULL);
  
  /* definition and creation of tmr_fireTimeout */
  osTimerDef(tmr_fireTimeout, callback_fireTimeout);
  tmr_fireTimeoutHandle = osTimerCreate(osTimer(tmr_fireTimeout), osTimerOnce, NULL);
  
  /* definition and creation of tmr_beforeFeed */
  osTimerDef(tmr_beforeFeed, callback_beforeFeed);
  tmr_beforeFeedHandle = osTimerCreate(osTimer(tmr_beforeFeed), osTimerOnce, NULL);
  
  /* definition and creation of tmr_delayRunning */
  osTimerDef(tmr_delayRunning, callback_delayRunning);
  tmr_delayRunningHandle = osTimerCreate(osTimer(tmr_delayRunning), osTimerOnce, NULL);
  
  /* definition and creation of tmr_full_blow */
  osTimerDef(tmr_full_blow, callback_full_blow);
  tmr_full_blowHandle = osTimerCreate(osTimer(tmr_full_blow), osTimerOnce, NULL);
  
  /* definition and creation of tmr_full_exchange */
  osTimerDef(tmr_full_exchange, callback_full_exchange);
  tmr_full_exchangeHandle = osTimerCreate(osTimer(tmr_full_exchange), osTimerOnce, NULL);
  
  /* definition and creation of tmr_boil_timeout */
  osTimerDef(tmr_boil_timeout, callback_boil_timeout);
  tmr_boil_timeoutHandle = osTimerCreate(osTimer(tmr_boil_timeout), osTimerOnce, NULL);
  
  /* definition and creation of tmr_switch_delay */
  osTimerDef(tmr_switch_delay, callback_switch_delay);
  tmr_switch_delayHandle = osTimerCreate(osTimer(tmr_switch_delay), osTimerOnce, NULL);
  
  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */
  
  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityBelowNormal, 0, 512);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);
  
  /* definition and creation of TaskScanKey */
  osThreadDef(TaskScanKey, StartTaskScanKey, osPriorityNormal, 0, 1024);
  TaskScanKeyHandle = osThreadCreate(osThread(TaskScanKey), NULL);
  
  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */
  
  /* Create the queue(s) */
  /* definition and creation of Q_Key */
  osMessageQDef(Q_Key, 4, uint8_t);
  Q_KeyHandle = osMessageCreate(osMessageQ(Q_Key), NULL);
  
  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /*�Զ�����Ϣ����*/
  //Triac�ڴ��,��Ϣ
  pool_TriacHandle = osPoolCreate(osPool(pool_triac));
  
  osMessageQDef(Q_Triac, 1, Triac);
  Q_TriacHandle = osMessageCreate(osMessageQ(Q_Triac), NULL);
  //Stove�ڴ��,��Ϣ
  pool_DataHandle = osPoolCreate(osPool(pool_data));
  
  osMessageQDef(Q_Data,1,Data);
  Q_DataHandle = osMessageCreate(osMessageQ(Q_Data), NULL);
  /* USER CODE END RTOS_QUEUES */
}

/* StartDefaultTask function */
void StartDefaultTask(void const * argument)
{
  
  /* USER CODE BEGIN StartDefaultTask */
  
  __IO osStatus status;
  __IO ThermValue v_ds18b20;
  __IO K_Value v_max6675; 
  
  __IO uint8_t cv = 0;
  
  // K �˲�����
  uint16_t k_sum = 0, k_a = 0;
  
  bool start_conversion = false;
  // bool readable = false;  // ÿ2���ȡ�¶�
  // ds18b20���þ���
  therm_reset();
  therm_setResolution(RESOLUTION);
  
  
  /* Infinite loop */
  for(;;)
  {
    // run led
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
    
    ///* rtc ʱ�� */
    // �ź���
    //read_rtc(); 
    
    /* ds18b20 */
    if (start_conversion) {
      v_ds18b20 = therm_read_temperature();
      start_conversion = false;
    }
    else {
      therm_start_conversion();
      start_conversion = true;
    }
    
    if (v_ds18b20.error == 0)
      cv = v_ds18b20.v;
    
    /* k��ż  �˲�
    * ��ʼ����A=��ʼֵ��S=A*N
    * S = S - A + C
    * A = S / N
    */
    v_max6675 = read_k();
    if (v_max6675.error == 0) {
      if (k_sum == 0)
        k_sum = v_max6675.v * N_KTEMP;
      else {
        k_sum = k_sum - k_a + v_max6675.v;
        k_a = k_sum / N_KTEMP;
      }
    }
    
    /* ����stove���� */
    // ����stove�ڴ棬����StartTaskScanKey�з���
    Data *p_data;
    p_data = (Data*)osPoolAlloc(pool_DataHandle);
    if (p_data != NULL) {
      //���£� ¯��
      p_data->cv = (int16_t)cv;
      p_data->k = k_a; // temp_k
      
      //printf("ds18b20:%d,k:%d\r\n", p_data->cv, p_data->k);
      
      //      //ʱ��
      //      p_data->time.day = Time_sd3088.day;
      //      p_data->time.hour = Time_sd3088.hour;  // todo: 24Сʱ��
      //      p_data->time.minute = Time_sd3088.minute;
      //      p_data->time.month = Time_sd3088.month;
      //      p_data->time.quantity = Time_sd3088.quantity;
      //      p_data->time.second = Time_sd3088.second;
      //      p_data->time.week = Time_sd3088.week;
      //      p_data->time.year = Time_sd3088.year;
      // �������
      status = osMessagePut(Q_DataHandle, (uint32_t)p_data, 0);
      if (status != osOK)
        printf("Put data into Q_Data failed:%x\r\n", status);
    }
    
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* StartTaskScanKey function */
void StartTaskScanKey(void const * argument)
{
  /* USER CODE BEGIN StartTaskScanKey */
  
  __IO KeyPressed key_pressed;  // ����
  __IO bool need_update = false;  // �Ƿ�ˢ����ʾ
  __IO bool power = false;
  uint8_t pv = 9;
  
  __IO uint8_t read_rtc_counter = 0;
  
  In_Schedule_Time in_schedule_time;  // �ڼƻ���
  
  bool m_start = false;
  bool m_stop = false;
  
  bool schedule_start_stove = false;  //  �ƻ�ģʽ����¯��
  
  // ��Ļ��
  __IO uint8_t screen_no = 0;
  
  // ����1-7 �ַ�
  char weekday_chr[7] = {'\x93', '\xa1','\xa2','\xa3','\xa4','\xa5','\xa6'};
  osStatus status;
  
  // �ַ�������ָ��
  p_info_stage = &info_stage[0];
  p_rtc_display = &rtc_display[0];
  p_info_message = &info_message[0];  
  
  p_schedule_time = &schedule_time[0];
  
  p_info_cv_pv = &info_cv_pv[0];  // cv, pv
  
  // dataData data = {0, 0, rtc_time};
  // 
  Data data = {0, 0};  // �Ƿ���Ҫ��ʼ��?
  osEvent evt_data;
  
  // ���洢������״̬���Ƿ�ƻ���ǰ��ȱ��Ϩ��
  init_data();  
  
  /* ��ȡ�洢���䷽�� �ƻ��� ״̬���� */
  // ���䷽
  read_recipe(recipe);
  // ��ǰ�䷽
  recipe_cur.level_0 = recipe[recipe[RECIPE_INDEX - 1] * 3];
  recipe_cur.level_1 = recipe[recipe[RECIPE_INDEX - 1] * 3 + 1];
  recipe_cur.level_2 = recipe[recipe[RECIPE_INDEX - 1] * 3 + 2];
  recipe_index = recipe[RECIPE_INDEX - 1];
  // ���ƻ�  
  read_schedule(&plan[0], 0);
  read_schedule(&plan[1], 1);
  read_schedule(&plan[2], 2);
  // ������
  read_from_rtc(RTC_ADDR_POW, power_of_blow, 3);
  // ���¶� PV
  read_from_rtc(RTC_ADDR_PV, &pv, 1); 
  
  
  // ��RTC��24Сʱ��
  read_rtc();
  if (Time_sd3088.flag_12or24 == 0) {
    if (Time_sd3088.am_or_pm) {
      Time_sd3088.hour += 12;
    }
    write_rtc(&Time_sd3088);
  }
  // display
  init_draw();
  hmi();
  
  // stage init
  stage_init();
  
  /* Infinite loop */
  for(;;)
  {
    // 1���rtc
    read_rtc_counter++;
    if (read_rtc_counter == 5) {
      read_rtc_counter = 0;
      read_rtc();
    }
    
    // ɨ�谴��
    Scan_Keyboard();
    key_pressed = getKey();
    
    /* ��ȡ¯���� */
    evt_data = osMessageGet(Q_DataHandle, 0);
    if (evt_data.status == osEventMessage) {
      data = *((Data *)evt_data.value.p);
      stove.data = data;
      
      // ��ȡ��Ļ��
      if (get_screen_no() != screen_no) {
        screen_no = get_screen_no();
        //set_pin_key(screen_no);  // ���°�����
      }
      
      // �����޸�ʱ����Ļʱ��ˢ��ʱ����ʾ
      if (screen_no != SCREEN_RTC) {
        rtc_time = Time_sd3088;
        // ʱ���ʽ 9:00 ����
        sprintf(rtc_display, "%02d:%02d \x95%c", rtc_time.hour, rtc_time.minute, 
                weekday_chr[rtc_time.week]);
      }
      
      // �����ڴ�
      status = osPoolFree(pool_DataHandle, evt_data.value.p);
      if (status != osOK)
        printf("Free Data memory error:%x\r\n", status);
      
      // ˢ����Ļ�� ʱ��
      need_update = true;
    }
    
    
    /*------------------------------------------------------------------------*/
    
    /* ����Ԥ���� ��������� */
    if (key_pressed != Key_None) {
      //printf("key:%d\r\n", key_pressed);
      need_update = true;
      
      /* 1�����ش�1�ĵ�Դ���� */
      // ������ֹͣ����֮�׶�1
      if (is_home_screen()) {
        /* ��Դ���� */
        
        // ֹͣ
        if ( (key_pressed == Key_Power) && (m_status.stage != -1) && (!stove.scheduled)) {
          strcpy(info_stage, "\x88\x89");  // �׶�״̬��ֹͣ
          stop(&data);
        }
        else if (((key_pressed == Key_Power)  || schedule_start_stove) && (m_status.stage == -1)) {  // ����
          if (data.k >= TEMP_COOLING) { //��65�� ��ȴ
            stage_0();
            fan_smoke_delay = BLOW_LEVEL_1;  // 1
            fan_exchange_delay = 1;
            //strcpy(info_stage, "\x80\x81");  // �׶�״̬�����
          }
          else {
            stage_1();
            //fan_smoke_delay = BLOW_LEVEL_2;  // ����ʱ�������ٶ�Ϊ�ӳ�2����
          }
          
          strcpy(info_stage, "\x80\x81");  // �׶�״̬�����
        }   
        
        if (key_pressed == Key_Power) {
          key_pressed = Key_None;  // ����Power������
        }
        
        /* 2���ƻ�ģʽ���� */
        if (key_pressed == Key_Mode) {
          if (m_status.stage == -1) {
            stove.scheduled = true;            
          }
          else {
            
            stove.scheduled = false;
          }
          
          if (m_status.stage == 6) {            
            stove.boil_mode = true;
            // ��ʾ ��ˮͼ�ꣿ
            // ������ˮģʽ��ʱ��ʱ��
            osTimerStart(tmr_boil_timeoutHandle, 50 * 60 * 1000);  // 50����
            
          }
          
          key_pressed = Key_None; 
        }
        // ���ֶ�����ʱ�������л����ƻ�
        
        /* 3�����ش�1�ġ������� */
        if (key_pressed == Key_Up && pv < PV_HIGH) {
          pv++;
        }
        else if (key_pressed == Key_Down && pv > PV_LOW) {
          pv--;  
        }
        
        if (key_pressed == Key_Down || key_pressed == Key_Up) {
          
          key_pressed = Key_None; // ʹ��Ϊ��
          // ���� pv
          save_to_rtc(RTC_ADDR_PV, &pv, 1);
          
        }
        // ˢ�½��棺 ���� 
        need_update = true;
      }  // home screen
      
      /* 4���Ƿ��� menu home key */
      if (key_pressed == Key_Menu) {
        change_root();
        key_pressed = Key_None; // ʹ��Ϊ��
      }
    } 
    // *
    // ˢ�½����¶�
    sprintf(info_cv_pv, "\x8a\x8b:%d \x9a\x9b: %d",stove.data.cv, pv);
    
    /* ����Ԥ���� end  */
    /*------------------------------------------------------------------------*/
    
    if (key_pressed != Key_None)
      osMessagePut(Q_KeyHandle, key_pressed, 0);
    
    // ���°�����
    set_pin_key(screen_no);  
    m2_CheckKey();
    
    /* ԤԼʱ���� */
    if (stove.scheduled) {
      Schedule plan_cur;
      // ���������õ��ƻ�
      switch (rtc_time.week == 0) {        
      case 0:
        plan_cur = plan[2];
        break;
      case 6:
        plan_cur = plan[1];
        break;
      default:
        plan_cur = plan[0];
      }
      check_schedule(rtc_time.hour, rtc_time.minute, &plan_cur, &in_schedule_time);
      schedule_start_stove = in_schedule_time.in_schedule; 
      // ��ʾԤԼ      
      sprintf(schedule_time, "%s %02d:%02d - %02d:%02d", "\x8d\x8e", 
              in_schedule_time.start_hour,
              in_schedule_time.start_min,
              in_schedule_time.end_hour,
              in_schedule_time.end_min
                ); 
      
    }
    /*------------------------------------------------------------------------*/    
    
    // ����triac�ڴ�, triac.c�з���
    Triac *p_triac;
    p_triac = (Triac*)osPoolAlloc(pool_TriacHandle);
    
    /* ����¯�£������ж������ٶȡ������ٶ� */
    if (p_triac != NULL) {
      // �����ٶ�
      p_triac->fan_smoke_delay = 50 + power_of_blow[0]*5;
      // ������
      if (m_status.M_running) {  
        // ���ϱ���
        recipe_cur.level_0 = recipe[recipe_index * 3];
        recipe_cur.level_1 = recipe[recipe_index * 3 + 1];
        recipe_cur.level_2 = recipe[recipe_index * 3 + 2];
        judge(&stove, p_triac, &recipe_cur);
      }
      
      // todo: ������
      if (m_status.M_starting) {
        
        
      }
      
      // ��ȴ���¶ȵ���65��ʱ��ֹͣ ���̺�ѭ��
      if (data.k <= (TEMP_RUN - 5) && m_status.M_cooling == true) {
        if (m_status.M_blow)
          m_status.M_blow = false;
        
        if (m_status.M_exchange)
          m_status.M_exchange = false;
        
      }
      
      // ���·��״̬
      if (p_triac->fan_smoke_is_on != m_status.M_blow && (!flag_full_blow)) {        
        flag_full_blow = true;
        osTimerStart(tmr_full_blowHandle, 3 * 1000);
      }
      if (flag_full_blow) {
        /* �����power=0������ */
        p_triac->fan_smoke_delay = 100;  
      }
      
      p_triac->fan_smoke_is_on = m_status.M_blow;      
      p_triac->fan_exchange_is_on = m_status.M_exchange;
      p_triac->feed_is_on = m_status.M_feed;
      
      p_triac->feed_duty = feed_duty;  // todo: ͨ����Ϣ����
      
      // triac ������Ϣ����
      status = osMessagePut(Q_TriacHandle, (uint32_t)p_triac, 0);
      if (status != osOK)
        printf("Put message into Q_Triac failed: %x\r\n", status);
    } 
    // if end: Tirac Queue 
    /*------------------------------------------------------------------------*/
    
    // �ж�¯�µ�70��?
    if (data.k >= TEMP_RUN && m_status.stage == 4) {
      stage_5();
      // ֹͣ���ʱ��ʱ��
      osTimerStop(tmr_fireTimeoutHandle);
      strcpy(info_stage, "\x98\x99");  // �׶�״̬������
    }
    
    // ��� ����
    if (m_status.M_fire)
      HAL_GPIO_WritePin(PORT_FIRE_ROD, FIRE_ROD, GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(PORT_FIRE_ROD, FIRE_ROD, GPIO_PIN_RESET);
    
    /*------------------------------------------------------------------------*/
    /* ˢ����ʾ */
    if ( m2_HandleKey() | need_update) {
      /* picture loop */
      u8g_FirstPage(&u8g);
      do
      {
        draw();
      } while( u8g_NextPage(&u8g) );
      
      //
      need_update = false;
    }
    
    // ˢ��IWDG
    //HAL_IWDG_Refresh(&hiwdg);
    
    osDelay(10);
  }
  /* USER CODE END StartTaskScanKey */
}

/* callback_blowDust function */
void callback_blowDust(void const * argument)
{
  /* USER CODE BEGIN callback_blowDust */
  
  // ��2 ��ʼ����70�붨ʱ��
  stage_2();
  feed_duty = 3;  // 3/10
  
  /* USER CODE END callback_blowDust */
}

/* callback_preFeed function */
void callback_preFeed(void const * argument)
{
  /* USER CODE BEGIN callback_preFeed */
  // Ԥ���Ͻ���
  // ��3 ��ʼԤ����
  
  stage_3();
  
  /* USER CODE END callback_preFeed */
}

/* callback_fireTimeout function */
void callback_fireTimeout(void const * argument)
{
  /* USER CODE BEGIN callback_fireTimeout */
  // ���ʧ��
  //alarm
  HAL_GPIO_WritePin(PORT_FIRE_ROD, FIRE_ROD, GPIO_PIN_RESET);
  printf("fire is failure.\r\n");
  /* USER CODE END callback_fireTimeout */
}

/* callback_beforeFeed function */
void callback_beforeFeed(void const * argument)
{
  /* USER CODE BEGIN callback_beforeFeed */
  // ��4 ����4���Ӻ󣬿�ʼ����
  
  stage_4();
  
  /* USER CODE END callback_beforeFeed */
}

/* callback_delayRunning function */
void callback_delayRunning(void const * argument)
{
  /* USER CODE BEGIN callback_delayRunning */
  
  stage_6();
  
  /* USER CODE END callback_delayRunning */
}

/* callback_full_blow function */
void callback_full_blow(void const * argument)
{
  /* USER CODE BEGIN callback_full_blow */
  // ���̷���ȫ��ʱ�䵽
  flag_full_blow = false;
  /* USER CODE END callback_full_blow */
}

/* callback_full_exchange function */
void callback_full_exchange(void const * argument)
{
  /* USER CODE BEGIN callback_full_exchange */
  
  /* USER CODE END callback_full_exchange */
}

/* callback_boil_timeout function */
void callback_boil_timeout(void const * argument)
{
  /* USER CODE BEGIN callback_boil_timeout */
  // ��ˮģʽ��ʱ�˳�
  ((Stove*)argument)->boil_mode = false;
  /* USER CODE END callback_boil_timeout */
}

/* callback_switch_delay function */
void callback_switch_delay(void const * argument)
{
  /* USER CODE BEGIN callback_switch_delay */
  
  /* USER CODE END callback_switch_delay */
}

/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
