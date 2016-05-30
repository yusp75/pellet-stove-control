#ifndef _USER_H
#define _USER_H

#include "stdbool.h"
#include "stdint.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "stdlib.h"
#include "stm32f1xx_hal.h"
#include "utils.h"

#define BLOW_DUST 60*1000  // �մ�60��
#define PRE_FEED 70*1000  //  Ԥ����ʱ��70��
#define FIRE_UNTIL_FEED 240*1000  // ���Ȱ�������4�ֺ�ʼ����
#define DELAY_RUNNING 120*1000  // �ӳٽ�������״̬��120���ʼ�ж�
#define FIRE_TIME_OUT 1200*1000  // ���ʱ 1200��
#define K_VALIDATE 60*1000  // �Ƿ����У�K��Чʱ��
#define TRY_DELAY 3*1000  // �ٴ��������
#define TIME_FEEDMANUAL 60  // �ֶ�����ʱ��
#define STOVE_TEMP_LIMIT 190  // ¯������
#define UP_SMOOTH 60*1000  // ����ƽ��
#define DOWN_SMOOTH 90*1000  // ����ƽ��

#define PELLET_WARN 280  // ������룬����ȼ���ٶ�
#define PELLET_MIN 320  // Ϩ��
#define PELLET_THRESHOLD 100  // �ٽ�ֵ
#define PELLET_CHK_INTERVAL 199  // �����

#define LED_RUN GPIOB->ODR ^= GPIO_PIN_5

#define DEBUG
#undef DEBUG


//��Ļ��
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
#define SCREEN_FEED 18  // �ֶ�������Ļ
#define SCREEN_LOG 19  // ��־
#define SCREEN_PELLET_ADD 20  // ���ϱ���

// ��������
#define BEEP_BUTTON 1
#define BEEP_ABSENCE 2 

// ��ȴ�¶�
#define COOLING_STOP 60
#define FLAME_DOWN 60
// �����¶�
#define TEMP_RUN 80  // 80

// �������ֵ
#define PV_LOW 9
#define PV_HIGH 30

#define RTC_ADDR_READ 0x65  //RTC���� ����ַ
#define RTC_ADDR_WRITE 0x64  //RTC���� д��ַ

/* RTC ���� 
*  �û����� 0x2c-0x71
*  ����1�� 0x2c-0x3f����־�� 
*  ����2�� 0x40-0x4e���䷽
*  ����3�� 0x4f-0x71���ƻ� 
*/

// �䷽�����ַ 0x40-0x4a
#define RTC_ADDR_RECIPE 0x40  // 3*3 = 9
#define RECIPE_DATA_LEN 9

// �ƻ������ַ 0x4f-0x6a
#define RTC_ADDR_SCHEDULE 0x4f // 3*9 = 27
#define SCHEDULE_DATA_LEN 9

// �Ƿ��ʼ����ַ
#define RTC_ADDR_INIT 0x2c
// ���̹��ʵ�ַ
#define RTC_ADDR_POW 0x2d  // 0x2d-0x2f,3��
// �����¶ȵ�ַ
#define RTC_ADDR_PV 0x30
// �Ƿ�ԤԼ��ַ
#define RTC_ADDR_SCHEDULE_STATE 0x31
// �䷽������ַ
#define RTC_ADDR_RECIPE_INDEX 0x32
// ȱ�ϱ���
#define RTC_ADDR_ABSENCE 0x33
// ��������
#define RTC_ADDR_SLLEP 0x34


// ¯������״̬

typedef struct
{
  const uint8_t address;  // �����ַ
  uint8_t schedule_is_enabled;  // �Ƿ����üƻ�
  uint8_t last_failure_is_absent;  // �ϴ�ʧ������Ϊȱ��
  uint8_t mode_is_auto;  // �Զ����ֶ�����ģʽ
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
  int8_t quantity;  // ����
} RTC_Time;


//�ƻ���
typedef struct
{
  int8_t start_0_hour;  // BCD ��ʽ 0123 --> 01:23
  int8_t start_0_min;
  int8_t end_0_hour;
  int8_t end_0_min;

  int8_t start_1_hour;  // BCD ��ʽ 0123 --> 01:23
  int8_t start_1_min;
  int8_t end_1_hour;
  int8_t end_1_min;

  uint8_t enabled;  // ��������?

} Schedule;


//data �����¶ȣ�һ��ʱ��
typedef struct
{
  int8_t cv;  // ���ڵ�ǰ�¶�
  int16_t k;  // K���ȵ�ż�¶�
  //RTC_Time time;  // ʱ��

} Data;

//¯
typedef struct
{
  bool scheduled;  // �ƻ�����
  bool boil_mode;  // ��ˮģʽ
  bool pellet_warn;  // �ϵ��ﾯ��

  uint8_t pv; // ���������¶�
  Data data;
  
  // ƽ���л�
  uint8_t level;
  uint8_t next_v;
  bool next_is_delaying;  //���ӳ���

} Stove;



/*
*ÿ���߼������󣬷���Triac״̬�����жϴ���
*/
typedef struct
{
  
  bool fan_smoke_is_on; // ����
  uint16_t fan_smoke_power;  // �ٷ��� 0~100
  
  bool fan_exchange_is_on; // ����
  uint16_t fan_exchange_delay; // �ٷ��� 0~100
  
  bool feed_is_on; // ����
  bool feed_by_manual;  // �ֶ�����
  bool feed_full;  // ȫ������
  uint8_t feed_duty; // n of 10s, ��С3�룬���7��

} Triac;


typedef struct
{
  bool M_cooling;  // ��ȴ
  bool M_blow;
  bool M_exchange;
  bool M_fire;
  bool M_feed;

  bool M_starting;
  bool M_running_delay;
  bool M_running;
  bool M_stopping;
  bool M_absensing;  // ȱ��
  bool M_idle;  // ����

  int8_t stage;  // �׶�1 2 3 4 5

} M_Status;


typedef struct
{
  int8_t level_0;
  int8_t level_1;
  int8_t level_2;

} RECIPE;  // �䷽

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

/* �������� */
void stage_init(void);
void stop(char *s);

/* �������º�K�¶ȸ���Triac״̬ */
void judge(Stove *p_stove, Triac *triac, RECIPE *recipe, uint8_t *power);
/* �����µ�CCRֵ */
uint32_t new_ccr(TIM_HandleTypeDef *htim, uint32_t delay);

/* �׶�״̬ */
void fn_info_stage(char *info, M_Status *status);
void stage(int8_t index);

/* ������־ */
void stove_log(uint32_t id);

/* �ͷ��ٶȽ��� */
uint8_t v_blow(uint8_t s[]);

void setTriac(uint8_t fan_smoke_power, uint8_t fan_exchange_delay, uint8_t feed_duty,
				M_Status *status, Triac *triac);
/* ����Ƿ��ڼƻ��� */
void check_schedule(uint8_t h, uint8_t m, Schedule *plan, In_Schedule_Time *in);
#endif
