/*
*�ļ� sd3088.h
*/
#ifndef SD3088_H
#define SD3088_H

#include "user.h"
//#include "malloc.h"

#define RTC_ADDR_READ 0x65  //RTC���� ����ַ
#define RTC_ADDR_WRITE 0x64  //RTC���� д��ַ

#define IDcode 0x72  //8�ֽ�ID����ʼ��ַ
#define	Bat_High_Addr 0x1A  //�������λ�Ĵ�����ַ
#define	Bat_Low_Addr 0x1B  //�����Ͱ�λ�Ĵ�����ַ

#define BCD2DEC(x) (((x) >> 4) * 10 + ((x) & 0x0f))
#define DEC2BCD(x) ((((x)/10) <<4 ) + ((x)%10))

extern RTC_Time Time_sd3088;	//��ʼ��ʱ�䣨����ʱ�䣺2014��11��12�� 14:59:55  ����һ��

/* ��ȡʱ�� */
uint8_t read_rtc(RTC_Time *rtc_time);
/* ����ʱ�� */
bool write_rtc(RTC_Time *t);
/* �������ݵ�RTC */
bool save_to_rtc(uint16_t address, uint8_t *data, uint16_t len);
/* ��ȡ���ݴ�RTC */
bool read_from_rtc(uint16_t address, uint8_t *data, uint16_t len);
/* ����������� */
bool save_recipe(int8_t *recipe);
/* ��ȡ������� */
bool read_recipe(int8_t *data);
/* ����ƻ�ʱ�� */
bool save_schedule(Schedule *plan, uint8_t index);
/* ��ȡ�ƻ�ʱ�� */
bool read_schedule(Schedule *plan, uint8_t index);
/* д��ʼ���� */
bool init_data(void);
/* �����Ƿ�ԤԼ  */
void save_schedule_state(bool scheduled);
#endif