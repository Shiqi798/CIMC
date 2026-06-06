/************************* 头文件包含 *************************/

#include "HeaderFiles.h"

/************************* 宏定义 *************************/

/************************* 函数实现 *************************/

/**
 * @brief  初始化LED
 * @param  无
 * @retval 无
 * @note   初始化LED1(PB12)和LED2(PB14)为推挽输出，默认熄灭
 */
void LED_Init(void)
{
    /* 使能GPIOB时钟 */
    rcu_periph_clock_enable(RCU_GPIOB);

    /* 配置LED1和LED2为输出模式 */
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_12 | GPIO_PIN_14);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12 | GPIO_PIN_14);
    gpio_bit_reset(GPIOB, GPIO_PIN_12 | GPIO_PIN_14); /* 默认熄灭 */
}

/**
 * @brief  切换LED1状态
 * @param  无
 * @retval 无
 * @note   每次调用切换一次LED1亮灭
 */
void led1_turn(void)
{
    static uint8_t led1_status = 0;
    if (led1_status == 0)
    {
        gpio_bit_set(GPIOB, GPIO_PIN_12); /* 点亮LED1 */
    }
    else
    {
        gpio_bit_reset(GPIOB, GPIO_PIN_12); /* 熄灭LED1 */
    }
    led1_status = !led1_status;
}

/**
 * @brief  点亮LED1
 * @param  无
 * @retval 无
 */
void led1_on(void)
{
    gpio_bit_set(GPIOB, GPIO_PIN_12);
}

/**
 * @brief  熄灭LED1
 * @param  无
 * @retval 无
 */
void led1_off(void)
{
    gpio_bit_reset(GPIOB, GPIO_PIN_12);
}

/**
 * @brief  点亮LED2
 * @param  无
 * @retval 无
 */
void led2_on(void)
{
    gpio_bit_set(GPIOB, GPIO_PIN_14);
}

/**
 * @brief  熄灭LED2
 * @param  无
 * @retval 无
 */
void led2_off(void)
{
    gpio_bit_reset(GPIOB, GPIO_PIN_14);
}

/************************* 文件结束 *************************/
