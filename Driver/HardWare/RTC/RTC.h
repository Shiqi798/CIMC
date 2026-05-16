#ifndef __RTC_H
#define __RTC_H
#include "HeaderFiles.h"
extern rtc_parameter_struct   rtc_initpara;
void RTC_Init(void);
uint8_t time_data_check(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second);
void rtc_setup(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second);
void rtc_show_time(void);
void rtc_show_alarm(void);
uint8_t usart_input_threshold(uint32_t value);
uint8_t rtc_set_time(uint8_t h, uint8_t m, uint8_t s);

#endif
