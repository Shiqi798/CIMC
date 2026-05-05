#ifndef __FILE_MGR_H
#define __FILE_MGR_H
#include "HeaderFiles.h"
/*********************log cmd*********************** */
#define SYSTEM_INIT "system init"
#define RTC_CONFIG "rtc config"
#define RTC_CONFIG_SUCCESS "rtc config success to"
#define RTC_CONFIG_FAIL "rtc config fail"
#define TEST_CMD "system hardware test"
#define TEST_SUCCESS "test ok"
#define TEST_TF_FAIL "test error: tf card not found"
#define TEST_FLASH_FAIL "test error: flash not found"
#define TEST_FAIL "test error(tf card & flash)"
#define RATIO "ratio config"
#define RATIO_SUCCESS "ratio config success to"
#define RATIO_FAIL "ratio config fail (out of range)"
#define LIMIT "limit config"
#define LIMIT_SUCCESS "limit config success to"
#define LIMIT_FAIL "limit config fail (out of range)"
#define SAMPLE_START_C "sample start - c"
#define SAMPLE_START_P "sample start - p"
#define SAMPLE_STOP_C "sample stop (command)"
#define SAMPLE_STOP_P "sample stop (key press)"
#define SAMPLE_CYCLE_CHANGE "cycle switch to"
#define HIDE_DATA "hide data"
#define UNHIDE_DATA "unhide data"

extern char g_sample_file[64];      // sample 文件名（含路径）
extern uint16_t g_sample_line;       // sample 已写行数

extern char g_over_file[64];         // overlimit 文件名
extern uint16_t g_over_line;

extern char g_hide_file[64];         // hide 文件名
extern uint16_t g_hide_line;

extern uint8_t is_fmount_successful; // SD卡挂载成功标志，0表示未成功，1表示成功
/*******************log cmd end********************* */



void file_get_time_text(char *buf, size_t len);
void file_log_set(void);
void file_mgr_init(void);
void file_write_sample(void);
void file_write_overlimit(void);
void file_write_hide(void);
void file_write_log(char *cmd);
void file_write_log_rtc_success(char past_time_text[32]);
uint8_t file_read_config(float *r, float *l);
uint8_t file_write_config(float r, float l);
void file_log_printf(const char *path, const char *fmt, ...);

void file_log0_load(void);

/* FatFs 风格的 fgets，从文件读取一行文本，遇到 '\n' 或 EOF 停止，返回 buf 或 NULL */
char *f_gets(char *buff, int len, FIL *fp);

#endif
