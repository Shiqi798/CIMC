#ifndef __MYSYSTICK_
#define __MYSYSTICK_
#include "HeaderFiles.h"
extern volatile uint32_t msTicks;
extern volatile uint32_t led1_turn_time;
extern volatile uint32_t adc_sample_cycle;
extern uint32_t adc_sample_start;
#define TIMERX_INT                       TIMER6
#define TIMERX_INT_IRQn                  TIMER6_IRQn
#define TIMERX_INT_IRQHandler            TIMER6_IRQHandler
#define TIMERX_INT_CLK                   RCU_TIMER6   /* TIMER6 时钟使能 */

uint32_t Gettim6Time(void);

uint8_t tim6_timeoutcheck(uint32_t* startTime, uint32_t timeout);

void tim6_functimer_init(void);
#endif
