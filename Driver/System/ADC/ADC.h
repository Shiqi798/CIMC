#ifndef __ADC_H
#define __ADC_H

/*********************** 头文件包含 **************************/

#include "HeaderFiles.h"

/*********************** 函数声明 *****************************/

/**
 * @brief 初始化ADC端口GPIO。
 */
void ADC_port_init(void);

/**
 * @brief 初始化ADC外设。
 */
void ADC_Init(void);

/**
 * @brief 读取ADC值。
 * @return 返回ADC值（0-4095），用户可根据需要转换为电压值。
 */
uint16_t ADC_get(void);

/**
 * @brief 读取两路ADC值。
 * @param ch0_value PC0/ADC_IN10结果输出地址。
 * @param ch1_value PC1/ADC_IN11结果输出地址。
 */
void ADC_get_pair(uint16_t *ch0_value, uint16_t *ch1_value);

/**
 * @brief 读取第二路ADC值。
 * @return PC1/ADC_IN11对应的原始值。
 */
uint16_t ADC_get_ch1(void);

#endif /* __ADC_H */

/**************************** 文件结束 ************************/
