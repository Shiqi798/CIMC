#ifndef __SLEEP_WAKE_H
#define __SLEEP_WAKE_H

/************************* 头文件 *************************/
#define RTC_MAGIC      0x7050
#include "HeaderFiles.h"

void PA0_wakeup_init(void);
void RTC_SetWakeup(uint32_t sec);
void UART_LightSleep_Enter(void);
void Standby_Enter(uint32_t sec);




#endif
