#include "RTC.h"
#include <string.h> // 添加此头文件以使用 memset
#include <stdio.h>

#define RTC_CLOCK_SOURCE_LXTAL
#define RTC_BKP_MARK 0x5050
#define RTC_BKP_LXTAL 0x7050

rtc_parameter_struct rtc_initpara;
rtc_alarm_struct rtc_alarm;
__IO uint32_t prescaler_a = 0, prescaler_s = 0;
uint32_t RTCSRC_FLAG = 0;

void RTC_Init(void)
{
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
    if(RTC_BKP0 != RTC_BKP_MARK)
    {
        rcu_osci_on(RCU_LXTAL);

        while(rcu_osci_stab_wait(RCU_LXTAL) != SUCCESS);
        rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
        rcu_periph_clock_enable(RCU_RTC);

        rtc_register_sync_wait();

        memset(&rtc_initpara, 0, sizeof(rtc_parameter_struct)); // 安全清零
        rtc_initpara.factor_asyn = 0x7F;
        rtc_initpara.factor_syn  = 0xFF;

        rtc_initpara.year = 0x24;
        rtc_initpara.month = RTC_MAR;
        rtc_initpara.date = 0x05;
        rtc_initpara.day_of_week = RTC_TUESDAY;

        rtc_initpara.hour = 0x12;
        rtc_initpara.minute = 0x00;
        rtc_initpara.second = 0x00;

        rtc_initpara.display_format = RTC_24HOUR;
        rtc_initpara.am_pm = RTC_AM;
        rtc_init(&rtc_initpara);

        RTC_BKP0 = RTC_BKP_MARK;
        exflash_erase_flag=1;
    }
    else
    {
        rcu_periph_clock_enable(RCU_RTC);
        rtc_register_sync_wait();
    }
}

/**
 * @brief       十进制转换为BCD码
 * @param       val : 要转换的十进制数 
 * @retval      BCD码
 */
static uint8_t rtc_dec2bcd(uint8_t val)
{
    uint8_t bcdhigh = 0;

    while (val >= 10)
    {
        bcdhigh++;
        val -= 10;
    }

    return ((uint8_t)(bcdhigh << 4) | val);
}

/**
 * @brief       RTC时间设置
 * @param       hour,min,sec: 小时,分钟,秒钟 
 * @retval      0,成功
 * 1,初始化失败
 */
uint8_t rtc_set_time(uint8_t h, uint8_t m, uint8_t s)
{
    rtc_parameter_struct rtc_time;
    ErrStatus error_status = ERROR;   
    
    // 【重要修复】先获取当前完整时间，防止调用 init 时修改掉原本的日期
    memset(&rtc_time, 0, sizeof(rtc_parameter_struct));
    rtc_current_time_get(&rtc_time); 
  
    rtc_time.hour   = rtc_dec2bcd(h);       
    rtc_time.minute = rtc_dec2bcd(m);       
    rtc_time.second = rtc_dec2bcd(s);      
    
    error_status = rtc_init(&rtc_time);   
    
    if (ERROR == error_status) return 1;
    
    return 0;  
}

/* 设置日期 */
void rtc_set_date(uint16_t y,uint8_t m,uint8_t d)
{
    rtc_parameter_struct date;
    
    memset(&date, 0, sizeof(rtc_parameter_struct));
    rtc_current_time_get(&date); 
    
    date.year = rtc_dec2bcd(y - 2000);
    date.month = rtc_dec2bcd(m);
    date.date = rtc_dec2bcd(d);
    rtc_init(&date);
}

