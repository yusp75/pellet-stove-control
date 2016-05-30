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
osThreadId TaskBeepHandle;
osMessageQId Q_KeyHandle;
osMessageQId Q_IrHandle;
osMessageQId Q_BeepHandle;
osMessageQId M_SwitchDelayHandle;
osTimerId tmr_blowDustHandle;
osTimerId tmr_preFeedHandle;
osTimerId tmr_fireTimeoutHandle;
osTimerId tmr_beforeFeedHandle;
osTimerId tmr_delayRunningHandle;
osTimerId tmr_full_blowHandle;
osTimerId tmr_full_exchangeHandle;
osTimerId tmr_boil_timeoutHandle;
osTimerId tmr_switch_delayHandle;
osTimerId tmr_k_validateHandle;
osTimerId tmr_feed_manualHandle;

/* USER CODE BEGIN Variables */

#define N_KTEMP 4  // ƽ��ֵ
osMessageQId Q_ErrorHandle;  // err

// �ⲿ����
//extern IWDG_HandleTypeDef hiwdg;
extern TIM_HandleTypeDef htim3;
extern uint8_t rx_buffer[10];
extern bool transfer_cplt;

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

// Zero�ڴ��
osPoolDef(pool_zero, 5, Zero);
osPoolId pool_ZeroHandle;

/*�ȵ�żK ��Ϣ*/
osPoolDef(pool_K, 1, K_Value); // Define Stove �ڴ��
osPoolId  pool_KHandle;
osMessageQId Q_KHandle;

/* �ṹʵ�� */
M_Status m_status;
RTC_Time rtc_time_read = {0};
RTC_Time rtc_time = {0};

/* Triac */
__IO uint16_t fan_smoke_delay = 3;
__IO uint16_t fan_exchange_delay = 3;
__IO uint8_t feed_duty = 30;
__IO bool flag_full_blow = false;  // ����ȫ��3��
__IO bool flag_full_exchange = false;  // ѭ��ȫ��3��

/* �䷽ */
uint8_t recipe_index = 0;
int8_t recipe[RECIPE_DATA_LEN] = {
  30, 45, 60,
  35, 50, 65,
  40, 55, 60
};

RECIPE recipe_cur;  // ��ǰ�䷽

/* ���̹��� */
uint8_t power_of_blow[3] = {1, 2, 2};  // 50 + x * 5

/* ʱ����ʾ */
char *p_rtc_display;
char rtc_display[30]="\0";

/* �ƻ� */
Schedule plan[3] = {
  {8, 0, 12, 0, 13, 0, 18, 0, 1},
  {8, 0, 12, 0, 13, 0, 18, 0, 1},
  {8, 0, 12, 0, 13, 0, 18, 0, 1}
};


char info_stage[20] = "\x88\x89";  // ��ʼ�� ֹͣ
char *p_info_stage;

// ״̬�ַ���
char info_message[10] = "\0";
char *p_info_message;
/* �¶�cv, pv�ַ��� */
char info_cv_pv[30] = "\0";
char *p_info_cv_pv;

/* ����״̬���� */

/*��ʾ�� ԤԼ*/
char schedule_time[30] = "\x8c\x8d\x8e 08:00-12:00";
char *p_schedule_time;
bool auto_or_manual = false;  // �Զ� or �ֶ�
bool m_stoped = false;
bool m_started = false;

/* ״̬��Ļ */
//uint8_t status_battery;

char info_blow[20]="\0";
char *p_info_blow;
char info_blow_2[30]="\0";
char *p_info_blow_2;
char info_feed_ex[20]="\0";  // ���ϣ�ѭ��
char *p_info_feed_ex;

// ״̬��Ļ extern
M2_EXTERN_VLIST(top_status);
M2_EXTERN_VLIST(el_top_home);

/* ȱ�� */
// ��������
uint8_t beep_count = 0;
bool is_absence = false;
bool m_update_absence = false;  // ����ȱ�ϱ����־

// ���ʧ�ܼ����� Ϩ�����
__IO uint8_t failure_fire = 1;
__IO uint8_t failure_flame = 1;
__IO bool m_failure_fire_2nd = false;
__IO bool m_failure_flame_2nd = false;

