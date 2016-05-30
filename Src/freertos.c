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
#include "queue_log.h"

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

// 外部变量
//extern IWDG_HandleTypeDef hiwdg;
extern TIM_HandleTypeDef htim3;
extern uint8_t rx_buffer[10];
extern bool transfer_cplt;
extern UART_HandleTypeDef huart2;

// 炉子参数
// Stove mystove;
/* 可控硅消息 */
osPoolDef(pool_triac, 1, Triac); // Define Triac 内存池
osPoolId  pool_TriacHandle;
osMessageQId Q_TriacHandle;

/* 炉子消息 */
osPoolDef(pool_data, 1, Data); // Define Stove 内存池
osPoolId  pool_DataHandle;
osMessageQId Q_DataHandle;

// Zero内存池
//osPoolDef(pool_zero, 1, Zero);
//osPoolId pool_ZeroHandle;
//
//osMessageQId Q_ErrorHandle;  // err

/* 热电偶K 消息 */
osPoolDef(pool_K, 1, K_Value); // Define Stove 内存池
osPoolId  pool_KHandle;
osMessageQId Q_KHandle;

/* 定时器消息 */
osMessageQId Q_TimerHandle;

/* 日志消息/内存池定义 */
osMessageQId Q_LogHandle;

osPoolDef(pool_Log, 12, Node);  // 12
osPoolId pool_LogHandle;

/* 超声波消息 */
osMessageQId Q_SonicDataHandle;

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
__IO bool auto_or_manual = false;  // 自动 or 手动
__IO bool m_stoped = false;
__IO bool m_started = false;

/* 状态屏幕 */
//uint8_t status_battery;

char info_blow[20] = "\0";
char *p_info_blow;
char info_blow_2[30] = "\0";
char *p_info_blow_2;
char info_feed_ex[20] = "\0";  // 送料，循环
char *p_info_feed_ex;

char info_pellet[10] = "\x9d: 0";  // 剩料
char *p_info_pellet;

// 状态屏幕 extern
M2_EXTERN_VLIST(top_status);
M2_EXTERN_VLIST(el_top_home);
M2_EXTERN_HLIST(el_top_log);
M2_EXTERN_XYLIST(el_top_pellet);
/* 缺料 */
// 蜂鸣计数
__IO uint8_t beep_count = 0;
__IO bool is_absence = false;
__IO bool m_update_absence = false;  // 更新缺料保存标志
// 缺料xbmp
bool pellet_alarm_once = true;

// 点火失败计数， 熄火计数
__IO uint8_t failure_fire = 1;
__IO uint8_t failure_flame = 1;
__IO bool m_failure_fire_2nd = false;
__IO bool m_failure_flame_2nd = false;

