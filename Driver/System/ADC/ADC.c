#include "ADC.h"


// 初始化ADC相关端口（外部调用）
void ADC_port_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);   // 使能GPIOC时钟
    rcu_periph_clock_enable(RCU_ADC0);    // 使能ADC0时钟

    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0);   // 设置PC0为模拟输入

    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);   // 配置ADC时钟

    ADC_Init();  // 初始化ADC

    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL); // 使能ADC软件触发
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

    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);     // 使能连续转换模式
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);               // 设置数据右对齐
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 1);            // 设置通道数为1

    adc_routine_channel_config(ADC0, 0, ADC_CHANNEL_10, ADC_SAMPLETIME_56);   // 配置常规通道

    adc_external_trigger_source_config(ADC0, ADC_ROUTINE_CHANNEL, ADC_EXTTRIG_INSERTED_T0_CH3);   // 设置外部触发源
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, ENABLE);                                 // 使能外部触发

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
    // 启动ADC转换
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);

    // 等待转换完成
    while(!adc_flag_get(ADC0, ADC_FLAG_EOC));

    // 读取ADC值
    return  ADC_RDATA(ADC0);
}

void adc_sample_cycle_update(uint32_t cycle)
{
    adc_sample_cycle =cycle;
    spi_flash_sample_cycle_update(cycle);
}

/**************************** 文件结束 *****************************/
