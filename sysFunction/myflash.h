#ifndef __MYFLASH_H
#define __MYFLASH_H

#include "HeaderFiles.h"

#define FLASH_ADDR_DEVICE_ID    0x000000UL /* 设备ID（保留）*/
#define FLASH_LEN_DEVICE_ID     64U
/////////////////////////////数据扇区///////////////////////////////////////////////////////
#define FLASH_ADDR_FLASH_DATA   0x00001000 /* 4KB 扇区：参数数据追加存储 */
#define FLASH_ADDR_LOG_STATE    0x00002000 /* 4KB 扇区：日志状态追加存储 */

///////////////////////////////end///////////////////////////////////////////////////
#define SPI_FLASH_PAGE_SIZE     256U
#define SPI_FLASH_SECTOR_SIZE   4096U
#define SPI_FLASH_RECORD_SIZE   32U        /* 每条追加记录固定 32 字节 */
#define SPI_FLASH_MAX_RECORDS   (SPI_FLASH_SECTOR_SIZE / SPI_FLASH_RECORD_SIZE)  /* 128 条记录 */

#define SPI_FLASH_CS_LOW()      gpio_bit_reset(GPIOA, GPIO_PIN_15)
#define SPI_FLASH_CS_HIGH()     gpio_bit_set(GPIOA, GPIO_PIN_15)

#define FLASH_ID_READ_SUCESS    1U
#define FLASH_ID_READ_FAILED    0U
#define FLASH_ID                0xC84013U

/* magic: 区分记录类型 */
#define FLASH_DATA_MAGIC        0xA5A50001 /* 参数数据 */
#define FLASH_EMPTY_MAGIC       0xFFFFFFFF /* 空白区域标记 */

//////////////////////////////////////////////////数据及结构体/////////////////////////////////////////////////////////
extern uint32_t power_count;    
extern float ratio_ch0; 
extern float limit_ch0; 
extern float dac_volt;
extern volatile uint32_t adc_sample_cycle;

/* 参数记录，固定32B，存在0x00001000 */
typedef struct {
    uint32_t magic;         // 0xA5A50001 - 标记有效性
    uint32_t power_count;   // 上电次数
    uint32_t sample_cycle;  // 采样周期(ms)
    float ratio_ch0;        // 比例系数
    float limit_ch0;        // 限值
    float dac_volt;         // DAC电压
    uint32_t reserved;      // 预留
} flash_data_t;             /* 总共32 字节 */

/************************************参数读写*****************************************/

/* 设备ID操作 */
uint8_t spi_flash_device_id_write(const char *device_id);
char *spi_flash_device_id_read(void);
//参数
uint8_t spi_flash_power_count_update(void);
uint32_t spi_flash_power_count_read(void);

uint8_t spi_flash_sample_cycle_update(uint8_t isprintf, uint32_t cycle);
uint32_t spi_flash_sample_cycle_read(void);

uint8_t spi_ratio_limit_read(float *r, float *l);
uint8_t spi_ratio_limit_write(float r, float l);

uint8_t spi_flash_dac_update(float dac_volt);
float spi_flash_dac_volt_read(void);

void spi_flash_erase(void); // 擦除整个Flash
/************************************参数操作************************************/

/**
 * @brief 追加写入一条参数记录到 Flash（0x00001000）
 * @param data: 指向 flash_data_t 结构体的指针
 * @retval 1 成功，0 失败（扇区满需要折叠）
 */
uint8_t flash_data_append_write(const flash_data_t *data);

/**
 * @brief 读取最新的参数记录（扫描找到最后一条有效记录）
 * @param data: 输出参数，指向 flash_data_t 结构体
 * @retval 1 找到有效记录，0 初始状态（全 0xFF）
 */
uint8_t flash_data_read_latest(flash_data_t *data);

/**
 * @brief 折叠（垃圾回收）：擦除扇区并写回最新记录
 * @retval 1 成功，0 失败
 */
uint8_t flash_data_fold(void);

#endif
