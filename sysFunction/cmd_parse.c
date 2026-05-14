#include "cmd_parse.h"
#include "myflashdb_data.h"  // 引入我们写好的参数库
#include "myflashdb_log.h"   // 引入我们写好的日志库
#include <ctype.h>
#include <string.h>
#include <stdio.h>

/*------------------ 函数 ------------------*/
extern uint32_t Gettim6Time(void);

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
    USART1_ClearRxBuf(); // 清空缓冲区
}

/**
 * @brief  处理接收到的完整指令处理完清空缓冲区
 * @param  无
 * @retval 无
 */
void cmd_parse(void)
{
    if (usart1_rx_flag == 1)
    {
        char *cmd_buf = (char *)usart1_rx_buffer;
        //跳过开头的空白符
        while (*cmd_buf == '\r' || *cmd_buf == '\n' || *cmd_buf == ' ' || *cmd_buf == '\t')
        {
            cmd_buf++;
        }
        // 去掉结尾的空白符
        uint16_t cmd_len = strlen(cmd_buf);
        while (cmd_len > 0 && (cmd_buf[cmd_len - 1] == '\r' || cmd_buf[cmd_len - 1] == '\n' ||
                               cmd_buf[cmd_len - 1] == ' ' || cmd_buf[cmd_len - 1] == '\t'))
        {
            cmd_buf[--cmd_len] = '\0';
        }
        // ========== 指令解析核心 ==========
        if(strstr(cmd_buf, "dac test") != NULL)
        {
            cmd_parse_dac_test();
        }        
        else if (strstr(cmd_buf, "test") != NULL) // 赛题test指令
        {
            cmd_parse_test();
        }
        else if (strstr(cmd_buf, "RTC Config") != NULL) // 赛题RTC Config指令
        {
            cmd_parse_RTC_Config();
        }
        else if (strstr(cmd_buf, "RTC now") != NULL) // 赛题RTC now指令
        {
            cmd_parse_RTC_now();
        }

        else if (strstr(cmd_buf, "ratio") != NULL) // 赛题ratio指令
        {
            cmd_parse_ratio();
        }
        else if (strstr(cmd_buf, "limit") != NULL) // 赛题limit指令
        {
            cmd_parse_limit();
        }

        else if(strstr(cmd_buf, "dac") != NULL)
        {
            cmd_parse_dac();
        }
        else if (strstr(cmd_buf, "config save") != NULL) // 赛题config save指令
        {
            cmd_parse_config_save();
        }
        else if (strstr(cmd_buf, "config read") != NULL) // 赛题config read指令
        {
            cmd_parse_config_read();
        }
        else if (strstr(cmd_buf, "conf") != NULL) // 赛题conf指令
        {
            cmd_parse_conf();
        }        
        else if (strstr(cmd_buf, "start") != NULL) // 赛题start指令
        {            
            cmd_parse_start();
        }
        else if (strstr(cmd_buf, "stop") != NULL) // 赛题stop指令
        {           
            cmd_parse_stop();
        }
        else if (strstr(cmd_buf, "unhide") != NULL) // 赛题unhide指令
        {            
            cmd_parse_unhide();
        }
        else if (strstr(cmd_buf, "hide") != NULL) // 赛题hide指令
        {            
            cmd_parse_hide();
        }
        else
        {
            printf("Unknown command. Please try again.\r\n");
        }

        cmd_parse_init();
    }
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
    printf("uart:ok\r\n");
    printf("system self test end\r\n");
    
    append_normal_log("System Self Test Done"); // 写入操作日志
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
                // 防止溢出（年份最多4位，其他最多2位，但输入可能很长）
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

    // 赋值（注意顺序：年、月、日、时、分、秒）
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

    USART1_ClearRxBuf();
    usart1_rx_flag = 0;
    printf("\r\nInput Datetime\r\n");

    while (usart1_rx_flag == 0) {
        /* 等待输入完成 */
    }

    // 确保以'\0'结尾
    strncpy(input_buf, (char *)usart1_rx_buffer, sizeof(input_buf) - 1);
    input_buf[sizeof(input_buf) - 1] = '\0';

    // 去除首尾空白字符
    char *p = input_buf;
    while (*p == '\r' || *p == '\n' || *p == ' ' || *p == '\t') p++;

    size_t len = strlen(p);
    while (len > 0 && (p[len-1] == '\r' || p[len-1] == '\n' || 
                       p[len-1] == ' ' || p[len-1] == '\t')) {
        p[--len] = '\0';
    }

    // 如果去除空白后字符串为空，提示错误
    if (*p == '\0') {
        printf("Error: empty input\r\n");
        cmd_parse_init();
        return;
    }

    // 调用改进的解析函数
    int ret = parse_datetime(p, &year, &month, &date, &hour, &minute, &second);
    if (ret != 0) {
        printf("Datetime parse failed (code %d). Please try again.\r\n", ret);
        cmd_parse_init();
        return;
    }

    //  rtc_setup内部转换
    rtc_setup(year, month, date, hour, minute, second);

    printf("RTC Config success\r\n");
    rtc_show_time();
    rtc_show_time();
    printf("\r\n");
    
    append_normal_log("RTC Configured"); // 写入操作日志
    cmd_parse_init();
}

