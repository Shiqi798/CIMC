#ifndef __DATA_PROCESS_H
#define __DATA_PROCESS_H

#include "HeaderFiles.h"

extern float dac_volt;

//数据处理
float data_adc_raw_to_volt(uint16_t raw);
float data_get_ch0_value(void);
float data_get_ch1_value(void);
uint8_t data_is_over_limit(float value, float limit);
uint8_t data_update_overlimit(float ch0, float ch1);

/*
旧命令行采样接口，先不参与编译
extern float adc_volt;
extern float eng_volt;
extern char encrypt_buf[24];
void data_calc_eng_volt(void);
uint8_t data_check_overlimit(void);
char* data_encrypt(void);
*/

//时间转换
uint32_t get_unix_time(void);
char* unix_to_str(uint32_t timestamp);

#endif 
