#include "data_process.h"

float adc_volt = 0.00f;      // ADC实际电压值
float dac_volt = 1.0f;
float eng_volt = 0.00f;      // 工程值=adc_volt*ratio
char encrypt_buf[24] = {0}; // 加密缓冲区，，8字节HEX=16位字符串

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

//加密字符串
char* data_encrypt(void)
{
    uint32_t unix_time = get_unix_time();

    //处理电压值的整数和小数部分
    uint16_t eng_volt_int = (uint16_t)eng_volt;
    float frac_part = eng_volt - (float)eng_volt_int;
    uint32_t frac_scaled = (uint32_t)lroundf(frac_part * 65536.0f);
    //四舍五入
    if (frac_scaled >= 65536UL) {
        eng_volt_int++;
        frac_scaled = 0;
    }
    uint16_t eng_volt_frac = (uint16_t)frac_scaled;
    //格式化输出为 16 字节的十六进制字符串
    snprintf(encrypt_buf, sizeof(encrypt_buf),
             "%08X%04X%04X",
             unix_time,
             eng_volt_int,
             eng_volt_frac);
    //超限，在末尾追加 '*' 标记 
    if (overlimit_flag == 1) {
        size_t current_len = strlen(encrypt_buf);
        if (current_len < sizeof(encrypt_buf) - 1) {
            encrypt_buf[current_len] = '*';
            encrypt_buf[current_len + 1] = '\0';
        }
    }
    
    return encrypt_buf;
}


#define BCD2DEC(val) (((val) >> 4) * 10 + ((val) & 0x0F))
//bcd！！！！
uint32_t get_unix_time(void)
{
    rtc_parameter_struct rtc_current;
    struct tm timeinfo = {0}; 
    rtc_current_time_get(&rtc_current);

    timeinfo.tm_year = BCD2DEC(rtc_current.year) + 100; 
    timeinfo.tm_mon  = BCD2DEC(rtc_current.month) - 1;  
    timeinfo.tm_mday = BCD2DEC(rtc_current.date);
    timeinfo.tm_hour = BCD2DEC(rtc_current.hour);
    timeinfo.tm_min  = BCD2DEC(rtc_current.minute);
    timeinfo.tm_sec  = BCD2DEC(rtc_current.second);
    
    timeinfo.tm_isdst = 0; // 明确关闭夏令时，防止时间诡异偏移

    return (uint32_t)mktime(&timeinfo) - 28800;
}


char* unix_to_str(uint32_t timestamp) 
{
    static char out_buf[20];
    timestamp += 28800;
    uint32_t rem = timestamp % 86400;
    uint32_t h = rem / 3600;
    uint32_t m = (rem % 3600) / 60;
    uint32_t s = rem % 60;
    //距离 0000年3月1日 的天数
    uint32_t days = timestamp / 86400 + 719468; 
    //推算年月
    uint32_t era = days / 146097;
    uint32_t doe = days - era * 146097;
    uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    uint32_t year = yoe + era * 400;
    uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    uint32_t mp = (5 * doy + 2) / 153;
    uint32_t day = doy - (153 * mp + 2) / 5 + 1;
    uint32_t month = mp + (mp < 10 ? 3 : -9);
    year += (month <= 2);

    out_buf[0] = (year / 1000) % 10 + '0';
    out_buf[1] = (year / 100) % 10 + '0';
    out_buf[2] = (year / 10) % 10 + '0';
    out_buf[3] = (year % 10) + '0';
    out_buf[4] = '-';
    
    out_buf[5] = (month / 10) + '0';
    out_buf[6] = (month % 10) + '0';
    out_buf[7] = '-';
    
    out_buf[8] = (day / 10) + '0';
    out_buf[9] = (day % 10) + '0';
    out_buf[10] = ' ';
    
    out_buf[11] = (h / 10) + '0';
    out_buf[12] = (h % 10) + '0';
    out_buf[13] = ':';
    
    out_buf[14] = (m / 10) + '0';
    out_buf[15] = (m % 10) + '0';
    out_buf[16] = ':';
    
    out_buf[17] = (s / 10) + '0';
    out_buf[18] = (s % 10) + '0';
    out_buf[19] = '\0'; //结尾
    return out_buf;
}


