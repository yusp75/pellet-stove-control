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
#define N_KTEMP 4  // 平均值

// 外部rtc变量
extern RTC_Time Time_sd3088;

// 外部 iwdg变量
//extern IWDG_HandleTypeDef hiwdg;
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

/* 结构实例 */
M_Status m_status;
RTC_Time rtc_time = {0};

// stove
Stove stove;

/* Triac */
__IO uint8_t fan_smoke_delay = 60;
__IO uint8_t fan_exchange_delay = 80;
__IO uint8_t feed_duty = 5;
__IO bool flag_full_blow = false;  // 排烟全速3秒
__IO bool flag_full_exchange = false;  // 循环全速3秒

/* 配方 */
int8_t recipe_index = 0;
int8_t recipe[RECIPE_INDEX] = {
  30, 45, 60,
  35, 50, 65,
  40, 55, 60,
  0
};

RECIPE recipe_cur;  // 当前配方

/* 排烟功率 */
uint8_t power_of_blow[3] = {9, 7, 6};  // 50 + x * 5

/* 时钟显示 */
char *p_rtc_display;
char rtc_display[20];

/* 计划 */
Schedule plan[3] = {
  {8, 0, 12, 0, 13, 0, 18, 0, 1},
  {8, 0, 12, 0, 13, 0, 18, 0, 1},
  {8, 0, 12, 0, 13, 0, 18, 0, 1}
};


char info_stage[8] = "\x88\x89";  // 初始： 停止
char *p_info_stage;

// 状态字符串
char info_message[8];
char *p_info_message;
/* 温度cv, pv字符串 */
char info_cv_pv[20];
char *p_info_cv_pv;

/* 运行状态参数 */
State run_state = {0x2d, 0, 0, 1};

