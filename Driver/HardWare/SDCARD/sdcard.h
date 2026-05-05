/*!
    \file  sdcard.h
    \brief SD 卡驱动头文件

    \version 2016-08-15, V1.0.0, firmware for GD32F4xx
    \version 2018-12-12, V2.0.0, firmware for GD32F4xx
*/

/*
    Copyright (c) 2018, GigaDevice Semiconductor Inc.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#ifndef SDCARD_H
#define SDCARD_H

#include "gd32f4xx.h"

/* SD 卡总线命令索引 */
#define SD_CMD_GO_IDLE_STATE                  ((uint8_t)0)   /* CMD0，进入空闲态 */
#define SD_CMD_ALL_SEND_CID                   ((uint8_t)2)   /* CMD2，发送 CID */
#define SD_CMD_SEND_RELATIVE_ADDR             ((uint8_t)3)   /* CMD3，发送 RCA */
#define SD_CMD_SET_DSR                        ((uint8_t)4)   /* CMD4，设置 DSR */
#define SD_CMD_SWITCH_FUNC                    ((uint8_t)6)   /* CMD6，切换功能 */
#define SD_CMD_SELECT_DESELECT_CARD           ((uint8_t)7)   /* CMD7，选择/取消选择 */
#define SD_CMD_SEND_IF_COND                   ((uint8_t)8)   /* CMD8，发送接口条件 */
#define SD_CMD_SEND_CSD                       ((uint8_t)9)   /* CMD9，发送 CSD */
#define SD_CMD_SEND_CID                       ((uint8_t)10)  /* CMD10，发送 CID */
#define SD_CMD_STOP_TRANSMISSION              ((uint8_t)12)  /* CMD12，停止传输 */
#define SD_CMD_SEND_STATUS                    ((uint8_t)13)  /* CMD13，发送状态 */
#define SD_CMD_GO_INACTIVE_STATE              ((uint8_t)15)  /* CMD15，进入非活动态 */
#define SD_CMD_SET_BLOCKLEN                   ((uint8_t)16)  /* CMD16，设置块长度 */
#define SD_CMD_READ_SINGLE_BLOCK              ((uint8_t)17)  /* CMD17，读单块 */
#define SD_CMD_READ_MULTIPLE_BLOCK            ((uint8_t)18)  /* CMD18，读多块 */
#define SD_CMD_WRITE_BLOCK                    ((uint8_t)24)  /* CMD24，写单块 */
#define SD_CMD_WRITE_MULTIPLE_BLOCK           ((uint8_t)25)  /* CMD25，写多块 */
#define SD_CMD_PROG_CSD                       ((uint8_t)27)  /* CMD27，编程 CSD */
#define SD_CMD_SET_WRITE_PROT                 ((uint8_t)28)  /* CMD28，设置写保护 */
#define SD_CMD_CLR_WRITE_PROT                 ((uint8_t)29)  /* CMD29，清除写保护 */
#define SD_CMD_SEND_WRITE_PROT                ((uint8_t)30)  /* CMD30，发送写保护 */
#define SD_CMD_ERASE_WR_BLK_START             ((uint8_t)32)  /* CMD32，擦除起始 */
#define SD_CMD_ERASE_WR_BLK_END               ((uint8_t)33)  /* CMD33，擦除结束 */
#define SD_CMD_ERASE                          ((uint8_t)38)  /* CMD38，擦除 */
#define SD_CMD_LOCK_UNLOCK                    ((uint8_t)42)  /* CMD42，锁定/解锁 */
#define SD_CMD_APP_CMD                        ((uint8_t)55)  /* CMD55，应用命令 */
#define SD_CMD_GEN_CMD                        ((uint8_t)56)  /* CMD56，通用命令 */

/* SD 卡应用命令索引 */
#define SD_APPCMD_SET_BUS_WIDTH               ((uint8_t)6)   /* ACMD6，设置总线宽度 */
#define SD_APPCMD_SD_STATUS                   ((uint8_t)13)  /* ACMD13，SD 状态 */
#define SD_APPCMD_SEND_NUM_WR_BLOCKS          ((uint8_t)22)  /* ACMD22，发送写块数 */
#define SD_APPCMD_SET_WR_BLK_ERASE_COUNT      ((uint8_t)23)  /* ACMD23，设置擦除块数 */
#define SD_APPCMD_SD_SEND_OP_COND             ((uint8_t)41)  /* ACMD41，发送操作条件 */
#define SD_APPCMD_SET_CLR_CARD_DETECT         ((uint8_t)42)  /* ACMD42，设置/清除检测 */
#define SD_APPCMD_SEND_SCR                    ((uint8_t)51)  /* ACMD51，发送 SCR */

