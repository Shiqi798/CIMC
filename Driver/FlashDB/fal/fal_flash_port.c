/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-17     armink       the first version
 */

#include "fal/fal.h"
#include "fal/fal_def.h"
#include "SPI_Flash.h"

static int init(void) {
    spi_flash_init();
    return 1;
}

static int read(long offset, uint8_t *buf, size_t size) {
    /* 使用强制类型转换，消除隐患和编译警告 */
    spi_flash_buffer_read(buf, (uint32_t)offset, (uint16_t)size);
    return size;
}

static int write(long offset, const uint8_t *buf, size_t size)
{
    uint32_t addr = (uint32_t)offset;
    uint8_t *pBuffer = (uint8_t *)buf;
    uint32_t NumByteToWrite = (uint32_t)size;

    uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;

    /* 计算地址在 256 字节页内的偏移量 */
    Addr = addr % 256;
    /* 计算当前页还剩下多少空间可以写 */
    count = 256 - Addr;
    /* 计算总共需要写满多少个完整的页 */
    NumOfPage = NumByteToWrite / 256;
    /* 计算写完完整页后，还剩下多少零散的字节 */
    NumOfSingle = NumByteToWrite % 256;

    /* 如果写入地址恰好是页的起始地址 (256 的整数倍) */
    if (Addr == 0)
    {
        if (NumOfPage == 0)
        {
            /* 数据量不足一页，直接写 */
            spi_flash_page_write(pBuffer, addr, NumByteToWrite);
        }
        else
        {
            /* 数据量超过一页，先写整页 */
            while (NumOfPage--)
            {
                spi_flash_page_write(pBuffer, addr, 256);
                addr += 256;
                pBuffer += 256;
            }
            /* 再写剩下不足一页的部分 */
            if (NumOfSingle != 0)
            {
                spi_flash_page_write(pBuffer, addr, NumOfSingle);
            }
        }
    }
    /* 如果写入地址不是页的起始地址 (存在偏移) */
    else
    {
        if (NumOfPage == 0)
        {
            /* 如果总数据量都没有超过一页的大小 */
            if (NumOfSingle > count)
            {
                /* 但加上偏移量后，跨越了当前页边界，需要分两次写 */
                temp = NumOfSingle - count;
                spi_flash_page_write(pBuffer, addr, count); // 写满当前页
                addr += count;
                pBuffer += count;
                spi_flash_page_write(pBuffer, addr, temp); // 写到下一页的开头
            }
            else
            {
                /* 没有跨页，安全，直接写 */
                spi_flash_page_write(pBuffer, addr, NumByteToWrite);
            }
        }
        else
        {
            /* 数据量很大，肯定跨页了 */
            NumByteToWrite -= count;
            NumOfPage = NumByteToWrite / 256;
            NumOfSingle = NumByteToWrite % 256;

            /* 1. 先把当前页剩下的那一点空间写满 */
            spi_flash_page_write(pBuffer, addr, count);
            addr += count;
            pBuffer += count;

            /* 2. 把中间完整的页一页一页写完 */
            while (NumOfPage--)
            {
                spi_flash_page_write(pBuffer, addr, 256);
                addr += 256;
                pBuffer += 256;
            }

            /* 3. 把最后剩下的尾巴写完 */
            if (NumOfSingle != 0)
            {
                spi_flash_page_write(pBuffer, addr, NumOfSingle);
            }
        }
    }
    return size;
}

static int erase(long offset, size_t size)
{
    spi_flash_buffer_erase((uint32_t)offset, (uint32_t)size);
    return size;
}

/* ===================== 设备结构体实例化 ===================== */

const struct fal_flash_dev nor_flash0 = {
    .name = "nor_flash0", // 必须和 fal_cfg.h 里写的设备名一模一样
    .addr = 0x00000000,   // 外部 SPI Flash 物理起始地址都是 0
    .len = 512 * 1024,    // 你的 Flash 总容量：512 KB
    .blk_size = 4096,     // 擦除的最小单位：4KB (一扇区)
    .ops = {init, read, write, erase},
    .write_gran = 1 // SPI NOR Flash 支持任意单字节写入，粒度设为 1
};