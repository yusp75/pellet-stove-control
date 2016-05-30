#include "u8g_arm.h"

u8g_t u8g;

extern SPI_HandleTypeDef hspi1;
extern __IO uint8_t PV;

extern RTC_Time rtc_time;
extern char *p_rtc_display;
extern char *p_info_cv_pv;
extern const char *log_message[];  // 报警信息数据

/* 窗口定义开始 */
M2_EXTERN_ALIGN(top_el_2l_menu);  // 菜单预先声明

// 1-主窗口
// 第1行：状态， 档位
extern char *p_info_stage;  // 阶段状态
char *power_level = "\x7f\x7f\x7f";

M2_LABELPTR(el_home_stage, "x1y47", (char const**)&p_info_stage);  // 显示工作阶段
M2_LABELPTR(el_home_level, "x1y47", (char const**)&power_level);  // 工作档位，处于工作时显示

//第2行： 室温，设置温度
M2_LABELPTR(el_home_lbl_cv_pv, "x1y31", (char const**)&p_info_cv_pv); //设置

//第3行：是否预约，或者预约启动时间
extern char *p_schedule_time;
M2_LABELPTR(el_home_schedule_time, "x1y16", (char const**)&p_schedule_time);

//第4行：时钟， 星期
M2_LABELPTR(el_home_rtc, "x1y1", (char const**)&p_rtc_display);
M2_LIST(list_h2) = {&el_home_stage, &el_home_level,&el_home_lbl_cv_pv, 
                    &el_home_schedule_time, &el_home_rtc};

M2_XYLIST(el_top_home, NULL, list_h2);

/*  2-设置时钟窗口 */

M2_S8NUM(el_u8_year, "c2", 15, 99, &rtc_time.year);
M2_LABEL(el_lbl_year, NULL, "\x91");  // 年

M2_S8NUM(el_u8_month, "c2", 1, 12, &rtc_time.month);
M2_LABEL(el_lbl_month, NULL, "\x92");  // 月

M2_S8NUM(el_u8_day, "c2", 1, 31, &rtc_time.day);
M2_LABEL(el_lbl_day, NULL, "\x93");  // 日

M2_S8NUM(el_u8_hour, "c2", 0, 23, &rtc_time.hour);
M2_LABEL(el_lbl_hour, NULL, "\x8f");  // 时

M2_S8NUM(el_u8_minute, "c2", 0, 59, &rtc_time.minute);
M2_LABEL(el_lbl_minute, NULL, "\x94");  // 分

M2_LIST(list_rtc_h1) = {&el_u8_year, &el_lbl_year, &el_u8_month, &el_lbl_month, &el_u8_day, &el_lbl_day };
M2_HLIST(hlist_rtc_h1, "h15", list_rtc_h1);

M2_LIST(list_rtc_h2) = {&el_u8_hour, &el_lbl_hour, &el_u8_minute, &el_lbl_minute};
M2_HLIST(hlist_rtc_h2, "h15", list_rtc_h2);

// 按钮
M2_BUTTON(el_btn_rtc_ok, NULL, "\x84\x85",fn_rtc_btn_ok);  // 确定
M2_BUTTON(el_btn_rtc_cancel, NULL, "\x86\x87", fn_back_menu);  // 取消
M2_SPACE(el_rtc_space, "w6h1");
M2_LIST(list_rtc_btns) = {&el_btn_rtc_ok, &el_rtc_space, &el_btn_rtc_cancel};
M2_HLIST(hlist_rtc_h3, "h15", list_rtc_btns);

M2_LIST(list_rtc_v) = {&hlist_rtc_h1, &hlist_rtc_h2, &hlist_rtc_h3};
M2_VLIST(el_top_rtc, "h15", list_rtc_v);


/* 3- 工作日时间段 周1-周5 */
extern Schedule plan[3];

// 01 工作日
M2_LABEL(el_lbl_day0, NULL, "\xa7\xa8\x93");
M2_TOGGLE(el_toggle_day0, NULL, &plan[0].enabled);
M2_LIST(list_plan_day0_h1) = {&el_lbl_day0, &el_toggle_day0};
M2_HLIST(hlist_day_0_h1, "h15", list_plan_day0_h1);

// 0 时间段1
M2_S8NUM(el_u8_day0_s1_h, "c2", 0, 23, &plan[0].start_0_hour);
M2_S8NUM(el_u8_day0_s1_m, "c2", 0, 59, &plan[0].start_0_min);
M2_LABEL(el_lbl_day1_sep, NULL, ":");
M2_LABEL(el_lbl_day1_sep2, NULL, "---");
M2_S8NUM(el_u8_day0_e1_h, "c2", 0, 23, &plan[0].end_0_hour);
M2_S8NUM(el_u8_day0_e1_m, "c2", 0, 59, &plan[0].end_0_min);
M2_LIST(list_plan_day0_h2) = {&el_u8_day0_s1_h, &el_lbl_day1_sep, &el_u8_day0_s1_m, \
  &el_lbl_day1_sep2, &el_u8_day0_e1_h, &el_lbl_day1_sep,&el_u8_day0_e1_m};
M2_HLIST(hlist_day_0_h2, "h15", list_plan_day0_h2);

