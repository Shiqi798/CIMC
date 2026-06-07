#ifndef __BOOT_PARAM_H
#define __BOOT_PARAM_H

#include "HeaderFiles.h"

/* ---- 参数区地址（与 Bootloader 一致）---- */
#define BOOT_PARAM_ADDR          0x08010000U
#define BOOT_PARAM_PAGE_SIZE     0x00001000U   /* 4K — GD32F470 page */

#define BOOT_CONTROL_MAGIC       0x424F4F54U   /* "BOOT" */

#define BOOT_FLAG_NORMAL         0x00U
#define BOOT_FLAG_UPDATE         0xA5U

/* ---- 参数区结构体（与 Bootloader 完全一致）---- */
typedef struct {
    uint32_t magic;       /* +0:  magic          */
    uint16_t device_id;   /* +4:  device ID      */
    uint8_t  baud_code;   /* +6:  baud code      */
    uint8_t  boot_flag;   /* +7:  boot flag      */
    uint32_t checksum;    /* +8:  CRC16 (低16位) */
} boot_param_t;

/* ---- API ---- */
void boot_param_set_flag(uint8_t flag);

#endif
