#ifndef __AD3344_H
#define __AD3344_H

#include "HeaderFiles.h"

//上电默认配置字
#define AD3344_DEFAULT_CONFIG                  ((uint16_t)0x058BU)

//配置寄存器位定义。O/S 位在单次启动和忙标志上共用同一位
#define AD3344_CONFIG_OS_SINGLE                ((uint16_t)0x8000U)
#define AD3344_CONFIG_OS_NOT_BUSY              AD3344_CONFIG_OS_SINGLE
#define AD3344_CONFIG_MUX_MASK                 ((uint16_t)0x7000U)
#define AD3344_CONFIG_PGA_MASK                 ((uint16_t)0x0E00U)
#define AD3344_CONFIG_MODE_MASK                ((uint16_t)0x0100U)
#define AD3344_CONFIG_DR_MASK                  ((uint16_t)0x00E0U)
#define AD3344_CONFIG_PULL_UP_EN               ((uint16_t)0x0008U)
#define AD3344_CONFIG_NOP_VALID                ((uint16_t)0x0002U)
#define AD3344_CONFIG_RESERVED                 ((uint16_t)0x0001U)

//cs
#define AD3344_CS_PE9                          GPIO_PIN_9

//输入通道选择
typedef enum {
    //差分
    AD3344_MUX_DIFF_0_1 = 0x0000U,
    AD3344_MUX_DIFF_0_3 = 0x1000U,
    AD3344_MUX_DIFF_1_3 = 0x2000U,
    AD3344_MUX_DIFF_2_3 = 0x3000U,
    //单端
    AD3344_MUX_SINGLE_0 = 0x4000U,
    AD3344_MUX_SINGLE_1 = 0x5000U,
    AD3344_MUX_SINGLE_2 = 0x6000U,
    AD3344_MUX_SINGLE_3 = 0x7000U
} ad3344_mux_t;

//PGA增益，4096为1倍
typedef enum {
    AD3344_PGA_6_144V = 0x0000U,
    AD3344_PGA_4_096V = 0x0200U,
    AD3344_PGA_2_048V = 0x0400U,
    AD3344_PGA_1_024V = 0x0600U,
    AD3344_PGA_0_512V = 0x0800U,
    AD3344_PGA_0_256V = 0x0A00U,
    AD3344_PGA_0_064V = 0x0C00U
} ad3344_pga_t;

//输出数据速度
typedef enum {
    AD3344_DR_6_25SPS  = 0x0000U,
    AD3344_DR_12_5SPS  = 0x0020U,
    AD3344_DR_25SPS    = 0x0040U,
    AD3344_DR_50SPS    = 0x0060U,
    AD3344_DR_100SPS   = 0x0080U,
    AD3344_DR_250SPS   = 0x00A0U,
    AD3344_DR_500SPS   = 0x00C0U,

    AD3344_DR_1000SPS  = 0x00E0U
} ad3344_data_rate_t;

//连续采样/单次
typedef enum {
    AD3344_MODE_CONTINUOUS = 0x0000U,
    AD3344_MODE_SINGLE     = 0x0100U
} ad3344_mode_t;

//配置结构体
typedef struct {
    ad3344_mux_t mux;
    ad3344_pga_t pga;
    ad3344_data_rate_t data_rate;
    ad3344_mode_t mode;
} ad3344_config_t;

//采样result
typedef struct {
    int16_t raw;
    float voltage;
} ad3344_sample_t;



/********************************函数******************** */
void AD3344_Init(void);
void AD3344_DeInit(void);

void AD3344_cmd(void);


//更新配置
void AD3344_Config(ad3344_mux_t mux,ad3344_pga_t pga,ad3344_data_rate_t data_rate,ad3344_mode_t mode);
void AD3344_SetMux(ad3344_mux_t mux);
void AD3344_SetPga(ad3344_pga_t pga);
void AD3344_SetDataRate(ad3344_data_rate_t data_rate);
void AD3344_SetMode(ad3344_mode_t mode);
//获取配置
ad3344_config_t AD3344_GetState(void);

uint16_t AD3344_GetConfig(void);

//参考电压设置
void AD3344_SetReferenceVoltage(float reference_voltage);
float AD3344_GetReferenceVoltage(void);

//读取原始值/电压值
int16_t AD3344_ReadRaw(void);
ad3344_sample_t AD3344_ReadSample(void);
uint8_t AD3344_ReadVoltage(float *voltage);
int16_t AD3344_ReadSingleRaw(ad3344_mux_t mux);
uint8_t AD3344_ReadSingleVoltage(ad3344_mux_t mux, float *voltage);
float AD3344_RawToVoltage(int16_t raw, ad3344_pga_t pga);

#endif
