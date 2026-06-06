#include "cmd_parse.h"

char *cmd_buf;

/*------------------ 函数 ------------------*/
extern uint32_t Gettim6Time(void);

typedef void (*cmd_handler_t)(void);

//邪恶指令集
typedef struct
{
    const char *name;
    cmd_handler_t handler;
    const char *success_log;
} cmd_entry_t;


//命令表名称匹配
static uint8_t cmd_name_match(const char *input, const char *name);
//包装condigsave
static void cmd_handle_config_save(void);
//指令分发
static uint8_t cmd_dispatch(const char *input);
//指令表+函数+无参数日志，，
static const cmd_entry_t g_cmd_table[] =
{
    {"dac test",    cmd_parse_dac_test,    NULL},
    {"lightsleep",  cmd_parse_lightsleep,  NULL},
    {"standby",     cmd_parse_standby,     NULL},
    {"sleep",       cmd_parse_deepsleep,   NULL},
    {"ad3344",      AD3344_cmd,            NULL},
    {"pt100",       cmd_parse_pt100,       NULL},
    {"sample read", cmd_parse_sample_read, NULL},
    {"over read",   cmd_parse_over_read,   NULL},
    {"hide read",   cmd_parse_hide_read,   NULL},
    {"log read",    cmd_parse_log_read,    NULL},
    {"test",        cmd_parse_test,        "test OK"},
    {"RTC Config",  cmd_parse_RTC_Config,  NULL},
    {"RTC now",     cmd_parse_RTC_now,     NULL},
    {"ratio",       cmd_parse_ratio,       NULL},
    {"limit",       cmd_parse_limit,       NULL},
    {"dac",         cmd_parse_dac,         NULL},
    {"config save", cmd_handle_config_save, "Config saved to Flash"},
    {"config read", cmd_parse_config_read, NULL},
    {"conf",        cmd_parse_conf,        NULL},
    {"start",       cmd_parse_start,       "Periodic Sampling START(CMD)"},
    {"stop",        cmd_parse_stop,        "Periodic Sampling STOP(CMD)"},
    {"unhide",      cmd_parse_unhide,      "Hide disabled"},
    {"hide",        cmd_parse_hide,        "Hide enabled"},
};


//////////////////////test函数补充///////////////////////////////////
static uint8_t selftest_flash(void)
{
    char device_id[64] = {0};
    
    // 从 KVDB 参数区读取队伍编号
    get_team_number(device_id, sizeof(device_id));

    if (strlen(device_id) == 0) {
        return 0;
    }

    return (strncmp(device_id, "Device_ID:", 10) == 0);
}

static uint8_t selftest_oled(void)
{
    OLED_Printf(0, 0, 16, "OLED SelfTest");
    OLED_Printf(0, 16, 16, "Refresh OK");
    OLED_Refresh();
    oled_idle_time = 3000; 
    return 1;
}

static uint8_t selftest_rtc(void)
{
    rtc_current_time_get(&rtc_initpara);

    return (rtc_initpara.month <= 0x12 &&
            rtc_initpara.date <= 0x31 &&
            rtc_initpara.hour <= 0x23 &&
            rtc_initpara.minute <= 0x59 &&
            rtc_initpara.second <= 0x59);
}

static uint8_t selftest_adc(void)
{
    uint16_t adc_value_raw = ADC_get();
    return (adc_value_raw <= 4095U);
}

static uint8_t selftest_dac(void)
{
    DAC_Init();
    DAC_Set(DAC_OUT0, 2048U);//1.5V
    DAC_Set(DAC_OUT1, 1024U);//0.82V

    return (DAC_Get(DAC_OUT0) == 2048U) && (DAC_Get(DAC_OUT1) == 1024U);
}
////////////////////////////////end////////////////////////////////


/**
 * @brief  清空USART1接收缓冲区
 * @param  无
 * @retval 无
 */
void cmd_parse_init(void)
{
    USART1_ClearRxBuf();
}



/**************************查表核心函数*3****************************** */
//
static uint8_t cmd_name_match(const char *input, const char *name)
{
    size_t name_len;
    if ((input == NULL) || (name == NULL))
    {
        return 0U;
    }
    name_len = strlen(name);
    if (strncmp(input, name, name_len) != 0)
    {
        return 0U;
    }
    return (input[name_len] == '\0') || isspace((unsigned char)input[name_len]);
}

