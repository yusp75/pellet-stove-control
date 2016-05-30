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

#define N_KTEMP 4  // 平均值
osMessageQId Q_ErrorHandle;  // err

// 外部变量
//extern IWDG_HandleTypeDef hiwdg;
extern TIM_HandleTypeDef htim3;
extern uint8_t rx_buffer[10];
extern bool transfer_cplt;

// 炉子参数
// Stove mystove;
/*可控硅消息*/
osPoolDef(pool_triac, 1, Triac); // Define Triac 内存池
osPoolId  pool_TriacHandle;
osMessageQId Q_TriacHandle;

/*炉子消息*/
osPoolDef(pool_data, 1, Data); // Define Stove 内存池
osPoolId  pool_DataHandle;
osMessageQId Q_DataHandle;

// Zero内存池
osPoolDef(pool_zero, 5, Zero);
osPoolId pool_ZeroHandle;

/*热电偶K 消息*/
osPoolDef(pool_K, 1, K_Value); // Define Stove 内存池
osPoolId  pool_KHandle;
osMessageQId Q_KHandle;

/* 结构实例 */
M_Status m_status;
RTC_Time rtc_time_read = {0};
RTC_Time rtc_time = {0};

/* Triac */
__IO uint16_t fan_smoke_delay = 3;
__IO uint16_t fan_exchange_delay = 3;
__IO uint8_t feed_duty = 30;
__IO bool flag_full_blow = false;  // 排烟全速3秒
__IO bool flag_full_exchange = false;  // 循环全速3秒

/* 配方 */
uint8_t recipe_index = 0;
int8_t recipe[RECIPE_DATA_LEN] = {
  30, 45, 60,
  35, 50, 65,
  40, 55, 60
};

RECIPE recipe_cur;  // 当前配方

/* 排烟功率 */
uint8_t power_of_blow[3] = {1, 2, 2};  // 50 + x * 5

/* 时钟显示 */
char *p_rtc_display;
char rtc_display[30]="\0";

/* 计划 */
Schedule plan[3] = {
  {8, 0, 12, 0, 13, 0, 18, 0, 1},
  {8, 0, 12, 0, 13, 0, 18, 0, 1},
  {8, 0, 12, 0, 13, 0, 18, 0, 1}
};


char info_stage[20] = "\x88\x89";  // 初始： 停止
char *p_info_stage;

// 状态字符串
char info_message[10] = "\0";
char *p_info_message;
/* 温度cv, pv字符串 */
char info_cv_pv[30] = "\0";
char *p_info_cv_pv;

/* 运行状态参数 */

/*显示： 预约*/
char schedule_time[30] = "\x8c\x8d\x8e 08:00-12:00";
char *p_schedule_time;
bool auto_or_manual = false;  // 自动 or 手动
bool m_stoped = false;
bool m_started = false;

/* 状态屏幕 */
//uint8_t status_battery;

char info_blow[20]="\0";
char *p_info_blow;
char info_blow_2[30]="\0";
char *p_info_blow_2;
char info_feed_ex[20]="\0";  // 送料，循环
char *p_info_feed_ex;

// 状态屏幕 extern
M2_EXTERN_VLIST(top_status);
M2_EXTERN_VLIST(el_top_home);

/* 缺料 */
// 蜂鸣计数
uint8_t beep_count = 0;
bool is_absence = false;
bool m_update_absence = false;  // 更新缺料保存标志

// 点火失败计数， 熄火计数
__IO uint8_t failure_fire = 1;
__IO uint8_t failure_flame = 1;
__IO bool m_failure_fire_2nd = false;
__IO bool m_failure_flame_2nd = false;

// 送料
// 1）手动
bool feed_by_manual = false;
char feed_progress[6] = "0%";
char *p_feed_progress;
uint32_t t_feed_manual = 0;
// 2）
bool feed_full = false;


