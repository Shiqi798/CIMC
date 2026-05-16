#include "sleep_wake.h"

void RTC_SetWakeup(uint32_t sec)
{
    // 防御性编程：1. 解锁 PMU 的备份域访问权限
    pmu_backup_write_enable();
    
    // 【核心修复1】2. 解锁 RTC 硬件寄存器的写访问权限
    // 写入特定的解锁序列（Key1, Key2）关闭 RTC 寄存器写保护
    // 单片机复位后，硬件会自动锁死RTC，必须解锁，否则下面的配置和清标志位全部静默失败！
    RTC_WPK = RTC_UNLOCK_KEY1;
    RTC_WPK = RTC_UNLOCK_KEY2;

    rtc_wakeup_disable();

    uint32_t timeout = 0xFFFFF;
    while((rtc_flag_get(RTC_FLAG_WTW) == RESET) && timeout--);

    if(timeout == 0)
    {
        printf("[ERR] WTW timeout\r\n");
        return;
    }

    // 1. 设置时钟源为 1Hz
    rtc_wakeup_clock_set(WAKEUP_CKSPRE);
    
    // 2. 修正时间偏移：硬件会多计一次，所以需要减 1
    if (sec > 0) {
        rtc_wakeup_timer_set(sec - 1); 
    } else {
        rtc_wakeup_timer_set(0); 
    }

    // 3. 配置外部中断线 22 (RTC 唤醒专用)
    exti_init(EXTI_22, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    
    // 4. 【关键修复2】开启中断前，因为前面解锁了RTC，所以现在标志位才能被真正清除，制造 0->1 的跳变条件
    rtc_flag_clear(RTC_FLAG_WT);
    exti_interrupt_flag_clear(EXTI_22);

    // 5. 使能中断并启动唤醒定时器
    rtc_interrupt_enable(RTC_INT_WAKEUP);
    exti_interrupt_enable(EXTI_22);
    nvic_irq_enable(RTC_WKUP_IRQn, 0, 0);

    rtc_wakeup_enable();
}

void RTC_WKUP_IRQHandler(void)
{
    if(rtc_flag_get(RTC_FLAG_WT) != RESET)
    {
        // 【核心修复3】中断里清除RTC标志位，同样需要确保RTC和PMU是解锁状态
        pmu_backup_write_enable();
        RTC_WPK = RTC_UNLOCK_KEY1;
        RTC_WPK = RTC_UNLOCK_KEY2;
        
        rtc_flag_clear(RTC_FLAG_WT);
        
        /* 👉 这里可以放你的唤醒逻辑 */
        // printf("Wakeup!\r\n");
    }

    exti_interrupt_flag_clear(EXTI_22);
}
