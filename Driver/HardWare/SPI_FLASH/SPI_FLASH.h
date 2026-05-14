#ifndef __SPI_FLASH_H
#define __SPI_FLASH_H

#include "HeaderFiles.h"
/***********************************Defines*************************************/
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
/************************************函数*****************************************/
/* initialize SPI1 GPIO and parameter */
void spi_flash_init(void);
/* erase the specified flash sector */
void spi_flash_sector_erase(uint32_t sector_addr);
/* erase the entire flash */
void spi_flash_bulk_erase(void);
/* write more than one byte to the flash */
void spi_flash_page_write(uint8_t *pbuffer, uint32_t write_addr, uint16_t num_byte_to_write);
/* write block of data to the flash */
void spi_flash_buffer_write(uint8_t *pbuffer, uint32_t write_addr, uint32_t num_byte_to_write);
/* read a block of data from the flash */
void spi_flash_buffer_read(uint8_t *pbuffer, uint32_t read_addr, uint16_t num_byte_to_read);
/* read flash identification */
uint32_t spi_flash_read_id(void);
/* initiate a read data byte (read) sequence from the flash */
void spi_flash_start_read_sequence(uint32_t read_addr);
/* read a byte from the SPI flash */
uint8_t spi_flash_read_byte(void);
/* send a byte through the SPI interface and return the byte received from the SPI bus */
uint8_t spi_flash_send_byte(uint8_t byte);
/* send a half word through the SPI interface and return the half word received from the SPI bus */
uint16_t spi_flash_send_halfword(uint16_t half_word);
/* enable the write access to the flash */
void spi_flash_write_enable(void);
/* poll the status of the write in progress (wip) flag in the flash's status register */
void spi_flash_wait_for_write_end(void);
void spi_flash_buffer_erase(uint32_t sector_addr, uint32_t num_byte_to_erase);



#endif
