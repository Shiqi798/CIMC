#include "ADC.h"


// 初始化ADC相关端口（外部调用）
void ADC_port_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);   // 使能GPIOC时钟
    rcu_periph_clock_enable(RCU_ADC0);    // 使能ADC0时钟

    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_2);   // 设置PC0/PC2为模拟输入

    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);   // 配置ADC时钟

    ADC_Init();  // 初始化ADC
    DMA_ADC_Init();
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL); // 启动DMA循环采样
}

/************************************************************
 * 函数名称:    ADC_Init
 * 功能描述:    初始化ADC并使能DMA
 * 参    数:    无
 * 返 回 值:    无
************************************************************/

void ADC_Init(void)
{
    adc_deinit();    // 复位ADC

    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);      // 使能连续转换模式
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);               // 设置数据右对齐
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 2);            // 设置通道数为2
    
    adc_routine_channel_config(ADC0, 0, ADC_CHANNEL_10, ADC_SAMPLETIME_56);   // 配置PC0/ADC_IN10
    adc_routine_channel_config(ADC0, 1, ADC_CHANNEL_12, ADC_SAMPLETIME_56);   // 配置PC2/ADC_IN12
    adc_oversample_mode_config(ADC0, ADC_OVERSAMPLING_ALL_CONVERT, ADC_OVERSAMPLING_SHIFT_6B, ADC_OVERSAMPLING_RATIO_MUL64);
    adc_oversample_mode_enable(ADC0);   // 配置并使能过采样模式，64倍采样，移位6位
    adc_dma_mode_enable(ADC0);          // 使能ADC DMA请求
    adc_dma_request_after_last_enable(ADC0);
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, DISABLE);           // 使用软件触发

    adc_enable(ADC0);        // 使能ADC
    delay_1ms(1);            // 延时1ms
    adc_calibration_enable(ADC0);    // 使能ADC校准
}


/************************************************************
 * 函数名称:    ADC_get
 * 功能描述:    读取ADC值
 * 参    数:    无
 * 返 回 值:    返回ADC值（0-4095），用户可根据需要转换为电压值。
 ************************************************************/
uint16_t ADC_get(void)
{
    uint16_t ch0_value = 0U;
    uint16_t ch1_value = 0U;
    ADC_get_pair(&ch0_value, &ch1_value);
    return ch0_value;
}

void ADC_get_pair(uint16_t *ch0_value, uint16_t *ch1_value)
{
    if ((ch0_value == NULL) || (ch1_value == NULL)) {
        return;
    }
    *ch0_value = adc_value[0];
    *ch1_value = adc_value[1];
}

uint16_t ADC_get_ch1(void)
{
    uint16_t ch0_value = 0U;
    uint16_t ch1_value = 0U;
    ADC_get_pair(&ch0_value, &ch1_value);
    return ch1_value;
}

void adc_sample_cycle_update(uint32_t cycle)
{
    adc_sample_cycle =cycle;
    spi_flash_sample_cycle_update(1, cycle);
}

/**************************** 文件结束 *****************************/
