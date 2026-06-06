#ifndef __APP_VARS_H
#define __APP_VARS_H

#include "Headerfiles.h"
#include "USART.h"

/************************* 全局参数 *************************/
extern float ratio_ch0;
extern float ratio_ch1;
extern float limit_ch0;
extern float limit_ch1;
extern float dac_volt;
extern uint8_t alarm_report_mode;
extern uint8_t usart1_baud_mode;
extern volatile uint32_t adc_sample_cycle;

#endif