// 日期超限检查独立封装
uint8_t time_data_check(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second)
{
    char error_msg[256] = "";
    int has_error = 0;

    if (year > 2099)
    {
        strcat(error_msg, "\r\n** Year input error! Range: 0-99 (for 2000-2099) **\r\n");
        has_error += 1;
    }
    if (month < 1 || month > 12)
    {
        strcat(error_msg, "\r\n** Month input error! Range: 1-12 **\r\n");
        has_error += 1;
    }
    uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint16_t full_year = 2000 + year;
    if ((full_year % 4 == 0 && full_year % 100 != 0) || (full_year % 400 == 0))
    {
        days_in_month[1] = 29;
    }
    if (month >= 1 && month <= 12)
    {
        if (date < 1 || date > days_in_month[month - 1])
        {
            char tmp[64];
            sprintf(tmp, "\r\n** Date input error! Range: 1-%d for month %d **\r\n", days_in_month[month - 1], month);
            strcat(error_msg, tmp);
            has_error += 1;
        }
    }
    if (hour > 23)
    {
        strcat(error_msg, "\r\n** Hour input error! Range: 0-23 **\r\n");
        has_error += 1;
    }
    if (minute > 59)
    {
        strcat(error_msg, "\r\n** Minute input error! Range: 0-59 **\r\n");
        has_error += 1;
    }
    if (second > 59)
    {
        strcat(error_msg, "\r\n** Second input error! Range: 0-59 **\r\n");
        has_error += 1;
    }
    if (has_error)
    {
        printf("%s", error_msg);
        return has_error;
    }
    return 0;
}

/*!
    \brief      RTC设置时间函数(一次性输入年月日时分秒6个值，直接设置当前时间)
*/
void rtc_setup(uint16_t year, uint8_t month, uint8_t date,
               uint8_t hour, uint8_t minute, uint8_t second)
{
    rtc_parameter_struct rtc_initpara;

    ///清空结构体，避免随机垃圾值写入硬件寄存器
    memset(&rtc_initpara, 0, sizeof(rtc_parameter_struct));

    /* ===== 完整配置）===== */
    rtc_initpara.factor_asyn = 0x7F;
    rtc_initpara.factor_syn  = 0xFF;
    rtc_initpara.display_format = RTC_24HOUR;
    rtc_initpara.am_pm  = RTC_AM;
    rtc_initpara.day_of_week = RTC_MONDAY; // 必须赋一个有效值防止硬件异常

    /* ===== 时间 ===== */
    rtc_initpara.hour   = rtc_dec2bcd(hour);
    rtc_initpara.minute = rtc_dec2bcd(minute);
    rtc_initpara.second = rtc_dec2bcd(second);

    /* ===== 日期 ===== */
    rtc_initpara.year  = rtc_dec2bcd((uint8_t)(year - 2000));
    rtc_initpara.month = rtc_dec2bcd(month);
    rtc_initpara.date  = rtc_dec2bcd(date);

    if (rtc_init(&rtc_initpara) != SUCCESS)
    {
        printf("RTC set failed\r\n");
        return;
    }

    rtc_register_sync_wait();
}

/*!
    \brief      display the current time
*/
void rtc_show_time(void)
{
    rtc_current_time_get(&rtc_initpara);

    printf("20%02d-%02d-%02d %02d:%02d:%02d \n\r",
           BCD2BYTE(rtc_initpara.year), BCD2BYTE(rtc_initpara.month), BCD2BYTE(rtc_initpara.date),
           BCD2BYTE(rtc_initpara.hour), BCD2BYTE(rtc_initpara.minute), BCD2BYTE(rtc_initpara.second));
}

/*!
    \brief      display the alram value
*/
void rtc_show_alarm(void)
{
    rtc_alarm_get(RTC_ALARM0, &rtc_alarm);
    printf("The alarm: %02d:%02d:%02d \n\r", BCD2BYTE(rtc_alarm.alarm_hour), BCD2BYTE(rtc_alarm.alarm_minute),
           BCD2BYTE(rtc_alarm.alarm_second));
}

/*!
    \brief      get the input character string and check if it is valid
*/
uint8_t usart_input_threshold(uint32_t value)
{
    uint32_t index = 0;
    uint32_t tmp[2] = {0, 0};

    while (index < 2)
    {
        while (RESET == usart_flag_get(USART0, USART_FLAG_RBNE));
        tmp[index++] = usart_data_receive(USART0);
        if ((tmp[index - 1] < 0x30) || (tmp[index - 1] > 0x39))
        {
            printf("\n\r please input a valid number between 0 and 9 \n\r");
            index--;
        }
    }

    index = (tmp[1] - 0x30) + ((tmp[0] - 0x30) * 10);
    if (index > value)
    {
        printf("\n\r please input a valid number between 0 and %d \n\r", value);
        return 0xFF;
    }

    index = (tmp[1] - 0x30) + ((tmp[0] - 0x30) << 4);
    return index;
}
