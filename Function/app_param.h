#ifndef __APP_PARAM_H
#define __APP_PARAM_H

#include "HeaderFiles.h"

/*************** 参数 ***************/
extern float ratio_ch0;
extern float ratio_ch1;
extern float limit_ch0;
extern float limit_ch1;
extern float dac_volt;
extern uint8_t alarm_report_mode;
extern uint8_t usart1_baud_mode;
extern volatile uint32_t adc_sample_cycle;

/*************** boot参数区 ***************/
#define BOOT_PARAM_ADDR 0x08010000U
#define BOOT_PARAM_PAGE_SIZE 0x00001000U
#define BOOT_CONTROL_MAGIC 0x424F4F54U

#define BOOT_FLAG_NORMAL 0x00U
#define BOOT_FLAG_UPDATE 0xA5U

//和bootloader保持一致，别乱动顺序
typedef struct {
    uint32_t magic;
    uint16_t device_id;
    uint8_t  baud_code;
    uint8_t  boot_flag;
    uint32_t checksum;
} boot_param_t;

void boot_param_set_flag(uint8_t flag);

#endif