// 送料
// 1）手动
__IO bool feed_by_manual = false;
char feed_progress[6] = "0%";
char *p_feed_progress;
__IO uint32_t t_feed_manual = 0;
// 2）
__IO bool feed_full = false;


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
  osThreadDef(defaultTask, StartDefaultTask, osPriorityBelowNormal, 0, 512);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of TaskScanKey */
  osThreadDef(TaskScanKey, StartTaskScanKey, osPriorityNormal, 0, 1024);
  TaskScanKeyHandle = osThreadCreate(osThread(TaskScanKey), NULL);

  /* definition and creation of TaskBeep */
  osThreadDef(TaskBeep, StartTaskBeep, osPriorityNormal, 0, 128);
  TaskBeepHandle = osThreadCreate(osThread(TaskBeep), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the queue(s) */
  /* definition and creation of Q_Key */
  osMessageQDef(Q_Key, 4, uint32_t);
  Q_KeyHandle = osMessageCreate(osMessageQ(Q_Key), NULL);

  /* definition and creation of Q_Ir */
  osMessageQDef(Q_Ir, 1, uint32_t);
  Q_IrHandle = osMessageCreate(osMessageQ(Q_Ir), NULL);

  /* definition and creation of Q_Beep */
  osMessageQDef(Q_Beep, 4, uint8_t);
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
  //  osMessageQDef(Q_Error,1,Zero);
  //  Q_ErrorHandle = osMessageCreate(osMessageQ(Q_Error), NULL);
  
  // zero 消息
  //  pool_ZeroHandle = osPoolCreate(osPool(pool_zero));
  
  // 热电偶K消息
  pool_KHandle = osPoolCreate(osPool(pool_K));  
  osMessageQDef(Q_K, 1, K_Value);
  Q_KHandle = osMessageCreate(osMessageQ(Q_K), NULL);
  
  
  // 定时器Timer消息
  osMessageQDef(Q_Timer, 5, uint32_t);
  Q_TimerHandle = osMessageCreate(osMessageQ(Q_Timer), NULL);
  
  
  // 创建日志消息/内存池
  osMessageQDef(Q_Log, 6, uint32_t);
  Q_LogHandle = osMessageCreate(osMessageQ(Q_Log), NULL);
  
  pool_LogHandle = osPoolCreate(osPool(pool_Log));
  
  // 超声波数据消息
  osMessageQDef(Q_SonicData, 2, uint16_t);
  Q_SonicDataHandle = osMessageCreate(osMessageQ(Q_SonicData), NULL);
  
  // 0-日志队列初始化
  InitLogQueue();  
  
  /* USER CODE END RTOS_QUEUES */
}

