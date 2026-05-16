#include "sleep_wake.h"

void RTC_SetWakeup(uint32_t sec)
{
    rtc_wakeup_disable();

    uint32_t timeout = 0xFFFFF;
    while((rtc_flag_get(RTC_FLAG_WTW) == RESET) && timeout--);

    if(timeout == 0)
    {
        printf("[ERR] WTW timeout\r\n");
        return;
    }

    rtc_flag_clear(RTC_FLAG_WT);

    rtc_wakeup_clock_set(WAKEUP_CKSPRE);
    rtc_wakeup_timer_set(sec);
    exti_init(EXTI_22, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    exti_interrupt_enable(EXTI_22);

    rtc_interrupt_enable(RTC_INT_WAKEUP);
    nvic_irq_enable(RTC_WKUP_IRQn, 0, 0);

    rtc_wakeup_enable();
}

void RTC_WKUP_IRQHandler(void)
{
    if(rtc_flag_get(RTC_FLAG_WT) != RESET)
    {
        rtc_flag_clear(RTC_FLAG_WT);


        /* 👉 这里可以放你的唤醒逻辑 */
        // printf("Wakeup!\r\n");
    }

    exti_interrupt_flag_clear(EXTI_22);
}
