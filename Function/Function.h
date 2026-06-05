#ifndef __FUNCTION_H
#define __FUNCTION_H

/************************* 头文件 *************************/

#include "HeaderFiles.h"

/************************* 宏定义 *************************/

/************************* 变量声明 *************************/
extern uint16_t oled_idle_time;
extern uint8_t oled_idle_refresh_flag;
extern uint8_t sampling_flag; // 采样标志位，0表示未采样，1表示正在采样
extern uint8_t overlimit_flag; // 超限标志位，0表示正常，1表示超限
extern uint8_t hide_flag;     // 加密标志位，0表示非加密状态，1表示加密状态
/************************* 函数声明 *************************/
void oled_idle_refresh(void); // OLED空闲刷新
void update_sample_cycle(uint32_t cycle); // 更新采样周期并保存到Flash
void sysFunction_Init(void);      	// 系统初始化
void sysFunction_loop(void);         // 用户功能

#endif

/****************************End*****************************/