static void cmd_handle_config_save(void)
{
    cmd_parse_config_save(1);
}
//查表分发器
static uint8_t cmd_dispatch(const char *input)
{
    uint16_t i;

    for (i = 0U; i < (uint16_t)(sizeof(g_cmd_table) / sizeof(g_cmd_table[0])); i++)
    {
        if (cmd_name_match(input, g_cmd_table[i].name))
        {
            g_cmd_table[i].handler();
            if (g_cmd_table[i].success_log != NULL)
            {
                //无参数日志
                append_normal_log(g_cmd_table[i].success_log);
            }
            return 1U;
        }
    }

    return 0U;
}

/**
 * @brief  处理接收到的完整指令处理完清空缓冲区
//指令处理外部调用函数
 */
void cmd_parse(void)
{
    if (usart1_rx_flag == 1)
    {
        cmd_buf = (char *)usart1_rx_buffer;
        while (*cmd_buf == '\r' || *cmd_buf == '\n' || *cmd_buf == ' ' || *cmd_buf == '\t')
        {
            cmd_buf++;
        }

        uint16_t cmd_len = strlen(cmd_buf);
        while (cmd_len > 0 && (cmd_buf[cmd_len - 1] == '\r' || cmd_buf[cmd_len - 1] == '\n' ||
                               cmd_buf[cmd_len - 1] == ' ' || cmd_buf[cmd_len - 1] == '\t'))
        {
            cmd_buf[--cmd_len] = '\0';
        }

        if (cmd_dispatch(cmd_buf) == 0U)
        {
            printf("[ERROR] Unknown Command\r\n");
        }

        cmd_parse_init();
    }
}

void cmd_parse_lightsleep(void)
{
    OLED_Printf(0, 0, 16, "Light Sleep  ");
    OLED_Refresh();
    printf("\r\nEnter light sleep, wake by USART1.\r\n");
    UART_LightSleep_Enter();
    printf("\r\nWake Up By USART1\r\n");
    OLED_Printf(0, 0, 16, "wake up uart  ");
    OLED_Refresh();
    oled_idle_time = 2000;
}

void cmd_parse_standby(void)
{
    uint32_t standby_sec = 10U;

    if (sscanf(cmd_buf, "standby %lu", &standby_sec) != 1 || standby_sec == 0U)
    {
        standby_sec = 10U;
    }

    OLED_Printf(0, 0, 16, "Standby Mode ");
    OLED_Refresh();
    printf("\r\nEnter standby, wake by RTC or PA0, sec=%lu.\r\n", standby_sec);
    Standby_Enter(standby_sec);
}

void cmd_parse_deepsleep(void)
{
    OLED_Printf(0, 0, 16, "Sleep Mode   ");
    OLED_Refresh();
    RTC_SetWakeup(10);                     
    pmu_flag_clear(PMU_FLAG_RESET_WAKEUP); 
    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);
    SystemInit();
    USART1_Init();
    printf("\r\nWake Up OK\r\n");
    OLED_Printf(0, 0, 16, "wake up ok    ");
    OLED_Refresh();
    oled_idle_time = 2000;
}



void cmd_parse_test(void)
{
    uint8_t flash_ok = selftest_flash();
    uint8_t oled_ok = selftest_oled();
    uint8_t rtc_ok = selftest_rtc();
    uint8_t adc_ok = selftest_adc();
    uint8_t dac_ok = selftest_dac();
    printf("\r\nsystem self test start\r\n");
    printf("flash:%s\r\n", flash_ok ? "ok" : "error");
    printf("oled:%s\r\n", oled_ok ? "ok" : "error");
    printf("rtc:%s\r\n", rtc_ok ? "ok" : "error");
    printf("adc:%s\r\n", adc_ok ? "ok" : "error");
    printf("dac:%s\r\n", dac_ok ? "ok" : "error");
//    delay_1ms(2);
    printf("uart:ok\r\n");
//    delay_1ms(2);
    printf("system self test end\r\n");
    cmd_parse_init(); // 处理完指令后清空缓冲区
}


