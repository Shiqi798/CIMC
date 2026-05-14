#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <stdint.h>
#include <fal.h>

/*
[I/FAL] ==================== FAL partition table ====================
[I/FAL] | name       | flash_dev  |   offset   |    length  |
[I/FAL] -------------------------------------------------------------
[I/FAL] | fdb_kvdb1  | nor_flash0 | 0x00000000 | 0x00008000 |
[I/FAL] | op_log     | nor_flash0 | 0x00008000 | 0x00010000 |
[I/FAL] | sample_log | nor_flash0 | 0x00018000 | 0x00030000 |
[I/FAL] | over_log   | nor_flash0 | 0x00048000 | 0x00020000 |
[I/FAL] | hide_log   | nor_flash0 | 0x00068000 | 0x00018000 |
[I/FAL] =============================================================
[I/FAL] RT-Thread Flash Abstraction Layer (V0.5.0) initialize success.
*/


/* ===================== 硬件设备定义 ===================== */
/* 声明在 fal_flash_port.c 中定义的 Flash 设备对象 */
extern const struct fal_flash_dev nor_flash0;

/* 设备表：FAL 管理的所有物理 Flash 设备 */
#define FAL_FLASH_DEV_TABLE  \
{                            \
    &nor_flash0,             \
}

/* ===================== 分区表定义 ===================== */
/* 使用编译期分区表，避免运行时从 Flash 扫描分区表 */
#define FAL_PART_HAS_TABLE_CFG

/* * 针对 512KB (4Mbit) Flash 的空间分配方案：
 * 1. 所有的偏移 (offset) 和长度 (length) 都必须是 4096 (0x1000) 的整数倍。
 * 2. 分区名称需与 FlashDB 初始化时传入的名字完全一致。
 */
#define FAL_PART_TABLE                                                                \
    {                                                                                 \
        /* 1. KVDB分区：用于存储系统配置、DAC值、限值等 (32KB) */                     \
        {FAL_PART_MAGIC_WORD, "fdb_kvdb1", "nor_flash0", 0x00000000, 32 * 1024, 0},   \
                                                                                      \
        /* 2. 操作日志分区：存储 RTC 设置、启动停止等字符串 (64KB) */                 \
        {FAL_PART_MAGIC_WORD, "op_log", "nor_flash0", 0x00008000, 64 * 1024, 0},      \
                                                                                      \
        /* 3. 普通采样分区：存储周期性的电压、比例数据 (192KB) */                     \
        {FAL_PART_MAGIC_WORD, "sample_log", "nor_flash0", 0x00018000, 192 * 1024, 0}, \
                                                                                      \
        /* 4. 超限数据分区：专门存储超限事件报警数据 (128KB) */                       \
        {FAL_PART_MAGIC_WORD, "over_log", "nor_flash0", 0x00048000, 128 * 1024, 0},   \
                                                                                      \
        /* 5. 隐藏数据分区：提高项要求的 HEX 格式加密数据 (96KB) */                   \
        {FAL_PART_MAGIC_WORD, "hide_log", "nor_flash0", 0x00068000, 96 * 1024, 0},    \
    }

#endif /* _FAL_CFG_H_ */