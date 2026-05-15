#include "sleep_wake.h"

void rtc_set_wakeup(uint16_t sec)
{
    rtc_wakeup_disable();

    while(rtc_flag_get(RTC_FLAG_WTW) == RESET);

    rtc_flag_clear(RTC_FLAG_WT);

    rtc_wakeup_clock_set(WAKEUP_CKSPRE);

    rtc_wakeup_timer_set(sec);

    exti_interrupt_flag_clear(EXTI_22);

    exti_init(EXTI_22,
              EXTI_INTERRUPT,
              EXTI_TRIG_RISING);

    exti_interrupt_enable(EXTI_22);

    rtc_interrupt_enable(RTC_INT_WAKEUP);

    nvic_irq_enable(RTC_WKUP_IRQn, 2U, 0U);

    rtc_wakeup_enable();
}

//*
void RTC_WKUP_IRQHandler(void)
{
    if(rtc_flag_get(RTC_FLAG_WT) != RESET)
    {
        rtc_flag_clear(RTC_FLAG_WT);

        exti_interrupt_flag_clear(EXTI_22);
    }
}