int parse_datetime(const char *str, 
                   uint16_t *year, uint8_t *month, uint8_t *date,
                   uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    // 参数检查
    if (!str || !year || !month || !date || !hour || !minute || !second) {
        printf("Error: NULL parameter\r\n");
        return -1;
    }

    const char *p = str;
    int field_count = 0;
    uint16_t values[6] = {0};   // 存储解析出的6个数值

    // 跳过可能的前导空白
    while (isspace((unsigned char)*p)) p++;

    // 主解析循环：提取所有连续数字块
    while (*p && field_count < 6) {
        if (isdigit((unsigned char)*p)) {
            // 开始解析一个数字
            uint32_t val = 0;
            while (isdigit((unsigned char)*p)) {
                val = val * 10 + (*p - '0');
                p++;
                // 防止溢出
                if (val > 9999) break;  // 年份最大2100，截断合理
            }
            values[field_count++] = (uint16_t)val;
        } else {
            p++;  // 跳过非数字字符
        }
    }

    // 必须恰好提取到6个数字
    if (field_count != 6) {
        printf("Error: expected 6 numbers, but only %d found\r\n", field_count);
        printf("Input: %s\r\n", str);
        return -2;
    }

    // 赋值（注意顺序年、月、日、时、分、秒）
    *year   = values[0];
    *month  = (uint8_t)values[1];
    *date   = (uint8_t)values[2];
    *hour   = (uint8_t)values[3];
    *minute = (uint8_t)values[4];
    *second = (uint8_t)values[5];

    // ---------- 范围验证与修正 ----------
    // 1. 年份处理：支持2位缩写（00-99 → 2000-2099）
    if (*year < 100) {
        *year += 2000;
    }
    // 年份必须为4位且在合理范围内
    if (*year < 2000 || *year > 2100) {
        printf("Error: year %d out of range (2000-2100)\r\n", *year);
        return -3;
    }

    // 2. 月份 (1-12)
    if (*month < 1 || *month > 12) {
        printf("Error: month %d out of range (1-12)\r\n", *month);
        return -4;
    }

    // 3. 日期 (1-31，简化检查，如需精确验证可调用日期有效性函数)
    if (*date < 1 || *date > 31) {
        printf("Error: day %d out of range (1-31)\r\n", *date);
        return -5;
    }

    // 4. 小时 (0-23)
    if (*hour > 23) {
        printf("Error: hour %d out of range (0-23)\r\n", *hour);
        return -6;
    }

    // 5. 分钟 (0-59)
    if (*minute > 59) {
        printf("Error: minute %d out of range (0-59)\r\n", *minute);
        return -7;
    }

    // 6. 秒 (0-59)
    if (*second > 59) {
        printf("Error: second %d out of range (0-59)\r\n", *second);
        return -8;
    }

    return 0;  // 成功
}