/* 命令类 */
#define SD_CCC_SWITCH                          BIT(10)       /* 类 10 */
#define SD_CCC_IO_MODE                         BIT(9)        /* 类 9 */
#define SD_CCC_APPLICATION_SPECIFIC            BIT(8)        /* 类 8 */
#define SD_CCC_LOCK_CARD                       BIT(7)        /* 类 7 */
#define SD_CCC_WRITE_PROTECTION                BIT(6)        /* 类 6 */
#define SD_CCC_ERASE                           BIT(5)        /* 类 5 */
#define SD_CCC_BLOCK_WRITE                     BIT(4)        /* 类 4 */
#define SD_CCC_BLOCK_READ                      BIT(2)        /* 类 2 */
#define SD_CCC_BASIC                           BIT(0)        /* 类 0 */

/* 传输模式 */
#define SD_DMA_MODE                           ((uint32_t)0x00000000) /* DMA 模式 */
#define SD_POLLING_MODE                       ((uint32_t)0x00000001) /* 轮询模式 */

/* 锁定状态 */
#define SD_LOCK                               ((uint8_t)0x05)        /* 锁定 */
#define SD_UNLOCK                             ((uint8_t)0x02)        /* 解锁 */

/* 支持的卡类型 */
typedef enum
{
    SDIO_STD_CAPACITY_SD_CARD_V1_1 = 0,   /* 标准容量 SD v1.1 */
    SDIO_STD_CAPACITY_SD_CARD_V2_0,       /* 标准容量 SD v2.0 */
    SDIO_HIGH_CAPACITY_SD_CARD,           /* 高容量 SD */
    SDIO_SECURE_DIGITAL_IO_CARD,          /* SDIO 卡 */
    SDIO_SECURE_DIGITAL_IO_COMBO_CARD,    /* SDIO 组合卡 */
    SDIO_MULTIMEDIA_CARD,                 /* MMC 卡 */
    SDIO_HIGH_CAPACITY_MULTIMEDIA_CARD,   /* 高容量 MMC */
    SDIO_HIGH_SPEED_MULTIMEDIA_CARD       /* 高速 MMC */
} sdio_card_type_enum;

/* CID 寄存器 */
typedef struct
{
    __IO uint8_t mid;                     /* 生产商 ID */
    __IO uint16_t oid;                    /* OEM/应用 ID */
    __IO uint32_t pnm0;                   /* 产品名 */
    __IO uint8_t pnm1;                    /* 产品名 */
    __IO uint8_t prv;                     /* 版本 */
    __IO uint32_t psn;                    /* 序列号 */
    __IO uint16_t mdt;                    /* 生产日期 */
    __IO uint8_t cid_crc;                 /* CRC7 */
} sd_cid_struct;

/* CSD 寄存器（v1.0/v2.0） */
typedef struct
{
    __IO uint8_t csd_struct;              /* CSD 结构 */
    __IO uint8_t taac;                    /* 访问时间 */
    __IO uint8_t nsac;                    /* 访问时间（时钟） */
    __IO uint8_t tran_speed;              /* 最大传输速率 */
    __IO uint16_t ccc;                    /* 命令类 */
    __IO uint8_t read_bl_len;             /* 最大读块长度 */
    __IO uint8_t read_bl_partial;         /* 读允许部分块 */
    __IO uint8_t write_blk_misalign;      /* 写块未对齐 */
    __IO uint8_t read_blk_misalign;       /* 读块未对齐 */
    __IO uint8_t dsp_imp;                 /* DSR 支持 */
    __IO uint32_t c_size;                 /* 设备大小 */
    __IO uint8_t vdd_r_curr_min;          /* 读电流最小 */
    __IO uint8_t vdd_r_curr_max;          /* 读电流最大 */
    __IO uint8_t vdd_w_curr_min;          /* 写电流最小 */
    __IO uint8_t vdd_w_curr_max;          /* 写电流最大 */
    __IO uint8_t c_size_mult;             /* 大小倍数 */
    __IO uint8_t erase_blk_en;            /* 单块擦除使能 */
    __IO uint8_t sector_size;             /* 扇区大小 */
    __IO uint8_t wp_grp_size;             /* 写保护组大小 */
    __IO uint8_t wp_grp_enable;           /* 写保护组使能 */
    __IO uint8_t r2w_factor;              /* 写速度因子 */
    __IO uint8_t write_bl_len;            /* 最大写块长度 */
    __IO uint8_t write_bl_partial;        /* 写允许部分块 */
    __IO uint8_t file_format_grp;         /* 文件格式组 */
    __IO uint8_t copy_flag;               /* 复制标志 */
    __IO uint8_t perm_write_protect;      /* 永久写保护 */
    __IO uint8_t tmp_write_protect;       /* 临时写保护 */
    __IO uint8_t file_format;             /* 文件格式 */
    __IO uint8_t csd_crc;                 /* CRC */
} sd_csd_struct;

/* 卡信息 */
typedef struct
{
    sd_cid_struct card_cid;               /* CID */
    sd_csd_struct card_csd;               /* CSD */
    sdio_card_type_enum card_type;        /* 卡类型 */
    uint32_t card_capacity;               /* 容量 */
    uint32_t card_blocksize;              /* 块大小 */
    uint16_t card_rca;                    /* RCA */
} sd_card_info_struct;

