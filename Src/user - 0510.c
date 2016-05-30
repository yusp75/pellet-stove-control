/*
*
* user.c
*/

#include "user.h"


extern M_Status m_status;
extern osTimerId tmr_blowDustHandle;
extern osTimerId tmr_preFeedHandle;
extern osTimerId tmr_fireTimeoutHandle;
extern osTimerId tmr_beforeFeedHandle;
extern osTimerId tmr_delayRunningHandle;
extern osTimerId tmr_switch_delayHandle;

extern  I2C_HandleTypeDef hi2c1;

//extern Stove stove;

// 0
void stage_init(void)
{
  m_status.M_cooling = false;
  m_status.M_blow = false;
  m_status.M_exchange = false;
  m_status.M_fire = false;
  m_status.M_feed = false;
  m_status.M_starting = false;
  m_status.M_running = false;
  m_status.M_stopping = false;
  m_status.M_idle = true;
  
  m_status.stage = -1;  
}


// 1
// ��ȴ
void stage_0(void)
{
  m_status.M_cooling = true;
  m_status.M_blow = true;
  m_status.M_exchange = true;
  m_status.M_fire = false;
  m_status.M_feed = false;
  
  //m_status.M_starting = false;
  m_status.M_running_delay = false;
  m_status.M_running = false;
  //m_status.M_stopping = false;
  m_status.M_idle = false;
  
  m_status.stage = -1;
  
#ifdef DEBUG  
  printf("stage 0, cooling\r\n");
#endif  
}


// 1
// ���̳���60��
void stage_1(void)
{
  m_status.M_cooling = false;
  m_status.M_blow = true;
  m_status.M_exchange = false;
  m_status.M_fire = false;
  m_status.M_feed = false;
  m_status.M_starting = true;
  m_status.M_running = false;
  m_status.M_stopping = false;
  m_status.M_idle = false;
  
  m_status.stage = 1;
  //
  osStatus status = osTimerStart(tmr_blowDustHandle, BLOW_DUST);
  if (status != osOK)
    error(1);
#ifdef DEBUG  
  printf("stage 1, blow dust\r\n");
#endif 
}

// 2
// ������70��
void stage_2(void)
{
  m_status.M_blow = false;
  m_status.M_exchange = false;
  m_status.M_fire = false;
  m_status.M_feed = false;
  //m_status.M_starting = true;
  m_status.M_idle = false;
  
  m_status.stage = 2;
  //
  osStatus status = osTimerStart(tmr_preFeedHandle, PRE_FEED);
  if (status != osOK)
    error(2);
#ifdef DEBUG   
  printf("stage 2, pre feed 70s\r\n");
#endif
}

// 3
// ����,��240���ʼ����

void stage_3(void)
{
  m_status.M_blow = true;
  m_status.M_exchange = false;
  m_status.M_fire = true;
  m_status.M_feed = false;
  //m_status.M_starting = true;
  m_status.M_idle = false;
  
  m_status.stage = 3;
  //
  osStatus status = osTimerStart(tmr_beforeFeedHandle, FIRE_UNTIL_FEED);
  if (status != osOK)
    error(3);
  
  // ��ʼ��¼���ʱ
  status = osTimerStart(tmr_fireTimeoutHandle, FIRE_TIME_OUT);
  if (status != osOK)
    error(13);
#ifdef DEBUG   
  printf("stage 3, fire\r\n");
#endif
}

// 4
// ����
// ��ʼ����
void stage_4(void)
{
  m_status.M_blow = true;
  m_status.M_exchange = false;
  m_status.M_fire = true;
  m_status.M_feed = true;
  //m_status.M_starting = true;
  m_status.M_idle = false;
  
  m_status.stage = 4;
#ifdef DEBUG   
  printf("stage 4, begin feed\r\n");
#endif
}

// 5
// �ӳ�����
void stage_5(void)
{
  m_status.M_blow = true;
  m_status.M_exchange = true;
  m_status.M_fire = false;
  m_status.M_feed = true;
  m_status.M_starting = false;
  m_status.M_running_delay = true;
  m_status.M_running = false;
  m_status.M_idle = false;
  
  m_status.stage = 5;
  //
  osStatus status = osTimerStart(tmr_delayRunningHandle, DELAY_RUNNING);
  if (status != osOK)
    error(5);
#ifdef DEBUG 
  printf("stage 5, delay before run\r\n");
#endif
}


// 6
// ����
void stage_6(void)
{
  m_status.M_blow = true;
  m_status.M_exchange = true;
  m_status.M_fire = false;
  m_status.M_feed = true;
  m_status.M_starting = false;
  m_status.M_running_delay = false;
  m_status.M_running = true;
  m_status.M_idle = false;
  
  m_status.stage = 6;
  
#ifdef DEBUG   
  printf("stage 6, run\r\n");
#endif
}

// 7
// ֹͣ
void stop(char *s)
{
  stage_init();  
  
  // ֹͣ��ʱ�� todo: ����stageֹͣtimer
  osTimerStop(tmr_blowDustHandle);
  osTimerStop(tmr_preFeedHandle);
  osTimerStop(tmr_fireTimeoutHandle);
  osTimerStop(tmr_beforeFeedHandle);
  osTimerStop(tmr_delayRunningHandle);
  
#ifdef DEBUG 
  printf("stop:%s\r\n", s);
#endif
}


/* error ���� */
void error(uint8_t error_no)
{
  
  switch(error_no) {
    
  case 5:
    printf("start timer for delay_running is failed\r\n");
  case 13:
    printf("start timer for fire timeout is failed\r\n");
    
  default:
    
    printf("Error\r\n");
  }
}


/* �ж� */

