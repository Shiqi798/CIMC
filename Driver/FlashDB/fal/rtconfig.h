/*
 * Minimal rtconfig.h for FAL library on bare-metal GD32F4xx
 * This file provides minimal RT-Thread compatibility macros needed by FAL
 */

#ifndef __RTCONFIG_H__
#define __RTCONFIG_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Use the project UART logger instead of libc printf inside FAL.
 * This avoids semihosting/stdio blocking when FAL reports errors during startup.
 */
extern void rs485_printf(const char *fmt, ...);

/* ========== Logging Macros ========== */
#define FAL_PRINTF          rs485_printf
#define log_i(fmt, ...)     FAL_PRINTF("[FAL INFO] " fmt "\r\n", ##__VA_ARGS__)
#define log_e(fmt, ...)     FAL_PRINTF("[FAL ERROR] " fmt "\r\n", ##__VA_ARGS__)
#define log_d(fmt, ...)     FAL_PRINTF("[FAL DEBUG] " fmt "\r\n", ##__VA_ARGS__)

/* ========== Memory/Utility Macros ========== */
#define FAL_SW_VERSION      "1.0.0"

/* ========== Alignment Macros ========== */
#ifndef ALIGN
#define ALIGN(size, align)  (((size) + (align) - 1) & ~((align) - 1))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(size, align)  ((size) & ~((align) - 1))
#endif

/* ========== RT-Thread Compatibility ========== */
#define RT_USING_FAL

#endif /* __RTCONFIG_H__ */