// 0 时间段2
M2_S8NUM(el_u8_day0_s2_h, "c2", 0, 23, &plan[0].start_1_hour);
M2_S8NUM(el_u8_day0_s2_m, "c2", 0, 59, &plan[0].start_1_min);
//M2_LABEL(el_lbl_day1_sep, NULL, ":");
//M2_LABEL(el_lbl_day1_sep2, NULL, "---");
M2_S8NUM(el_u8_day0_e2_h, "c2", 0, 23, &plan[0].end_1_hour);
M2_S8NUM(el_u8_day0_e2_m, "c2", 0, 59, &plan[0].end_1_min);
M2_LIST(list_plan_day0_h3) = {&el_u8_day0_s2_h, &el_lbl_day1_sep, &el_u8_day0_s2_m, \
  &el_lbl_day1_sep2, &el_u8_day0_e2_h, &el_lbl_day1_sep,&el_u8_day0_e2_m};
M2_HLIST(hlist_day_0_h3, "h15", list_plan_day0_h3);

// 按钮
M2_BUTTON(el_btn_day0_ok, NULL, "\x84\x85",fn_schedule_btn_ok);  // 确定 保存起始时间
M2_BUTTON(el_btn_day0_cancel, NULL, "\x86\x87", fn_schedule_btn_cancel);  // 取消
M2_SPACE(el_day0_space, "w6h1");
M2_LIST(list_day0_btns) = {&el_btn_day0_ok, &el_day0_space, &el_btn_day0_cancel};
M2_HLIST(hlist_day0_h4, "h15", list_day0_btns);


M2_LIST(list_day0_v) = {&hlist_day_0_h1, &hlist_day_0_h2, &hlist_day_0_h3, &hlist_day0_h4};
M2_VLIST(vlist_day_0, NULL, list_day0_v);


// 06 星期六
M2_LABEL(el_lbl_day6, NULL, "\x96\x97\xa6");
M2_TOGGLE(el_toggle_day6, "h15", &plan[0].enabled);
M2_LIST(list_plan_day6_h1) = {&el_lbl_day6, &el_toggle_day6};
M2_HLIST(hlist_day_6_h1, "h15", list_plan_day6_h1);

// 6 时间段1
M2_S8NUM(el_u8_day6_s1_h, "c2", 0, 23, &plan[1].start_0_hour);
M2_S8NUM(el_u8_day6_s1_m, "c2", 0, 59, &plan[1].start_0_min);
//M2_LABEL(el_lbl_day1_sep, NULL, ":");
//M2_LABEL(el_lbl_day1_sep2, NULL, "---");
M2_S8NUM(el_u8_day6_e1_h, "c2", 0, 23, &plan[1].end_0_hour);
M2_S8NUM(el_u8_day6_e1_m, "c2", 0, 59, &plan[1].end_0_min);
M2_LIST(list_plan_day6_h2) = {&el_u8_day6_s1_h, &el_lbl_day1_sep, &el_u8_day6_s1_m, \
  &el_lbl_day1_sep2, &el_u8_day6_e1_h, &el_lbl_day1_sep,&el_u8_day6_e1_m};
M2_HLIST(hlist_day_6_h2, "h15", list_plan_day6_h2);

// 6 时间段2
M2_S8NUM(el_u8_day6_s2_h, "c2", 0, 23, &plan[1].start_1_hour);
M2_S8NUM(el_u8_day6_s2_m, "c2", 0, 59, &plan[1].start_1_min);
//M2_LABEL(el_lbl_day1_sep, NULL, ":");
//M2_LABEL(el_lbl_day1_sep2, NULL, "---");
M2_S8NUM(el_u8_day6_e2_h, "c2", 0, 23, &plan[1].end_1_hour);
M2_S8NUM(el_u8_day6_e2_m, "c2", 0, 59, &plan[1].end_1_min);
M2_LIST(list_plan_day6_h3) = {&el_u8_day6_s2_h, &el_lbl_day1_sep, &el_u8_day6_s2_m, \
  &el_lbl_day1_sep2, &el_u8_day6_e2_h, &el_lbl_day1_sep,&el_u8_day6_e2_m};
M2_HLIST(hlist_day6_h3, "h15", list_plan_day6_h3);

M2_BUTTON(el_btn_day6_ok, NULL, "\x84\x85",fn_schedule_btn_ok);  // 确定 保存起始时间
M2_BUTTON(el_btn_day6_cancel, NULL, "\x86\x87", fn_schedule_btn_cancel);  // 取消
M2_SPACE(el_day6_space, "w6h1");
M2_LIST(list_day6_btns) = {&el_btn_day6_ok, &el_day6_space, &el_btn_day6_cancel};
M2_HLIST(hlist_day6_h4, "h15", list_day6_btns);


M2_LIST(list_day6_v) = {&hlist_day_6_h1, &hlist_day_6_h2, &hlist_day6_h3, &hlist_day6_h4};
M2_VLIST(vlist_day_6, NULL, list_day6_v);


// 07 星期天
M2_LABEL(el_lbl_day7, NULL, "\x96\x97\x93");
M2_TOGGLE(el_toggle_day7, "h15", &plan[0].enabled);
M2_LIST(list_plan_day7_h1) = {&el_lbl_day7, &el_toggle_day7};
M2_HLIST(hlist_day_7_h1, "h15", list_plan_day7_h1);

