#ifndef __DAC_H
#define __DAC_H
#include "HeaderFiles.h"
#include "gd32f4xx_dac.h"

void DAC_Init(void);
void DAC_DeInit(void);
void DAC_ChannelEnable(uint8_t channel);
void DAC_ChannelDisable(uint8_t channel);
void DAC_OutputBufferEnable(uint8_t channel);
void DAC_OutputBufferDisable(uint8_t channel);
void DAC_Set(uint8_t channel, uint16_t data);
uint16_t DAC_Get(uint8_t channel);
uint16_t DAC_VoltageToData(float voltage);

//PA4-DAC_OUT0, PA5-DAC_OUT1
void DAC_SetVoltage(uint8_t channel, float voltage);


#endif
