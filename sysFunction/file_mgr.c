#include "file_mgr.h"
#include "ff.h"
#include "diskio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define FILE_MAX_RECORDS     10U
#define FILE_PATH_LEN        64U

#define FLASH_ADDR_LOG_ID    0x00004000UL
#define FLASH_LEN_LOG_ID     4U

char g_sample_file[64];      // sample 文件名（含路径）
uint16_t g_sample_line;       // sample 已写行数

char g_over_file[64];         // overlimit 文件名
uint16_t g_over_line;

char g_hide_file[64];         // hide 文件名
uint16_t g_hide_line;

FATFS myfs;
FIL fdst;
UINT br, bw;


uint8_t is_fmount_successful = 0; // SD卡挂载成功标志，0表示未成功，1表示成功

void file_log_set(void)
{
    char log_path[FILE_PATH_LEN];
    snprintf(log_path, sizeof(log_path), "0:/log/log%lu.txt", power_count);
    f_open(&fdst, log_path, FA_CREATE_ALWAYS | FA_WRITE); //创建并打开日志文件，准备写入。
    f_close(&fdst);//关闭文件
}

void file_mgr_init(void)
{
	uint16_t k = 5;
	DSTATUS stat = 0;

	do
	{
		stat = disk_initialize(0); 			//初始化SD卡（设备号0）,物理驱动器编号,每个物理驱动器（如硬盘、U 盘等）通常都被分配一个唯一的编号。
	}while((stat != 0) && (--k));			//如果初始化失败，重试最多k次。
    
    f_mount(0, &myfs); 
	if(RES_OK == stat)						 //返回挂载结果（FR_OK 表示成功）。
	{                
        is_fmount_successful = 1; // SD卡挂载成功
		f_mkdir("0:/sample");
        f_mkdir("0:/overLimit");
        f_mkdir("0:/hideData");
        f_mkdir("0:/log");
        char log_path[FILE_PATH_LEN];
        snprintf(log_path, sizeof(log_path), "0:/log/log%lu.txt", power_count);
	} 
}



/** 将BCD编码转换为二进制数。
 *  @param bcd BCD编码字节
 *  @return 转换后的二进制值
 */ 
static uint8_t file_bcd_to_bin(uint8_t bcd)
{
    return (uint8_t)((((bcd >> 4) & 0x0F) * 10U) + (bcd & 0x0F));
}


/** 获取当前RTC时间并格式化为时间戳字符串（YYYYMMDDhhmmss）。
 *  @param buf 输出缓冲区
 *  @param len 缓冲区长度
 */
static void file_get_timestamp(char *buf, size_t len)
{
    rtc_current_time_get(&rtc_initpara);

    const uint8_t year_bin = file_bcd_to_bin(rtc_initpara.year);
    const uint8_t month_bin = file_bcd_to_bin(rtc_initpara.month);
    const uint8_t date_bin = file_bcd_to_bin(rtc_initpara.date);
    const uint8_t hour_bin = file_bcd_to_bin(rtc_initpara.hour);
    const uint8_t minute_bin = file_bcd_to_bin(rtc_initpara.minute);
    const uint8_t second_bin = file_bcd_to_bin(rtc_initpara.second);

    const uint16_t full_year = 2000U + (uint16_t)year_bin;

    snprintf(buf, len, "%04u%02u%02u%02u%02u%02u",
             (unsigned)full_year,
             (unsigned)month_bin,
             (unsigned)date_bin,
             (unsigned)hour_bin,
             (unsigned)minute_bin,
             (unsigned)second_bin);
}


/** 获取当前RTC时间并格式化为可读字符串（YYYY-MM-DD hh:mm:ss）。
 *  @param buf 输出缓冲区
 *  @param len 缓冲区长度
 */
void file_get_time_text(char *buf, size_t len)
{
    rtc_current_time_get(&rtc_initpara);

    const uint8_t year_bin = file_bcd_to_bin(rtc_initpara.year);
    const uint8_t month_bin = file_bcd_to_bin(rtc_initpara.month);
    const uint8_t date_bin = file_bcd_to_bin(rtc_initpara.date);
    const uint8_t hour_bin = file_bcd_to_bin(rtc_initpara.hour);
    const uint8_t minute_bin = file_bcd_to_bin(rtc_initpara.minute);
    const uint8_t second_bin = file_bcd_to_bin(rtc_initpara.second);

    const uint16_t full_year = 2000U + (uint16_t)year_bin;

    snprintf(buf, len, "%04u-%02u-%02u %02u:%02u:%02u",
             (unsigned)full_year,
             (unsigned)month_bin,
             (unsigned)date_bin,
             (unsigned)hour_bin,
             (unsigned)minute_bin,
             (unsigned)second_bin);
}




