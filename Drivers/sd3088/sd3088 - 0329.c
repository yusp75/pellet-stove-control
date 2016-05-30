/*
* SD3088时钟芯片 读写
* 文件 sd3088.h
* http://git.oschina.net/maizhi/small-pellet-sove-control-system
*作者 于
*版本 v1.1
*/
#include "sd3088.h"


extern I2C_HandleTypeDef hi2c1;
extern osMutexId mutex_rtcHandle;

RTC_Time Time_sd3088 = {0x55, 0x59, 0x2,0x1, 0x01, 0x12, 0x11, 0x14};	//初始化时间结构体变量（设置时间：2014年11月12日 02:59:55 pm 星期一）
uint8_t  rtc_data[7];  //通用数据缓存器


/* 读取时间 */
uint8_t read_rtc(void)
{
  uint8_t battery[2];
  
  if (mutex_rtcHandle != NULL) {  
    osStatus status = osMutexWait(mutex_rtcHandle, osWaitForever);
    if (status == osOK) {      
      if(HAL_I2C_Mem_Read(&hi2c1, RTC_ADDR_READ, 0x0, 1, &rtc_data[0], 7, 1) == HAL_OK) {
        
        Time_sd3088.second = BCD2DEC(rtc_data[0] & 0x7f);
        Time_sd3088.minute = BCD2DEC(rtc_data[1] & 0x7f);
        Time_sd3088.hour = BCD2DEC(rtc_data[2] & 0x1f);
        Time_sd3088.am_or_pm = (rtc_data[2] & 0x10) >> 7;
        Time_sd3088.flag_12or24 = (rtc_data[2] & 0x80) >> 8;
        Time_sd3088.week = BCD2DEC(rtc_data[3] & 0x07);
        Time_sd3088.day = BCD2DEC(rtc_data[4] & 0x3f);
        Time_sd3088.month = BCD2DEC(rtc_data[5] & 0x1f);
        Time_sd3088.year = BCD2DEC(rtc_data[6]);
        
      }
      
      // 电量 0x1a 0x1b
      if(HAL_I2C_Mem_Read(&hi2c1, RTC_ADDR_READ, 0x1a, 1, battery, 2, 1) == HAL_OK) {    
        Time_sd3088.quantity = (battery[0] >> 7) * 255 + battery[1];
        // printf("quality:%x\r\n",Time_sd3088.quantity);
        
      }
      osMutexRelease(mutex_rtcHandle);
    }
        
  }
return 0;
}


/* 允许写 */
bool enable_write(void)
{
  bool result = false;
  uint8_t wcr = 0x80;
  
  // 写 0x10  
  if(HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, 0x10, 1, &wcr, 1, 1) == HAL_OK) {    
    //  写 0x0f   
    wcr = 0xff;
    if(HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, 0x0f, 1, &wcr, 1, 1) == HAL_OK)
      result = true;
    
  }
  
  return result;
  
}  


/* 禁止写 */
bool disable_write(void)
{
  bool result = false;
  uint8_t wcr = 0x7b;
  
  // 写 0x0f  
  if(HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, 0x0f, 1, &wcr, 1, 1) == HAL_OK) {    
    //  写 0x10   
    wcr = 0x0;
    if(HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, 0x10, 1, &wcr, 1, 1) == HAL_OK)
      result = true;
    
  }
  
  return result;
  
}


/* 保存时间 */
bool write_rtc(RTC_Time *t)
{
  bool result = false;
  // 计算星期
  uint8_t y, m , d;
  y = t->year + 2000;
  m = t->month;
  d = t->day;
  
  if (m ==1 || m ==2) {
    
    m += 12;  //把一月和二月看成是上一年的十三月和十四月，例：如果是2004-1-10则换算成：2003-13-10来代入公式计算
    y--;
  }
  
  // 使用基姆拉尔森公式计算星期
  uint8_t week = (d + 2*m + 3 * (m+1) / 5 + y + y/4 - y/100 + y/400) % 7;
  
  uint8_t data[7];
  data[0] = 0;
  data[1] = DEC2BCD(t->minute);
  data[2] = DEC2BCD(t->hour) | 0x80;
  data[3] = week;
  data[4] = DEC2BCD(t->day);
  data[5] = DEC2BCD(t->month);
  data[6] = DEC2BCD(t->year);
  
  if (mutex_rtcHandle != NULL) {  
    osStatus status = osMutexWait(mutex_rtcHandle, osWaitForever);
    if (status == osOK) { 
      if (enable_write()) {
        
        // 写时间数据    
        if(HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, 0x0, 1, &data[0], 7, 1) == HAL_OK) {
          result = true;
        }
        
        disable_write();
      }
      osMutexRelease(mutex_rtcHandle);
    }
    
  }
  return result;
}


