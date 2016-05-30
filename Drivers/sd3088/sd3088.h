/*
*文件 sd3088.h
*/
#ifndef SD3088_H
#define SD3088_H

#include "user.h"
//#include "malloc.h"

#define RTC_ADDR_READ 0x65  //RTC器件 读地址
#define RTC_ADDR_WRITE 0x64  //RTC器件 写地址

#define IDcode 0x72  //8字节ID号起始地址
#define	Bat_High_Addr 0x1A  //电量最高位寄存器地址
#define	Bat_Low_Addr 0x1B  //电量低八位寄存器地址

#define BCD2DEC(x) (((x) >> 4) * 10 + ((x) & 0x0f))
#define DEC2BCD(x) ((((x)/10) <<4 ) + ((x)%10))

extern RTC_Time Time_sd3088;	//初始化时间（设置时间：2014年11月12日 14:59:55  星期一）

/* 读取时间 */
uint8_t read_rtc(RTC_Time *rtc_time);
/* 保存时间 */
bool write_rtc(RTC_Time *t);
/* 保存数据到RTC */
bool save_to_rtc(uint16_t address, uint8_t *data, uint16_t len);
/* 读取数据从RTC */
bool read_from_rtc(uint16_t address, uint8_t *data, uint16_t len);
/* 保存配比数据 */
bool save_recipe(int8_t *recipe);
/* 读取配比数据 */
bool read_recipe(int8_t *data);
/* 保存计划时间 */
bool save_schedule(Schedule *plan, uint8_t index);
/* 读取计划时间 */
bool read_schedule(Schedule *plan, uint8_t index);
/* 写初始数据 */
bool init_data(void);
/* 保存是否预约  */
void save_schedule_state(bool scheduled);
#endif