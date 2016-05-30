#ifndef _USER_H
#define _USER_H

#include "stdbool.h"
#include "stdint.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "stdlib.h"
#include "stm32f1xx_hal.h"
#include "utils.h"

#define BLOW_DUST 60*1000  // 空吹60秒
#define PRE_FEED 70*1000  //  预送料时间70秒
#define FIRE_UNTIL_FEED 240*1000  // 加热棒工作，4分后开始送料
#define DELAY_RUNNING 120*1000  // 延迟进入运行状态，120秒后开始判断
#define FIRE_TIME_OUT 1200*1000  // 点火超时 1200秒
#define K_VALIDATE 60*1000  // 是否运行，K有效时间
#define TRY_DELAY 3*1000  // 再次启动间隔
#define TIME_FEEDMANUAL 60  // 手动送料时间
#define STOVE_TEMP_LIMIT 190  // 炉温限制
#define UP_SMOOTH 60*1000  // 升档平滑
#define DOWN_SMOOTH 90*1000  // 降档平滑

#define PELLET_WARN 280  // 警戒距离，减少燃烧速度
#define PELLET_MIN 320  // 熄火
#define PELLET_THRESHOLD 100  // 临界值
#define PELLET_CHK_INTERVAL 199  // 检查间隔

#define LED_RUN GPIOB->ODR ^= GPIO_PIN_5

#define DEBUG
#undef DEBUG


//屏幕号
#define SCREEN_HOME 0
#define SCREEN_MENU 1
#define SCREEN_RTC 10
#define SCREEN_POWER 11
#define SCREEN_RECIPE 12
#define SCREEN_SCHEDULE 13
#define SCREEN_STATUS 14
#define SCREEN_RECIPE_SELECT 15
#define SCREEN_SCHEDULE_EXIT_CONFIRM 16
#define SCREEN_MESSAGE 17
#define SCREEN_FEED 18  // 手动送料屏幕
#define SCREEN_LOG 19  // 日志
#define SCREEN_PELLET_ADD 20  // 填料报警

// 声音类型
#define BEEP_BUTTON 1
#define BEEP_ABSENCE 2 

// 冷却温度
#define COOLING_STOP 60
#define FLAME_DOWN 60
// 工作温度
#define TEMP_RUN 80  // 80

// 最低设置值
#define PV_LOW 9
#define PV_HIGH 30

#define RTC_ADDR_READ 0x65  //RTC器件 读地址
#define RTC_ADDR_WRITE 0x64  //RTC器件 写地址

/* RTC 保存 
*  用户数据 0x2c-0x71
*  数据1： 0x2c-0x3f，标志量 
*  数据2： 0x40-0x4e，配方
*  数据3： 0x4f-0x71，计划 
*/

// 配方保存地址 0x40-0x4a
#define RTC_ADDR_RECIPE 0x40  // 3*3 = 9
#define RECIPE_DATA_LEN 9

// 计划保存地址 0x4f-0x6a
#define RTC_ADDR_SCHEDULE 0x4f // 3*9 = 27
#define SCHEDULE_DATA_LEN 9

// 是否初始化地址
#define RTC_ADDR_INIT 0x2c
// 排烟功率地址
#define RTC_ADDR_POW 0x2d  // 0x2d-0x2f,3个
// 设置温度地址
#define RTC_ADDR_PV 0x30
// 是否预约地址
#define RTC_ADDR_SCHEDULE_STATE 0x31
// 配方索引地址
#define RTC_ADDR_RECIPE_INDEX 0x32
// 缺料保存
#define RTC_ADDR_ABSENCE 0x33
// 待机保存
#define RTC_ADDR_SLLEP 0x34


// 炉子运行状态

typedef struct
{
  const uint8_t address;  // 保存地址
  uint8_t schedule_is_enabled;  // 是否启用计划
  uint8_t last_failure_is_absent;  // 上次失败是因为缺料
  uint8_t mode_is_auto;  // 自动，手动运行模式
} State;

