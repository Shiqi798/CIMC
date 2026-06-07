#include "HeaderFiles.h"
#include "Systick.h"
/************************* 全局变量定义 *************************/

/* 系统毫秒计数器 */
volatile uint32_t msTicks = 0;
volatile uint32_t led1_turn_time = 500;    // 500ms
uint32_t adc_sample_start = 0;
/************************* 函数实现 *************************/

/**
 * @brief  SysTick中断服务函数
 * @param  无
 * @retval 无
 * @note   每1毫秒调用一次，更新系统毫秒计数器
 */

uint32_t Gettim6Time(void)
{
    return msTicks;
}

/**
 * @brief  非阻塞式延时检查
 * @param  startTime: 指向存储开始时间的指针
 * @param  timeout: 超时时间（毫秒）
 * @retval uint8_t: 超时状态
 *          0 - 未超时
 *          1 - 超时
 */
uint8_t tim6_timeoutcheck(uint32_t *startTime, uint32_t timeout)
{
    if (*startTime == 0)
    {
        *startTime = Gettim6Time();
    }
    uint32_t currentTime = Gettim6Time();
    if ((currentTime - *startTime) >= timeout)
    {
        /* 超时，重置开始时间 */
        *startTime = currentTime;
        return 1;
    }
    else
    {
        /* 未超时 */
        return 0;
    }
}

void tim6_functimer_init(void)
{
    uint32_t arr = 1000 - 1;
    uint16_t psc = 120 - 1;
    timer_parameter_struct timer_initpara; /* timer_initpara用于存放定时器的参数 */

    /* 使能外设定时器时钟 */
    rcu_periph_clock_enable(TIMERX_INT_CLK); /* 使能TIMERX的时钟 */

    /* 复位定时器 */
    timer_deinit(TIMERX_INT);                /* 复位TIMERX */
    timer_struct_para_init(&timer_initpara); /* 初始化timer_initpara为默认值 */

    /* 配置定时器参数 */
    timer_initpara.prescaler = psc;                     /* 设置预分频值 */
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;    /* 设置对齐模式为边沿对齐模式 */
    timer_initpara.counterdirection = TIMER_COUNTER_UP; /* 设置向上计数模式 */
    timer_initpara.period = arr;                        /* 设置自动重装载值 */
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;    /* 设置时钟分频因子 */
    timer_initpara.repetitioncounter = 0;               /* 设置重复计数器值 */
    timer_init(TIMERX_INT, &timer_initpara);            /* 根据参数初始化定时器 */

    /* 使能定时器及其中断 */
    timer_interrupt_flag_clear(TIMERX_INT, TIMER_INT_FLAG_UP); /* 清除定时器更新中断标志 */

    timer_interrupt_enable(TIMERX_INT, TIMER_INT_UP); /* 使能定时器的更新中断 */

    nvic_irq_enable(TIMERX_INT_IRQn, 3, 2); /* 配置NVIC设置优先级，抢占优先级1，响应优先级3 */

    timer_enable(TIMERX_INT); /* 使能定时器TIMERX */
}


void TIMERX_INT_IRQHandler(void)
{

    if (timer_interrupt_flag_get(TIMERX_INT, TIMER_INT_FLAG_UP) == SET) /* 判断定时器更新中断是否发生 */
    {
        msTicks++; /* 毫秒计数器加1 */

        if (msTicks % 5 == 0)
        {
            Key_Tick(); // 更新按键状态

        }

        if (oled_idle_time > 0)
        {
            if(oled_idle_time == 1)
            {
                function_idle_refresh_request();
            }
            oled_idle_time--;
        }


        timer_interrupt_flag_clear(TIMERX_INT, TIMER_INT_FLAG_UP); /* 清除定时器更新中断标志 */
    }
}
