#ifndef __MYFLASH_H
#define __MYFLASH_H

#include "HeaderFiles.h"

#define FLASH_ADDR_DEVICE_ID    0x000000UL /* 起始地址 */
#define FLASH_LEN_DEVICE_ID     64U        /* 存储长度（足够存"2025-CIMC-xxxx"） */

#define FLASH_ADDR_POWER_COUNT  0x00002000 /* 存储上电次数的地址 */
#define FLASH_LEN_POWER_COUNT   4U         /* 上电次数占4字节 */

#define FLASH_ADDR_SAMPLE_CYCLE 0x00003000 /* 存储采样周期的地址 */
#define FLASH_LEN_SAMPLE_CYCLE  4U         /* 采样周期占4字节 */

#define FLASH_ADDR_SAMPLE      0x00004000   // sample 状态存储地址（占用一个4KB扇区）
#define FLASH_ADDR_OVERLIMIT   0x00005000   // overlimit
#define FLASH_ADDR_HIDEDATA    0x00006000   // hided data
#define FLASH_ADDR_RATIO       0x00007000   // ratio
#define FLASH_ADDR_LIMIT       0x00008000   // limit
#define FLASH_ADDR_LOG0        0x0009000    // log0文件暂存

#define SPI_FLASH_PAGE_SIZE     256U
#define SPI_FLASH_SECTOR_SIZE   4096U

#define SPI_FLASH_CS_LOW()      gpio_bit_reset(GPIOA, GPIO_PIN_15)
#define SPI_FLASH_CS_HIGH()     gpio_bit_set(GPIOA, GPIO_PIN_15)

#define FLASH_ID_READ_SUCESS    1U
#define FLASH_ID_READ_FAILED    0U
#define FLASH_ID                0xC84013U

/* 有效数据标志（可选，用于区分未写入状态） */
#define FLASH_DATA_MAGIC        0xA5U /* 有效数据标记 */
extern uint32_t power_count;    

/************************************调用函数*****************************************/

/* 设备ID操作 */
uint8_t spi_flash_device_id_write(const char *device_id); // 写入设备ID，成功返回1，失败返回0
char *spi_flash_device_id_read(void);                     // 读取设备ID，返回指向字符串的

uint8_t spi_flash_power_count_update(void); // 写入上电次数，成功返回1，失败返回0
uint32_t spi_flash_power_count_read(void);   // 读取上电次数，返回次数值

uint8_t spi_flash_sample_cycle_update(uint32_t cycle); // 更新采样周期，成功返回1，失败返回0
uint32_t spi_flash_sample_cycle_read(void); // 读取采样周期，返回周期值
void spi_flash_erase(void); // 擦除整个Flash
void log_states_save_all(void); // 保存所有状态到Flash
void log_states_save_sample(void);
void log_states_save_over(void);
void log_states_save_hide(void);
void log_states_load_all(void); // 从Flash加载所有状态


uint8_t spi_ratio_limit_read(float *r, float *l);
uint8_t spi_ratio_limit_write(float r, float l);
void spi_ratio_limit_erase(void);

#endif
