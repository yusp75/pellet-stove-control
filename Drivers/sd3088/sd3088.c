/*
* SD3088ʱ��оƬ ��д
* �ļ� sd3088.h
* http://git.oschina.net/maizhi/small-pellet-sove-control-system
*���� ��
*�汾 v1.1
*/
#include "sd3088.h"


extern I2C_HandleTypeDef hi2c1;
extern osMutexId mutex_rtcHandle;

//RTC_Time Time_sd3088 = {0x55, 0x59, 0x2,0x1, 0x01, 0x12, 0x11, 0x14};	//��ʼ��ʱ��ṹ�����������ʱ�䣺2014��11��12�� 02:59:55 pm ����һ��
uint8_t  rtc_data[7];  //ͨ�����ݻ�����


/* ��ȡʱ�� */
uint8_t read_rtc(RTC_Time *rtc_time)
{
  uint8_t battery[2];  
  
  if(HAL_I2C_Mem_Read(&hi2c1, RTC_ADDR_READ, 0x0, 1, &rtc_data[0], 7, 1) == HAL_OK) {      
    rtc_time->second = BCD2DEC(rtc_data[0] & 0x7f);
    rtc_time->minute = BCD2DEC(rtc_data[1] & 0x7f);
    rtc_time->hour = BCD2DEC(rtc_data[2] & 0x3f);
    rtc_time->am_or_pm = (rtc_data[2] & 0x10) >> 7;
    rtc_time->flag_12or24 = (rtc_data[2] & 0x80) >> 8;
    rtc_time->week = rtc_data[3] & 0x07;
    // printf("w_read:%d,w:%d\r\n", rtc_data[3], rtc_time->week);
    rtc_time->day = BCD2DEC(rtc_data[4] & 0x3f);
    rtc_time->month = BCD2DEC(rtc_data[5] & 0x1f);
    rtc_time->year = BCD2DEC(rtc_data[6]);
  }
  
  // ���� 0x1a 0x1b
  if(HAL_I2C_Mem_Read(&hi2c1, RTC_ADDR_READ, 0x1a, 1, battery, 2, 1) == HAL_OK) {    
    rtc_time->quantity = (battery[0] >> 7) * 255 + battery[1];
    // printf("quality:%x\r\n",rtc_time->quantity);       
    
  }
  
  return 0;
}


/* ����д */
bool enable_write(void)
{
  bool result = false;
  uint8_t wcr = 0x80;
  
  // д 0x10  
  if(HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, 0x10, 1, &wcr, 1, 1) == HAL_OK) {    
    //  д 0x0f   
    wcr = 0xff;
    if(HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, 0x0f, 1, &wcr, 1, 1) == HAL_OK)
      result = true;    
  }
  
  return result;
  
}  


/* ��ֹд */
bool disable_write(void)
{
  bool result = false;
  uint8_t wcr = 0x7b;
  
  // д 0x0f  
  if(HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, 0x0f, 1, &wcr, 1, 1) == HAL_OK) {    
    //  д 0x10   
    wcr = 0x0;
    if(HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, 0x10, 1, &wcr, 1, 1) == HAL_OK)
      result = true;    
  }
  
  return result;
  
}


/* ����ʱ�� */
bool write_rtc(RTC_Time *t)
{
  bool result = false;
  // ��������
  uint16_t y, m , d;
  y = t->year + 2000;
  m = t->month;
  d = t->day;
  
  if (m ==1 || m ==2) {
    
    m += 12;  //��һ�ºͶ��¿�������һ���ʮ���º�ʮ���£����������2004-1-10����ɣ�2003-13-10�����빫ʽ����
    y--;
  }
  
  // ʹ�û�ķ����ɭ��ʽ��������
  uint16_t week = (d + 2*m + 3*(m+1)/5 + y + y/4 - y/100 + y/400 + 1) % 7;
  
  uint8_t data[7];
  data[0] = 0;
  data[1] = DEC2BCD(t->minute);
  data[2] = DEC2BCD(t->hour) | 0x80;
  data[3] = (uint8_t)week;
  // printf("w_write:%d\r\n", week);
  data[4] = DEC2BCD(t->day);
  data[5] = DEC2BCD(t->month);
  data[6] = DEC2BCD(t->year);
  
  
  if (enable_write()) {
    
    // дʱ������    
    if(HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, 0x0, 1, &data[0], 7, 1) == HAL_OK) {
      result = true;
    }
    
    disable_write();
  }
  
  return result;
}