void cmd_parse_RTC_Config(void)
{
    uint16_t year;
    uint8_t month, date, hour, minute, second;
    char input_buf[64];

    printf("\r\nInput Datetime (YYYY-MM-DD HH:MM:SS):\r\n");

    USART1_ClearRxBuf();
    usart1_rx_flag = 0;
    while (usart1_rx_flag == 0);
    strncpy(input_buf, (char *)usart1_rx_buffer, sizeof(input_buf) - 1);
    input_buf[sizeof(input_buf) - 1] = '\0';
    usart1_rx_flag = 0;

    char *p = input_buf;

    while (*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t') p++;

    size_t len = strlen(p);
    while (len > 0 &&
          (p[len-1] == ' ' || p[len-1] == '\r' ||
           p[len-1] == '\n' || p[len-1] == '\t'))
    {
        p[--len] = '\0';
    }

    if (*p == '\0')
    {
        printf("Error: empty input\r\n");
        cmd_parse_init();
        return;
    }

    if (sscanf(p, "%hu-%hhu-%hhu %hhu:%hhu:%hhu",
               &year, &month, &date,
               &hour, &minute, &second) != 6)
    {
        printf("Format error! Use: YYYY-MM-DD HH:MM:SS\r\n");
        cmd_parse_init();
        return;
    }

    if (year < 2000 || year > 2099 ||
        month < 1 || month > 12 ||
        date  < 1 || date  > 31 ||
        hour  > 23 ||
        minute > 59 ||
        second > 59)
    {
        printf("Invalid datetime range\r\n");
        cmd_parse_init();
        return;
    }

    if ((month == 2 && date > 29) ||
        ((month == 4 || month == 6 || month == 9 || month == 11) && date > 30))
    {
        printf("Invalid date\r\n");
        cmd_parse_init();
        return;
    }
    
    //调用rtc_setup，废弃之前拆分调用可能引起的野指针异常
    rtc_setup(year, month, date, hour, minute, second);
    
    printf("RTC Config OK\r\n");
    rtc_show_time();
    printf("\r\n");

    append_normal_log("RTC Config OK");
    cmd_parse_init();
}


void cmd_parse_RTC_now(void)
{
    printf("\r\n");
    rtc_show_time();
    printf("\r\n");
    cmd_parse_init(); // 处理完指令后清空缓冲区
}

void cmd_parse_conf(void)
{    
    printf("\r\ncurrent config\r\n");
    printf("ratio:%.1f\r\n", ratio_ch0);
    printf("limit:%.1f\r\n", limit_ch0);
    printf("dac:%.1fV\r\n",dac_volt);
    printf("sample cycle:%ds\r\n",adc_sample_cycle/1000);
    cmd_parse_init(); // 处理完指令后清空缓冲区
}

void cmd_parse_ratio(void)
{
    float new_ratio_ch0 = 0.00f;

    printf("\r\nratio = %.1f\r\n", ratio_ch0);
    printf("Input value(0-100): \r\n");
    cmd_parse_init(); // 处理完指令后清空缓冲区
    while (usart1_rx_flag == 0)
    {
        /* wait for input */
    }

    sscanf((char *)usart1_rx_buffer, "%f", &new_ratio_ch0);
    if (new_ratio_ch0 >= 0.0f && new_ratio_ch0 <= 100.0f)
    {
        ratio_ch0 = new_ratio_ch0;
        printf("\r\nratio set ok\r\n");
        printf("ratio=%.1f\r\n", ratio_ch0);
        append_normal_log("ratio set ok %.1f", ratio_ch0); // 写入操作日志
    }
    else
    {
        printf("\r\nratio invalid\r\n");
        printf("ratio= %.1f\r\n", ratio_ch0);
    }
    cmd_parse_init(); // 处理完指令后清空缓冲区和标志
}

void cmd_parse_limit(void)
{
    float new_limit_ch0 = 0.0f;
    printf("\r\nLimit = %.1f\r\n", limit_ch0);
    printf("Input value(0-500): \r\n");
    cmd_parse_init(); // 处理完指令后清空缓冲区

    while (usart1_rx_flag == 0)
    {
        /* wait for input */
    }

    sscanf((char *)usart1_rx_buffer, "%f", &new_limit_ch0);
    if (new_limit_ch0 >= 0.0f && new_limit_ch0 <= 500.0f)
    {
        limit_ch0 = new_limit_ch0;
        printf("\r\nlimit set ok\r\n");
        printf("limit= %.1f\r\n", limit_ch0);    
        append_normal_log("limit set ok %.1f", limit_ch0); // 写入操作日志
    }
    else
    {
        printf("\r\nlimit invalid\r\n");
        printf("limit= %.1f\r\n", limit_ch0);
    }
    cmd_parse_init(); // 处理完指令后清空缓冲区和标志
}

void cmd_parse_config_save(uint8_t printf_flag)
{
    if (printf_flag)
    {
        printf("save parameters to flash\r\n");
        printf("\r\nratio:%.1f \r\n", ratio_ch0);
        printf("limit:%.1f \r\n", limit_ch0);
        printf("dac:%.1f \r\n", dac_volt);
        printf("sample cycle:%ds \r\n", adc_sample_cycle / 1000);
    }
    data_cfg_t temp_cfg;
    get_data_config(&temp_cfg); // 先读出来保底
    temp_cfg.ratio_ch0    = ratio_ch0;
    temp_cfg.limit_ch0    = limit_ch0;
    temp_cfg.dac_volt     = dac_volt;
    temp_cfg.sample_cycle = adc_sample_cycle;
    set_data_config(&temp_cfg); // 写入FlashDB
    
    adc_sample_start = 0; // 重置采样计时器以立即应用新的采样周期
    cmd_parse_init(); // 处理完指令后清空缓冲区
}

void cmd_parse_config_read(void)
{
    data_cfg_t temp_cfg;
    get_data_config(&temp_cfg); 
    
    ratio_ch0        = temp_cfg.ratio_ch0;
    limit_ch0        = temp_cfg.limit_ch0;
    dac_volt         = temp_cfg.dac_volt;
    adc_sample_cycle = temp_cfg.sample_cycle;

    printf("\r\nread parameters from flash\r\n");
    printf("ratio:%.1f \r\n", ratio_ch0);
    printf("limit:%.1f \r\n", limit_ch0);
    printf("dac:%.1f \r\n", dac_volt); 
    printf("sample cycle: %ds \r\n", adc_sample_cycle / 1000);
    
    append_normal_log("Config read ok"); // 写入操作日志
    cmd_parse_init(); // 处理完指令后清空缓冲区
}


volatile uint8_t dac_test_flag = 0;
volatile uint16_t dac_test_count = 0;
void cmd_parse_dac_test(void)
{
    dac_test_flag = 1;
    dac_test_count=9000;

    cmd_parse_init(); // 处理完指令后清空缓冲区
}

void cmd_parse_dac(void)
{
    float new_dac = 0.0f;
    printf("\r\ndac=%.1fV\r\n", dac_volt);
    printf("Input value(0-3.0): \r\n");
    cmd_parse_init(); // 处理完指令后清空缓冲区

    while (usart1_rx_flag == 0)
    {
        /* wait for input */
    }

    sscanf((char *)usart1_rx_buffer, "%f", &new_dac);
    if (new_dac >= 0.0f && new_dac <= 3.0f)
    {
        dac_volt = new_dac;
        printf("\r\ndac set ok\r\n");
        printf("dac= %.1f\r\n", dac_volt);
        append_normal_log("dac set ok %.1f", dac_volt); // 写入操作日志
    }
    else
    {
        printf("\r\ndac invalid\r\n");
        printf("dac= %.1f\r\n", dac_volt);
    }
    DAC_SetVoltage(0, dac_volt);
    DAC_SetVoltage(1, dac_volt);
    cmd_parse_init(); // 处理完指令后清空缓冲区和标志
}


void cmd_parse_pt100(void)
{
    float voltage = 0.0f;
    float res_ohm = 0.0f;
    float temp_c = 0.0f;

    if (PT100_ReadVoltage(&voltage) != 0U) {
        res_ohm = PT100_VoltageToResistance(voltage);
        temp_c = PT100_ResistanceToTemp(res_ohm);
        printf("\r\nPT100 cfg=0x%04X algo=%s V=%.6fV R=%.3fohm T=%.2fC\r\n",
               AD3344_GetConfig(), PT100_GetAlgoName(), voltage, res_ohm, temp_c);
    } else {
        printf("\r\nPT100 read failed\r\n");
    }

    cmd_parse_init();
}

uint8_t dac_test_flag6=0;
uint8_t dac_test_flag3=0;
uint8_t dac_test_flag0=0;
void dac_test_tick(void)
{
    if(dac_test_flag == 1)
    {
        OLED_Printf(0,0,16,"DAC Test     ");
        OLED_Printf(0,16,16,"DAC:1.0V ");
        OLED_Refresh();
        printf("\r\nDAC Test Start\r\n");
        printf("DAC Output:1.0V\r\n");
        DAC_SetVoltage(0, 1.0f);
        DAC_SetVoltage(1, 1.0f);   
        dac_test_flag=0; 
        append_normal_log("DAC Test Started");
    }  
        if(dac_test_flag6==1)
        {
            OLED_Printf(0,16,16,"DAC:1.5V ");
            OLED_Refresh();
            printf("DAC Output:1.5V\r\n");
            DAC_SetVoltage(0, 1.5f);
            DAC_SetVoltage(1, 1.5f);
            dac_test_flag6=0;
        }
        if(dac_test_flag3==1)
        {
            OLED_Printf(0,16,16,"DAC:2.0V ");
            OLED_Refresh();
            printf("DAC Output:2.0V\r\n");
            DAC_SetVoltage(0, 2.0f);
            DAC_SetVoltage(1, 2.0f);
            dac_test_flag3=0;
        }
        if(dac_test_flag0==1)
        {
            OLED_Clear();
            OLED_Printf(0,0,16,"system idle");
            OLED_Refresh();
            dac_test_flag=0;
            printf("DAC Test End\r\n");
            DAC_SetVoltage(0, dac_volt);
            DAC_SetVoltage(1, dac_volt);
            dac_test_flag0=0;
            append_normal_log("DAC Test Ended"); 
        }
}

void cmd_parse_start(void)
{
    printf("\r\nPeriodic Sampling\r\n");
    printf("Sample cycle: %ds\r\n", adc_sample_cycle / 1000);
    overlimit_flag = 0; // 重置超限标志
    sampling_flag = 1;
    sample_result_show();
    adc_sample_start = 0;
    cmd_parse_init(); // 处理完指令后清空缓冲区
}

void cmd_parse_stop(void)
{
    sampling_flag = 0;
    printf("\r\nPeriodic Sampling STOP\r\n");
    
    hide_flag = 0; // 停止采样取消加密
    overlimit_flag = 0; // 重置超限标志
    cmd_parse_init(); 
    oled_idle_time=10;
}

void sample_result_show(void)
{
    float ch1_eng_volt;
    uint8_t ch0_over;
    uint8_t ch1_over;

    data_calc_eng_volt();
    ch1_eng_volt = (((float)ADC_get_ch1() * 3.3f) / 4095.0f) * ratio_ch1;
    ch0_over = (eng_volt > limit_ch0) ? 1U : 0U;
    ch1_over = (ch1_eng_volt > limit_ch1) ? 1U : 0U;
    overlimit_flag = ((ch0_over != 0U) || (ch1_over != 0U)) ? 1U : 0U;

    if (hide_flag == 1)
    {
        char* encrypt_str = data_encrypt();
        printf("%s\r\n", encrypt_str);
        append_hide_log(eng_volt, encrypt_str);
    }
    else
    {
        rtc_show_time();
        printf(" ch0=%.2fV ch1=%.2fV", eng_volt, ch1_eng_volt);

        if (ch0_over == 0U && ch1_over == 0U)
        {
            printf("\r\n");
            append_sample_log(eng_volt, ratio_ch0, (float)adc_sample_cycle);
        }
        else
        {
            printf(" OverLimit!\r\n");
            if (ch0_over != 0U)
            {
                append_over_log_ch("CH0", eng_volt, limit_ch0);
                if (alarm_report_mode == 1U)
                {
                    printf("%s | CH0 | %.2f | %.2f\r\n", unix_to_str(get_unix_time()), limit_ch0, eng_volt);
                }
            }
            if (ch1_over != 0U)
            {
                append_over_log_ch("CH1", ch1_eng_volt, limit_ch1);
                if (alarm_report_mode == 1U)
                {
                    printf("%s | CH1 | %.2f | %.2f\r\n", unix_to_str(get_unix_time()), limit_ch1, ch1_eng_volt);
                }
            }
        }
    }

    OLED_Printf(0, 16, 16, "%.2fV  ", eng_volt);
}

void cmd_parse_hide(void)
{
    hide_flag = 1;
    cmd_parse_init(); // 处理完指令后清空缓冲区
}

void cmd_parse_unhide(void)
{
    hide_flag = 0;
    cmd_parse_init(); 
}


void cmd_parse_sample_read(void)
{
    print_latest_sample_logs(10);
    cmd_parse_init(); 
}

void cmd_parse_over_read(void)
{
    print_latest_over_logs(10);
    cmd_parse_init(); 
}
void cmd_parse_hide_read(void)
{
    print_latest_hide_logs(10);
    cmd_parse_init(); 
}
void cmd_parse_log_read(void)
{
    print_latest_normal_logs(20);
    cmd_parse_init(); 
}