/** 向指定文件写入一行文本，必要时创建新文件。
 *  @param path 文件路径
 *  @param line 写入内容
 *  @param create_new 是否创建新文件
 *  @return FatFs结果码
 */
static FRESULT file_write_line(const char *path, const char *line, uint8_t create_new)
{
    FRESULT fr;

    if (create_new)
    {
        fr = f_open(&fdst, path, FA_CREATE_ALWAYS | FA_WRITE);
    }
    else
    {
        fr = f_open(&fdst, path, FA_OPEN_ALWAYS | FA_WRITE);
    }

    if (fr != FR_OK)
    {
        return fr;
    }

    if (!create_new)
    {
        // Move write pointer to end for appending
        f_lseek(&fdst, f_size(&fdst));
    }

    UINT bw = 0;
    fr = f_write(&fdst, line, (UINT)strlen(line), &bw);
    f_close(&fdst);

    return fr;
}

void file_log_printf(const char *path, const char *fmt, ...)
{
    char buf[128];
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if(len <= 0) 
		return;

    if(len >= 2 && buf[len-2] == '\r' && buf[len-1] == '\n')
	{
        buf[len-2] = '\0';
    }

    file_write_line(path, buf, 0U); 
}

void file_write_sample(void)
{
    // 检查是否需要新建文件（计数器已满 或 文件名为空）
    uint8_t create_new = ((g_sample_line==0)?1:0); // 计数器为0时创建新文件
    if (create_new) {
        char ts[FILE_PATH_LEN];
        file_get_timestamp(ts, sizeof(ts));
        snprintf(g_sample_file, sizeof(g_sample_file), "0:/sample/sampleData%s.txt", ts);
        g_sample_line = 0;   // 新文件计数器归零
    }

    char time_str[32];
    file_get_time_text(time_str, sizeof(time_str));

    char line[160];
    snprintf(line, sizeof(line), "%s %.2fV\r\n", time_str, eng_volt);

    file_write_line(g_sample_file, line, create_new);   // 追加模式
    g_sample_line=(g_sample_line + 1)%FILE_MAX_RECORDS; // 更新行计数
	log_states_save_sample();
}


void file_write_overlimit(void)
{
    data_calc_eng_volt();
    data_check_overlimit();

    uint8_t create_new = g_over_line==0?1:0; // 计数器为0时创建新文件
    if (create_new) {
        char ts[FILE_PATH_LEN];
        file_get_timestamp(ts, sizeof(ts));
        snprintf(g_over_file, sizeof(g_over_file), "0:/overLimit/overLimit%s.txt", ts);

        g_over_line = 0;
    }

    char time_str[32];
    file_get_time_text(time_str, sizeof(time_str));

    char line[160];
    snprintf(line, sizeof(line), "%s %.2fV limit %.2fV\r\n",
             time_str, eng_volt, limit_ch0);

    file_write_line(g_over_file, line, create_new);
    g_over_line=(g_over_line + 1)%FILE_MAX_RECORDS;
	log_states_save_over();
}


void file_write_hide(void)
{
    data_calc_eng_volt();
    data_check_overlimit();

    uint8_t create_new = ((g_hide_line==0)?1:0); // 计数器为0时创建新文件
    if (create_new) {
        char ts[FILE_PATH_LEN];
        file_get_timestamp(ts, sizeof(ts));
        snprintf(g_hide_file, sizeof(g_hide_file), "0:/hideData/hideData%s.txt", ts);
        g_hide_line = 0;
    }

    char time_str[32];
    file_get_time_text(time_str, sizeof(time_str));

    const char *enc = data_encrypt();
    char line[160];
    snprintf(line, sizeof(line), "%s %.2fV \r\nhide: %s\r\n",
             time_str, eng_volt, enc ? enc : "NULL");

    file_write_line(g_hide_file, line, create_new);
    g_hide_line=(g_hide_line + 1)%FILE_MAX_RECORDS;
	log_states_save_hide();
}
/** 写入命令与响应日志。
 *  @param cmd 命令字符串
 *  @param res 响应字符串
 */
