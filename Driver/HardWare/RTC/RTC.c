#include "RTC.h"

#define RTC_CLOCK_SOURCE_LXTAL
#define BKP_VALUE 0x32F0

rtc_parameter_struct rtc_initpara;
rtc_alarm_struct rtc_alarm;
__IO uint32_t prescaler_a = 0, prescaler_s = 0;
uint32_t RTCSRC_FLAG = 0;



#define RTC_BKP_LXTAL  0x7050

uint8_t RTC_Init(void)
{
    uint16_t prescaler_a = 127;   // 0x7F
    uint16_t prescaler_s = 255;   // 0xFF
    uint8_t need_init = 0;
    uint16_t bkpflag;

    /* ===== 0. 使能后备域 ===== */
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();

    bkpflag = RTC_BKP0;
    rcu_osci_on(RCU_LXTAL);

    if (rcu_osci_stab_wait(RCU_LXTAL) != SUCCESS)
    {
        /* ❌ 直接失败，不再 fallback */
        return 1;
    }

    /* 选择 LXTAL 作为 RTC 时钟源 */
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);

    /* ===== 2. 使能 RTC ===== */
    rcu_periph_clock_enable(RCU_RTC);

    rtc_register_sync_wait();

    /* ===== 3. 分频器完整校验 ===== */
    uint32_t psc = RTC_PSC;
    uint32_t asyn = (psc >> 16) & 0x7F;
    uint32_t syn  = psc & 0x7FFF;

    if ((asyn != prescaler_a) || (syn != prescaler_s))
    {
        need_init = 1;
    }
    if (bkpflag != RTC_BKP_LXTAL)
    {
        need_init = 1;
    }


    if (need_init)
    {
        rtc_parameter_struct rtc_initpara;

        rtc_initpara.factor_asyn = prescaler_a;
        rtc_initpara.factor_syn  = prescaler_s;
        rtc_initpara.display_format = RTC_24HOUR;

        if (rtc_init(&rtc_initpara) != SUCCESS)
            return 2;

        RTC_BKP0 = RTC_BKP_LXTAL;
    }

    /* ===== 6. 清理唤醒状态 ===== */
    rtc_wakeup_disable();

    while(rtc_flag_get(RTC_FLAG_WTW) == RESET);

    rtc_flag_clear(RTC_FLAG_WT);

    exti_interrupt_flag_clear(EXTI_22);

    pmu_flag_clear(PMU_FLAG_WAKEUP);

    return 0;
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
 * @brief       BCD码转换为十进制数据
 * @param       val : 要转换的BCD码 
 * @retval      十进制数据

static uint8_t rtc_bcd2dec(uint8_t val)
{
    uint8_t temp = 0;
    temp = (val >> 4) * 10;
    return (temp + (val & 0X0F));
}
 */
/**
 * @brief       RTC时间设置
 * @param       hour,min,sec: 小时,分钟,秒钟 
 * @param       ampm        : AM/PM, 0=AM/24H; 1=PM/12H;
 * @retval      0,成功
 *              1,初始化失败
 */
uint8_t rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec, uint8_t ampm)
{
    ErrStatus error_status = ERROR;   
  
    rtc_initpara.hour   = rtc_dec2bcd(hour);       
    rtc_initpara.minute = rtc_dec2bcd(min);       
    rtc_initpara.second = rtc_dec2bcd(sec);      
    rtc_initpara.am_pm  = ((uint32_t)ampm & 0X01);
    
    error_status = rtc_init(&rtc_initpara);   /* 根据参数初始化RTC寄存器 */
    
    if (ERROR == error_status) return 1;
    
    return 0;  
}

/**
 * @brief       RTC日期设置
 * @param       year,month,date : 年(0~99),月(1~12),日(1~31)
 * @param       week            : 星期(1~7,代表周一~周日;0,非法!)
 * @retval      0,成功
 *              1,初始化失败
 */
uint8_t rtc_set_date(uint8_t year, uint8_t month, uint8_t date)
{
   ErrStatus error_status = ERROR;   
  
    rtc_initpara.year        = rtc_dec2bcd(year);       
    rtc_initpara.month       = rtc_dec2bcd(month);       
    rtc_initpara.date        = rtc_dec2bcd(date);      
    
    error_status = rtc_init(&rtc_initpara);   /* 根据参数初始化RTC寄存器 */
    
    if (ERROR == error_status) return 1;
    
    return 0;  
}