/* SD 错误码 */
typedef enum
{
    SD_OUT_OF_RANGE = 0,                  /* 参数超范围 */
    SD_ADDRESS_ERROR,                     /* 地址未对齐 */
    SD_BLOCK_LEN_ERROR,                   /* 块长度错误 */
    SD_ERASE_SEQ_ERROR,                   /* 擦除序列错误 */
    SD_ERASE_PARAM,                       /* 擦除参数错误 */
    SD_WP_VIOLATION,                      /* 写保护违规 */
    SD_LOCK_UNLOCK_FAILED,                /* 锁定/解锁失败 */
    SD_COM_CRC_ERROR,                     /* 命令 CRC 错误 */
    SD_ILLEGAL_COMMAND,                   /* 非法命令 */
    SD_CARD_ECC_FAILED,                   /* ECC 失败 */
    SD_CC_ERROR,                          /* 控制器错误 */
    SD_GENERAL_UNKNOWN_ERROR,             /* 未知错误 */
    SD_CSD_OVERWRITE,                     /* CSD 覆盖错误 */
    SD_WP_ERASE_SKIP,                     /* 写保护擦除跳过 */
    SD_CARD_ECC_DISABLED,                 /* 未使用 ECC */
    SD_ERASE_RESET,                       /* 擦除复位 */
    SD_AKE_SEQ_ERROR,                     /* 认证序列错误 */

    SD_CMD_CRC_ERROR,                     /* 响应 CRC 错误 */
    SD_DATA_CRC_ERROR,                    /* 数据 CRC 错误 */
    SD_CMD_RESP_TIMEOUT,                  /* 响应超时 */
    SD_DATA_TIMEOUT,                      /* 数据超时 */
    SD_TX_UNDERRUN_ERROR,                 /* 发送 FIFO 欠载 */
    SD_RX_OVERRUN_ERROR,                  /* 接收 FIFO 溢出 */
    SD_START_BIT_ERROR,                   /* 起始位错误 */

    SD_VOLTRANGE_INVALID,                 /* 电压范围无效 */
    SD_PARAMETER_INVALID,                 /* 参数无效 */
    SD_OPERATION_IMPROPER,                /* 操作不当 */
    SD_FUNCTION_UNSUPPORTED,              /* 功能不支持 */
    SD_ERROR,                             /* 通用错误 */
    SD_OK                                 /* 正常 */
} sd_error_enum;

typedef enum
{
  SD_NO_TRANSFER = 0,                     /* 无传输 */
  SD_TRANSFER_IN_PROGRESS                 /* 传输中 */
} sd_transfer_state_enum;

extern uint32_t sd_scr[2];                /* SCR */

/* 函数声明 */
/* 初始化 SD 卡并进入待机态 */
sd_error_enum sd_init(void);
/* 初始化并获取 CID/CSD */
sd_error_enum sd_card_init(void);
/* 配置时钟/电压并获取卡类型 */
sd_error_enum sd_power_on(void);
/* 关闭 SDIO 电源 */
sd_error_enum sd_power_off(void);

/* 配置总线模式 */
sd_error_enum sd_bus_mode_config(uint32_t busmode);
/* 配置传输模式 */
sd_error_enum sd_transfer_mode_config(uint32_t txmode);

/* 读单块 */
sd_error_enum sd_block_read(uint32_t *preadbuffer, uint32_t readaddr, uint16_t blocksize);
/* 读多块 */
sd_error_enum sd_multiblocks_read(uint32_t *preadbuffer, uint32_t readaddr, uint16_t blocksize, uint32_t blocksnumber);
/* 写单块 */
sd_error_enum sd_block_write(uint32_t *pwritebuffer, uint32_t writeaddr, uint16_t blocksize);
/* 写多块 */
sd_error_enum sd_multiblocks_write(uint32_t *pwritebuffer, uint32_t writeaddr, uint16_t blocksize, uint32_t blocksnumber);
/* 擦除连续区域 */
sd_error_enum sd_erase(uint32_t startaddr, uint32_t endaddr);
/* 处理中断 */
sd_error_enum sd_interrupts_process(void);

/* 选择/取消选择卡 */
sd_error_enum sd_card_select_deselect(uint16_t cardrca);
/* 获取卡状态（R1） */
sd_error_enum sd_cardstatus_get(uint32_t *pcardstatus);
/* 获取 SD 状态（512bit） */
sd_error_enum sd_sdstatus_get(uint32_t *psdstatus);
/* 停止传输 */
sd_error_enum sd_transfer_stop(void);
/* 锁定/解锁卡 */
sd_error_enum sd_lock_unlock(uint8_t lockstate);

/* 获取传输状态 */
sd_transfer_state_enum sd_transfer_state_get(void);
/* 获取容量（KB） */
uint32_t sd_card_capacity_get(void);
/* 获取卡详细信息 */
sd_error_enum sd_card_information_get(sd_card_info_struct *pcardinfo);

#endif /* SDCARD_H */
