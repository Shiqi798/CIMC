#ifndef __CMD_PARSE_H
#define __CMD_PARSE_H

#include "HeaderFiles.h"

/*------------------ 定义------------------*/

/*------------------ 变量------------------*/
extern float ratio_ch0;
extern float limit_ch0;
extern volatile uint8_t dac_test_flag;
extern volatile uint16_t dac_test_count;
extern uint8_t dac_test_flag6;
extern uint8_t dac_test_flag3;
extern uint8_t dac_test_flag0;
/*------------------ 函数------------------*/
void cmd_parse_init(void);
void cmd_parse(void);

void cmd_parse_deepsleep(void);
void cmd_parse_lightsleep(void);
void cmd_parse_standby(void);
void cmd_parse_test(void);
void cmd_parse_RTC_Config(void);
void cmd_parse_RTC_now(void);
void cmd_parse_conf(void);
void cmd_parse_ratio(void);
void cmd_parse_limit(void);
void cmd_parse_dac_test(void);
void dac_test_tick(void);
void cmd_parse_dac(void);
void cmd_parse_pt100(void);
void cmd_parse_config_save(uint8_t printf_flag);
void cmd_parse_config_read(void);
void cmd_parse_start(void);
void cmd_parse_stop(void);
void sample_result_show(void);
void cmd_parse_hide(void);
void cmd_parse_unhide(void);

void cmd_parse_sample_read(void);
void cmd_parse_over_read(void);
void cmd_parse_hide_read(void);
void cmd_parse_log_read(void);

#endif 