// 7 时间段1
M2_S8NUM(el_u8_day7_s1_h, "c2", 0, 23, &plan[2].start_0_hour);
M2_S8NUM(el_u8_day7_s1_m, "c2", 0, 59, &plan[2].start_0_min);
//M2_LABEL(el_lbl_day1_sep, NULL, ":");
//M2_LABEL(el_lbl_day1_sep2, NULL, "---");
M2_S8NUM(el_u8_day7_e1_h, "c2", 0, 23, &plan[2].end_0_hour);
M2_S8NUM(el_u8_day7_e1_m, "c2", 0, 59, &plan[2].end_0_min);
M2_LIST(list_plan_day7_h2) = {&el_u8_day7_s1_h, &el_lbl_day1_sep, &el_u8_day7_s1_m, \
  &el_lbl_day1_sep2, &el_u8_day7_e1_h, &el_lbl_day1_sep,&el_u8_day7_e1_m};
M2_HLIST(hlist_day_7_h2, "h15", list_plan_day7_h2);

// 7 时间段2
M2_S8NUM(el_u8_day7_s2_h, "c2", 0, 23, &plan[2].start_1_hour);
M2_S8NUM(el_u8_day7_s2_m, "c2", 0, 59, &plan[2].start_1_min);
//M2_LABEL(el_lbl_day1_sep, NULL, ":");
//M2_LABEL(el_lbl_day1_sep2, NULL, "---");
M2_S8NUM(el_u8_day7_e2_h, "c2", 0, 23, &plan[2].end_1_hour);
M2_S8NUM(el_u8_day7_e2_m, "c2", 0, 59, &plan[2].end_1_min);
M2_LIST(list_plan_day7_h3) = {&el_u8_day7_s2_h, &el_lbl_day1_sep, &el_u8_day7_s2_m, \
  &el_lbl_day1_sep2, &el_u8_day7_e2_h, &el_lbl_day1_sep,&el_u8_day7_e2_m};
M2_HLIST(hlist_day7_h3, "h15", list_plan_day7_h3);

M2_BUTTON(el_btn_day7_ok, NULL, "\x84\x85", fn_schedule_btn_ok);  // 确定 保存起始时间
M2_BUTTON(el_btn_day7_cancel, NULL, "\x86\x87", fn_schedule_btn_cancel);  // 取消
M2_SPACE(el_day7_space, "w6h1");
M2_LIST(list_day7_btns) = {&el_btn_day7_ok, &el_day7_space, &el_btn_day7_cancel};
M2_HLIST(hlist_day7_h4, "h15", list_day7_btns);


M2_LIST(list_day7_v) = {&hlist_day_7_h1, &hlist_day_7_h2, &hlist_day7_h3, &hlist_day7_h4};
M2_VLIST(vlist_day_7, NULL, list_day7_v);


/* 4- 配方 */
extern uint8_t recipe_index;
extern int8_t recipe[SCHEDULE_DATA_LEN];

// 选择窗口 ------ （1）
M2_LABEL(el_lbl_sel_title, NULL, "\x9e\x9f");

M2_BOX(el_box_sel_h1,"h1w100");
M2_LIST(list_sel_rp_h1) = {&el_box_sel_h1};
M2_HLIST(hlist_sel_rp_h1, "h3", list_sel_rp_h1);

M2_SPACE(el_rp_sel_space, "w6h6");

M2_LABEL(el_lbl_sel_c0, NULL, "1# ");
M2_RADIO(el_radio_rp0, "h15v0", &recipe_index);
M2_LABEL(el_lbl_sel_c1, NULL, "2# ");
M2_RADIO(el_radio_rp1, "h15v1", &recipe_index);
M2_LABEL(el_lbl_sel_c2, NULL, "3# ");
M2_RADIO(el_radio_rp2, "h15v2", &recipe_index);
M2_LIST(list_rdio_rp) = {&el_lbl_sel_c0, &el_radio_rp0, &el_rp_sel_space,
                         &el_lbl_sel_c1, &el_radio_rp1, &el_rp_sel_space,
                         &el_lbl_sel_c2, &el_radio_rp2                        
};

M2_HLIST(hlist_radio_rp, "h15", list_rdio_rp);

M2_BUTTON(el_btn_sel_ok, NULL, "\x84\x85", fn_sel_rp_ok);
M2_BUTTON(el_btn_sel_cancel, NULL, "\x86\x87", fn_back_menu);
//M2_BUTTON(el_btn_sel_edit, NULL, "Edit", fn_sel_rp_edit);
M2_LIST(list_sel_btns) = {&el_btn_sel_cancel, &el_btn_sel_ok};
M2_HLIST(hlist_btns_rp, "h15", list_sel_btns);

M2_LIST(list_sel_rp_v) = {&el_lbl_sel_title, &hlist_sel_rp_h1, &hlist_radio_rp, 
                          &hlist_btns_rp};
M2_VLIST(vlist_sel_rp, NULL, list_sel_rp_v);

// 编辑窗口 ------ （2）
M2_LABEL(el_lbl_rp0, NULL, "\x9e\x9f 1 ");
M2_S8NUM(el_p0_c0, "c2", 20, 60, &recipe[0]);
M2_S8NUM(el_p0_c1, "c2", 20, 70, &recipe[1]);
M2_S8NUM(el_p0_c2, "c2", 20, 80, &recipe[2]);
M2_LIST(list_rp0) = {&el_day7_space, &el_lbl_rp0, &el_p0_c0, &el_p0_c1, &el_p0_c2};
M2_HLIST(hlist_rp0, "h15", list_rp0);