/* �������ݵ�RTC */
bool save_to_rtc(uint16_t address, uint8_t *data, uint16_t len)
{
  bool result = false;
  
  if ( address<0x2c || address>0x71 || len>70 )
    return false;
  
  if (enable_write()) {    
    if (HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, address, 1, data, len, 1) == HAL_OK) {
      result = true;
    }
    disable_write();
  }
  
  return result;    
}


/* ��ȡ���ݴ�RTC */
bool read_from_rtc(uint16_t address, uint8_t *data, uint16_t len)
{
  
  bool result = false;
  
  if ( address<0x2c || address>0x71 || len>70 )
    return false;
  
  // ����
  
  if(HAL_I2C_Mem_Read(&hi2c1, RTC_ADDR_READ, address, 1, data, len, 1) == HAL_OK) {
    result = true;
  }
  
  return result;
}


/* ����������� */
bool save_recipe(int8_t *recipe)
{
  bool result = false;
  if (enable_write()) { 
    if (HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, RTC_ADDR_RECIPE, 1, 
                          (uint8_t *)recipe, RECIPE_DATA_LEN, 1) == HAL_OK) {
                            result = true;
                          }
    disable_write();
  }

  return result;
}

/* ��ȡ������� */
bool read_recipe(int8_t *data)
{
  bool result = false;
  if (HAL_I2C_Mem_Read(&hi2c1, RTC_ADDR_READ, RTC_ADDR_RECIPE, 1, 
                       (uint8_t *)data, RECIPE_DATA_LEN, 1) == HAL_OK) {
                         result = true;
                       }
  else {
    // ��ʼд�䷽Ĭ������    
    
  }
  
  return result; 
}


/* ����ƻ�ʱ�� */
bool save_schedule(Schedule *plan, uint8_t index)
{
  bool result = false;
  
  uint8_t address = RTC_ADDR_SCHEDULE + index * 9;
  if (enable_write()) { 
    if (HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, address, 1, 
                          (uint8_t *)plan, SCHEDULE_DATA_LEN, 1) == HAL_OK) {
                            result = true;
                          }
    disable_write();
  }

  return result;
}


/* ��ȡ�ƻ�ʱ�� */
bool read_schedule(Schedule *plan, uint8_t index)
{
  bool result = false;
  uint8_t address = RTC_ADDR_SCHEDULE + index * 9;
  
  
  if (HAL_I2C_Mem_Read(&hi2c1, RTC_ADDR_READ, address, 1, 
                       (uint8_t *)plan, SCHEDULE_DATA_LEN, 1) == HAL_OK) {
                         result = true;
                       }
  
  return result;
}


extern int8_t recipe[RECIPE_DATA_LEN];
extern Schedule plan[3];
extern uint8_t power_of_blow[3];

/* д��ʼ���� */
bool init_data(void)
{
  // ��ȡ�Ƿ��ʼ����־ 
  bool result = false;  
  uint8_t initialized = 0;
  
  result = read_from_rtc(RTC_ADDR_INIT, &initialized, 1);
  if (initialized !=2 ) {
    // д�䷽
    save_schedule(&plan[0], 0);
    save_schedule(&plan[0], 1);
    save_schedule(&plan[0], 2);
    // �ƻ�
    save_recipe(recipe);
    // �����ٶ�
    save_to_rtc(RTC_ADDR_POW, power_of_blow, 3);
    // �Ѿ���ʼ��
    initialized = 2;
    printf("initilaize!\r\n");
    save_to_rtc(RTC_ADDR_INIT, &initialized, 1);
  }
  
  return result;  
  
}

/* �����Ƿ�ԤԼ  */
void save_schedule_state(bool scheduled)
{
  save_to_rtc(RTC_ADDR_SCHEDULE_STATE, (uint8_t*)&scheduled, 1);
  
}