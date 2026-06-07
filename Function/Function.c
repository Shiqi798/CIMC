#include "HeaderFiles.h"

uint8_t oled_idle_refresh_flag = 0;
uint16_t oled_idle_time = 0;

static uint8_t function_auto_sample_flag = 0;

char *DEVICE_ID = "2026639584";

uint8_t sampling_flag = 0;
uint8_t overlimit_flag = 0;
uint8_t hide_flag = 0;

////////////////////////////Func//////////////////////////////
static void function_param_load(void);
static void function_oled_show_idle(void);
static void function_led_update(void);
static void function_nvic_config(void);
static uint8_t function_is_power_on_reset(void);

void sysFunction_Init(void)
{
    char current_dev_id[64];

    __enable_irq();
    SystemInit();
    systick_config();

    RTC_Init();
    USART1_Init();
    LED_Init();
    ADC_port_init();
    DAC_Init();
    spi_flash_init();

    fal_init();
    flashdata_init();
    flash_log_init();
    exflash_erase_flag = 0;

    OLED_Init();

    get_team_number(current_dev_id, sizeof(current_dev_id));
    if ((strcmp(current_dev_id, "DEFAULT_TEAM") == 0) ||
        (strncmp(current_dev_id, "Device_ID:", 10) == 0)) {
        set_team_number(DEVICE_ID);
    }

    function_oled_show_idle();
    function_nvic_config();
    function_param_load();

    overlimit_flag = 0;
    hide_flag = 0;
    function_auto_sample_flag = 0;

    if (function_is_power_on_reset() != 0U) {
        set_power_count();
    }

    AD3344_Init();
    tim6_functimer_init();
}

void sysFunction_loop(void)
{
    while (1)
    {
        function_led_update();
        msg_poll();
        oled_idle_refresh();
    }
}

void function_sample_state_set(uint8_t on)
{
    function_auto_sample_flag = (on != 0U) ? 1U : 0U;
}

uint8_t function_sample_state_get(void)
{
    return function_auto_sample_flag;
}

void function_idle_refresh_request(void)
{
    oled_idle_refresh_flag = 1U;
}

void oled_idle_refresh(void)
{
    if (oled_idle_refresh_flag != 0U)
    {
        function_oled_show_idle();
        oled_idle_refresh_flag = 0U;
    }
}

static void function_param_load(void)
{
    data_cfg_t sys_cfg;

    get_data_config(&sys_cfg);
    adc_sample_cycle = sys_cfg.sample_cycle;
    msg_device_id = sys_cfg.device_id;
    ratio_ch0 = sys_cfg.ratio_ch0;
    ratio_ch1 = sys_cfg.ratio_ch1;
    limit_ch0 = sys_cfg.limit_ch0;
    limit_ch1 = sys_cfg.limit_ch1;
    dac_volt = sys_cfg.dac_volt;
    alarm_report_mode = sys_cfg.alarm_report_mode;
    usart1_baud_mode = sys_cfg.baud_mode;

    DAC_SetVoltage(0, dac_volt);
    DAC_SetVoltage(1, dac_volt);

    USART1_Init();
    delay_1ms(5);
}

static void function_oled_show_idle(void)
{
    OLED_ShowString(0, 0, (u8*)DEVICE_ID, 16);
    OLED_ShowString(0, 16, "IDLE        ", 16);
    OLED_Refresh();
}

static void function_led_update(void)
{
    static uint32_t led1_turn_start = 0;
    static uint32_t oled_sample_start = 0;

    if (tim6_timeoutcheck(&led1_turn_start, 1000) != 0U) {
        led1_turn();
    }

    if ((sampling_flag != 0U) || (function_sample_state_get() != 0U)) {
        led2_on();
        if (tim6_timeoutcheck(&oled_sample_start, 1000) != 0U) {
            OLED_ShowString(0, 0, (u8*)DEVICE_ID, 16);
            OLED_ShowString(0, 16, "AutoSample  ", 16);
            OLED_Refresh();
        }
    } else {
        led2_off();
    }
}

static void function_nvic_config(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
    nvic_irq_enable(SDIO_IRQn, 0, 0);
}

static uint8_t function_is_power_on_reset(void)
{
    uint8_t ret;

    ret = (rcu_flag_get(RCU_FLAG_PORRST) == SET) ? 1U : 0U;
    rcu_all_reset_flag_clear();
    return ret;
}

/****************************End*****************************/

/*
 * old debug path
 *
 * main loop:
 *     function_key_update();
 *     dac_test_tick();
 *
 * key sample start/stop:
 * static void function_key_update(void)
 * {
 *     if (Key_Check(sample_s, KEY_DOWN) != 0U) {
 *         if (sampling_flag == 0U) {
 *             overlimit_flag = 0;
 *             sampling_flag = 1;
 *             adc_sample_start = 0;
 *             append_normal_log("Periodic Sampling START(Key)");
 *         } else {
 *             sampling_flag = 0;
 *             hide_flag = 0;
 *             overlimit_flag = 0;
 *             oled_idle_time = 10;
 *             append_normal_log("Periodic Sampling STOP(Key)");
 *         }
 *     }
 * }
 *
 * DAC test:
 * volatile uint8_t dac_test_flag = 0;
 * volatile uint16_t dac_test_count = 0;
 * uint8_t dac_test_flag6 = 0;
 * uint8_t dac_test_flag3 = 0;
 * uint8_t dac_test_flag0 = 0;
 *
 * TIM6 used to count:
 *     if (dac_test_count > 0) {
 *         dac_test_count--;
 *         if (dac_test_count == 6000) dac_test_flag6 = 1;
 *         if (dac_test_count == 3000) dac_test_flag3 = 1;
 *         if (dac_test_count == 0)    dac_test_flag0 = 1;
 *     }
 *
 * void dac_test_tick(void)
 * {
 *     if (dac_test_flag == 1) {
 *         OLED_Printf(0,0,16,"DAC Test     ");
 *         OLED_Printf(0,16,16,"DAC:1.0V ");
 *         OLED_Refresh();
 *         printf("\r\nDAC Test Start\r\n");
 *         DAC_SetVoltage(0, 1.0f);
 *         DAC_SetVoltage(1, 1.0f);
 *         dac_test_flag = 0;
 *     }
 *     if (dac_test_flag6 == 1) {
 *         OLED_Printf(0,16,16,"DAC:1.5V ");
 *         OLED_Refresh();
 *         DAC_SetVoltage(0, 1.5f);
 *         DAC_SetVoltage(1, 1.5f);
 *         dac_test_flag6 = 0;
 *     }
 *     if (dac_test_flag3 == 1) {
 *         OLED_Printf(0,16,16,"DAC:2.0V ");
 *         OLED_Refresh();
 *         DAC_SetVoltage(0, 2.0f);
 *         DAC_SetVoltage(1, 2.0f);
 *         dac_test_flag3 = 0;
 *     }
 *     if (dac_test_flag0 == 1) {
 *         OLED_Clear();
 *         OLED_Printf(0,0,16,"system idle");
 *         OLED_Refresh();
 *         DAC_SetVoltage(0, dac_volt);
 *         DAC_SetVoltage(1, dac_volt);
 *         dac_test_flag0 = 0;
 *     }
 * }
 */