M2_LABEL(el_lbl_rp1, NULL, "\x9e\x9f 2 ");
M2_S8NUM(el_p1_c0, "c2", 20, 60, &recipe[3]);
M2_S8NUM(el_p1_c1, "c2", 20, 70, &recipe[4]);
M2_S8NUM(el_p1_c2, "c2", 20, 80, &recipe[5]);
M2_LIST(list_rp1) = { &el_day7_space, &el_lbl_rp1, &el_p1_c0, &el_p1_c1, &el_p1_c2};
M2_HLIST(hlist_rp1, "h15", list_rp1);


M2_LABEL(el_lbl_rp2, NULL, "\x9e\x9f 3 ");
M2_S8NUM(el_p2_c0, "c2", 20, 60, &recipe[6]);
M2_S8NUM(el_p2_c1, "c2", 20, 70, &recipe[7]);
M2_S8NUM(el_p2_c2, "c2", 20, 80, &recipe[8]);
M2_LIST(list_rp2) = {&el_day7_space, &el_lbl_rp2, &el_p2_c0, &el_p2_c1, &el_p2_c2};
M2_HLIST(hlist_rp2, "h15", list_rp2);

M2_BUTTON(el_rp_cancel, NULL, "\x86\x87", fn_recipe_cancel);
M2_BUTTON(el_rp_ok, NULL, "\x84\x85 ", fn_recipe_ok);
M2_LIST(el_btn_rp) = {&el_rp_ok, &el_rp_cancel};
M2_HLIST(hlist_rp_btn, "h15", el_btn_rp);

M2_LIST(list_rp) = {&hlist_rp0, &hlist_rp1, &hlist_rp2, &hlist_rp_btn};
M2_VLIST(el_top_rp, NULL, list_rp);

/* 5- 消息窗口 */
extern char info_message[8];
extern char *p_info_message;

M2_LABELPTR(el_message, "h15", (char const**)&p_info_message);
M2_BUTTON(el_message_ok, NULL, "\x84\x85", fn_back_menu);
M2_LIST(list_message) = {&el_message, &el_message_ok};
M2_VLIST(el_top_message, NULL, list_message);

/* 6- 排烟风机调节  */
extern uint8_t power_of_blow[3];

M2_SPACE(el_pow_space, "w6h1");
//M2_LABEL(el_lbl_pow_h0, "h15", "\xa0\xa9");
M2_LABEL(el_lbl_pow_h1, NULL, "\xa0\xa9");
M2_LIST(list_pow_h0) = {&el_lbl_pow_h1};
M2_HLIST(hlist_pow_h0, "h15", list_pow_h0);

M2_BOX(el_box_h1,"h1w100");
M2_LIST(list_pow_h1) = {&el_box_h1};
M2_HLIST(hlist_pow_h1, "h3", list_pow_h1);

M2_LABEL(el_lbl_pow_h5, NULL, "\x9c\xbf ");
M2_S8NUM(el_pow_c1, "c2", 0, 7, (int8_t*)&power_of_blow[0]);
M2_S8NUM(el_pow_c2, "c2", 0, 7, (int8_t*)&power_of_blow[1]);
M2_S8NUM(el_pow_c3, "c2", 0, 7, (int8_t*)&power_of_blow[2]);
M2_LIST(list_pow_h5) = {&el_lbl_pow_h5, &el_pow_c1, &el_pow_c2, &el_pow_c3};
M2_HLIST(hlist_pow_h5, "h15", list_pow_h5);

M2_BUTTON(el_pow_cancel, "c2", "\x86\x87", fn_pow_cancel);
M2_BUTTON(el_pow_ok, "c2", "\x84\x85 ", fn_pow_ok);
M2_LIST(list_pow_btn) = {&el_pow_ok, &el_pow_cancel};
M2_HLIST(hlist_pow_btn, "h15", list_pow_btn);

M2_LIST(list_pow_top) = { //&el_lbl_pow_h0,
  &hlist_pow_h0, &hlist_pow_h1, &hlist_pow_h5, &hlist_pow_btn};
M2_VLIST(el_top_pow, "h15", list_pow_top);

/* 状态屏幕 */
extern char *p_info_blow;
extern char *p_info_blow_2;
extern char *p_info_feed_ex;
extern char *p_info_pellet;
extern M_Status m_status;

/* 7- 状态 */
M2_LABEL(el_lbl_status_h0, "x90y49", "\xaa\xab");  // 状态

M2_LABELPTR(el_lbl_status_h1, "x1y45", (char const**)&p_info_blow);  // 排烟温度

M2_LABELPTR(el_lbl_status_h2, "x1y31", (char const**)&p_info_blow_2);  // 排烟速度
M2_TOGGLE (el_toggle_status_h2, "x100y31", (unsigned char*)&m_status.M_blow);

M2_LABELPTR(el_lbl_status_h3, "x1y16", (char const**)&p_info_feed_ex);  // 送料，换风
M2_TOGGLE (el_toggle_status_h3, "x100y16", (unsigned char*)&m_status.M_feed);

M2_LABELPTR(el_lbl_status_h4, "x1y1", (char const**)&p_info_pellet);  // 消耗的料

M2_LIST(list_status) = {&el_lbl_status_h0,&el_lbl_status_h1, &el_lbl_status_h2,
                        &el_toggle_status_h2,&el_lbl_status_h3, 
                        &el_toggle_status_h3, &el_lbl_status_h4};
