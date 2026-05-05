#ifndef __KEY_H
#define __KEY_H

#include "HeaderFiles.h"

#define KEY_PRESSED 			1
#define KEY_UNPRESSED 			0
#define KEY_HOLD				0X01
#define KEY_DOWN				0X02
#define KEY_UP					0X04
#define KEY_SINGLE				0X08
#define KEY_DOUBLE				0X10
#define KEY_LONG				0X20
#define KEY_REPEAT				0X40

#define KEY_TIME_DOUBLE				0
#define KEY_TIME_LONG				1000
#define KEY_TIME_REPEAT				100
#define KEY_COUNT					4

#define DEBOUNCE_CNT 				2 // 连续采样次数

#define sample_s				0 // 采样启停
#define sample_cycle1			1 // 采样周期5s
#define sample_cycle2			2 // 采样周期10s
#define sample_cycle3			3 // 采样周期15s

extern int16_t CalcCount;
extern int16_t CalcMode;
extern uint8_t LED_Mode ;
extern int16_t Count[3];
extern uint8_t g_key_stable_level[KEY_COUNT];


void Key_Init(void);
uint8_t Key_Check(uint8_t n,uint8_t Flag);
void Key_Tick(void);//按键核心控制


#endif