/* StartDefaultTask function */
void StartDefaultTask(void const * argument)
{

  /* USER CODE BEGIN StartDefaultTask */
  
  osStatus status;
  ThermValue v_ds18b20 = {0, 0}; 
  uint8_t cv = 0;
  
  stove_log(0);  // 欢迎您
  
  bool start_conversion = false;  // 开始转换
  // bool onec_check_ds18b20 = true;  // 检查ds18b20一次
  // bool exist_ds18b20 = false;  // ds18b20是否存在
  // bool readable = false;  // 每2秒读取温度
  // ds18b20设置精度
  if (therm_reset()) {
    therm_setResolution(RESOLUTION);
    // exist_ds18b20 = true;  
  }
  else {
    stove_log(11);  // 11-无ds18b20
  }
  //
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);  // 开启Pin15中断
  
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
  __IO bool m_k_is_valid = false;
  
  uint8_t read_rtc_counter = 0;
  
  In_Schedule_Time in_schedule_time;  // 在计划内
  
  //冷却状态
  __IO bool m_start_cooling = false;
  __IO bool m_stop_cooling = false;
  __IO bool m_idle_cooling = false;  
  
  // 检查K热电偶一次
  bool once_check_K = true;
  
  // stove 变量
  Stove stove = {0};  
  
  __IO bool schedule_start_stove = false;  //  计划模式启动炉子
  
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
  p_info_pellet = &info_pellet[0];
  
  // 
  __IO Data data = {0, 0}; 
  osEvent evt_data;
  
  uint16_t status_k_temp = 0;  // 临时K温度
  
  bool in_testing = false;  // 测试中
  uint16_t v_tesing;  // 串口拿到的测试速度
  
  bool started_after_cooling = false;
  // 热电偶
  K_Value v_max6675 = {0, 0};
  // K 滤波变量
  uint16_t k_sum = 0, k_a = 0;    
  
  // 超声波检测
  uint16_t pellet_consume_distance = 0;// 已经消耗的料距离
  uint16_t check_interval = 0;  // 每10秒检测1次
  uint8_t command_sonic = 0x55;
  
  uint16_t pellet_sum = 0, pellet_a = 0;
  uint16_t pellet_alarm_delay = 0;
  
  // 变量定义结束
  /*--------------------------------------------------------------------------*/
  
  // 改RTC到24小时制
  read_rtc(&rtc_time_read);
  //  if (rtc_time.flag_12or24 == 0) {
  //    if (rtc_time.am_or_pm) {
  //      rtc_time.hour += 12;
  //    }
  //    write_rtc(&rtc_time);
  //  }
  
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
  
  // 读保存数据结束  
  /*--------------------------------------------------------------------------*/  
  // 显示
  init_draw();
  hmi();
  
  stage_init(); // 阶段初始化  
  
  /* Infinite loop */
  for(;;)
  {     
    /* 每1秒读rtc */
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
      
      // log 热电偶断线
      if (v_max6675.error==1 && once_check_K) {
        stove_log(10);
        once_check_K = false;
      }
    }    
    
    
    /* 日志信息处理 */
    osEvent evt_log = osMessageGet(Q_LogHandle, 0);
    if (evt_log.status == osEventMessage) {
      uint8_t log_v = evt_log.value.v;
      
      push(log_v, rtc_time_read.minute, rtc_time_read.hour);
    }
    
    /* 扫描按键 */
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
        snprintf(rtc_display, 30, "%02d:%02d \x95%c", rtc_time.hour, rtc_time.minute, 
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
        if (!auto_or_manual) {  // 非预约时
          if (key_pressed==Key_Power) {
            // 停止
            if (m_status.stage>=1) {
              m_stoped = true;
              m_started = false; 
              
              // 停掉（全局）全速送料
              feed_full = false;
            }
            
            // 启动
            if(m_status.stage==-1) {
              m_started = true; 
              m_stoped = false;
              
              // 手动按键，可以2次启动
              failure_fire = 1;
              failure_flame = 1;
            } 
            
            key_pressed = Key_None;
          }
        }
        else {  // 预约时
          if (key_pressed==Key_Power) {            
            key_pressed = Key_None;
            schedule_exit_confirm();            
          }
          
        }
        
        /* 2、计划模式按键 */
        if (key_pressed == Key_Mode) {
          // 自动
          if (m_status.stage==-1 && !auto_or_manual) {
            auto_or_manual = true; 
            save_schedule_state(auto_or_manual); // 保存是否预约到RTC
          }
          //          else if (auto_or_manual) {            
          //            key_pressed = Key_None;
          //            schedule_exit_confirm();            
          //          }
          
          // 大火，烧水
          if (m_status.stage==6 && !stove.boil_mode) {
            stove.boil_mode = true;
            // 显示 烧水图标？
            //strcpy(info_stage, "\xc0\xad");  // info 烧水
            // 启动烧水模式超时定时器
            osTimerStart(tmr_boil_timeoutHandle, 50 * 60 * 1000);  // 50分钟            
          }
          else if (m_status.stage==6 && stove.boil_mode) {
            stove.boil_mode = false;
            //strcpy(info_stage, "\x98\x99");  //info 运行
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
        
        
        /* 5、 拦截窗1的 → */
        if (key_pressed==Key_Right) {
          m2_SetRoot(&el_top_log);
          key_pressed = Key_None;
        }
        
        // 刷新界面： 按键 
        need_update = true;        
        
      }  // home screen
      
      // 状态/日志 屏幕
      switch (is_status_or_log_screen()) {
      case 1:
        if (key_pressed==Key_Menu) {
          m2_SetRoot(&el_top_home);
        }
        key_pressed = Key_None;
        break;
        
      case 2:
        if (key_pressed==Key_Menu) {
          m2_SetRoot(&el_top_home);
        }
        if (key_pressed==Key_Up) {
          fn_log_up_down(0);
        }
        
        if (key_pressed==Key_Down) {
          fn_log_up_down(1);
        }     
        key_pressed = Key_None;
      }
      
      
      /* 6、是否是 menu home key */
      if (key_pressed == Key_Menu) {
        change_root();
        key_pressed = Key_None; // 使键为空
      }
      
    } 
    /* 按键预处理 end  */
    /*------------------------------------------------------------------------*/
    // *
    // 刷新界面温度
    snprintf(info_cv_pv, 30, "\x8a\x8b:%d \x9a\x9b: %d",stove.data.cv, pv);
    snprintf(info_blow, 20, "\xbb\xbc\x8b\xbe: [%d]", status_k_temp);
    
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
      snprintf(schedule_time, 30, "%s %02d:%02d-%02d:%02d", "\x8d\x8e", 
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
      snprintf(schedule_time, 30, "%s", "\x8c\x8d\x8e");
      
    }
    
    /* 颗粒料量检测 */
    if (pellet_consume_distance >= PELLET_WARN ) {
      stove.pellet_warn = true;
      // alert
      if (pellet_alarm_once) {
        pellet_alarm_delay++;
        
        if (pellet_alarm_delay>= 499) {
          m2_SetRoot(&el_top_pellet);
          osMessagePut(Q_BeepHandle, (uint32_t)BEEP_ABSENCE, 0);
          pellet_alarm_delay = 0;
        }
      }
    }
    else {
      stove.pellet_warn = false;
      pellet_alarm_once = true;
      pellet_alarm_delay = 0;
    }    
    
    if (pellet_consume_distance >= PELLET_MIN ) {
      m_stoped = true;
      // alert
    }
    
    /* 状态处理 */
    /*----------------------------------------------------------------------*/
    // 热机：启动/停止前，如果大于 60℃时需要冷却
    //（1）
    if (m_started && !m_start_cooling && data.k>=COOLING_STOP) {
      m_start_cooling = true;
      m_started = false;
      stove_log(7);
    }
    
    //（2）
    if (m_stoped && !m_stop_cooling && data.k>=COOLING_STOP) {
      m_stop_cooling = true;
      m_stoped = false;
      stove_log(8);
    }
    
    //（3）
    if (m_status.M_idle && !m_idle_cooling && data.k>=TEMP_RUN) {
      m_idle_cooling = true;
      strncpy(info_stage, "\xc4\xc5", 20); // 空闲：冷却
      stove_log(9);
    }
    
    if ((m_start_cooling || m_stop_cooling || m_idle_cooling) && !m_status.M_cooling) { 
      stage(0);
      fan_smoke_delay = power_of_blow[2];  // 1 *风速*
      fan_exchange_delay = 1;
    }   
    
    
    // 冷却，温度低于60℃时，停止 吹烟和循环
    // 若启动前冷却，则启动
    if (data.k<COOLING_STOP && m_status.M_cooling) {
      if (m_start_cooling) {
        started_after_cooling = true;        
        m_start_cooling = false;
        
      }
      
      if (m_stop_cooling) { 
        stop("cooling for stop");
        m_stop_cooling = false;
        
      }
      
      if (m_idle_cooling) {
        stop("cooling for stop");
        m_idle_cooling = false;
        //printf("idle cooling, stop\r\n");
      }
      
      m_status.M_cooling = false;
    }
    
    
    // 冷机：启动， 停止
    if (m_started && !m_start_cooling || started_after_cooling) {
      // 启动前 init：
      
      m_k_is_valid = false; // 1）m_k_is_valid需置0， 对于第2次启动      
      stove.next_is_delaying = false; // 2）延迟换料标志置0      
      stove.level = 0; // 3）燃烧等级      
      stove.boil_mode = false; // 4）烧水模式
      // 5）启动送料为55      
      fan_smoke_delay = power_of_blow[2]; // 阶段1排烟风速，取等级3值
      
      // 阶段1
      stage(1);
      
      m_started = false; 
      started_after_cooling = false;    
    }
    
    // 停止 ***
    if (m_stoped && !m_stop_cooling) {
      m_stoped = false;
      stop("normal");
      // 清除烧水模式
      stove.boil_mode = false;
    }
    
    /*------------------------------------------------------------------------*/
    
    //定时器消息处理
    osEvent evt_timer = osMessageGet(Q_TimerHandle, 0); 
    if (evt_timer.status == osEventMessage) {
      TMR_EVENT timer_event = (TMR_EVENT)evt_timer.value.v;
      switch(timer_event) {
        // 1- 吹烟
      case TMR_BLOW_DUST: 
        // 阶段2 开始送料70秒定时器
        stage(2);
        feed_full = true;
        
        break;
        
        // 2-预送料
      case TMR_PREFEED:              
        feed_full = false;  // 预送料结束         
        stage(3);  // 段3 开始预加热
        
        break;
        
        // 3-点火超时
      case TMR_FIRE_TIMEOUT:
        if (failure_fire > 0) {
          stop("failure fire");
          m_started = true;
          failure_fire--;
        }  
        else {
          m_stoped = true;
          m_failure_fire_2nd = true;
        }
        
        break;
        
        // 4-开始送料
      case TMR_BEFORE_FEED:
        // 段4 加热4分钟后，开始送料        
        stage(4);
        
        break;
        
        // 5-延迟判断
      case TMR_DELAY_RUNNING:
        
        stage(6); // 点火成功，延迟一会儿
        // 已经运行，更新
        // 手动按键，可以2次启动
        if (m_status.M_running) {
          failure_fire = 1;
          failure_flame = 1;
        }
        is_absence = false;
        m_update_absence = false;
        
        break;
        
        // 6-全速排烟
      case TMR_FULL_BLOW:
        flag_full_blow = false;
        
        break;
        
        // 7-烧水超时
      case TMR_BOIL_TIMEOUT:
        // 烧水模式超时退出
        stove.boil_mode = false;
        break;
        
        // 8-档位平滑切换
      case TMR_SWITCH_DELAY:
        // 先降料后降风，先加料后加风
        if (M_SwitchDelayHandle != NULL) {
          osMessagePut(M_SwitchDelayHandle, (uint32_t)1, 0);
        }
        break;
        
        // 9-k有效 延迟验证
      case TMR_k_VALIDATE:
        /* 判定K是否有效 */
        stage(5);        
        osTimerStop(tmr_fireTimeoutHandle);  // 停止点火超时定时器
        
        break;
        
      } 
      // switch end
    }
    
    // 定时器消息 end
    /*----------------------------------------------------------------------------*/    
    
    // 燃烧中熄火，原因： 缺料
    if (m_status.M_running && data.k<FLAME_DOWN) {
      
      is_absence = true;
      m_update_absence = true;
      // 显示缺料
      strcpy(info_stage, "\xac\x9d");  // 缺料
      // error
      stove_log(12);
      
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
    // 判断烟气温度是否到80℃
    // 1分钟内依旧大于80℃，则可以判定为运行
    if (data.k>=TEMP_RUN && m_status.stage==4 && !m_k_is_valid) {
      m_k_is_valid = true;  
      osTimerStart(tmr_k_validateHandle, K_VALIDATE);
      //printf("k validate is start.\r\n");
    }
    
    if (data.k<TEMP_RUN && m_status.stage==4 && m_k_is_valid) {
      m_k_is_valid = false;
      osTimerStop(tmr_k_validateHandle);
      //printf("k validate is stop.\r\n");
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
        snprintf(feed_progress, 6, "%02d%%", t_feed_manual*10/6);
        //
      }
      //  
    }
    /*------------------------------------------------------------------------*/
    
    // 送料比例
    recipe_cur.level_0 = recipe[recipe_index*3];
    recipe_cur.level_1 = recipe[recipe_index*3+1];
    recipe_cur.level_2 = recipe[recipe_index*3+2];
    
    // 分配triac内存, triac.c中返还
    if (pool_TriacHandle != NULL) {
      Triac *p_triac = (Triac *)osPoolAlloc(pool_TriacHandle);      
      
      /* 根据炉温，室温判断送料速度、排烟速度 */
      if (p_triac != NULL) {    
        // 运行，送料比例和风速动态改变
        if (m_status.M_running) {  
          
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
        snprintf(info_blow_2, 30, "\x9c\xbf\xbd\xbe: [%d]", p_triac->fan_smoke_power);
        snprintf(info_feed_ex, 20, "\x9c\x9d\xbd\xbe: [%d]", p_triac->feed_duty); 
        
        /*----------------------------------------------------------------------*/ 
        
        // 阶段1： 吹烟
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
    
    // 阶段文字显示
    fn_info_stage(info_stage, &m_status);
    // 烧水时
    if (stove.boil_mode && m_status.stage==6) {
      strcpy(info_stage, "\xc0\xad");  // info 烧水
    };
    
    
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
    
    /* 发送0x55 到uart2  */
    osEvent evt_pellet_check = osMessageGet(Q_SonicDataHandle, 0);
    if (evt_pellet_check.status == osEventMessage) {
      pellet_sum = pellet_sum - pellet_a + evt_pellet_check.value.v;
      pellet_a = pellet_sum / 2;
      
      pellet_consume_distance = pellet_a;
      snprintf(info_pellet, 10, "\x9d: %d",pellet_consume_distance);
      //printf("distance:%d\r\n", pellet_consume_distance);
    }
    check_interval++;
    if (check_interval >= PELLET_CHK_INTERVAL) {  // 99， 499
      HAL_UART_Transmit(&huart2, &command_sonic, 1, 1);
      check_interval = 0;
    }
    
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
  // 吹烟
  osMessagePut(Q_TimerHandle, (uint32_t)TMR_BLOW_DUST, 0);
  
  /* USER CODE END callback_blowDust */
}

/* callback_preFeed function */
void callback_preFeed(void const * argument)
{
  /* USER CODE BEGIN callback_preFeed */
  // 预送料
  osMessagePut(Q_TimerHandle, (uint32_t)TMR_PREFEED, 0);
  
  /* USER CODE END callback_preFeed */
}

/* callback_fireTimeout function */
void callback_fireTimeout(void const * argument)
{
  /* USER CODE BEGIN callback_fireTimeout */
  
  // 关闭点火棒
  HAL_GPIO_WritePin(PORT_FIRE_ROD, FIRE_ROD, GPIO_PIN_RESET);
  // 停止点火超时定时器
  osTimerStop(tmr_fireTimeoutHandle);
  
  osMessagePut(Q_TimerHandle, (uint32_t)TMR_FIRE_TIMEOUT, 0);
  
  /* USER CODE END callback_fireTimeout */
}

/* callback_beforeFeed function */
void callback_beforeFeed(void const * argument)
{
  /* USER CODE BEGIN callback_beforeFeed */
  
  // 开始送料
  osMessagePut(Q_TimerHandle, (uint32_t)TMR_BEFORE_FEED, 0);
  
  /* USER CODE END callback_beforeFeed */
}

/* callback_delayRunning function */
void callback_delayRunning(void const * argument)
{
  /* USER CODE BEGIN callback_delayRunning */
  
  // 延迟运行判断
  osMessagePut(Q_TimerHandle, (uint32_t)TMR_DELAY_RUNNING, 0);
  
  /* USER CODE END callback_delayRunning */
}

/* callback_full_blow function */
void callback_full_blow(void const * argument)
{
  /* USER CODE BEGIN callback_full_blow */
  
  // 排烟风扇全速时间到，结束
  osMessagePut(Q_TimerHandle, (uint32_t)TMR_FULL_BLOW, 0);
  
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
  osMessagePut(Q_TimerHandle, (uint32_t)TMR_BOIL_TIMEOUT, 0);
  
  /* USER CODE END callback_boil_timeout */
}

/* callback_switch_delay function */
void callback_switch_delay(void const * argument)
{
  /* USER CODE BEGIN callback_switch_delay */
  
  // 档位平滑切换
  // 先降料后降风，先加料后加风
  osMessagePut(Q_TimerHandle, (uint32_t)TMR_SWITCH_DELAY, 0);
  
  /* USER CODE END callback_switch_delay */
}

/* callback_k_validate function */
void callback_k_validate(void const * argument)
{
  /* USER CODE BEGIN callback_k_validate */
  
  /* K已经有效 */
  osMessagePut(Q_TimerHandle, (uint32_t)TMR_k_VALIDATE, 0);
  
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
      osDelay(300);
    }
  }
}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