M2_XYLIST(top_status, "h14r", list_status);


/* 8- 确定对话框 */
M2_LABEL(el_lbl_confirm_exit_h1, NULL, "\xb8\x84\x85\xb9\xba?");  // 确定退出？
M2_BUTTON(el_btn_confirm_exit_cancel, NULL, "\x86\x87 ", fn_status_cancel);
M2_BUTTON(el_btn_confirm_exit_ok, NULL, "\x84\x85 ", fn_status_ok);
M2_LIST(list_confirm) = {&el_btn_confirm_exit_cancel, &el_btn_confirm_exit_ok};
M2_HLIST(hlist_confirm, "h15", list_confirm);

M2_LIST(list_confirm_v) = {&el_lbl_confirm_exit_h1, &hlist_confirm};
M2_VLIST(vlist_top_confirm, "h15", list_confirm_v);

/* 9- 日志信息窗口 */
static uint8_t el_log_first = 0;
static uint8_t el_log_cnt = 12;
static char s[40]={'\0'};

const char *el_getstr_log(uint8_t idx, uint8_t msg) 
{  
  el_log_cnt = getQueueSize();
  if (msg == M2_STRLIST_MSG_GET_STR) {
    Node *log_data = element(idx);
    if (log_data != NULL){
      if (log_data->idx<=14) {
        snprintf(s, 40, "%02d:%02d %s", log_data->hour, log_data->min, 
                 *(log_message+log_data->idx));       
      }
    }
    else {
      return (const char *)"-";    
    } 
  
    return (const char *)s; 
  }
  return "";
}

M2_STRLIST(el_log_strlist, "l4w106", &el_log_first, &el_log_cnt, el_getstr_log);
M2_SPACE(el_log_space, "W1h1");
M2_VSB(el_log_vsb, "l4W4r1", &el_log_first, &el_log_cnt);

M2_LIST(list_log) = { &el_log_strlist, &el_log_space, &el_log_vsb };
M2_HLIST(el_hlist_log, NULL, list_log);
M2_ALIGN(el_top_log, "-1|1W64H64", &el_hlist_log);

/* 10- 手动送料 */

extern char *p_feed_progress;

M2_LABELPTR(el_lbl_feed_progess, NULL, (char const**)&p_feed_progress);
M2_BUTTON(el_btn_feed_manual_ok, NULL, "\xc3\xc2", fn_feed_manual_ok);  // 启动
M2_BUTTON(el_btn_feed_manual_cancel, NULL, "\x88\x89", fn_feed_manual_cancel);  // 停止
M2_LIST(list_btns_feed_manual) = {&el_btn_feed_manual_ok, &el_btn_feed_manual_cancel};
M2_HLIST(hlist_btn_feed_manual, "h15", list_btns_feed_manual);

M2_LIST(list_feed_manual) ={&el_lbl_feed_progess, &hlist_btn_feed_manual};
M2_VLIST(el_top_feed_manual, "h15", list_feed_manual); 


/* 11- 启动窗口 */
// 窗口 0
extern const char logo[];

M2_XBMLABELP(el_xbm_logo, NULL, 100, 37, logo);
M2_LIST(el_list_logo) = {&el_xbm_logo};
M2_HLIST(el_hlist_logo, NULL, el_list_logo);
M2_ALIGN(el_top_logo, "-1|1W64H64", &el_hlist_logo);


/* 12- 提示加料 */
extern const char img_fill[];
M2_XBMLABELP(el_img_fill, "x1y2", 64, 60, img_fill);
M2_LABEL(el_lbl_pellet_h1, "x90,y31", "\xd7\xd8\x9d");  // 请加料
M2_BUTTON(el_btn_pellet_ok, "x90y16", "\xd4\xd5\xd6", fn_pellet_ok);
M2_LIST(list_pellet) = {&el_img_fill, &el_lbl_pellet_h1, &el_btn_pellet_ok};
M2_XYLIST(el_top_pellet, NULL, list_pellet);


/* 尾-菜单 */

m2_menu_entry menu_data[] =
{
  { "\x9e\x9f", NULL},  // 配方
  { ".   \xb6\xb7", &vlist_sel_rp},  // 选择
  { ".   \xb4\xb5",&el_top_rp},  // 编辑
  
  {"\xc1\xc2\x9c\x9d", &el_top_feed_manual}, // 手动送料
  
  { "\x8d\x8e", NULL },  // 预约
  { ".   \xa7\xa8\x93", &vlist_day_0 },  // 工作日
  { ".   \x96\x97\xa6", &vlist_day_6 },  // 星期六
  { ".   \x96\x97\x93", &vlist_day_7 },  // 星期日  
  
  { "\x8f\x90", &el_top_rtc},  // 时钟
  
  { "\x9c\xbf", &el_top_pow },  // 排烟风机功率
  
  { "\xaa\xab", &top_status},  // 状态
  { NULL, NULL },
};

static uint8_t el_2lme_first = 0;
static uint8_t el_2lme_cnt = 3;

M2_2LMENU(el_2lme_strlist, "l4e15F3W47", &el_2lme_first, &el_2lme_cnt, menu_data, 0x2b, 0x2d, '\0');
M2_SPACE(el_2lme_space, "W1h1");
M2_VSB(el_2lme_vsb, "l4W4r1", &el_2lme_first, &el_2lme_cnt);
M2_LIST(list_2lme_strlist) = { &el_2lme_strlist, &el_2lme_space, &el_2lme_vsb };
M2_HLIST(el_2lme_hlist, NULL, list_2lme_strlist);
M2_ALIGN(top_el_2l_menu, "-1|1W64H64", &el_2lme_hlist);