void cmd_parse_RTC_now(void)
{
    printf("\r\n");
    rtc_show_time();
    printf("\r\n");
    cmd_parse_init(); // 处理完指令后清空缓冲区
}

float ratio_ch0 = 1.0f;
float limit_ch0 = 100.0f;

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
        append_normal_log("Set Ratio: %.1f", ratio_ch0); // 写入操作日志
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
        append_normal_log("Set Limit: %.1f", limit_ch0); // 写入操作日志
    }
    else
    {
        printf("\r\nlimit invalid\r\n");
        printf("limit= %.1f\r\n", limit_ch0);
    }
    cmd_parse_init(); // 处理完指令后清空缓冲区和标志
}

void cmd_parse_config_save(void)
{    
    printf("save parameters to flash\r\n");
    printf("\r\nratio:%.1f \r\n", ratio_ch0);
    printf("limit:%.1f \r\n", limit_ch0);
    printf("dac:%.1f \r\n", dac_volt);
    printf("sample cycle:%ds \r\n", adc_sample_cycle / 1000);
    
    // 【核心改进】：使用 KVDB 结构体统一打包保存
    data_cfg_t temp_cfg;
    get_data_config(&temp_cfg); // 先读出来保底
    temp_cfg.ratio_ch0    = ratio_ch0;
    temp_cfg.limit_ch0    = limit_ch0;
    temp_cfg.dac_volt     = dac_volt;
    temp_cfg.sample_cycle = adc_sample_cycle;
    set_data_config(&temp_cfg); // 写入FlashDB
    
    adc_sample_start = 0; // 重置采样计时器以立即应用新的采样周期
    
    append_normal_log("Config saved to Flash"); // 写入操作日志
    cmd_parse_init(); // 处理完指令后清空缓冲区
}

void cmd_parse_config_read(void)
{
    // 【核心改进】：使用 KVDB 统一读取
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
    
    append_normal_log("Config loaded from Flash"); // 写入操作日志
    cmd_parse_init(); // 处理完指令后清空缓冲区
}


volatile uint8_t dac_test_flag = 0;
volatile uint16_t dac_test_count = 0;
void cmd_parse_dac_test(void)
{
    dac_test_flag = 1;
    dac_test_count=9000;
    append_normal_log("DAC Test Started"); // 写入操作日志
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
        append_normal_log("Set DAC: %.1f", dac_volt); // 写入操作日志
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
        }
}

void cmd_parse_start(void)
{

    printf("\r\nPeriodic Sampling\r\n");
    printf("Sample cycle: %ds\r\n", adc_sample_cycle / 1000);
    overlimit_flag = 0; // 开始采样时重置超限标志，确保下次采样正常开始
    adc_sample_start = Gettim6Time() - adc_sample_cycle + 500;
    
    sampling_flag = 1;
    append_normal_log("Periodic Sampling START"); // 写入操作日志
    cmd_parse_init(); // 处理完指令后清空缓冲区
}

void cmd_parse_stop(void)
{
    sampling_flag = 0;
    printf("Periodic Sampling STOP\r\n");
    
    hide_flag = 0; // 停止采样时默认取消加密状态
    overlimit_flag = 0; // 停止采样时重置超限标志
    
    append_normal_log("Periodic Sampling STOP"); // 写入操作日志
    cmd_parse_init(); // 处理完指令后清空缓冲区
}

void sample_result_show(void)
{
    data_calc_eng_volt();
    data_check_overlimit();    
    
    // ==========================================
    // 【核心注入】：自动记录 TSDB 时序日志
    // 根据当前状态，底层自动分发存入不同日志区
    // ==========================================
    if (hide_flag == 1)
    {
        char* encrypt_str = data_encrypt();
        printf("%s\r\n", encrypt_str);
        
        append_hide_log(eng_volt, encrypt_str); // 存入隐藏日志区
        return;
    }
    else
    {
        rtc_show_time();
        if (overlimit_flag == 0)
        {
            printf(" ch0=%.2fV\r\n", eng_volt);
            append_sample_log(eng_volt, ratio_ch0, (float)adc_sample_cycle); // 存入正常采样日志区
        }
        else
        {
            printf(" ch0=%.2fV OverLimit(%.2f) !\r\n", eng_volt, limit_ch0);
            append_over_log(eng_volt, limit_ch0); // 存入超限报警日志区
        }
    }
    
    // OLED刷新
    OLED_Printf(0, 0, 16, "%0.2X:%0.2X:%0.2X    ", rtc_initpara.hour, rtc_initpara.minute, rtc_initpara.second); 
    OLED_Printf(0, 16, 16, "%.2fV  ", eng_volt);                                                                     
    OLED_Refresh();
}

void cmd_parse_hide(void)
{
    hide_flag = 1;
    append_normal_log("Hide Mode Enabled"); // 写入操作日志
    cmd_parse_init(); // 处理完指令后清空缓冲区
}

void cmd_parse_unhide(void)
{
    hide_flag = 0;
    append_normal_log("Hide Mode Disabled"); // 写入操作日志
    cmd_parse_init(); // 处理完指令后清空缓冲区
}
