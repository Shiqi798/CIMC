#ifndef __CMD_PARSE_H
#define __CMD_PARSE_H

#include "HeaderFiles.h"

/*------------------ 定义------------------*/

/*------------------ 变量------------------*/
extern float ratio_ch0;
extern float limit_ch0;


/*------------------ 函数------------------*/
void cmd_parse_init(void);
void cmd_parse(void);

void cmd_parse_test(void);
void cmd_parse_RTC_Config(void);
void cmd_parse_RTC_now(void);
void cmd_parse_conf(void);
void cmd_parse_ratio(void);
void cmd_parse_limit(void);
void cmd_parse_config_save(void);
void cmd_parse_config_read(void);
void cmd_parse_start(void);
void cmd_parse_stop(void);
void sample_result_show(void);
void cmd_parse_hide(void);
void cmd_parse_unhide(void);

#endif 