void judge(Stove *p_stove, Triac *triac, RECIPE *recipe, uint8_t *power)
{
  uint8_t level_cur = 0;
  uint8_t next_level_blow;  // ��һ���ȼ����ͷ�
  uint8_t next_level_feed;  // ��һ���ȼ�������
  
  // �²� ��t
  int8_t delta_t = (int8_t)p_stove->pv - (p_stove->data).cv;

  // �²�С��1
  if (delta_t <= 1) {
    next_level_blow = *power;
    next_level_feed = recipe->level_0;
    level_cur = 1;
  }
  
  // �²�� 1��3
  if (delta_t>1 && delta_t<=3) {
    next_level_blow = *(power+1);
    next_level_feed = recipe->level_1;
    level_cur = 2;
  }
  
  // �²����3������ˮģʽ
  if (delta_t>3 || p_stove->boil_mode) {
    next_level_blow = *(power+2);
    next_level_feed = recipe->level_2;
    level_cur = 3;
  }
    
  // ����¯��
  if (p_stove->data.k >= STOVE_TEMP_LIMIT && (!p_stove->boil_mode)) {
    next_level_feed -= 50;  // ��1��,50��20ms    
  }
  // ��ֹ��ֵ����
  if (next_level_feed > 100) {
    next_level_feed = 50;
    info("feed value error");
  }
  if (next_level_blow > 10) {
    next_level_blow = 5;
    info("blow value error");
  }
  

  // ���� 1-->2 2-->3
  if (p_stove->level < level_cur) {
    // ������ʱ��_1
    osTimerStart(tmr_switch_delayHandle, UP_SMOOTH);
    info("up delay begin");
  }
  
  // ���� // 2-->1 3-->2
  if (p_stove->level > level_cur) {
    // ������ʱ��_2
    osTimerStart(tmr_switch_delayHandle, DOWN_SMOOTH);
    info("down delay begin");
  }
  
  // �ȱ��ϣ�������
  if (p_stove->level != level_cur) {
    p_stove->level = level_cur;
    // �ı�����
    triac->feed_duty = next_level_feed;
    // ��¼״̬
    p_stove->next_is_delaying = true;
    p_stove->next_v = next_level_blow;
    
    //printf("level_cur:%d\r\n", level_cur);
    
  }  
  else if (p_stove->level == level_cur) {
    triac->fan_smoke_power = next_level_blow;
    triac->feed_duty = next_level_feed;
  }    
  
  //
}

/* �趨��� */
void setTriac(uint8_t fan_smoke_power, uint8_t fan_exchange_delay, uint8_t feed_duty,
              M_Status *status, Triac *triac)
{
  triac->fan_smoke_power = fan_smoke_power;
  triac->fan_exchange_delay = fan_exchange_delay;
  triac->feed_duty = feed_duty; 
}

/* �����µ�CCRֵ */
uint32_t new_ccr(TIM_HandleTypeDef *htim, uint32_t delay)
{
  uint32_t count = __HAL_TIM_GET_COUNTER(htim);
  uint32_t ccr = count + delay;
  if (ccr > 0xffff)
    ccr = 0xffff - count + delay;
  
  return ccr;
}

/* ����Ƿ��ڼƻ��� */
void check_schedule(uint8_t h, uint8_t m, Schedule *plan, In_Schedule_Time *in)
{
  
  
  uint16_t start_1, end_1, start_2, end_2, cur;
  
  cur = h * 100 + m; 
  
  // ʱ���1
  start_1 = plan->start_0_hour * 100 + plan->start_0_min;
  end_1 = plan->end_0_hour * 100 + plan->end_0_min;  
  // ʱ���2
  start_2 = plan->start_1_hour * 100 + plan->start_1_min;
  end_2 = plan->end_1_hour * 100 + plan->end_1_min;
  
  if (cur >= start_1 && cur <= end_1) {
    in->in_schedule = true;
    
    in->start_hour = plan->start_0_hour;
    in->start_min = plan->start_0_min;
    in->end_hour = plan->end_0_hour;
    in->end_min = plan->end_0_min;
  }  
  else if (cur >= start_2 && cur <= end_2) {
    in->in_schedule = true;
    
    in->start_hour = plan->start_1_hour;
    in->start_min = plan->start_1_min;
    in->end_hour = plan->end_1_hour;
    in->end_min = plan->end_1_min;
  }
  else if (cur > end_1 && cur < start_2) {    
    in->in_schedule = false;
    
    in->start_hour = plan->start_1_hour;
    in->start_min = plan->start_1_min;
    in->end_hour = plan->end_1_hour;
    in->end_min = plan->end_1_min;    
  }
  else {    
    in->in_schedule = false;
    
    in->start_hour = plan->start_0_hour;
    in->start_min = plan->start_0_min;
    in->end_hour = plan->end_0_hour;
    in->end_min = plan->end_0_min;
  }
  
}

/* info��Ϣ��ӡ */
void info(char *info)
{
  printf("info:%s\r\n", info);
}


/* �ͷ��ٶȽ��� */
uint8_t v_blow(uint8_t s[])
{
  uint8_t v=0;

  if (strcmp((char const*)s,"b00") == 0) v = 0;
  if (strcmp((char const*)s,"b10") == 0) v = 10;
  if (strcmp((char const*)s,"b20") == 0) v = 20;
  if (strcmp((char const*)s,"b30") == 0) v = 30;
  if (strcmp((char const*)s,"b40") == 0) v = 40;
  if (strcmp((char const*)s,"b50") == 0) v = 50;
  if (strcmp((char const*)s,"b60") == 0) v = 60;
  if (strcmp((char const*)s,"b70") == 0) v = 70;
  if (strcmp((char const*)s,"b80") == 0) v = 80;
  if (strcmp((char const*)s,"b90") == 0) v = 90; 
  
  return v;
}