// ����
// 1���ֶ�
bool feed_by_manual = false;
char feed_progress[6] = "0%";
char *p_feed_progress;
uint32_t t_feed_manual = 0;
// 2��
bool feed_full = false;


/* �Զ����������-------------------------------------------------------------*/

/* USER CODE END Variables */

/* Function prototypes -------------------------------------------------------*/
void StartDefaultTask(void const * argument);
void StartTaskScanKey(void const * argument);
void StartTaskBeep(void const * argument);
void callback_blowDust(void const * argument);
void callback_preFeed(void const * argument);
void callback_fireTimeout(void const * argument);
void callback_beforeFeed(void const * argument);
void callback_delayRunning(void const * argument);
void callback_full_blow(void const * argument);
void callback_full_exchange(void const * argument);
void callback_boil_timeout(void const * argument);
void callback_switch_delay(void const * argument);
void callback_k_validate(void const * argument);
void callback_feed_manual(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* USER CODE BEGIN FunctionPrototypes */
void beep(uint8_t *beep_count);  // ������
/* USER CODE END FunctionPrototypes */

/* Hook prototypes */

/* Init FreeRTOS */

void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  
  /* USER CODE END Init */

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

  /* definition and creation of tmr_k_validate */
  osTimerDef(tmr_k_validate, callback_k_validate);
  tmr_k_validateHandle = osTimerCreate(osTimer(tmr_k_validate), osTimerOnce, NULL);

  /* definition and creation of tmr_feed_manual */
  osTimerDef(tmr_feed_manual, callback_feed_manual);
  tmr_feed_manualHandle = osTimerCreate(osTimer(tmr_feed_manual), osTimerPeriodic, NULL);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityBelowNormal, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of TaskScanKey */
  osThreadDef(TaskScanKey, StartTaskScanKey, osPriorityNormal, 0, 1380);
  TaskScanKeyHandle = osThreadCreate(osThread(TaskScanKey), NULL);

  /* definition and creation of TaskBeep */
  osThreadDef(TaskBeep, StartTaskBeep, osPriorityNormal, 0, 128);
  TaskBeepHandle = osThreadCreate(osThread(TaskBeep), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the queue(s) */
  /* definition and creation of Q_Key */
  osMessageQDef(Q_Key, 2, uint32_t);
  Q_KeyHandle = osMessageCreate(osMessageQ(Q_Key), NULL);

  /* definition and creation of Q_Ir */
  osMessageQDef(Q_Ir, 1, uint32_t);
  Q_IrHandle = osMessageCreate(osMessageQ(Q_Ir), NULL);

  /* definition and creation of Q_Beep */
  osMessageQDef(Q_Beep, 1, uint8_t);
  Q_BeepHandle = osMessageCreate(osMessageQ(Q_Beep), NULL);

  /* definition and creation of M_SwitchDelay */
  osMessageQDef(M_SwitchDelay, 2, uint32_t);
  M_SwitchDelayHandle = osMessageCreate(osMessageQ(M_SwitchDelay), NULL);

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
  
  // err ��Ϣ
  osMessageQDef(Q_Error,5,Zero);
  Q_ErrorHandle = osMessageCreate(osMessageQ(Q_Error), NULL);
  
  // zero ��Ϣ
  pool_ZeroHandle = osPoolCreate(osPool(pool_zero));
  
  // �ȵ�żK��Ϣ
  pool_KHandle = osPoolCreate(osPool(pool_K));
  
  osMessageQDef(Q_K, 1, K_Value);
  Q_KHandle = osMessageCreate(osMessageQ(Q_K), NULL);
  
  /* USER CODE END RTOS_QUEUES */
}

/* StartDefaultTask function */
void StartDefaultTask(void const * argument)
{

  /* USER CODE BEGIN StartDefaultTask */
  
  osStatus status;
  ThermValue v_ds18b20; 
  uint8_t cv = 0;
  
  bool start_conversion = false;
  
  // bool readable = false;  // ÿ2���ȡ�¶�
  // ds18b20���þ���
  therm_reset();
  therm_setResolution(RESOLUTION);
  
  /* Infinite loop */
  for(;;)
  {
    // ����ָʾ��
    LED_RUN;
    
#ifdef DEBUG
    // print t7

    osEvent evc_err = osMessageGet(Q_ErrorHandle, 0);
    if (evc_err.status == osEventMessage) {
      Zero *z = (Zero*)evc_err.value.p;
      //      printf("t1:%d,t3:%d,t4:%d,t6:%d,t7:%d,T1:%d\r\n", 
      //             z->t1,z->t3,z->t4,z->t6,z->t7,z->T1);
      printf("t_prev:%d, t_cur:%d, i:%d, T_1:%d\r\n", z->t1,z->t3,z->i, z->T1);
      if (z->cr)
        printf("\r\n");
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_1);
      osPoolFree(pool_ZeroHandle, (Zero*)evc_err.value.p);
    }
    
#endif 
    
    //
    read_k();
    //
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
   
    /* ����stove���� */
    // ����stove�ڴ棬����StartTaskScanKey�з���
    if (pool_DataHandle != NULL) {
      Data *p_data = (Data*)osPoolAlloc(pool_DataHandle);
      
      if (p_data != NULL) {
        //���£� ¯��
        p_data->cv = (int16_t)cv;
        //p_data->k = k_a; // temp_k
        
        // �������
        status = osMessagePut(Q_DataHandle, (uint32_t)p_data, 0);
        if (status != osOK)
          printf("Put data into Q_Data failed:%x\r\n", status);
      }  // put
    }
    
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* StartTaskScanKey function */
void StartTaskScanKey(void const * argument)
{
  /* USER CODE BEGIN StartTaskScanKey */
  
  KeyPressed key_pressed;  // ����
  bool need_update = false;  // �Ƿ�ˢ����ʾ
  uint8_t pv = 9;  // ���õ��¶�
  
  // �ﵽ���е�K�¶��Ƿ���Ч
  bool m_k_is_valid = false;
  
  uint8_t read_rtc_counter = 0;
  
  In_Schedule_Time in_schedule_time;  // �ڼƻ���
  
  //__IO bool m_started = false;
  bool m_start_cooling = false;
  bool m_stop_cooling = false;
  bool m_idle_cooling = false;
  
  // stove ����
  Stove stove = {0};  
  
  bool schedule_start_stove = false;  //  �ƻ�ģʽ����¯��
  
  // ��Ļ��
  uint8_t screen_no = 0;
  
  // ����1-7 �ַ�
  char weekday_chr[] = {'\x93', '\xa1','\xa2','\xa3','\xa4','\xa5','\xa6'};
  osStatus status;
  
  // �ַ�������ָ��
  p_info_stage = &info_stage[0];
  p_rtc_display = &rtc_display[0];
  p_info_message = &info_message[0];   
  p_schedule_time = &schedule_time[0];    
  p_info_cv_pv = &info_cv_pv[0];  // cv, pv  
  p_info_blow = &info_blow[0];
  p_info_blow_2 = &info_blow_2[0];
  p_info_feed_ex = &info_feed_ex[0];  
  p_feed_progress = &feed_progress[0];  
  
  // 
  __IO Data data = {0, 0}; 
  osEvent evt_data;
  
  uint16_t status_k_temp = 0;  // ��ʱK�¶�
  
  bool in_testing = false;  // ������
  uint16_t v_tesing;  // �����õ��Ĳ����ٶ�
  
  bool started_after_cooling = false;
  // �ȵ�ż
  K_Value v_max6675;
  // K �˲�����
  uint16_t k_sum = 0, k_a = 0;
  
  // �����������
  /*--------------------------------------------------------------------------*/
  // ���洢������״̬���Ƿ�ƻ���ǰ��ȱ��Ϩ��
  init_data();  
  
  /* ��ȡ�洢���䷽�� �ƻ��� ״̬���� */
  // ���䷽
  read_recipe(recipe);
  // ���䷽����
  if (!read_from_rtc(RTC_ADDR_RECIPE_INDEX, &recipe_index, 1)) {
    recipe_index = 0;
  }
  if (recipe_index > 2) {
    recipe_index = 0;
  }
  // ��ǰ�䷽
  recipe_cur.level_0 = recipe[recipe_index*3];
  recipe_cur.level_1 = recipe[recipe_index+1];
  recipe_cur.level_2 = recipe[recipe_index+2];
  // 
  // ���ƻ�  
  read_schedule(&plan[0], 0);
  read_schedule(&plan[1], 1);
  read_schedule(&plan[2], 2);
  // ������
  read_from_rtc(RTC_ADDR_POW, power_of_blow, 3);
  // ���¶� PV
  read_from_rtc(RTC_ADDR_PV, &pv, 1); 
  
  // ��ԤԼ
  read_from_rtc(RTC_ADDR_SCHEDULE_STATE, (uint8_t*)&auto_or_manual, 1);
  
  // ��RTC��24Сʱ��
  //  read_rtc(&rtc_time);
  //  if (rtc_time.flag_12or24 == 0) {
  //    if (rtc_time.am_or_pm) {
  //      rtc_time.hour += 12;
  //    }
  //    write_rtc(&rtc_time);
  //  }
  
  // ���������ݽ���  
  /*--------------------------------------------------------------------------*/  
  // ��ʾ
  init_draw();
  hmi();
  
  // �׶γ�ʼ��
  stage_init();
  
  /* Infinite loop */
  for(;;)
  { 
    // 1���rtc
    read_rtc_counter++;
    if (read_rtc_counter >= 49) {
      read_rtc_counter = 0;
      read_rtc(&rtc_time_read);
      // ��ص���
      //status_battery = rtc_time_read.quantity;
    }
    
    /* k��ż  �˲�
    * ��ʼ����A=��ʼֵ��S=A*N
    * S = S - A + C
    * A = S / N
    */
    // ����Ϣ����ȡK
    osEvent evt_k = osMessageGet(Q_KHandle,0);
    
    if (evt_k.status == osEventMessage) {
      K_Value *p_k = (K_Value*)evt_k.value.p;
      v_max6675.v = p_k->v;
      v_max6675.error = p_k->error;
      // �ͷ�memory
      osPoolFree(pool_KHandle, (K_Value *)evt_k.value.p);      
      
      if (v_max6675.error == 0) {
        if (k_sum == 0) {
          k_sum = v_max6675.v * N_KTEMP;
        }
        else {
          k_sum = k_sum - k_a + v_max6675.v;
        }
          k_a = k_sum / N_KTEMP;
        // ���µ���ʾ����
        status_k_temp = k_a; 
        data.k = k_a;  // ������k
      }
    }
    // get
    
    // ɨ�谴��
    Scan_Keyboard();
    key_pressed = getKey();
    // ��IR������
    if (key_pressed == Key_None) {
      //IR ���ڼ���
      key_pressed = getIrKey();
    }
    
    /*------------------------------------------------------------------------*/
    // ������ ����
    if (key_pressed != Key_None) {
      if (Q_BeepHandle != NULL) {
        osMessagePut(Q_BeepHandle, (uint32_t)BEEP_BUTTON, 0);
      }
    }
    
    /*------------------------------------------------------------------------*/
    /* ��ȡ¯���� */
    evt_data = osMessageGet(Q_DataHandle, 0);
    if (evt_data.status == osEventMessage) {
      Data *p_data = (Data *)evt_data.value.p;
      stove.data = *p_data;
      data.cv = p_data->cv;
      // ����K��״̬��Ļ ------��1) 
      //status_k_temp = data.k;
      // ��ȡ��Ļ��
      if (get_screen_no() != screen_no) {
        screen_no = get_screen_no();
        //set_pin_key(screen_no);  // ���°����� ---������
      }
      
      // �����޸�ʱ����Ļʱ��ˢ��ʱ����ʾ
      if (screen_no != SCREEN_RTC) {
        // ʱ���ʽ 9:00 ����
        rtc_time = rtc_time_read;
        sprintf(rtc_display, "%02d:%02d \x95%c", rtc_time.hour, rtc_time.minute, 
                weekday_chr[rtc_time.week]);
      }
      
      // �����ڴ�
      status = osPoolFree(pool_DataHandle, p_data);
      if (status != osOK)
        printf("Free Data memory error:%x\r\n", status);
      
      // ˢ����Ļ�� ʱ��
      need_update = true;
    }    
    
    /*------------------------------------------------------------------------*/
    
    /* ����Ԥ������ ��������� */
    if (key_pressed != Key_None) {
      //printf("key:%d\r\n", key_pressed);
      need_update = true;
      
      /* 1�����ش�1�ĵ�Դ���� */
      // ������ֹͣ����֮�׶�1
      if (is_home_screen()) {
        
        /* 1�� ��Դ���� */        
        if (!auto_or_manual) {
          if ( key_pressed==Key_Power) {
            if (m_status.stage!=-1) {
              m_stoped = true;
              // ͣ����ȫ�֣�ȫ������
              feed_full = false;
            }
            else {
              m_started = true; 
              // �ֶ�����������2������
              failure_fire = 1;
              failure_flame = 1;
            } 
            key_pressed = Key_None;
          }
        }
        
        /* 2���ƻ�ģʽ���� */
        if (key_pressed == Key_Mode) {
          // �Զ�
          if (m_status.stage==-1 && !auto_or_manual) {
            auto_or_manual = true; 
            printf("auto, on\r\n");
          }
          else if (auto_or_manual) {            
            //            auto_or_manual = false;
            //            m_stoped = true;
            //            printf("auto, off\r\n");
            schedule_exit_confirm();
            
          }
          // ����RTC����
          save_schedule_state(auto_or_manual);
          
          // �����ˮ
          if (m_status.stage==6 && !stove.boil_mode) {
            stove.boil_mode = true;
            // ��ʾ ��ˮͼ�ꣿ
            strcpy(info_stage, "\xc0\xad");  // info ��ˮ
            // ������ˮģʽ��ʱ��ʱ��
            osTimerStart(tmr_boil_timeoutHandle, 50 * 60 * 1000);  // 50����            
          }
          else if (m_status.stage==6 && stove.boil_mode) {
            stove.boil_mode = false;
            strcpy(info_stage, "\x98\x99");  //info ����
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
        
        
        /* 4�� ���ش�1�� �� */
        
        if (key_pressed==Key_Left) {
          m2_SetRoot(&top_status);
          key_pressed = Key_None;
        }
        // ˢ�½��棺 ���� 
        need_update = true;
      }  // home screen
      
      // ״̬��Ļ
      if (is_status_screen()) {
        if (key_pressed==Key_Right || key_pressed==Key_Menu) {
          m2_SetRoot(&el_top_home);
        }
        key_pressed = Key_None;
      }
      
      /* 5���Ƿ��� menu home key */
      if (key_pressed == Key_Menu) {
        change_root();
        key_pressed = Key_None; // ʹ��Ϊ��
      }
      
    } 
    /* ����Ԥ���� end  */
    /*------------------------------------------------------------------------*/
    // *
    // ˢ�½����¶�
    sprintf(info_cv_pv, "\x8a\x8b:%d \x9a\x9b: %d",stove.data.cv, pv);
    sprintf(info_blow, "\xbb\xbc\x8b\xbe: [%d]", status_k_temp);
    
    /* ԤԼʱ���� */
    if (auto_or_manual) {
      Schedule plan_cur;
      // ���������õ��ƻ�
      switch (rtc_time.week) {        
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
      if (!m_failure_fire_2nd && !m_failure_flame_2nd) {
        if (schedule_start_stove && m_status.stage==-1) {  // �ƻ���
          m_started = true;
        }
      }
      
      if (!schedule_start_stove && m_status.stage!=-1){  // �����ƻ���
        m_stoped = true;
      }
      //  
    }
    else { //��ԤԼ
      sprintf(schedule_time, "%s", "\x8c\x8d\x8e");
      
    }
    
    /* ״̬���� */
    /*----------------------------------------------------------------------*/
    // --- 65��ʱ��Ҫ��ȴ
    //��1��
    if ( m_started && (!m_start_cooling) && data.k>=TEMP_COOLING) {
      m_start_cooling = true;
      m_started = false;
      
      strcpy(info_stage, "\xc3\xc2: \xc4\xc5"); // ��������ȴ
    }
    //��2��
    if (m_stoped && (!m_stop_cooling) && data.k>=TEMP_COOLING) {
      m_stop_cooling = true;
      m_stoped = false;
      
      strcpy(info_stage, "\x88\x89: \xc4\xc5"); // ֹͣ����ȴ
    }
    
    //��3��
    if (m_status.M_idle && (!m_idle_cooling) && data.k>=TEMP_COOLING) {
      m_idle_cooling = true;
      strcpy(info_stage, "\xc4\xc5"); // ���У���ȴ
    }
    
    if ((m_start_cooling || m_stop_cooling || m_idle_cooling) && !m_status.M_cooling) {
      stage_0();
      fan_smoke_delay = power_of_blow[2];  // 1 *����*
      fan_exchange_delay = 1;
    }      
    //
    if (((m_started || started_after_cooling) && !m_start_cooling) || started_after_cooling) {
      // ����ǰ init��
      
      m_k_is_valid = false; // 1��m_k_is_valid����0�� ���ڵ�2������      
      stove.next_is_delaying = false; // 2���ӳٻ��ϱ�־��0      
      stove.level = 0; // 3��ȼ�յȼ�      
      stove.boil_mode = false; // 4����ˮģʽ
      // 5����������Ϊ55      
      fan_smoke_delay = power_of_blow[2]; // �׶�1���̷��٣�ȡ�ȼ�3ֵ
      
      // �׶�1
      stage_1();
      m_started = false; 
      started_after_cooling = false;
      strcpy(info_stage, "\x80\x81");  // �׶�״̬�����       
    }
    // ֹͣ ***
    if (m_stoped && !m_stop_cooling) {
      m_stoped = false;
      stop("normal");
      strcpy(info_stage, "\x88\x89");  // �׶�״̬��ֹͣ
    }
    
    // ��ȴ���¶ȵ���60��ʱ��ֹͣ ���̺�ѭ��
    // ����ǰ��ȴ��������
    if (data.k<=(TEMP_RUN-10) && m_status.M_cooling) {
      if (m_start_cooling) {
        started_after_cooling = true;
        
        m_start_cooling = false;
        printf("after cooling, start\r\n");
      }
      
      if (m_stop_cooling) { 
        stop("cooling for stop");
        strcpy(info_stage, "\x88\x89");  // �׶�״̬��ֹͣ
        m_stop_cooling = false;
        printf("after cooling, stop\r\n");
      }
      
      if (m_idle_cooling) {
        stop("cooling for stop");
        strcpy(info_stage, "\x88\x89");  // �׶�״̬��ֹͣ
        m_idle_cooling = false;
        printf("idle cooling, stop\r\n");
      }
      
      m_status.M_cooling = false;
    }
    
    // ȼ����Ϩ��ԭ�� ȱ��
    if (m_status.M_running && data.k<=FLAME_DOWN) {
      
      is_absence = true;
      m_update_absence = true;
      // ��ʾȱ��
      strcpy(info_stage, "\xac\x9d");  // ȱ��
      // error
      
      // ������ 10s
      if (Q_BeepHandle != NULL) {
        osMessagePut(Q_BeepHandle, (uint32_t)BEEP_ABSENCE, 0);
      }
      
      // ����2������
      if (failure_flame > 0) {
        // ֹͣ
        stop("flame failure stop");
        m_started = true;
        // ����
        failure_flame--;  
        // �ӳ�һ���
        // osDelay(TRY_DELAY);
      }
      else {
        m_stoped = true; 
        m_failure_flame_2nd = true;
      }
      
      // 5���Ӻ�ر�
    }
    
    // ����/���� ȱ��״̬
    if (m_update_absence) {        
      // ����
      save_to_rtc(RTC_ADDR_ABSENCE, (uint8_t*)&is_absence, 1);
      m_update_absence = false;
    }
    // �����������У���ȡ��ȱ��״̬
    
    // ���ڲ���
    if (m_status.stage == -1) {
      if(transfer_cplt) {   
        printf("\r\n");
        
        v_tesing = atoi((char const*)rx_buffer);
        if (v_tesing==0) {
          in_testing = false;
        }
        else {
          in_testing = true;         
        }
        transfer_cplt = false;
      }
      
    }
    if (in_testing) {
      m_status.M_blow = in_testing;
      fan_smoke_delay = v_tesing;
    }
    //
    /*------------------------------------------------------------------------*/
    
    if (key_pressed != Key_None) {
      if (Q_KeyHandle != NULL) {
        osMessagePut(Q_KeyHandle, (uint32_t)key_pressed, 0);
      }
    }
    
    // ���°�����
    set_pin_key(screen_no);  
    m2_CheckKey(); 
    
    /*------------------------------------------------------------------------*/    
    // �ж������¶��Ƿ�70��
    // 1���������ɴ���70�棬������ж�Ϊ����
    if (data.k>=TEMP_RUN && m_status.stage==4 && !m_k_is_valid) {
      m_k_is_valid = true;
      osTimerStart(tmr_k_validateHandle, K_VALIDATE);
      //printf("k validate is start.\r\n");
    }
    
    if (data.k<TEMP_RUN && m_status.stage==4 && m_k_is_valid) {
      m_k_is_valid = false;
      osTimerStop(tmr_k_validateHandle);
      printf("k validate is stop.\r\n");
    }
    
    // ��� ����
    if (m_status.M_fire)
      HAL_GPIO_WritePin(PORT_FIRE_ROD, FIRE_ROD, GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(PORT_FIRE_ROD, FIRE_ROD, GPIO_PIN_RESET);
    
    /*------------------------------------------------------------------------*/
    
    // �ֶ�����
    if (feed_by_manual) {    
      if (t_feed_manual >= TIME_FEEDMANUAL) {
        feed_by_manual = false;
        strcpy(feed_progress, "100%");
        
        osTimerStop(tmr_feed_manualHandle);
      }
      else {
        sprintf(feed_progress, "%02d%%", t_feed_manual*10/6);
        //
      }
      //  
    }
    /*------------------------------------------------------------------------*/
    
    // ����triac�ڴ�, triac.c�з���
    if (pool_TriacHandle != NULL) {
      Triac *p_triac = (Triac *)osPoolAlloc(pool_TriacHandle);      
      
      /* ����¯�£������ж������ٶȡ������ٶ� */
      if (p_triac != NULL) {    
        // ���У����ϱ����ͷ��ٶ�̬�ı�
        if (m_status.M_running) {  
          // ���ϱ���
          recipe_cur.level_0 = recipe[recipe_index*3];
          recipe_cur.level_1 = recipe[recipe_index*3+1];
          recipe_cur.level_2 = recipe[recipe_index*3+2];
          
          stove.pv = pv;  // ����pv
          
          // ���ݡ�T�������ٺ�����
          if (!stove.next_is_delaying) {
            judge(&stove, p_triac, &recipe_cur, power_of_blow);
          }
          
          // -����ӳ���Ϣ------
          osEvent evt_switch = osMessageGet(M_SwitchDelayHandle, 0);
          if (evt_switch.status == osEventMessage) {
            stove.next_is_delaying = false;
            //p_triac->fan_smoke_power = stove.next_v;  //�����������ı�
            fan_smoke_delay = stove.next_v;
          }
          //
        }
        
        // �������ϡ����̵�״̬��Ļ  ------ ��2��      
        sprintf(info_blow_2, "\x9c\xbf\xbd\xbe: [%d]", p_triac->fan_smoke_power);
        sprintf(info_feed_ex, "\x9c\x9d\xbd\xbe: [%d]", p_triac->feed_duty); 
        
        /*----------------------------------------------------------------------*/ 
        
        // �׶�1�� ����
        if (m_status.stage==1) {
          fan_smoke_delay = power_of_blow[2];  
        }
        
        // ������Ϣ
        p_triac->fan_smoke_is_on = m_status.M_blow;      
        p_triac->fan_exchange_is_on = m_status.M_exchange;
        p_triac->feed_is_on = m_status.M_feed;
        
        p_triac->feed_by_manual = feed_by_manual;
        p_triac->feed_full = feed_full;
        
        p_triac->fan_smoke_power = fan_smoke_delay;  // + 04.24 
        
        
        // Ԥ���Ͻ׶�
        if (m_status.stage == 2) {
          p_triac->feed_duty = feed_duty;  // todo: ͨ����Ϣ����
        }
        
        // triac ������Ϣ����
        if (Q_TriacHandle != NULL) {
          status = osMessagePut(Q_TriacHandle, (uint32_t)p_triac, 0);
          
          if (status != osOK)
            printf("Put message into Q_Triac failed: %x\r\n", status);
        }  // put
      }
      // if end: Tirac Queue 
    }
    // 
    
    /*------------------------------------------------------------------------*/
    
    /* ˢ����ʾ */
    if ( m2_HandleKey() | need_update) {
      /* picture loop */
      u8g_FirstPage(&u8g);
      do
      {
        m2_Draw();
      } while( u8g_NextPage(&u8g) );
      
      //
      need_update = false;
    }
    
    // ��ʾ���Ͻ���
    if (is_feed_manual_screen()) {
      uint16_t w= t_feed_manual/6;
      u8g_DrawBox(&u8g, 10,10,50, w);
      u8g_DrawStr(&u8g, 20, 20, "hi");
    }
    
    // ˢ��IWDG
    //HAL_IWDG_Refresh(&hiwdg);
    
    osDelay(20);
  }
  // ɨ������� end
  /*--------------------------------------------------------------------------*/
  /* USER CODE END StartTaskScanKey */
}

/* StartTaskBeep function */
void StartTaskBeep(void const * argument)
{
  /* USER CODE BEGIN StartTaskBeep */
  
  uint8_t type = 0;
  uint8_t beep_key = 0;
  uint8_t beep_alarm = 0;
  
  /* Infinite loop */
  for(;;)
  { 
    // ȡ������Ϣ
    osEvent evt = osMessageGet(Q_BeepHandle, 0);
    if (evt.status == osEventMessage) {      
      type = (uint8_t)evt.value.v;      
      switch(type) {    
      case BEEP_BUTTON:
        beep_key = 1;
        break;
      case BEEP_ABSENCE:
        beep_alarm = 10;
        break;
      }
    }
    beep(&beep_key);
    beep(&beep_alarm);
    
    osDelay(50);
  }
  /* USER CODE END StartTaskBeep */
}

/* callback_blowDust function */
void callback_blowDust(void const * argument)
{
  /* USER CODE BEGIN callback_blowDust */
  
  // ��2 ��ʼ����70�붨ʱ��
  stage_2();
  feed_duty = 20;  // 3/10
  feed_full = true;
  /* USER CODE END callback_blowDust */
}

/* callback_preFeed function */
void callback_preFeed(void const * argument)
{
  /* USER CODE BEGIN callback_preFeed */
  // Ԥ���Ͻ���
  // ��3 ��ʼԤ����
  feed_full = false;
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
  // ֹͣ���ʱ��ʱ��
  osTimerStop(tmr_fireTimeoutHandle);
  
  if (failure_fire > 0) {
    stop("failure fire");
    m_started = true;
    failure_fire--;
    //osDelay(TRY_DELAY);
  }
  else {
    m_stoped = true;
    m_failure_fire_2nd = true;
  }
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
  
  // ���ɹ����ӳ�һ���
  stage_6();
  // �Ѿ����У�����
  // �ֶ�����������2������
  if (m_status.M_running) {
    failure_fire = 1;
    failure_flame = 1;
  }
  is_absence = false;
  m_update_absence = false;
  
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
  
  // ��λƽ���л�
  // �Ƚ��Ϻ󽵷磬�ȼ��Ϻ�ӷ�
  uint16_t v = 1;
  if (M_SwitchDelayHandle != NULL) {
    osMessagePut(M_SwitchDelayHandle, (uint32_t)v, osWaitForever);
  }
  info("smooth delay reach");
  /* USER CODE END callback_switch_delay */
}

/* callback_k_validate function */
void callback_k_validate(void const * argument)
{
  /* USER CODE BEGIN callback_k_validate */
  
  /* �ж�K�Ƿ���Ч */
  stage_5();
  // ֹͣ���ʱ��ʱ��
  osTimerStop(tmr_fireTimeoutHandle);
  strcpy(info_stage, "\x98\x99 5");  // �׶�״̬������  
  
  /* USER CODE END callback_k_validate */
}

/* callback_feed_manual function */
void callback_feed_manual(void const * argument)
{
  /* USER CODE BEGIN callback_feed_manual */
  
  // ÿ1s
  t_feed_manual++;
  
  /* USER CODE END callback_feed_manual */
}

/* USER CODE BEGIN Application */
// ������
void beep(uint8_t *beep_count)
{  
  if(*beep_count > 0) {
    GPIOE->BSRR = GPIO_PIN_15;
    osDelay(50);
    GPIOE->BSRR = (uint32_t)GPIO_PIN_15 << 16; 
    (*beep_count)--;
    if (*beep_count > 0) {
      osDelay(150);
    }
  }
}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/