/* 窗口定义结束  */

/* 通讯函数 */
uint8_t u8g_com_hw_spi_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr)
{
  switch(msg)
  {
  case U8G_COM_MSG_STOP:
    break;
    
  case U8G_COM_MSG_INIT:
    break;
    
  case U8G_COM_MSG_ADDRESS:                     /* define cmd (arg_val = 0) or data mode (arg_val = 1) */
    if (arg_val == 0) {
      SH1106_DC_LOW;
    }
    else
    {
      SH1106_DC_HIGH;
    }
    break;
    
  case U8G_COM_MSG_CHIP_SELECT:
    if ( arg_val == 0 )
    {
      /* disable */
      /* this delay is required to avoid that the display is switched off too early */
      SH1106_CS_DISABLE;
    }
    else
    {
      /* enable */
      SH1106_CS_ENABLE;
    }
    break;
    
  case U8G_COM_MSG_RESET:
    // 拉低CS
    SH1106_CS_ENABLE;    
    //HAL_GPIO_WritePin(SH1106_RES_Port, SH1106_RES_Pin, (GPIO_PinState)arg_val);
    SH1106_RESET;
    SH1106_CS_DISABLE;
    osDelay(20);
    break;
    
  case U8G_COM_MSG_WRITE_BYTE:
    // 使能CS
    SH1106_CS_ENABLE;
    
    HAL_SPI_Transmit(&hspi1, &arg_val, 1, 1);
    
    // 禁用CS
    SH1106_CS_DISABLE;
    break;
    
  case U8G_COM_MSG_WRITE_SEQ:
  case U8G_COM_MSG_WRITE_SEQ_P:
    {
      register uint8_t *ptr = arg_ptr;
      SH1106_CS_ENABLE;
//      while( arg_val > 0 )
//      {
//        HAL_SPI_Transmit(&hspi1, ptr, 1, 1);  // 待优化
//        ptr++;
//        arg_val--;
//      }

      if(HAL_SPI_Transmit_DMA(&hspi1, ptr, arg_val) == HAL_OK) {
        osDelay(5);
      }
        
    }
    break;
  }
  return 1;
}
// 发送结束
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  SH1106_CS_DISABLE;
}


/* init */
void init_draw(void)
{
  
  u8g_InitComFn(&u8g, &u8g_dev_sh1106_128x64_hw_spi, u8g_com_hw_spi_fn);
  u8g_SetDefaultForegroundColor(&u8g);
  
}


void hmi(void)
{
  
  // 默认第1屏
  //m2_Init(&el_top_home, m2_es_arm_u8g, m2_eh_6bs, m2_gh_u8g_bfs);
  m2_Init(&el_top_logo, m2_es_arm_u8g, m2_eh_6bs, m2_gh_u8g_bfs);
  m2_SetU8g(&u8g, m2_u8g_box_icon);
  m2_SetFont(0, (const void *)u8g_font_fireflyR11);
  u8g_FirstPage(&u8g);
  do
  {
    m2_Draw();
  } while( u8g_NextPage(&u8g) );
  
  osDelay(5000);
  m2_SetRoot(&el_top_home);  
}


/* set root for 不同的screen */
void change_root(void)
{
  m2_rom_void_p root = m2_GetRoot();
  if (root == (void const*)&top_el_2l_menu)
    m2_SetRoot(&el_top_home);
  else if (root == (void const*)&el_top_home)
    m2_SetRoot(&top_el_2l_menu);
  else
    m2_SetRoot(&top_el_2l_menu);
  
}

/* 是否位于home窗口 */
uint8_t is_home_screen(void)
{
  m2_rom_void_p root = m2_GetRoot();
  if (root == (void const*)&el_top_home)
    return 1;
  else
    return 0;
}

/* 是否位于status/log窗口 */
uint8_t is_status_or_log_screen(void)
{
  m2_rom_void_p root = m2_GetRoot();
  if (root == (void const*)&top_status) {
    return 1;
  }
  
  if (root == (void const*)&el_top_log) {
    return 2;
  }
  
  return 0;
}

/* 是否位于手动送料窗口 */
uint8_t is_feed_manual_screen(void)
{
  m2_rom_void_p root = m2_GetRoot();
  if (root == (void const*)&el_top_feed_manual)
    return 1;
  else
    return 0;
}


/*RTC 按钮*/

// RTC时钟 OK按钮
void fn_rtc_btn_ok(m2_el_fnarg_p fnarg)
{
  bool result = false;
  result = write_rtc(&rtc_time);
  
  if (result) {
    strcpy(info_message, "\xae\xaf\xb0\xb1");  // 保存成功
    m2_SetRoot(&el_top_message);
  }
  else
    strcpy(info_message, "\xae\xaf\xb2\xb3");  // 保存失败
  
}