/*显示： 预约*/
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
  /*自定义消息队列*/
  //Triac内存池,消息
  pool_TriacHandle = osPoolCreate(osPool(pool_triac));
  
  osMessageQDef(Q_Triac, 1, Triac);
  Q_TriacHandle = osMessageCreate(osMessageQ(Q_Triac), NULL);
  //Stove内存池,消息
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
  
  // K 滤波变量
  uint16_t k_sum = 0, k_a = 0;
  
  bool start_conversion = false;
  // bool readable = false;  // 每2秒读取温度
  // ds18b20设置精度
  therm_reset();
  therm_setResolution(RESOLUTION);
  
  
  /* Infinite loop */
  for(;;)
  {
    // run led
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
    
    ///* rtc 时钟 */
    // 信号锁
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
    
    /* k电偶  滤波
    * 初始化：A=初始值，S=A*N
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
    
    /* 更新stove数据 */
    // 分配stove内存，函数StartTaskScanKey中返还
    Data *p_data;
    p_data = (Data*)osPoolAlloc(pool_DataHandle);
    if (p_data != NULL) {
      //室温， 炉温
      p_data->cv = (int16_t)cv;
      p_data->k = k_a; // temp_k
      
      //printf("ds18b20:%d,k:%d\r\n", p_data->cv, p_data->k);
      
      //      //时间
      //      p_data->time.day = Time_sd3088.day;
      //      p_data->time.hour = Time_sd3088.hour;  // todo: 24小时制
      //      p_data->time.minute = Time_sd3088.minute;
      //      p_data->time.month = Time_sd3088.month;
      //      p_data->time.quantity = Time_sd3088.quantity;
      //      p_data->time.second = Time_sd3088.second;
      //      p_data->time.week = Time_sd3088.week;
      //      p_data->time.year = Time_sd3088.year;
      // 放入队列
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
  
  __IO KeyPressed key_pressed;  // 按键
  __IO bool need_update = false;  // 是否刷新显示
  __IO bool power = false;
  uint8_t pv = 9;
  
  __IO uint8_t read_rtc_counter = 0;
  
  In_Schedule_Time in_schedule_time;  // 在计划内
  
  bool m_start = false;
  bool m_stop = false;
  
  bool schedule_start_stove = false;  //  计划模式启动炉子
  
  // 屏幕号
  __IO uint8_t screen_no = 0;
  
  // 星期1-7 字符
  char weekday_chr[7] = {'\x93', '\xa1','\xa2','\xa3','\xa4','\xa5','\xa6'};
  osStatus status;
  
  // 字符串关联指针
  p_info_stage = &info_stage[0];
  p_rtc_display = &rtc_display[0];
  p_info_message = &info_message[0];  
  
  p_schedule_time = &schedule_time[0];
  
  p_info_cv_pv = &info_cv_pv[0];  // cv, pv
  
  // dataData data = {0, 0, rtc_time};
  // 
  Data data = {0, 0};  // 是否需要初始化?
  osEvent evt_data;
  
  // 读存储的运行状态：是否计划，前次缺料熄火
  init_data();  
  
  /* 读取存储的配方， 计划， 状态数据 */
  // 读配方
  read_recipe(recipe);
  // 当前配方
  recipe_cur.level_0 = recipe[recipe[RECIPE_INDEX - 1] * 3];
  recipe_cur.level_1 = recipe[recipe[RECIPE_INDEX - 1] * 3 + 1];
  recipe_cur.level_2 = recipe[recipe[RECIPE_INDEX - 1] * 3 + 2];
  recipe_index = recipe[RECIPE_INDEX - 1];
  // 读计划  
  read_schedule(&plan[0], 0);
  read_schedule(&plan[1], 1);
  read_schedule(&plan[2], 2);
  // 读排烟
  read_from_rtc(RTC_ADDR_POW, power_of_blow, 3);
  // 读温度 PV
  read_from_rtc(RTC_ADDR_PV, &pv, 1); 
  
  
  // 改RTC到24小时制
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
    // 1秒读rtc
    read_rtc_counter++;
    if (read_rtc_counter == 5) {
      read_rtc_counter = 0;
      read_rtc();
    }
    
    // 扫描按键
    Scan_Keyboard();
    key_pressed = getKey();
    
    /* 读取炉数据 */
    evt_data = osMessageGet(Q_DataHandle, 0);
    if (evt_data.status == osEventMessage) {
      data = *((Data *)evt_data.value.p);
      stove.data = data;
      
      // 读取屏幕号
      if (get_screen_no() != screen_no) {
        screen_no = get_screen_no();
        //set_pin_key(screen_no);  // 更新按键绑定
      }
      
      // 不在修改时间屏幕时，刷新时间显示
      if (screen_no != SCREEN_RTC) {
        rtc_time = Time_sd3088;
        // 时间格式 9:00 周六
        sprintf(rtc_display, "%02d:%02d \x95%c", rtc_time.hour, rtc_time.minute, 
                weekday_chr[rtc_time.week]);
      }
      
      // 返还内存
      status = osPoolFree(pool_DataHandle, evt_data.value.p);
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
        /* 电源按键 */
        
        // 停止
        if ( (key_pressed == Key_Power) && (m_status.stage != -1) && (!stove.scheduled)) {
          strcpy(info_stage, "\x88\x89");  // 阶段状态：停止
          stop(&data);
        }
        else if (((key_pressed == Key_Power)  || schedule_start_stove) && (m_status.stage == -1)) {  // 启动
          if (data.k >= TEMP_COOLING) { //如65， 冷却
            stage_0();
            fan_smoke_delay = BLOW_LEVEL_1;  // 1
            fan_exchange_delay = 1;
            //strcpy(info_stage, "\x80\x81");  // 阶段状态：点火
          }
          else {
            stage_1();
            //fan_smoke_delay = BLOW_LEVEL_2;  // 启动时，炊烟速度为延迟2毫秒
          }
          
          strcpy(info_stage, "\x80\x81");  // 阶段状态：点火
        }   
        
        if (key_pressed == Key_Power) {
          key_pressed = Key_None;  // 消除Power键代码
        }
        
        /* 2、计划模式按键 */
        if (key_pressed == Key_Mode) {
          if (m_status.stage == -1) {
            stove.scheduled = true;            
          }
          else {
            
            stove.scheduled = false;
          }
          
          if (m_status.stage == 6) {            
            stove.boil_mode = true;
            // 显示 烧水图标？
            // 启动烧水模式超时定时器
            osTimerStart(tmr_boil_timeoutHandle, 50 * 60 * 1000);  // 50分钟
            
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
        // 刷新界面： 按键 
        need_update = true;
      }  // home screen
      
      /* 4、是否是 menu home key */
      if (key_pressed == Key_Menu) {
        change_root();
        key_pressed = Key_None; // 使键为空
      }
    } 
    // *
    // 刷新界面温度
    sprintf(info_cv_pv, "\x8a\x8b:%d \x9a\x9b: %d",stove.data.cv, pv);
    
    /* 按键预处理 end  */
    /*------------------------------------------------------------------------*/
    
    if (key_pressed != Key_None)
      osMessagePut(Q_KeyHandle, key_pressed, 0);
    
    // 更新按键绑定
    set_pin_key(screen_no);  
    m2_CheckKey();
    
    /* 预约时间检查 */
    if (stove.scheduled) {
      Schedule plan_cur;
      // 根据星期拿到计划
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
      // 显示预约      
      sprintf(schedule_time, "%s %02d:%02d - %02d:%02d", "\x8d\x8e", 
              in_schedule_time.start_hour,
              in_schedule_time.start_min,
              in_schedule_time.end_hour,
              in_schedule_time.end_min
                ); 
      
    }
    /*------------------------------------------------------------------------*/    
    
    // 分配triac内存, triac.c中返还
    Triac *p_triac;
    p_triac = (Triac*)osPoolAlloc(pool_TriacHandle);
    
    /* 根据炉温，室温判断送料速度、排烟速度 */
    if (p_triac != NULL) {
      // 排烟速度
      p_triac->fan_smoke_delay = 50 + power_of_blow[0]*5;
      // 若运行
      if (m_status.M_running) {  
        // 送料比例
        recipe_cur.level_0 = recipe[recipe_index * 3];
        recipe_cur.level_1 = recipe[recipe_index * 3 + 1];
        recipe_cur.level_2 = recipe[recipe_index * 3 + 2];
        judge(&stove, p_triac, &recipe_cur);
      }
      
      // todo: 启动中
      if (m_status.M_starting) {
        
        
      }
      
      // 冷却，温度低于65℃时，停止 吹烟和循环
      if (data.k <= (TEMP_RUN - 5) && m_status.M_cooling == true) {
        if (m_status.M_blow)
          m_status.M_blow = false;
        
        if (m_status.M_exchange)
          m_status.M_exchange = false;
        
      }
      
      // 更新风机状态
      if (p_triac->fan_smoke_is_on != m_status.M_blow && (!flag_full_blow)) {        
        flag_full_blow = true;
        osTimerStart(tmr_full_blowHandle, 3 * 1000);
      }
      if (flag_full_blow) {
        /* 会出现power=0的问题 */
        p_triac->fan_smoke_delay = 100;  
      }
      
      p_triac->fan_smoke_is_on = m_status.M_blow;      
      p_triac->fan_exchange_is_on = m_status.M_exchange;
      p_triac->feed_is_on = m_status.M_feed;
      
      p_triac->feed_duty = feed_duty;  // todo: 通过消息传递
      
      // triac 放入消息队列
      status = osMessagePut(Q_TriacHandle, (uint32_t)p_triac, 0);
      if (status != osOK)
        printf("Put message into Q_Triac failed: %x\r\n", status);
    } 
    // if end: Tirac Queue 
    /*------------------------------------------------------------------------*/
    
    // 判断炉温到70℃?
    if (data.k >= TEMP_RUN && m_status.stage == 4) {
      stage_5();
      // 停止点火超时定时器
      osTimerStop(tmr_fireTimeoutHandle);
      strcpy(info_stage, "\x98\x99");  // 阶段状态：运行
    }
    
    // 输出 点火棒
    if (m_status.M_fire)
      HAL_GPIO_WritePin(PORT_FIRE_ROD, FIRE_ROD, GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(PORT_FIRE_ROD, FIRE_ROD, GPIO_PIN_RESET);
    
    /*------------------------------------------------------------------------*/
    /* 刷新显示 */
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
    
    // 刷新IWDG
    //HAL_IWDG_Refresh(&hiwdg);
    
    osDelay(10);
  }
  /* USER CODE END StartTaskScanKey */
}

/* callback_blowDust function */
void callback_blowDust(void const * argument)
{
  /* USER CODE BEGIN callback_blowDust */
  
  // 段2 开始送料70秒定时器
  stage_2();
  feed_duty = 3;  // 3/10
  
  /* USER CODE END callback_blowDust */
}

/* callback_preFeed function */
void callback_preFeed(void const * argument)
{
  /* USER CODE BEGIN callback_preFeed */
  // 预送料结束
  // 段3 开始预加热
  
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
  
  stage_6();
  
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
  
  /* USER CODE END callback_switch_delay */
}

/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