void file_write_log(char *cmd)
{
    if (cmd == NULL)
    {
        return;
    }

    char time_text[32];
    char line[160];
    char path[FILE_PATH_LEN];

    file_get_time_text(time_text, sizeof(time_text));
    snprintf(path, sizeof(path), "0:/log/log%lu.txt", power_count);    
    if(strcmp(cmd,TEST_CMD ) == 0)
    {
        snprintf(line, sizeof(line), "%s %s\r\n", time_text, cmd);
    }
    else if(strcmp(cmd,RATIO_SUCCESS ) == 0)
    {
        snprintf(line, sizeof(line), "%s %s %.2f\r\n", time_text, cmd, ratio_ch0);
    }
    else if(strcmp(cmd,LIMIT_SUCCESS ) == 0)
    {
        snprintf(line, sizeof(line), "%s %s %.2f\r\n", time_text, cmd, limit_ch0);
    }
    else if(strcmp(cmd,SAMPLE_START_C ) == 0)
    {
        snprintf(line, sizeof(line), "%s %s %lus (command)\r\n", time_text, "sample start - cycle", (unsigned long)(adc_sample_cycle/1000));
    }
    else if(strcmp(cmd,SAMPLE_START_P ) == 0)
    {
        snprintf(line, sizeof(line), "%s %s %lus (key press)\r\n", time_text, "sample start - cycle", (unsigned long)(adc_sample_cycle/1000));
    }
    else if(strcmp(cmd,SAMPLE_CYCLE_CHANGE ) == 0)
    {
        snprintf(line, sizeof(line), "%s %s %lus\r\n", time_text, cmd, (unsigned long)(adc_sample_cycle/1000));
    }
    else
    {
        snprintf(line, sizeof(line), "%s %s\r\n", time_text, cmd);
    }

    file_write_line(path, line, 0U);
    f_close(&fdst); // 再次关闭文件
}

void file_log0_load(void)
{
    power_count = 0; // 确保load的是log0
    char line[160];
    spi_flash_buffer_read((uint8_t *)line, FLASH_ADDR_LOG0, sizeof(line));
    f_open(&fdst, "0:/log/log0.txt", FA_CREATE_ALWAYS | FA_WRITE); // 创建并打开log0文件
    file_write_log(SYSTEM_INIT); // 记录系统初始化日志
    file_write_line("0:/log/log0.txt", line, 0U);
    file_write_log(TEST_CMD);
    file_write_log(TEST_TF_FAIL); // 记录TF卡测试失败日志
    f_close(&fdst);
}

void file_write_log_rtc_success(char past_time_text[32])
{
    char time_text[32];
    char line[160];
    char path[FILE_PATH_LEN];

    file_get_time_text(time_text, sizeof(time_text));
    snprintf(path, sizeof(path), "0:/log/log%lu.txt", power_count);
    snprintf(line, sizeof(line), "%s %s %s\r\n",past_time_text , RTC_CONFIG_SUCCESS,time_text);
    if(power_count==0)
    {
        spi_flash_sector_erase(FLASH_ADDR_LOG0);
        spi_flash_page_write((uint8_t *)line, FLASH_ADDR_LOG0, sizeof(line)); // 将日志暂存到Flash
    }
    file_write_line(path, line, 0U);
}

uint8_t file_write_config(float r, float l)
{
    const char *config_path = "0:/config.ini";
    FRESULT fr;
    char line_buf[64]; // 临时拼接单行内容

    // ========== 第一步：创建新文件，写入[Ratio]段头 ==========
    fr = file_write_line(config_path, "[Ratio]\r\n", 1); // create_new=1：覆盖创建
    if (fr != FR_OK)
    {
        return 0U;
    }

    // ========== 第二步：追加写入Ratio的Ch0（等号带空格，匹配读取格式） ==========
    snprintf(line_buf, sizeof(line_buf), "Ch0 = %.2f\r\n", r);
    fr = file_write_line(config_path, line_buf, 0); // create_new=0：追加
    if (fr != FR_OK)
    {
        return 0U;
    }

    // ========== 第三步：追加空行（匹配读取的配置文件格式） ==========
    fr = file_write_line(config_path, "\r\n", 0);
    if (fr != FR_OK)
    {
        return 0U;
    }

    // ========== 第四步：追加写入[Limit]段头 ==========
    fr = file_write_line(config_path, "[Limit]\r\n", 0);
    if (fr != FR_OK)
    {
        return 0U;
    }

    // ========== 第五步：追加写入Limit的Ch0（等号带空格） ==========
    snprintf(line_buf, sizeof(line_buf), "Ch0 = %.2f\r\n", l);
    fr = file_write_line(config_path, line_buf, 0);
    if (fr != FR_OK)
    {
        return 0U;
    }

    // 所有行写入成功
    return 1U;
}