//RTC
typedef struct
{
  int8_t second;
  int8_t minute;
  int8_t hour;
  int8_t am_or_pm;
  int8_t flag_12or24;
  int8_t week;
  int8_t day;
  int8_t month;
  int8_t year;
  int8_t quantity;  // 电量
} RTC_Time;


//计划表
typedef struct
{
  int8_t start_0_hour;  // BCD 格式 0123 --> 01:23
  int8_t start_0_min;
  int8_t end_0_hour;
  int8_t end_0_min;

  int8_t start_1_hour;  // BCD 格式 0123 --> 01:23
  int8_t start_1_min;
  int8_t end_1_hour;
  int8_t end_1_min;

  uint8_t enabled;  // 该天启用?

} Schedule;


//data 两个温度，一个时间
typedef struct
{
  int8_t cv;  // 室内当前温度
  int16_t k;  // K型热电偶温度
  //RTC_Time time;  // 时间

} Data;

//炉
typedef struct
{
  bool scheduled;  // 计划启用
  bool boil_mode;  // 烧水模式
  bool pellet_warn;  // 料到达警戒

  uint8_t pv; // 室内设置温度
  Data data;
  
  // 平滑切换
  uint8_t level;
  uint8_t next_v;
  bool next_is_delaying;  //在延迟中

} Stove;



/*
*每次逻辑结束后，发送Triac状态，让中断处理
*/
typedef struct
{
  
  bool fan_smoke_is_on; // 排烟
  uint16_t fan_smoke_power;  // 百分数 0~100
  
  bool fan_exchange_is_on; // 换风
  uint16_t fan_exchange_delay; // 百分数 0~100
  
  bool feed_is_on; // 送料
  bool feed_by_manual;  // 手动送料
  bool feed_full;  // 全速送料
  uint8_t feed_duty; // n of 10s, 最小3秒，最大7秒

} Triac;


typedef struct
{
  bool M_cooling;  // 冷却
  bool M_blow;
  bool M_exchange;
  bool M_fire;
  bool M_feed;

  bool M_starting;
  bool M_running_delay;
  bool M_running;
  bool M_stopping;
  bool M_absensing;  // 缺料
  bool M_idle;  // 空闲

  int8_t stage;  // 阶段1 2 3 4 5

} M_Status;


typedef struct
{
  int8_t level_0;
  int8_t level_1;
  int8_t level_2;

} RECIPE;  // 配方

typedef struct
{
  bool in_schedule;
  int8_t start_hour;
  int8_t start_min;
  int8_t end_hour;
  int8_t end_min;  
} In_Schedule_Time;

typedef struct
{ 
  uint32_t t1;
  uint32_t t3;
//  uint32_t t4;
//  uint32_t t6;
//  uint32_t t7;
  uint8_t cr;
  uint8_t i;
  uint32_t T1;
  
} Zero;


typedef enum
{
  TMR_BLOW_DUST = 0x1,
  TMR_PREFEED,
  TMR_FIRE_TIMEOUT,
  TMR_BEFORE_FEED,
  TMR_DELAY_RUNNING,
  TMR_FULL_BLOW,
  TMR_BOIL_TIMEOUT,
  TMR_SWITCH_DELAY,
  TMR_k_VALIDATE,
  
} TMR_EVENT;

/* 函数声明 */
void stage_init(void);
void stop(char *s);

/* 根据室温和K温度更新Triac状态 */
void judge(Stove *p_stove, Triac *triac, RECIPE *recipe, uint8_t *power);
/* 计算新的CCR值 */
uint32_t new_ccr(TIM_HandleTypeDef *htim, uint32_t delay);

/* 阶段状态 */
void fn_info_stage(char *info, M_Status *status);
void stage(int8_t index);

/* 发送日志 */
void stove_log(uint32_t id);

/* 送风速度解析 */
uint8_t v_blow(uint8_t s[]);

void setTriac(uint8_t fan_smoke_power, uint8_t fan_exchange_delay, uint8_t feed_duty,
				M_Status *status, Triac *triac);
/* 检查是否在计划内 */
void check_schedule(uint8_t h, uint8_t m, Schedule *plan, In_Schedule_Time *in);
#endif
