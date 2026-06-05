#ifndef __MYFLASHDB_LOG_H
#define __MYFLASHDB_LOG_H
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


/****************************define**************************** */
#define MAX_OP_STR_LEN 64 // 操作日志的最大字符串长度

////////////////////////////////结构体////////////////////////

//采样日志
typedef struct {
    float voltage;
    float ratio;
    uint16_t sample_cycle;
} sample_log_t;

//超限日志
typedef struct {
    float voltage;
    float limit;
} over_log_t;

//隐藏日志
typedef struct {
    float voltage;
    char hide_data[18]; 
} hide_log_t;


////////////////////////////函数/////////////////////////////////////////////////////

//初始化所有 4 个日志数据库
//return 0:成功, -1:失败
int flash_log_init(void);

//写
void append_sample_log(float voltage, float ratio, uint16_t sample_cycle);
void append_over_log(float voltage, float limit);
void append_hide_log(float voltage, const char* hide_str);

void append_normal_log(const char *fmt, ...);

//打印count条日志
void print_latest_sample_logs(int count);
void print_latest_over_logs(int count);
void print_latest_hide_logs(int count);
void print_latest_normal_logs(int count);
//清空函数
void clear_all_sample_logs(void);
void clear_all_over_logs(void);
void clear_all_hide_logs(void);
void clear_all_normal_logs(void);
void clear_all_logs(void);

#endif

