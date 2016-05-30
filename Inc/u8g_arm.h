/* �˻����� hmi
* hmi.h
*/

#ifndef _HMI_H
#define _HMI_H

#include "u8g.h"
#include "m2.h"
#include "m2ghu8g.h"
#include "stm32f1xx_hal.h"

#include "user.h"
#include "sd3088.h"
#include "stdbool.h"
#include "string.h"

#include "queue_log.h"


#define BUTTON_PORT GPIOA
#define BUTTON_PIN GPIO_PIN_0

// OLED ��Ŷ���
#define SH1106_RES_Port  GPIOC
#define SH1106_RES_Pin  GPIO_PIN_14
#define SH1106_DC_Port  GPIOC
#define SH1106_DC_Pin  GPIO_PIN_15
#define SH1106_CS_Port  GPIOA
#define SH1106_CS_Pin  (GPIO_PIN_8)

// OLED������
#define SH1106_CS_ENABLE SH1106_CS_Port->BSRR = (uint32_t)SH1106_CS_Pin << 16
#define SH1106_CS_DISABLE SH1106_CS_Port->BSRR = SH1106_CS_Pin

#define SH1106_DC_HIGH SH1106_DC_Port->BSRR = (uint32_t)SH1106_DC_Pin
#define SH1106_DC_LOW SH1106_DC_Port->BSRR = (uint32_t)SH1106_DC_Pin << 16

#define SH1106_RESET SH1106_RES_Port->BSRR = SH1106_RES_Pin;

#define LOG_DISPLAY_NUM 10  // ��־��Ϣ��ʾ����
#define LOG_MESSAGE_NUM 12

// extern
extern u8g_t u8g;


void hmi(void);
uint8_t u8g_com_hw_spi_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr);
void init_draw(void);
void draw(void);
void fn_ok(m2_el_fnarg_p fnarg);

// hmi
// rtc: button Cancel, ok
void fn_rtc_btn_ok(m2_el_fnarg_p fnarg);
// ������ ok
void fn_schedule_btn_ok(m2_el_fnarg_p fnarg);
void fn_schedule_btn_cancel(m2_el_fnarg_p fnarg);
// �䷽ ok cancel
void fn_sel_rp_ok(m2_el_fnarg_p fnarg);
void fn_sel_rp_edit(m2_el_fnarg_p fnarg);

void fn_recipe_ok(m2_el_fnarg_p fnarg);
void fn_recipe_cancel(m2_el_fnarg_p fnarg);
// ���̹��� ok cancel
void fn_pow_ok(m2_el_fnarg_p fnarg);
void fn_pow_cancel(m2_el_fnarg_p fnarg);
// ���ز˵�
void fn_back_menu();

/* ȷ���˳�Diaglog */
// OK
void fn_status_ok();
void schedule_exit_confirm();
//
void fn_status_cancel();
void change_root(void);
/* �Ƿ�λ��home���� */
uint8_t is_home_screen(void);
/* �Ƿ�λ��status���� */
uint8_t is_status_or_log_screen(void);
/* �Ƿ�λ���ֶ����ϴ��� */
uint8_t is_feed_manual_screen(void);
// ��ȡ��Ļ��
uint8_t get_screen_no(void);  
/* ������Ű��� */
void set_pin_key(uint8_t screen);
/* �ֶ����ϰ�ť */
void fn_feed_manual_ok ();
void fn_feed_manual_cancel ();

/* ������ */
void fn_pellet_ok();

void fn_log_up_down(uint8_t dir); // log��־
#endif