/* 工作日 ok cancel*/
// OK
void fn_schedule_btn_ok(m2_el_fnarg_p fnarg)
{
  m2_rom_void_p root = m2_GetRoot();
  bool result = false;
  
  if (root == (void *)&vlist_day_0) {
    result = save_schedule(&plan[0],0);
    printf("plan 0 is save.\r\n");
  }
  if (root == (void *)&vlist_day_6) {
    result = save_schedule(&plan[1],1);
    printf("plan 1 is save.\r\n");
  }
  if (root == (void *)&vlist_day_7) {
    result = save_schedule(&plan[2],2);
    printf("plan 2 is save.\r\n");
  }
  
  if (result) {
    strcpy(info_message, "\xae\xaf\xb0\xb1");  // 保存成功
    m2_SetRoot(&el_top_message);
  }
  else
    strcpy(info_message, "\xae\xaf\xb2\xb3");  // 保存失败
  
}

// Cancel
void fn_schedule_btn_cancel(m2_el_fnarg_p fnarg)
{
  m2_rom_void_p root = m2_GetRoot();
  
  if (root == (void *)&vlist_day_0)
    read_schedule(&plan[0], 0);
  
  if (root == (void *)&vlist_day_6)
    save_schedule(&plan[1],1);
  
  if (root == (void *)&vlist_day_7)
    save_schedule(&plan[2],2);
  
  fn_back_menu();
  
}

/* 配方 ok cancel */
// 选择
void fn_sel_rp_ok(m2_el_fnarg_p fnarg)
{
  // 保存配方索引
  if (save_to_rtc(RTC_ADDR_RECIPE_INDEX, &recipe_index, 1)) {    
    strcpy(info_message, "\xae\xaf\xb0\xb1");  // 保存成功
    m2_SetRoot(&el_top_message);
  }
  else {
    strcpy(info_message, "\xae\xaf\xb2\xb3");  // 保存失败
  }
}

void fn_sel_rp_edit(m2_el_fnarg_p fnarg)
{
  m2_SetRoot(&el_top_rp);
}

// 编辑
// OK
void fn_recipe_ok(m2_el_fnarg_p fnarg)
{
  bool result = false;
  result = save_recipe(recipe);
  
  if (result) {
    strcpy(info_message, "\xae\xaf\xb0\xb1");
    m2_SetRoot(&el_top_message);
  }
  else
    strcpy(info_message, "\xae\xaf\xb2\xb3");   
  //
}

// Cancel 从存储器读回数据
void fn_recipe_cancel(m2_el_fnarg_p fnarg)
{
  read_recipe(recipe);
  
  // 返回菜单
  fn_back_menu();
}

/* 排烟功率 ok cancel */
// OK
void fn_pow_ok(m2_el_fnarg_p fnarg)
{
  bool result = false;
  result = save_to_rtc(RTC_ADDR_POW, power_of_blow, 3);
  
  if (result) {
    strcpy(info_message, "\xae\xaf\xb0\xb1");  // 保存成功
    m2_SetRoot(&el_top_message);
  }
  else
    strcpy(info_message, "\xae\xaf\xb2\xb3");   // 保存失败
  //
}

// Cancel 从存储器读回数据
void fn_pow_cancel(m2_el_fnarg_p fnarg)
{
  read_from_rtc(RTC_ADDR_POW, power_of_blow, 3);
  
  // 返回菜单
  fn_back_menu();
}


// 返回菜单
void fn_back_menu()
{
  m2_SetRoot(&top_el_2l_menu);
  
}

/* 确定退出Diaglog */
// OK 
extern __IO bool auto_or_manual;  // 自动 or 手动
extern __IO bool m_stoped;

void fn_status_ok()
{
  auto_or_manual = false;
  m_stoped = true;
  // 保存是否预约状态
  save_schedule_state(auto_or_manual);
  
  m2_SetRoot(&el_top_home);
}
// Cancel
void fn_status_cancel()
{
  m2_SetRoot(&el_top_home);
}

void schedule_exit_confirm()
{
  m2_SetRoot(&vlist_top_confirm);
  
}


/* 获得窗口编号 */
uint8_t get_screen_no(void)
{
  uint8_t screen_no = 0;
  m2_rom_void_p root = m2_GetRoot();
  
  // 主页
  if (root == (void const*)&el_top_home) {
    screen_no = SCREEN_HOME;
  }
  // 菜单
  if (root == (void const*)&top_el_2l_menu) {
    screen_no = SCREEN_MENU;
  }
  // RTC时钟
  if (root == (void const*)&el_top_rtc) {
    screen_no = SCREEN_RTC;
  }
  // 排烟功率
  if (root == (void const*)&el_top_pow) {
    screen_no = SCREEN_POWER;
  }
  // 送料配方
  if (root == (void const*)&el_top_rp) {
    screen_no = SCREEN_RECIPE;
  }   
  // 计划预约
  if (root == (void const*)&vlist_day_0) {
    screen_no = SCREEN_SCHEDULE;
  } 
  if (root == (void const*)&vlist_day_6) {
    screen_no = SCREEN_SCHEDULE;
  } 
  if (root == (void const*)&vlist_day_7) {
    screen_no = SCREEN_SCHEDULE;
  }   
  // 确认退出自动
  if (root == (void const*)&vlist_top_confirm) {
    screen_no = SCREEN_SCHEDULE_EXIT_CONFIRM;
  } 
  // 状态
  if (root == (void const*)&top_status) {
    screen_no = SCREEN_STATUS;
  } 
  // 配方选择
  if (root == (void const*)&vlist_sel_rp) {
    screen_no = SCREEN_RECIPE_SELECT;
  } 
  
  if (root == (void const*)&el_top_message) {
    screen_no = SCREEN_STATUS;
  } 
  
  if (root == (void const*)&el_top_feed_manual) {
    screen_no = SCREEN_FEED;
  } 
  
  if (root == (void const*)&el_top_log) {
    screen_no = SCREEN_LOG;
 
  } 
  // 加料报警    
  if (root == (void const*)&el_top_pellet) {
    screen_no = SCREEN_PELLET_ADD; 
  }
  
  return screen_no;
  
}