/** 从配置文件读取比例与限值。
 *  @param r 输出比例值
 *  @param l 输出限值
 *  @return 成功读取返回1，否则返回0
 */
uint8_t file_read_config(float *r, float *l)
{
    // 入参合法性检查
    if (r == NULL || l == NULL)
    {
        return 0U;
    }

    FIL fp;
    FRESULT res = f_open(&fp, "0:/config.ini", FA_READ);
    if (res != FR_OK)
    {
        return 0U;
    }

    char line[64];
    uint8_t in_ratio = 0U;    // 是否进入[Ratio]段
    uint8_t in_limit = 0U;    // 是否进入[Limit]段
    uint8_t got_ratio = 0U;   // 是否读到Ratio的Ch0
    uint8_t got_limit = 0U;   // 是否读到Limit的Ch0

    // 逐行读取文件
    while (f_gets(line, sizeof(line), &fp) != NULL)
    {
        char *p = line;
        // 跳过行首空白字符（空格、制表符、换行）
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
        {
            p++;
            if (*p == '\0') break;
        }

        // 修剪行尾的空白/换行符
        size_t len = strlen(p);
        while (len > 0)
        {
            char last = p[len - 1];
            if (last == '\r' || last == '\n' || last == ' ' || last == '\t')
            {
                p[--len] = '\0';
            }
            else
            {
                break;
            }
        }

        if (p[0] == '\0' || p[0] == ';' || p[0] == '#')
        {
            continue;
        }

        // 匹配INI段（兼容段名前后空格）
        if (p[0] == '[')
        {
            in_ratio = (strstr(p, "Ratio") != NULL) ? 1U : 0U;
            in_limit = (strstr(p, "Limit") != NULL) ? 1U : 0U;
            continue;
        }

        // 读取对应段的Ch0值（兼容等号前后空格）
        char *ch0_ptr = strstr(p, "Ch0");
        if (ch0_ptr != NULL && (in_ratio || in_limit))
        {
            char *eq_ptr = strchr(ch0_ptr, '=');
            if (eq_ptr != NULL)
            {
                // 跳过等号后的空格，取数值
                char *val_ptr = eq_ptr + 1;
                while (*val_ptr == ' ' || *val_ptr == '\t') val_ptr++;

                if (in_ratio && !got_ratio)
                {
                    *r = (float)atof(val_ptr);
                    got_ratio = 1U;
                }
                else if (in_limit && !got_limit)
                {
                    *l = (float)atof(val_ptr);
                    got_limit = 1U;
                }
            }
        }

        // 提前退出：两个值都读到了就不用继续读
        if (got_ratio && got_limit)
        {
            break;
        }
    }

    // 关闭文件
    f_close(&fp);

    // 两个值都读到返回1，否则返回0
    return (got_ratio && got_limit) ? 1U : 0U;
}



/* 从 FatFs 文件对象读取一行文本。
 * 参数:
 *  - buff: 输出缓冲区
 *  - len: 缓冲区长度
 *  - fp: 指向已打开的 FIL 文件对象
 * 返回: 成功返回 buff，遇到 EOF 且未读到任何数据或出错返回 NULL
 */
char *f_gets(char *buff, int len, FIL *fp)
{
    if (buff == NULL || len <= 0 || fp == NULL)
    {
        return NULL;
    }

    int idx = 0;
    UINT br = 0;
    BYTE ch = 0;
    FRESULT fr;

    while (idx < len - 1)
    {
        fr = f_read(fp, &ch, 1U, &br);
        if (fr != FR_OK)
        {
            return NULL;
        }

        if (br == 0U)
        {
            break; /* EOF */
        }

        if (ch == '\r')
        {
            continue; /* skip CR */
        }

        if (ch == '\n')
        {
            buff[idx] = '\0';
            return buff;
        }

        buff[idx++] = (char)ch;
    }

    if (idx == 0)
    {
        return NULL; /* nothing read */
    }

    buff[idx] = '\0';
    return buff;
}