// 日期超限检查独立封装
uint8_t time_data_check(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second)
{
    // 数值超限判断，串口返回数值输入错误的提示（包含具体月份的天数范围以及闰年）
    //  Check all input ranges and accumulate errors
    char error_msg[256] = "";
    int has_error = 0;

    // Check year range (assuming 2000-2099)
    if (year > 2099)
    {
        strcat(error_msg, "\r\n** Year input error! Range: 0-99 (for 2000-2099) **\r\n");
        has_error += 1;
    }
    // Check month range
    if (month < 1 || month > 12)
    {
        strcat(error_msg, "\r\n** Month input error! Range: 1-12 **\r\n");
        has_error += 1;
    }
    // Days in each month
    uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    // Leap year check
    uint16_t full_year = 2000 + year;
    if ((full_year % 4 == 0 && full_year % 100 != 0) || (full_year % 400 == 0))
    {
        days_in_month[1] = 29;
    }
    // Check date range (only if month is valid)
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
    // Check hour range
    if (hour > 23)
    {
        strcat(error_msg, "\r\n** Hour input error! Range: 0-23 **\r\n");
        has_error += 1;
    }
    // Check minute range
    if (minute > 59)
    {
        strcat(error_msg, "\r\n** Minute input error! Range: 0-59 **\r\n");
        has_error += 1;
    }
    // Check second range
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
    \param[in]   year, month, date, hour, minute, second
    \param[out] none
    \retval     none
*/
void rtc_setup(uint16_t year, uint8_t month, uint8_t date,
               uint8_t hour, uint8_t minute, uint8_t second)
{
    rtc_parameter_struct rtc_initpara;

    /* ===== 完整配置（必须）===== */
    rtc_initpara.factor_asyn = 0x7F;
    rtc_initpara.factor_syn  = 0xFF;
    rtc_initpara.display_format = RTC_24HOUR;

    /* ===== 时间 ===== */
    rtc_initpara.hour   = rtc_dec2bcd(hour);
    rtc_initpara.minute = rtc_dec2bcd(minute);
    rtc_initpara.second = rtc_dec2bcd(second);
    rtc_initpara.am_pm  = RTC_AM;

    /* ===== 日期 ===== */
    rtc_initpara.year  = rtc_dec2bcd(year - 2000);
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
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rtc_show_time(void)
{
    //    uint32_t time_subsecond = 0;
    //    uint8_t subsecond_ss = 0,subsecond_ts = 0,subsecond_hs = 0;

    rtc_current_time_get(&rtc_initpara);

    /* get the subsecond value of current time, and convert it into fractional format */
    //    time_subsecond = rtc_subsecond_get();
    //    subsecond_ss=(1000-(time_subsecond*1000+1000)/400)/100;
    //    subsecond_ts=(1000-(time_subsecond*1000+1000)/400)%100/10;
    //    subsecond_hs=(1000-(time_subsecond*1000+1000)/400)%10;

    printf("20%02d-%02d-%02d %02d:%02d:%02d \n\r",
           BCD2BYTE(rtc_initpara.year), BCD2BYTE(rtc_initpara.month), BCD2BYTE(rtc_initpara.date),
           BCD2BYTE(rtc_initpara.hour), BCD2BYTE(rtc_initpara.minute), BCD2BYTE(rtc_initpara.second));
}

/*!
    \brief      display the alram value
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rtc_show_alarm(void)
{
    rtc_alarm_get(RTC_ALARM0, &rtc_alarm);
    printf("The alarm: %02d:%02d:%02d \n\r", BCD2BYTE(rtc_alarm.alarm_hour), BCD2BYTE(rtc_alarm.alarm_minute),
           BCD2BYTE(rtc_alarm.alarm_second));
}

/*!
    \brief      get the input character string and check if it is valid
    \param[in]  none
    \param[out] none
    \retval     input value in BCD mode
*/
uint8_t usart_input_threshold(uint32_t value)
{
    uint32_t index = 0;
    uint32_t tmp[2] = {0, 0};

    while (index < 2)
    {
        while (RESET == usart_flag_get(USART0, USART_FLAG_RBNE))
            ;
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
