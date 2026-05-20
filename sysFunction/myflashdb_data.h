#ifndef __MYFLASHDB_DATA_H
#define __MYFLASHDB_DATA_H
#include "HeaderFiles.h"
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
*/

// KVDB 键值宏定义 
#define DATA_CFG_KEY    "data_config"  // 总配置结构体
#define TEAM_NUM_KEY    "team_number" 
#define POWER_CNT_KEY   "power_count"  

extern uint8_t exflash_erase_flag;


typedef struct {
    uint32_t sample_cycle;  
    float    ratio_ch0;    
    float    limit_ch0;    
    float    dac_volt; 
} data_cfg_t;

int flashdata_init(void);

void set_team_number(const char *team_str);
void get_team_number(char *out_str, int max_len);

void set_power_count(void);
uint32_t get_power_count(void);

void set_data_config(data_cfg_t *cfg);
void get_data_config(data_cfg_t *out_cfg);

void flashdata_clear(void);
#endif 
