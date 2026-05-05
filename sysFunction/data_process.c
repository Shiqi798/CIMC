#include "data_process.h"

float adc_volt = 0.00f;      // ADC采集的实际电压值（保留2位小数）
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
    const uint16_t *unix_time_ptr = rtc_to_unix_time();
    uint32_t unix_time = ((uint32_t)unix_time_ptr[0] << 16) | unix_time_ptr[1];

    uint16_t eng_volt_int = (uint16_t)eng_volt;
    float frac_part = eng_volt - (float)eng_volt_int;
    uint32_t frac_scaled = (uint32_t)lroundf(frac_part * 65536.0f);

    if (frac_scaled >= 65536UL) {
        eng_volt_int++;
        frac_scaled = 0;
    }
    uint16_t eng_volt_frac = (uint16_t)frac_scaled;

    snprintf(encrypt_buf, sizeof(encrypt_buf),
             "%08X%04X%04X",
             unix_time,
             eng_volt_int,
             eng_volt_frac);

    if (overlimit_flag == 1) {
        size_t current_len = strlen(encrypt_buf);
        if (current_len < sizeof(encrypt_buf) - 1) {
            encrypt_buf[current_len] = '*';
            encrypt_buf[current_len + 1] = '\0';
        }
    }
    return encrypt_buf;
}

/*!
    \brief      转换为unix时间戳（读取当前RTC时间），返回4字节16位字符串
    \param[in]  none
    \param[out] none
    \retval     指向2个uint16_t的静态数组（高16位/低16位）
*/
const uint16_t *rtc_to_unix_time(void)
{
    rtc_current_time_get(&rtc_initpara);

    static const uint16_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    static uint16_t unix_time_str[2];

    // BCD码转十进制函数
    #define BCD_TO_BIN(bcd) ((((bcd) >> 4) & 0x0F) * 10 + ((bcd) & 0x0F))

    // 转换所有BCD码为十进制值
    const uint8_t year_bin = BCD_TO_BIN(rtc_initpara.year);      // 0-99
    const uint8_t month_bin = BCD_TO_BIN(rtc_initpara.month);    // 1-12
    const uint8_t date_bin = BCD_TO_BIN(rtc_initpara.date);      // 1-31
    const uint8_t hour_bin = BCD_TO_BIN(rtc_initpara.hour);      // 0-23
    const uint8_t minute_bin = BCD_TO_BIN(rtc_initpara.minute);  // 0-59
    const uint8_t second_bin = BCD_TO_BIN(rtc_initpara.second);  // 0-59

    const uint16_t full_year = 2000U + (uint16_t)year_bin;
    const uint8_t is_leap = (uint8_t)(((full_year % 4U == 0U) && (full_year % 100U != 0U)) || (full_year % 400U == 0U));

    uint32_t total_days = 0U;

    // 计算从1970年到当前年份之前的天数
    for (uint16_t y = 1970U; y < full_year; y++)
    {
        total_days += 365U + (uint32_t)(((y % 4U == 0U) && (y % 100U != 0U)) || (y % 400U == 0U));
    }

    // 计算当年已过去的天数
    for (uint8_t m = 1U; m < month_bin; m++)
    {
        total_days += days_in_month[m - 1U];
        if ((m == 2U) && is_leap)
        {
            total_days += 1U;
        }
    }

    total_days += (uint32_t)(date_bin - 1U);

    // 计算总秒数
    const uint32_t unix_time = total_days * 86400U + hour_bin * 3600U + minute_bin * 60U + second_bin-8*3600U; // 减去8小时的秒数，转换为UTC时间

    unix_time_str[0] = (uint16_t)(unix_time >> 16);
    unix_time_str[1] = (uint16_t)(unix_time & 0xFFFFU);

    return unix_time_str;
}