/* 设置针脚按键 */
void set_pin_key(uint8_t screen)
{
  // 清除按键设定
  m2_SetPin(M2_KEY_SELECT, NULL);
  m2_SetPin(M2_KEY_EXIT, NULL);  
  m2_SetPin(M2_KEY_PREV, NULL);
  m2_SetPin(M2_KEY_NEXT, NULL);
  m2_SetPin(M2_KEY_DATA_UP, NULL);
  m2_SetPin(M2_KEY_DATA_DOWN, NULL);  
  m2_SetPin(M2_KEY_HOME, NULL);
  
  switch(screen) {
    
  case SCREEN_HOME:
    m2_SetPin(M2_KEY_DATA_UP, Key_Up);
    m2_SetPin(M2_KEY_DATA_DOWN, Key_Down);    
    m2_SetPin(M2_KEY_HOME, Key_Menu);
    break;
    
  case SCREEN_MENU:
    m2_SetPin(M2_KEY_SELECT, Key_Mode);
    m2_SetPin(M2_KEY_DATA_UP, Key_Up);
    m2_SetPin(M2_KEY_DATA_DOWN, Key_Down);
    
    m2_SetPin(M2_KEY_HOME, Key_Menu);
    break;
    
  case SCREEN_POWER:
  case SCREEN_RECIPE: 
  case SCREEN_SCHEDULE:  
    m2_SetPin(M2_KEY_PREV, Key_Left);
    m2_SetPin(M2_KEY_NEXT, Key_Right);
    m2_SetPin(M2_KEY_DATA_UP, Key_Up);
    m2_SetPin(M2_KEY_DATA_DOWN, Key_Down);
    
    m2_SetPin(M2_KEY_SELECT, Key_Mode);
    m2_SetPin(M2_KEY_HOME, Key_Menu);
    break;
    
  case SCREEN_SCHEDULE_EXIT_CONFIRM:
    m2_SetPin(M2_KEY_SELECT, Key_Mode);
    m2_SetPin(M2_KEY_PREV, Key_Left);
    m2_SetPin(M2_KEY_NEXT, Key_Right);
    break;
      
  case SCREEN_STATUS:
    break;
  
  case SCREEN_RECIPE_SELECT:
  case SCREEN_FEED:  
    m2_SetPin(M2_KEY_SELECT, Key_Mode);    
    m2_SetPin(M2_KEY_PREV, Key_Left);
    m2_SetPin(M2_KEY_NEXT, Key_Right);
    break;
    
  case SCREEN_MESSAGE:
    m2_SetPin(M2_KEY_SELECT, Key_Mode);
    break;
    
  case SCREEN_LOG:
    m2_SetPin(M2_KEY_DATA_UP, Key_Up);
    m2_SetPin(M2_KEY_DATA_DOWN, Key_Down);
    m2_SetPin(M2_KEY_PREV, Key_Left);
    m2_SetPin(M2_KEY_NEXT, Key_Right);
    break;  
    
  case SCREEN_PELLET_ADD:  // 20
    m2_SetPin(M2_KEY_SELECT, Key_Mode); 
    m2_SetPin(M2_KEY_HOME, Key_Menu);
    
  default:
    // Setup keys
    m2_SetPin(M2_KEY_SELECT, Key_Mode);
    m2_SetPin(M2_KEY_EXIT, Key_Power);
    
    m2_SetPin(M2_KEY_PREV, Key_Left);
    m2_SetPin(M2_KEY_NEXT, Key_Right);
    m2_SetPin(M2_KEY_DATA_UP, Key_Up);
    m2_SetPin(M2_KEY_DATA_DOWN, Key_Down);
    
    m2_SetPin(M2_KEY_HOME, Key_Menu);
    
  }
  
}

/* 手动送料按钮 */

extern bool feed_by_manual;
extern M_Status m_status;
extern uint32_t t_feed_manual;
extern osTimerId tmr_feed_manualHandle;

void fn_feed_manual_ok()
{
  if(m_status.stage == -1) {
    feed_by_manual = true;
    t_feed_manual = 0;
    
    osTimerStart(tmr_feed_manualHandle, 1000);
  }
}

void fn_feed_manual_cancel()
{
  
  feed_by_manual = false;
  osTimerStop(tmr_feed_manualHandle);
  fn_back_menu();
}

extern bool pellet_alarm_once;
void fn_pellet_ok() 
{
  pellet_alarm_once = false;
  // back
  m2_SetRoot(&el_top_home); 
  
}

/* 修改滚动条显示起始位置 */
void fn_log_up_down(uint8_t dir) {
  //uint8_t queue_siez = getQueueSize();
  if (el_log_cnt>=4) {
    if (dir) {
      
      if (el_log_first<el_log_cnt-4)  // 9 向上滚动
        el_log_first++;
    }
    else {
      if(el_log_first>=1)  // 4 向下滚动
        el_log_first --;
    }
  }
  //printf("el_log_first:%d\r\n",el_log_first);
}