/* 自定义变量结束-------------------------------------------------------------*/

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
void beep(uint8_t *beep_count);  // 蜂鸣器
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
  
  /*自定义消息队列*/
  //Triac内存池,消息
  pool_TriacHandle = osPoolCreate(osPool(pool_triac));
  
  osMessageQDef(Q_Triac, 1, Triac);
  Q_TriacHandle = osMessageCreate(osMessageQ(Q_Triac), NULL);
  //Stove内存池,消息
  pool_DataHandle = osPoolCreate(osPool(pool_data));
  
  osMessageQDef(Q_Data,1,Data);
  Q_DataHandle = osMessageCreate(osMessageQ(Q_Data), NULL);
  
  // err 消息
  osMessageQDef(Q_Error,5,Zero);
  Q_ErrorHandle = osMessageCreate(osMessageQ(Q_Error), NULL);
  
  // zero 消息
  pool_ZeroHandle = osPoolCreate(osPool(pool_zero));
  
  // 热电偶K消息
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
  
  // bool readable = false;  // 每2秒读取温度
  // ds18b20设置精度
  therm_reset();
  therm_setResolution(RESOLUTION);
  
  /* Infinite loop */
  for(;;)
  {
    // 运行指示灯
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
   
    /* 更新stove数据 */
    // 分配stove内存，函数StartTaskScanKey中返还
    if (pool_DataHandle != NULL) {
      Data *p_data = (Data*)osPoolAlloc(pool_DataHandle);
      
      if (p_data != NULL) {
        //室温， 炉温
        p_data->cv = (int16_t)cv;
        //p_data->k = k_a; // temp_k
        
        // 放入队列
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
  
  KeyPressed key_pressed;  // 按键
  bool need_update = false;  // 是否刷新显示
  uint8_t pv = 9;  // 设置的温度
  
  // 达到运行的K温度是否有效
  bool m_k_is_valid = false;
  
  uint8_t read_rtc_counter = 0;
  
  In_Schedule_Time in_schedule_time;  // 在计划内
  
  //__IO bool m_started = false;
  bool m_start_cooling = false;
  bool m_stop_cooling = false;
  bool m_idle_cooling = false;
  
  // stove 变量
  Stove stove = {0};  
  
  bool schedule_start_stove = false;  //  计划模式启动炉子
  
  // 屏幕号
  uint8_t screen_no = 0;
  
  // 星期1-7 字符
  char weekday_chr[] = {'\x93', '\xa1','\xa2','\xa3','\xa4','\xa5','\xa6'};
  osStatus status;
  
  // 字符串关联指针
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
  
  uint16_t status_k_temp = 0;  // 临时K温度
  
  bool in_testing = false;  // 测试中
  uint16_t v_tesing;  // 串口拿到的测试速度
  
  bool started_after_cooling = false;
  // 热电偶
  K_Value v_max6675;
  // K 滤波变量
  uint16_t k_sum = 0, k_a = 0;
  
  // 变量定义结束
  /*--------------------------------------------------------------------------*/
  // 读存储的运行状态：是否计划，前次缺料熄火
  init_data();  
  
  /* 读取存储的配方， 计划， 状态数据 */
  // 读配方
  read_recipe(recipe);
  // 读配方索引
  if (!read_from_rtc(RTC_ADDR_RECIPE_INDEX, &recipe_index, 1)) {
    recipe_index = 0;
  }
  if (recipe_index > 2) {
    recipe_index = 0;
  }
  // 当前配方
  recipe_cur.level_0 = recipe[recipe_index*3];
  recipe_cur.level_1 = recipe[recipe_index+1];
  recipe_cur.level_2 = recipe[recipe_index+2];
  // 
  // 读计划  
  read_schedule(&plan[0], 0);
  read_schedule(&plan[1], 1);
  read_schedule(&plan[2], 2);
  // 读排烟
  read_from_rtc(RTC_ADDR_POW, power_of_blow, 3);
  // 读温度 PV
  read_from_rtc(RTC_ADDR_PV, &pv, 1); 
  
  // 读预约
  read_from_rtc(RTC_ADDR_SCHEDULE_STATE, (uint8_t*)&auto_or_manual, 1);
  
  // 改RTC到24小时制
  //  read_rtc(&rtc_time);
  //  if (rtc_time.flag_12or24 == 0) {
  //    if (rtc_time.am_or_pm) {
  //      rtc_time.hour += 12;
  //    }
  //    write_rtc(&rtc_time);
  //  }
  
  // 读保存数据结束  
  /*--------------------------------------------------------------------------*/  
  // 显示
  init_draw();
  hmi();
  
  // 阶段初始化
  stage_init();
  
  /* Infinite loop */
  for(;;)
  { 
    // 1秒读rtc
    read_rtc_counter++;
    if (read_rtc_counter >= 49) {
      read_rtc_counter = 0;
      read_rtc(&rtc_time_read);
      // 电池电量
      //status_battery = rtc_time_read.quantity;
    }
    
    /* k电偶  滤波
    * 初始化：A=初始值，S=A*N
    * S = S - A + C
    * A = S / N
    */
    // 从消息里拿取K
    osEvent evt_k = osMessageGet(Q_KHandle,0);
    
    if (evt_k.status == osEventMessage) {
      K_Value *p_k = (K_Value*)evt_k.value.p;
      v_max6675.v = p_k->v;
      v_max6675.error = p_k->error;
      // 释放memory
      osPoolFree(pool_KHandle, (K_Value *)evt_k.value.p);      
      
      if (v_max6675.error == 0) {
        if (k_sum == 0) {
          k_sum = v_max6675.v * N_KTEMP;
        }
        else {
          k_sum = k_sum - k_a + v_max6675.v;
        }
          k_a = k_sum / N_KTEMP;
        // 更新到显示界面
        status_k_temp = k_a; 
        data.k = k_a;  // 存数据k
      }
    }
    // get
    
    // 扫描按键
    Scan_Keyboard();
    key_pressed = getKey();
    // 有IR按键？
    if (key_pressed == Key_None) {
      //IR 低于键盘
      key_pressed = getIrKey();
    }
    
    /*------------------------------------------------------------------------*/
    // 发声： 按键
    if (key_pressed != Key_None) {
      if (Q_BeepHandle != NULL) {
        osMessagePut(Q_BeepHandle, (uint32_t)BEEP_BUTTON, 0);
      }
    }
    
    /*------------------------------------------------------------------------*/
    /* 读取炉数据 */
    evt_data = osMessageGet(Q_DataHandle, 0);
    if (evt_data.status == osEventMessage) {
      Data *p_data = (Data *)evt_data.value.p;
      stove.data = *p_data;
      data.cv = p_data->cv;
      // 更新K到状态屏幕 ------（1) 
      //status_k_temp = data.k;
      // 读取屏幕号
      if (get_screen_no() != screen_no) {
        screen_no = get_screen_no();
        //set_pin_key(screen_no);  // 更新按键绑定 ---》移走
      }
      
      // 不在修改时间屏幕时，刷新时间显示
      if (screen_no != SCREEN_RTC) {
        // 时间格式 9:00 周六
        rtc_time = rtc_time_read;
        sprintf(rtc_display, "%02d:%02d \x95%c", rtc_time.hour, rtc_time.minute, 
                weekday_chr[rtc_time.week]);
      }
      
      // 返还内存
      status = osPoolFree(pool_DataHandle, p_data);
      if (status != osOK)
        printf("Free Data memory error:%x\r\n", status);
      
      // 刷新屏幕： 时间
      need_update = true;
    }    
    
    /*------------------------------------------------------------------------*/
    
    /* 按键预处理， 放入键队列 */
    if (key_pressed != Key_None) {
      //printf("key:%d\r\n", key_pressed);
      need_update = true;
      
      /* 1、拦截窗1的电源按键 */
      // 若运行停止，反之阶段1
      if (is_home_screen()) {
        
        /* 1、 电源按键 */        
        if (!auto_or_manual) {
          if ( key_pressed==Key_Power) {
            if (m_status.stage!=-1) {
              m_stoped = true;
              // 停掉（全局）全速送料
              feed_full = false;
            }
            else {
              m_started = true; 
              // 手动按键，可以2次启动
              failure_fire = 1;
              failure_flame = 1;
            } 
            key_pressed = Key_None;
          }
        }
        
        /* 2、计划模式按键 */
        if (key_pressed == Key_Mode) {
          // 自动
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
          // 更新RTC保存
          save_schedule_state(auto_or_manual);
          
          // 大火，烧水
          if (m_status.stage==6 && !stove.boil_mode) {
            stove.boil_mode = true;
            // 显示 烧水图标？
            strcpy(info_stage, "\xc0\xad");  // info 烧水
            // 启动烧水模式超时定时器
            osTimerStart(tmr_boil_timeoutHandle, 50 * 60 * 1000);  // 50分钟            
          }
          else if (m_status.stage==6 && stove.boil_mode) {
            stove.boil_mode = false;
            strcpy(info_stage, "\x98\x99");  //info 运行
          }
          
          key_pressed = Key_None; 
        }
        // 正手动运行时，不能切换到计划
        
        /* 3、拦截窗1的↑↓按键 */
        if (key_pressed == Key_Up && pv < PV_HIGH) {
          pv++;
        }
        else if (key_pressed == Key_Down && pv > PV_LOW) {
          pv--;  
        }
        
        if (key_pressed == Key_Down || key_pressed == Key_Up) {
          
          key_pressed = Key_None; // 使键为空
          // 保存 pv
          save_to_rtc(RTC_ADDR_PV, &pv, 1);
          
        }
        
        
        /* 4、 拦截窗1的 ← */
        
        if (key_pressed==Key_Left) {
          m2_SetRoot(&top_status);
          key_pressed = Key_None;
        }
        // 刷新界面： 按键 
        need_update = true;
      }  // home screen
      
      // 状态屏幕
      if (is_status_screen()) {
        if (key_pressed==Key_Right || key_pressed==Key_Menu) {
          m2_SetRoot(&el_top_home);
        }
        key_pressed = Key_None;
      }
      
      /* 5、是否是 menu home key */
      if (key_pressed == Key_Menu) {
        change_root();
        key_pressed = Key_None; // 使键为空
      }
      
    } 
    /* 按键预处理 end  */
    /*------------------------------------------------------------------------*/
    // *
    // 刷新界面温度
    sprintf(info_cv_pv, "\x8a\x8b:%d \x9a\x9b: %d",stove.data.cv, pv);
    sprintf(info_blow, "\xbb\xbc\x8b\xbe: [%d]", status_k_temp);
    
    /* 预约时间检查 */
    if (auto_or_manual) {
      Schedule plan_cur;
      // 根据星期拿到计划
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
      // 显示预约      
      sprintf(schedule_time, "%s %02d:%02d - %02d:%02d", "\x8d\x8e", 
              in_schedule_time.start_hour,
              in_schedule_time.start_min,
              in_schedule_time.end_hour,
              in_schedule_time.end_min
                ); 
      if (!m_failure_fire_2nd && !m_failure_flame_2nd) {
        if (schedule_start_stove && m_status.stage==-1) {  // 计划内
          m_started = true;
        }
      }
      
      if (!schedule_start_stove && m_status.stage!=-1){  // 出到计划外
        m_stoped = true;
      }
      //  
    }
    else { //无预约
      sprintf(schedule_time, "%s", "\x8c\x8d\x8e");
      
    }
    
    /* 状态处理 */
    /*----------------------------------------------------------------------*/
    // --- 65℃时需要冷却
    //（1）
    if ( m_started && (!m_start_cooling) && data.k>=TEMP_COOLING) {
      m_start_cooling = true;
      m_started = false;
      
      strcpy(info_stage, "\xc3\xc2: \xc4\xc5"); // 启动：冷却
    }
    //（2）
    if (m_stoped && (!m_stop_cooling) && data.k>=TEMP_COOLING) {
      m_stop_cooling = true;
      m_stoped = false;
      
      strcpy(info_stage, "\x88\x89: \xc4\xc5"); // 停止：冷却
    }
    
    //（3）
    if (m_status.M_idle && (!m_idle_cooling) && data.k>=TEMP_COOLING) {
      m_idle_cooling = true;
      strcpy(info_stage, "\xc4\xc5"); // 空闲：冷却
    }
    
    if ((m_start_cooling || m_stop_cooling || m_idle_cooling) && !m_status.M_cooling) {
      stage_0();
      fan_smoke_delay = power_of_blow[2];  // 1 *风速*
      fan_exchange_delay = 1;
    }      
    //
    if (((m_started || started_after_cooling) && !m_start_cooling) || started_after_cooling) {
      // 启动前 init：
      
      m_k_is_valid = false; // 1）m_k_is_valid需置0， 对于第2次启动      
      stove.next_is_delaying = false; // 2）延迟换料标志置0      
      stove.level = 0; // 3）燃烧等级      
      stove.boil_mode = false; // 4）烧水模式
      // 5）启动送料为55      
      fan_smoke_delay = power_of_blow[2]; // 阶段1排烟风速，取等级3值
      
      // 阶段1
      stage_1();
      m_started = false; 
      started_after_cooling = false;
      strcpy(info_stage, "\x80\x81");  // 阶段状态：点火       
    }
    // 停止 ***
    if (m_stoped && !m_stop_cooling) {
      m_stoped = false;
      stop("normal");
      strcpy(info_stage, "\x88\x89");  // 阶段状态：停止
    }
    
    // 冷却，温度低于60℃时，停止 吹烟和循环
    // 启动前冷却，则启动
    if (data.k<=(TEMP_RUN-10) && m_status.M_cooling) {
      if (m_start_cooling) {
        started_after_cooling = true;
        
        m_start_cooling = false;
        printf("after cooling, start\r\n");
      }
      
      if (m_stop_cooling) { 
        stop("cooling for stop");
        strcpy(info_stage, "\x88\x89");  // 阶段状态：停止
        m_stop_cooling = false;
        printf("after cooling, stop\r\n");
      }
      
      if (m_idle_cooling) {
        stop("cooling for stop");
        strcpy(info_stage, "\x88\x89");  // 阶段状态：停止
        m_idle_cooling = false;
        printf("idle cooling, stop\r\n");
      }
      
      m_status.M_cooling = false;
    }
    
    // 燃烧中熄火，原因： 缺料
    if (m_status.M_running && data.k<=FLAME_DOWN) {
      
      is_absence = true;
      m_update_absence = true;
      // 显示缺料
      strcpy(info_stage, "\xac\x9d");  // 缺料
      // error
      
      // 蜂鸣器 10s
      if (Q_BeepHandle != NULL) {
        osMessagePut(Q_BeepHandle, (uint32_t)BEEP_ABSENCE, 0);
      }
      
      // 存在2次启动
      if (failure_flame > 0) {
        // 停止
        stop("flame failure stop");
        m_started = true;
        // 启动
        failure_flame--;  
        // 延迟一会儿
        // osDelay(TRY_DELAY);
      }
      else {
        m_stoped = true; 
        m_failure_flame_2nd = true;
      }
      
      // 5分钟后关闭
    }
    
    // 保存/更新 缺料状态
    if (m_update_absence) {        
      // 保存
      save_to_rtc(RTC_ADDR_ABSENCE, (uint8_t*)&is_absence, 1);
      m_update_absence = false;
    }
    // 若是正常运行，则取消缺料状态
    
    // 串口测试
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
    
    // 更新按键绑定
    set_pin_key(screen_no);  
    m2_CheckKey(); 
    
    /*------------------------------------------------------------------------*/    
    // 判断烟气温度是否到70℃
    // 1分钟内依旧大于70℃，则可以判定为运行
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
    
    // 输出 点火棒
    if (m_status.M_fire)
      HAL_GPIO_WritePin(PORT_FIRE_ROD, FIRE_ROD, GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(PORT_FIRE_ROD, FIRE_ROD, GPIO_PIN_RESET);
    
    /*------------------------------------------------------------------------*/
    
    // 手动送料
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
    
    // 分配triac内存, triac.c中返还
    if (pool_TriacHandle != NULL) {
      Triac *p_triac = (Triac *)osPoolAlloc(pool_TriacHandle);      
      
      /* 根据炉温，室温判断送料速度、排烟速度 */
      if (p_triac != NULL) {    
        // 运行，送料比例和风速动态改变
        if (m_status.M_running) {  
          // 送料比例
          recipe_cur.level_0 = recipe[recipe_index*3];
          recipe_cur.level_1 = recipe[recipe_index*3+1];
          recipe_cur.level_2 = recipe[recipe_index*3+2];
          
          stove.pv = pv;  // 读入pv
          
          // 根据△T决定风速和料速
          if (!stove.next_is_delaying) {
            judge(&stove, p_triac, &recipe_cur, power_of_blow);
          }
          
          // -检查延迟消息------
          osEvent evt_switch = osMessageGet(M_SwitchDelayHandle, 0);
          if (evt_switch.status == osEventMessage) {
            stove.next_is_delaying = false;
            //p_triac->fan_smoke_power = stove.next_v;  //风晚于料做改变
            fan_smoke_delay = stove.next_v;
          }
          //
        }
        
        // 更新送料、排烟到状态屏幕  ------ （2）      
        sprintf(info_blow_2, "\x9c\xbf\xbd\xbe: [%d]", p_triac->fan_smoke_power);
        sprintf(info_feed_ex, "\x9c\x9d\xbd\xbe: [%d]", p_triac->feed_duty); 
        
        /*----------------------------------------------------------------------*/ 
        
        // 阶段1： 炊烟
        if (m_status.stage==1) {
          fan_smoke_delay = power_of_blow[2];  
        }
        
        // 发送消息
        p_triac->fan_smoke_is_on = m_status.M_blow;      
        p_triac->fan_exchange_is_on = m_status.M_exchange;
        p_triac->feed_is_on = m_status.M_feed;
        
        p_triac->feed_by_manual = feed_by_manual;
        p_triac->feed_full = feed_full;
        
        p_triac->fan_smoke_power = fan_smoke_delay;  // + 04.24 
        
        
        // 预送料阶段
        if (m_status.stage == 2) {
          p_triac->feed_duty = feed_duty;  // todo: 通过消息传递
        }
        
        // triac 放入消息队列
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
    
    /* 刷新显示 */
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
    
    // 显示送料进度
    if (is_feed_manual_screen()) {
      uint16_t w= t_feed_manual/6;
      u8g_DrawBox(&u8g, 10,10,50, w);
      u8g_DrawStr(&u8g, 20, 20, "hi");
    }
    
    // 刷新IWDG
    //HAL_IWDG_Refresh(&hiwdg);
    
    osDelay(20);
  }
  // 扫描键部分 end
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
    // 取响铃信息
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
  
  // 段2 开始送料70秒定时器
  stage_2();
  feed_duty = 20;  // 3/10
  feed_full = true;
  /* USER CODE END callback_blowDust */
}

/* callback_preFeed function */
void callback_preFeed(void const * argument)
{
  /* USER CODE BEGIN callback_preFeed */
  // 预送料结束
  // 段3 开始预加热
  feed_full = false;
  stage_3();
  
  /* USER CODE END callback_preFeed */
}

/* callback_fireTimeout function */
void callback_fireTimeout(void const * argument)
{
  /* USER CODE BEGIN callback_fireTimeout */
  // 点火失败
  //alarm
  HAL_GPIO_WritePin(PORT_FIRE_ROD, FIRE_ROD, GPIO_PIN_RESET);
  printf("fire is failure.\r\n");
  // 停止点火超时定时器
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
  // 段4 加热4分钟后，开始送料
  
  stage_4();
  
  /* USER CODE END callback_beforeFeed */
}

/* callback_delayRunning function */
void callback_delayRunning(void const * argument)
{
  /* USER CODE BEGIN callback_delayRunning */
  
  // 点火成功，延迟一会儿
  stage_6();
  // 已经运行，更新
  // 手动按键，可以2次启动
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
  // 排烟风扇全速时间到
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
  // 烧水模式超时退出
  ((Stove*)argument)->boil_mode = false;
  /* USER CODE END callback_boil_timeout */
}

/* callback_switch_delay function */
void callback_switch_delay(void const * argument)
{
  /* USER CODE BEGIN callback_switch_delay */
  
  // 档位平滑切换
  // 先降料后降风，先加料后加风
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
  
  /* 判定K是否有效 */
  stage_5();
  // 停止点火超时定时器
  osTimerStop(tmr_fireTimeoutHandle);
  strcpy(info_stage, "\x98\x99 5");  // 阶段状态：运行  
  
  /* USER CODE END callback_k_validate */
}

/* callback_feed_manual function */
void callback_feed_manual(void const * argument)
{
  /* USER CODE BEGIN callback_feed_manual */
  
  // 每1s
  t_feed_manual++;
  
  /* USER CODE END callback_feed_manual */
}

/* USER CODE BEGIN Application */
// 蜂鸣器
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
