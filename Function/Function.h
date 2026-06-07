#ifndef __FUNCTION_H
#define __FUNCTION_H

#include "HeaderFiles.h"

extern uint16_t oled_idle_time;
extern uint8_t sampling_flag;
extern uint8_t overlimit_flag;
extern uint8_t hide_flag;

//主控
void sysFunction_Init(void);
void sysFunction_loop(void);
void oled_idle_refresh(void);

//给msg_app用
void function_sample_state_set(uint8_t on);
uint8_t function_sample_state_get(void);
void function_idle_refresh_request(void);

#endif

/****************************End*****************************/
