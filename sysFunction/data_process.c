#include "data_process.h"

float adc_volt = 0.00f;      // ADC采集的实际电压值（保留2位小数）
float dac_volt = 1.0f;
float eng_volt = 0.00f;      // 工程值=adc_volt*ratio（赛题4.1）
char encrypt_buf[24] = {0}; // 加密缓冲区：8字节HEX=16位字符串（赛题4），可追加'*'

void data_calc_eng_volt(void)
{
    // 将ADC值转换为电压（假设Vref = 3.3V，12位ADC）
    adc_volt = round(((ADC_get() / 4095.0) * 3.3) * 100) / 100; // 保留两位小数
    eng_volt = round(adc_volt * ratio_ch0 * 100) / 100;         // 计算工程值并保留两位小数
}

uint8_t data_check_overlimit(void)
{
    if (eng_volt > limit_ch0)
    {
        overlimit_flag = 1;
        return 1;
    }
    else
    {
        overlimit_flag = 0;
        return 0;
    }
}

/**
 * @brief 生成加密数据字符串。
 *
 * 将当前时间戳与电压值（整数与小数部分）编码为固定长度的十六进制字符串，
 * 并在超限条件下追加标记字符。
 *
 * @return 指向加密结果缓冲区的指针（以空字符结尾）。
 */
char* data_encrypt(void)
{
    /* 1. 直接获取 32 位时间戳，无需拼接 */
    uint32_t unix_time = get_unix_time();

    /* 2. 处理电压值的整数和小数部分 */
    uint16_t eng_volt_int = (uint16_t)eng_volt;
    float frac_part = eng_volt - (float)eng_volt_int;
    uint32_t frac_scaled = (uint32_t)lroundf(frac_part * 65536.0f);

    /* 处理四舍五入导致的进位 */
    if (frac_scaled >= 65536UL) {
        eng_volt_int++;
        frac_scaled = 0;
    }
    uint16_t eng_volt_frac = (uint16_t)frac_scaled;

    /* 3. 格式化输出为 16 字节的十六进制字符串 */
    snprintf(encrypt_buf, sizeof(encrypt_buf),
             "%08X%04X%04X",
             unix_time,
             eng_volt_int,
             eng_volt_frac);

    /* 4. 如果超限，在末尾追加 '*' 标记 */
    if (overlimit_flag == 1) {
        size_t current_len = strlen(encrypt_buf);
        if (current_len < sizeof(encrypt_buf) - 1) {
            encrypt_buf[current_len] = '*';
            encrypt_buf[current_len + 1] = '\0';
        }
    }
    
    return encrypt_buf;
}


uint32_t get_unix_time(void)
 {
    // 定义 GD32 的时间参数结构
    rtc_parameter_struct rtc_current;

    struct tm timeinfo = {0}; 
    rtc_current_time_get(&rtc_current);

    // tm_year 是从 1900 年开始算的。
    timeinfo.tm_year = rtc_initpara.year + 100; 
    
    // tm_mon 规定是从 0 开始的 (0=1月, 11=12月)
    timeinfo.tm_mon  = rtc_initpara.month - 1;  
    
    timeinfo.tm_mday = rtc_initpara.date;
    timeinfo.tm_hour = rtc_initpara.hour;
    timeinfo.tm_min  = rtc_initpara.minute;
    timeinfo.tm_sec  = rtc_initpara.second;

    /* 5. 一键转换！mktime 会自动把上面的信息转换成从 1970 年开始的秒数 */
    return (uint32_t)mktime(&timeinfo)+28800;
}