/* 保存数据到RTC */
bool save_to_rtc(uint16_t address, uint8_t *data, uint16_t len)
{
  bool result = false;
  
  if ( (address<0x2c) || (address>0x71) || (len>70) )
    return false;
  
  if (mutex_rtcHandle != NULL) {  
    osStatus status = osMutexWait(mutex_rtcHandle, osWaitForever);
    if (status == osOK) { 
      if (enable_write()) {    
        if (HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, address, 1, data, len, 1) == HAL_OK) {
          result = true;
         }
        disable_write();
      }
      osMutexRelease(mutex_rtcHandle);
    }
    // 读回做检验？    
  }
  return result;    
}


/* 读取数据从RTC */
bool read_from_rtc(uint16_t address, uint8_t *data, uint16_t len)
{
  
  bool result = false;
  
  if ( (address<0x2c) || (address>0x71) || (len>70) )
    return false;
  
  // 读回
  if (mutex_rtcHandle != NULL) {  
    osStatus status = osMutexWait(mutex_rtcHandle, osWaitForever);
    if (status == osOK) { 
      if(HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, address, 1, data, len, 1) == HAL_OK) {
        result = true;
      }
      osMutexRelease(mutex_rtcHandle);
    }    
  }
  return result;
}


/* 保存配比数据 */
bool save_recipe(int8_t *recipe)
{
  bool result = false;
  // 配比9个数据； 索引1个数据
  if (mutex_rtcHandle != NULL) {  
    osStatus status = osMutexWait(mutex_rtcHandle, osWaitForever);
    if (status == osOK) { 
      if (enable_write()) { 
        if (HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, RTC_ADDR_RECIPE, 1, 
                              (uint8_t *)recipe, RECIPE_INDEX, 1) == HAL_OK) {
                                result = true;
                              }
        disable_write();
      }
      osMutexRelease(mutex_rtcHandle);
    }    
  }
  return result;
}

/* 读取配比数据 */
bool read_recipe(int8_t *data)
{
  bool result = false;
  // 配比9个数据； 索引1个数据
  if (mutex_rtcHandle != NULL) {  
    osStatus status = osMutexWait(mutex_rtcHandle, osWaitForever);
    if (status == osOK) { 
      if (enable_write()) {
        if (HAL_I2C_Mem_Read(&hi2c1, RTC_ADDR_WRITE, RTC_ADDR_RECIPE, 1, 
                             (uint8_t *)data, RECIPE_INDEX, 1) == HAL_OK) {
                               result = true;
                             }
        else {
          // 初始写配方默认数据
          
          
        }
        osMutexRelease(mutex_rtcHandle);
      }    
    }
    
  }
 return result; 
}


/* 保存计划时间 */
bool save_schedule(Schedule *plan, uint8_t index)
{
  bool result = false;
  uint8_t address = RTC_ADDR_SCHEDULE + index * 9;
  
  if (mutex_rtcHandle != NULL) {  
    osStatus status = osMutexWait(mutex_rtcHandle, osWaitForever);
    if (status == osOK) { 
      if (enable_write()) { 
        if (HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR_WRITE, address, 1, 
                              (uint8_t *)plan, SCHEDULE_DATA_LEN, 1) == HAL_OK) {
                                result = true;
        }
        disable_write();
      }
      osMutexRelease(mutex_rtcHandle);
    }    
  }
  return result;
}


/* 读取计划时间 */
bool read_schedule(Schedule *plan, uint8_t index)
{
  bool result = false;
  uint8_t address = RTC_ADDR_SCHEDULE + index * 9;
  
  if (mutex_rtcHandle != NULL) {  
    osStatus status = osMutexWait(mutex_rtcHandle, osWaitForever);
    if (status == osOK) { 
      
      if (HAL_I2C_Mem_Read(&hi2c1, RTC_ADDR_WRITE, address, 1, 
                           (uint8_t *)plan, SCHEDULE_DATA_LEN, 1) == HAL_OK) {
                             result = true;
                           }
      
      osMutexRelease(mutex_rtcHandle);
    }    
  }
  return result;
}


extern int8_t recipe[RECIPE_INDEX];
extern Schedule plan[3];
extern uint8_t power_of_blow[3];

/* 写初始数据 */
bool init_data(void)
{
  // 读取是否初始化标志 
  bool result = false;  
  uint8_t initialized = 0;
  
  result = read_from_rtc(RTC_ADDR_INIT, &initialized, 1);
  if (initialized !=2 ) {
    // 写配方
    save_schedule(&plan[0], 0);
    save_schedule(&plan[0], 1);
    save_schedule(&plan[0], 2);
    // 计划
    save_recipe(recipe);
    // 排烟速度
    save_to_rtc(RTC_ADDR_POW, power_of_blow, 3);
    // 已经初始化
    initialized = 2;
    printf("initilaize!\r\n");
    save_to_rtc(RTC_ADDR_INIT, &initialized, 1);
  }
  
  return result;  
  
}