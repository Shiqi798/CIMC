#include "sleep_wake.h"

static uint32_t sleep_systick_ctrl = 0U;
static uint32_t sleep_systick_load = 0U;
static uint32_t sleep_systick_val = 0U;
static uint32_t sleep_timer6_int_enabled = 0U;



///////////////////////////////////////////deepsleep//////////////////////////////////////////////////
void PA0_wakeup_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_SYSCFG);

    /* PA0 is used as PMU WKUP for standby and EXTI0 for deepsleep wakeup. */
    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_0);

    syscfg_exti_line_config(EXTI_SOURCE_GPIOA, EXTI_SOURCE_PIN0);
    exti_init(EXTI_0, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    exti_interrupt_flag_clear(EXTI_0);
    nvic_irq_enable(EXTI0_IRQn, 1, 0);

    pmu_wakeup_pin_enable();
}

void RTC_SetWakeup(uint32_t sec)
{
    PA0_wakeup_init();
    exti_interrupt_flag_clear(EXTI_0);

    pmu_backup_write_enable();

    RTC_WPK = RTC_UNLOCK_KEY1;
    RTC_WPK = RTC_UNLOCK_KEY2;

    rtc_wakeup_disable();

    {
        uint32_t timeout = 0xFFFFFU;

        while((rtc_flag_get(RTC_FLAG_WTW) == RESET) && timeout--) {
        }

        if(timeout == 0U) {
            printf("[ERR] WTW timeout\r\n");
            return;
        }
    }

    rtc_wakeup_clock_set(WAKEUP_CKSPRE);

    if(sec > 0U) {
        rtc_wakeup_timer_set(sec - 1U);
    } else {
        rtc_wakeup_timer_set(0U);
    }

    exti_init(EXTI_22, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    rtc_flag_clear(RTC_FLAG_WT);
    exti_interrupt_flag_clear(EXTI_22);

    rtc_interrupt_enable(RTC_INT_WAKEUP);
    exti_interrupt_enable(EXTI_22);
    nvic_irq_enable(RTC_WKUP_IRQn, 0, 0);

    rtc_wakeup_enable();
}

void RTC_WKUP_IRQHandler(void)
{
    if(rtc_flag_get(RTC_FLAG_WT) != RESET) {
        pmu_backup_write_enable();
        RTC_WPK = RTC_UNLOCK_KEY1;
        RTC_WPK = RTC_UNLOCK_KEY2;
        rtc_flag_clear(RTC_FLAG_WT);
    }

    exti_interrupt_flag_clear(EXTI_22);
}

void EXTI0_IRQHandler(void)
{
    if(exti_interrupt_flag_get(EXTI_0) != RESET) {
        exti_interrupt_flag_clear(EXTI_0);
    }
}

///////////////////////////////lightsleep//////////////////////////////////////////

void sleep_tick_sources_suspend(void)
{
    if((TIMER_DMAINTEN(TIMERX_INT) & TIMER_INT_UP) != 0U) {
        sleep_timer6_int_enabled = 1U;
    } else {
        sleep_timer6_int_enabled = 0U;
    }

    timer_interrupt_disable(TIMERX_INT, TIMER_INT_UP);
    nvic_irq_disable(TIMERX_INT_IRQn);
    timer_interrupt_flag_clear(TIMERX_INT, TIMER_INT_FLAG_UP);
    NVIC_ClearPendingIRQ(TIMERX_INT_IRQn);

    sleep_systick_ctrl = SysTick->CTRL;
    sleep_systick_load = SysTick->LOAD;
    sleep_systick_val = SysTick->VAL;
    SysTick->CTRL = 0U;
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
}

void sleep_tick_sources_resume(void)
{
    SysTick->LOAD = sleep_systick_load;
    SysTick->VAL = sleep_systick_val;
    SysTick->CTRL = sleep_systick_ctrl;

    timer_interrupt_flag_clear(TIMERX_INT, TIMER_INT_FLAG_UP);
    if(sleep_timer6_int_enabled != 0U) {
        timer_interrupt_enable(TIMERX_INT, TIMER_INT_UP);
        nvic_irq_enable(TIMERX_INT_IRQn, 3, 2);
    }
}

void UART_LightSleep_Enter(void)
{
    sleep_tick_sources_suspend();

    USART1_ClearRxBuf();
    reset_usart1_rx_dma();

    if(usart_flag_get(USART1, USART_FLAG_IDLE) != RESET) {
        (void)USART_STAT0(USART1);
        (void)USART_DATA(USART1);
    }
    NVIC_ClearPendingIRQ(USART1_IRQn);

    pmu_to_sleepmode(WFI_CMD);

    sleep_tick_sources_resume();
}

///////////////////////////////////////待机///////////////////////////////////////
void Standby_Enter(uint32_t sec)
{
    PA0_wakeup_init();
    RTC_SetWakeup(sec);

    pmu_flag_clear(PMU_FLAG_RESET_WAKEUP);
    pmu_flag_clear(PMU_FLAG_RESET_STANDBY);

    pmu_to_standbymode();
}


//////////////////////////////////////////cmd//////////////////////////////////////////////////

