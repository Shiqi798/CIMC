#ifndef __DATA_PROCESS_H
#define __DATA_PROCESS_H

#include "HeaderFiles.h"

extern float adc_volt;    // ADC采集的实际电压值（保留2位小数）
extern float eng_volt;    // 工程值
extern float dac_volt;
extern char encrypt_buf[24]; // 加密用

//数据处理
float data_adc_raw_to_volt(uint16_t raw);
void data_calc_eng_volt(void);
uint8_t data_check_overlimit(void);
char* data_encrypt(void);

//时间转换
uint32_t get_unix_time(void);
char* unix_to_str(uint32_t timestamp);

#endif 
