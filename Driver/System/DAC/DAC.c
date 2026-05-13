#include "DAC.h"

// 初始化DAC时钟、GPIO和两个输出通道、
//PA4-DAC_OUT0, PA5-DAC_OUT1
void DAC_Init(void)
{
	rcu_periph_clock_enable(RCU_DAC);
	rcu_periph_clock_enable(RCU_GPIOA);
	gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4 | GPIO_PIN_5);

	dac_deinit(DAC0);
	dac_output_buffer_enable(DAC0, DAC_OUT0);
	dac_output_buffer_enable(DAC0, DAC_OUT1);
	dac_enable(DAC0, DAC_OUT0);
	dac_enable(DAC0, DAC_OUT1);

	dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, 0U);
	dac_data_set(DAC0, DAC_OUT1, DAC_ALIGN_12B_R, 0U);
}

// 复位DAC外设
void DAC_DeInit(void)
{
	dac_deinit(DAC0);
}

// 使能指定DAC输出通道
void DAC_ChannelEnable(uint8_t channel)
{
	dac_enable(DAC0, channel);
}

// 关闭指定DAC输出通道
void DAC_ChannelDisable(uint8_t channel)
{
	dac_disable(DAC0, channel);
}

// 使能指定DAC输出缓冲器
void DAC_OutputBufferEnable(uint8_t channel)
{
	dac_output_buffer_enable(DAC0, channel);
}

// 关闭指定DAC输出缓冲器
void DAC_OutputBufferDisable(uint8_t channel)
{
	dac_output_buffer_disable(DAC0, channel);
}

// 向指定DAC通道写入12位原始数据
void DAC_Set(uint8_t channel, uint16_t data)
{
	if (data > 4095U) {
		data = 4095U;
	}

	dac_data_set(DAC0, channel, DAC_ALIGN_12B_R, data);
}

// 读取指定DAC通道的当前输出值
uint16_t DAC_Get(uint8_t channel)
{
	return dac_output_value_get(DAC0, channel);
}

// 将输入电压换算为12位DAC原始数据。
uint16_t DAC_VoltageToData(float voltage)
{
	if (voltage <= 0.0f) {
		return 0U;
	}

	if (voltage >= 3.3f) {
		return 4095U;
	}

	return (uint16_t)lroundf((voltage / 3.3f) * 4095.0f);
}

// 按输入电压设置指定DAC通道输出
//PA4-DAC_OUT0, PA5-DAC_OUT1
void DAC_SetVoltage(uint8_t channel, float voltage)
{
	DAC_Set(channel, DAC_VoltageToData(voltage));
}


