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
extern osMessageQId Q_LogHandle;

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
  m_status.M_running_delay = false;
  m_status.M_running = false;
  m_status.M_stopping = false;
  m_status.M_idle = true;
  
  m_status.stage = -1;  
}

// �׶� 1-6
void stage(int8_t index)
{
  osStatus status;
  
  //ȫ���ر�  
  m_status.M_cooling = false;  // ��ȴ
  m_status.M_blow = false;
  m_status.M_exchange = false;
  m_status.M_fire = false;
  m_status.M_feed = false;
  
  m_status.M_starting = false;
  m_status.M_running_delay = false;
  m_status.M_running = false;
  m_status.M_stopping = false;
  
  switch(index) {
  case 0:  // ��ȴ
    m_status.M_cooling = true;
    m_status.M_blow = true;
    m_status.M_exchange = true;
    
    break;
    
  case 1:  // 1-����
    m_status.M_blow = true;
    m_status.M_idle = false;
    
    status = osTimerStart(tmr_blowDustHandle, BLOW_DUST);  // freertos: L1118
    if (status != osOK)
      stove_log(14);
    
    stove_log(1);
    
    break;
    
  case 2: // 2-Ԥ����
    status = osTimerStart(tmr_preFeedHandle, PRE_FEED);  // L1130
    if (status != osOK)
      stove_log(14);
    
    stove_log(2);
    
    break;
    
  case 3:  // 3-���
    m_status.M_blow = true;
    m_status.M_fire = true;
    
    status = osTimerStart(tmr_beforeFeedHandle, FIRE_UNTIL_FEED);  // L1166
    if (status != osOK)
      stove_log(3);
    
    // ��ʼ��¼���ʱ
    status = osTimerStart(tmr_fireTimeoutHandle, FIRE_TIME_OUT);
    if (status != osOK)
      stove_log(13);
    
    stove_log(3);
    
    break;
    
  case 4:  // 4-��ʼ����
    m_status.M_blow = true;
    m_status.M_fire = true;
    m_status.M_feed = true;
    
    stove_log(4);
    
    break;
    
  case 5:  // 5-�ӳ�����
    m_status.M_blow = true;
    m_status.M_exchange = true;
    m_status.M_feed = true;
    m_status.M_running_delay = true;  
    
    status = osTimerStart(tmr_delayRunningHandle, DELAY_RUNNING);
    if (status != osOK)
      stove_log(14);
    
    stove_log(5);
    
    break;
    
  case 6:  // 6-����
    m_status.M_blow = true;
    m_status.M_exchange = true;
    m_status.M_feed = true;
    m_status.M_running = true;
    
    stove_log(6);
    
    break;
    
  }
  m_status.stage = index;       
  
}


// ֹͣ
void stop(char *s)
{
  stage_init();    
  m_status.M_idle = true;
  
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


/* �ж� */

void judge(Stove *p_stove, Triac *triac, RECIPE *recipe, uint8_t *power)
{
  uint8_t level_cur = 0;
  uint8_t next_level_blow;  // ��һ���ȼ����ͷ�
  uint8_t next_level_feed;  // ��һ���ȼ�������
  
  // �²� ��t
  int8_t delta_t = (int8_t)p_stove->pv - (p_stove->data).cv;
  
  // �²�С��1
  if (delta_t<=1 || p_stove->pellet_warn) {
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
    //info("feed value error");
  }
  if (next_level_blow > 10) {
    next_level_blow = 5;
    //info("blow value error");
  }
  
  
  // ���� 1-->2 2-->3
  if (p_stove->level < level_cur) {
    // ������ʱ��_1
    osTimerStart(tmr_switch_delayHandle, UP_SMOOTH);
    //info("up delay begin");
  }
  
  // ���� // 2-->1 3-->2
  if (p_stove->level > level_cur) {
    // ������ʱ��_2
    osTimerStart(tmr_switch_delayHandle, DOWN_SMOOTH);
    //info("down delay begin");
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

/* �׶�״̬ */
void fn_info_stage(char *info, M_Status *status) 
{
  
  switch(status->stage) {
    
  case -1:
    strcpy(info, "\x88\x89"); // ֹͣ
    break;
  case 0:
    strcpy(info, "\xc4\xc5"); // ��ȴ
    break;
  case 1:
    strcpy(info, "\x80\x81 1"); // ���1
    break;
  case 2:
    strcpy(info, "\x80\x81 2"); // ���2
    break;
  case 3:
    strcpy(info, "\x80\x81 3"); // ���3
    break;  
  case 4:
    strcpy(info, "\x80\x81 4"); // ���4
    break;
  case 5:
    strcpy(info, "\x80\x81 5"); // ���5
    break;  
  case 6:
    strcpy(info, "\x98\x99"); // ����
    break;
    
  }  
  
}


/* ������־ */
void stove_log(uint32_t id)
{
  if (Q_LogHandle != NULL) {
    osMessagePut(Q_LogHandle, id, 0); 
  }
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

