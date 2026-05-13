#ifndef __DATA_PROCESS_H
#define __DATA_PROCESS_H

#include "HeaderFiles.h"

extern float adc_volt;    // ADC采集的实际电压值（保留2位小数）
extern float eng_volt;    // 工程值=adc_volt*ratio（赛题4.1）
extern float dac_volt;
extern char encrypt_buf[24];// 加密缓冲区：8字节HEX=16位字符串（赛题4）


void data_calc_eng_volt(void);
uint8_t data_check_overlimit(void);
char* data_encrypt(void);
const uint16_t *rtc_to_unix_time(void);




#endif 
