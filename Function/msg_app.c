#include "msg_app.h"

typedef msg_result_t (*msg_handler_t)(msg_frame_t *frame);

typedef struct
{
    uint16_t cmd;
    msg_handler_t handler;
} msg_cmd_entry_t;

static uint8_t msg_reboot_pending = 0U;
static uint8_t msg_auto_report_flag = 0U;
static uint8_t msg_adc_boot_ready = 0U;
static uint8_t msg_alarm_count = 0U;
static char msg_alarm_channel[2][4] = {0};
static float msg_alarm_value[2] = {0.0f};
static float msg_alarm_limit[2] = {0.0f};
static uint32_t msg_auto_report_start = 0U;
static uint8_t msg_sleep_pending = 0U;

////////////////////////////函数声明//////////////////////////////
static void msg_set_time_by_unix(uint32_t timestamp);
static msg_result_t msg_cmd_reboot(msg_frame_t *frame);
static msg_result_t msg_cmd_version(msg_frame_t *frame);
static msg_result_t msg_cmd_set_time(msg_frame_t *frame);
static msg_result_t msg_cmd_get_time(msg_frame_t *frame);
static msg_result_t msg_cmd_get_id(msg_frame_t *frame);
static msg_result_t msg_cmd_get_baud(msg_frame_t *frame);
static msg_result_t msg_cmd_set_id(msg_frame_t *frame);
static msg_result_t msg_cmd_set_baud(msg_frame_t *frame);

static void msg_wait_adc_boot_ready(void);
static void msg_alarm_check(const char *channel, float value, float limit);
static msg_result_t msg_build_auto_report(void);
static msg_result_t msg_cmd_set_dac(msg_frame_t *frame);
static msg_result_t msg_cmd_auto_start(msg_frame_t *frame);
static msg_result_t msg_cmd_auto_stop(msg_frame_t *frame);
static msg_result_t msg_cmd_get_ch0(msg_frame_t *frame);
static msg_result_t msg_cmd_get_ch1(msg_frame_t *frame);
static msg_result_t msg_cmd_get_ch2(msg_frame_t *frame);
static msg_result_t msg_cmd_set_ratio_ch0(msg_frame_t *frame);
static msg_result_t msg_cmd_set_ratio_ch1(msg_frame_t *frame);
static msg_result_t msg_cmd_set_sample_cycle(msg_frame_t *frame);

static msg_result_t msg_cmd_sleep(msg_frame_t *frame);
static msg_result_t msg_cmd_get_limits(msg_frame_t *frame);
static msg_result_t msg_cmd_get_limit0(msg_frame_t *frame);
static msg_result_t msg_cmd_get_limit1(msg_frame_t *frame);
static msg_result_t msg_cmd_set_limit0(msg_frame_t *frame);
static msg_result_t msg_cmd_set_limit1(msg_frame_t *frame);
static msg_result_t msg_cmd_set_alarm_report(msg_frame_t *frame);
static msg_result_t msg_cmd_get_alarm_logs(msg_frame_t *frame);
static msg_result_t msg_cmd_clear_alarm_logs(msg_frame_t *frame);
static msg_result_t msg_cmd_enter_upgrade(msg_frame_t *frame);

//协议命令表
static const msg_cmd_entry_t msg_cmd_table[] =
{
    {0x0101U, msg_cmd_reboot},
    {0x0104U, msg_cmd_version},
    {0x0105U, msg_cmd_set_time},
    {0x0106U, msg_cmd_get_time},
    {0x01A1U, msg_cmd_set_id},
    {0x01A2U, msg_cmd_set_baud},
    {0x0111U, msg_cmd_get_id},
    {0x0112U, msg_cmd_get_baud},
    {0x0201U, msg_cmd_get_ch0},
    {0x0202U, msg_cmd_get_ch1},
    {0x0221U, msg_cmd_get_ch2},
    {0x0241U, msg_cmd_set_ratio_ch0},
    {0x0242U, msg_cmd_set_ratio_ch1},
    {0x0261U, msg_cmd_set_sample_cycle},
    {0x0301U, msg_cmd_set_dac},
    {0x0302U, msg_cmd_auto_start},
    {0x0303U, msg_cmd_auto_stop},
    {0x03AAU, msg_cmd_sleep},
    {0x0400U, msg_cmd_get_limits},
    {0x0401U, msg_cmd_get_limit0},
    {0x0402U, msg_cmd_get_limit1},
    {0x0411U, msg_cmd_set_limit0},
    {0x0412U, msg_cmd_set_limit1},
    {0x0501U, msg_cmd_enter_upgrade},
    {0x0601U, msg_cmd_set_alarm_report},
    {0x0602U, msg_cmd_get_alarm_logs},
    {0x0603U, msg_cmd_clear_alarm_logs},
};

static void msg_set_time_by_unix(uint32_t timestamp)
{
    uint32_t rem;
    uint32_t h;
    uint32_t m;
    uint32_t s;
    uint32_t days;
    uint32_t era;
    uint32_t doe;
    uint32_t yoe;
    uint32_t year;
    uint32_t doy;
    uint32_t mp;
    uint32_t day;
    uint32_t month;

    timestamp += 28800U;
    rem = timestamp % 86400U;
    h = rem / 3600U;
    m = (rem % 3600U) / 60U;
    s = rem % 60U;

    days = timestamp / 86400U + 719468U;
    era = days / 146097U;
    doe = days - era * 146097U;
    yoe = (doe - doe / 1460U + doe / 36524U - doe / 146096U) / 365U;
    year = yoe + era * 400U;
    doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
    mp = (5U * doy + 2U) / 153U;
    day = doy - (153U * mp + 2U) / 5U + 1U;
    if (mp < 10U) {
        month = mp + 3U;
    } else {
        month = mp - 9U;
    }
    year += (month <= 2U);

    rtc_setup((uint16_t)year, (uint8_t)month, (uint8_t)day,
              (uint8_t)h, (uint8_t)m, (uint8_t)s);
}

static msg_result_t msg_cmd_reboot(msg_frame_t *frame)
{
    msg_reboot_pending = 1U;
    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_version(msg_frame_t *frame)
{
    uint8_t payload[4];

    payload[0] = 0x02U;
    payload[1] = 0x00U;
    payload[2] = 0x01U;
    payload[3] = 0x00U;
    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, frame->cmd, payload, 4U);
}

static msg_result_t msg_cmd_set_time(msg_frame_t *frame)
{
    if (frame->length != 4U) {
        return msg_build_error();
    }
    msg_set_time_by_unix(msg_read_u32(frame->payload));
    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_get_time(msg_frame_t *frame)
{
    uint8_t payload[4];

    msg_write_u32(payload, get_unix_time());
    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, frame->cmd, payload, 4U);
}

static msg_result_t msg_cmd_set_id(msg_frame_t *frame)
{
    uint16_t new_id;
    data_cfg_t cfg;

    if (frame->length != 2U) {
        return msg_build_error();
    }

    new_id = msg_read_u16(frame->payload);
    if ((new_id == 0U) || (new_id == MSG_BROADCAST_ID)) {
        return msg_build_error();
    }

    msg_device_id = new_id;
    get_data_config(&cfg);
    cfg.device_id = msg_device_id;
    set_data_config(&cfg);

    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_get_id(msg_frame_t *frame)
{
    uint8_t payload[2];

    msg_write_u16(payload, msg_device_id);
    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, frame->cmd, payload, 2U);
}

static msg_result_t msg_cmd_get_baud(msg_frame_t *frame)
{
    uint8_t payload[1];

    payload[0] = usart1_baud_mode;
    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, frame->cmd, payload, 1U);
}

static msg_result_t msg_cmd_set_baud(msg_frame_t *frame)
{
    data_cfg_t cfg;
    uint8_t baud_mode;

    if (frame->length != 1U) {
        return msg_build_error();
    }

    baud_mode = frame->payload[0];
    if ((baud_mode < 0x11U) || (baud_mode > MSG_BAUD_115200)) {
        return msg_build_error();
    }

    usart1_baud_mode = baud_mode;
    get_data_config(&cfg);
    cfg.baud_mode = usart1_baud_mode;
    set_data_config(&cfg);

    msg_reboot_pending = 1U;
    return msg_build_ok(frame->cmd);
}

msg_result_t msg_app_handle_cmd(msg_frame_t *frame)
{
    uint16_t i;

    if ((frame->type == MSG_TYPE_HEART) && (frame->cmd == MSG_CMD_HEART_FIND) &&
        (frame->device_id == MSG_BROADCAST_ID)) {
        return msg_build_frame(msg_device_id, MSG_TYPE_HEART, MSG_CMD_HEART_BEAT, NULL, 0U);
    }

    if (frame->type != MSG_TYPE_CMD) {
        return msg_build_error();
    }

    for (i = 0U; i < (sizeof(msg_cmd_table) / sizeof(msg_cmd_table[0])); i++) {
        if (frame->cmd == msg_cmd_table[i].cmd) {
            return msg_cmd_table[i].handler(frame);
        }
    }

    return msg_build_error();
}

void msg_app_after_send(void)
{
    if (msg_reboot_pending != 0U) {
        delay_1ms(80U);
        NVIC_SystemReset();
    }

    if (msg_sleep_pending != 0U) {
        msg_sleep_pending = 0U;
        delay_1ms(20U);
        function_idle_refresh_request();
        oled_idle_refresh();
        RTC_SetWakeup(10U);
        pmu_flag_clear(PMU_FLAG_RESET_WAKEUP);
        pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);
        SystemInit();
        USART1_Init();
        printf("instrument wakeup\r\n");
        function_idle_refresh_request();
        oled_idle_refresh();
    }
}

////////////////////////////采样和告警//////////////////////////////
static void msg_wait_adc_boot_ready(void)
{
    if (msg_adc_boot_ready != 0U) {
        return;
    }

    delay_1ms(20U);
    msg_adc_boot_ready = 1U;
}

static void msg_alarm_check(const char *channel, float value, float limit)
{
    if (value > limit) {
        append_over_log_ch(channel, value, limit);
        if ((alarm_report_mode == 1U) && (msg_alarm_count < 2U)) {
            strncpy(msg_alarm_channel[msg_alarm_count], channel, sizeof(msg_alarm_channel[msg_alarm_count]) - 1);
            msg_alarm_channel[msg_alarm_count][sizeof(msg_alarm_channel[msg_alarm_count]) - 1] = '\0';
            msg_alarm_value[msg_alarm_count] = value;
            msg_alarm_limit[msg_alarm_count] = limit;
            msg_alarm_count++;
        }
    }
}

static msg_result_t msg_build_auto_report(void)
{
    uint8_t payload[12];
    float ch0_value;
    float ch1_value;

    msg_wait_adc_boot_ready();
    ch0_value = data_adc_raw_to_volt(ADC_get()) * ratio_ch0;
    ch1_value = data_adc_raw_to_volt(ADC_get_ch1()) * ratio_ch1;
    msg_alarm_check("CH0", ch0_value, limit_ch0);
    msg_alarm_check("CH1", ch1_value, limit_ch1);
    overlimit_flag = ((ch0_value > limit_ch0) || (ch1_value > limit_ch1)) ? 1U : 0U;

    msg_write_u32(&payload[0], get_unix_time());
    msg_write_float(&payload[4], ch0_value);
    msg_write_float(&payload[8], ch1_value);

    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, 0x0302U, payload, 12U);
}

////////////////////////////采样命令//////////////////////////////
static msg_result_t msg_cmd_set_dac(msg_frame_t *frame)
{
    uint16_t dac_raw;
    data_cfg_t cfg;

    if (frame->length != 2U) {
        return msg_build_error();
    }

    dac_raw = msg_read_u16(frame->payload);
    if (dac_raw > 4095U) {
        return msg_build_error();
    }

    dac_volt = data_adc_raw_to_volt(dac_raw);
    DAC_Set(DAC_OUT0, dac_raw);
    DAC_Set(DAC_OUT1, dac_raw);

    get_data_config(&cfg);
    cfg.dac_volt = dac_volt;
    set_data_config(&cfg);

    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_auto_start(msg_frame_t *frame)
{
    if (frame->length != 0U) {
        return msg_build_error();
    }

    msg_auto_report_flag = 1U;
    function_sample_state_set(1U);
    overlimit_flag = 0U;
    msg_auto_report_start = Gettim6Time();

    return msg_build_auto_report();
}

static msg_result_t msg_cmd_auto_stop(msg_frame_t *frame)
{
    if (frame->length != 0U) {
        return msg_build_error();
    }

    msg_auto_report_flag = 0U;
    function_sample_state_set(0U);
    overlimit_flag = 0U;
    msg_auto_report_start = 0U;

    oled_idle_time = 10;
    function_idle_refresh_request();

    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_get_ch0(msg_frame_t *frame)
{
    uint8_t payload[4];
    float ch_value;

    msg_wait_adc_boot_ready();
    ch_value = data_adc_raw_to_volt(ADC_get()) * ratio_ch0;
    msg_alarm_check("CH0", ch_value, limit_ch0);
    msg_write_float(payload, ch_value);
    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, frame->cmd, payload, 4U);
}

static msg_result_t msg_cmd_get_ch1(msg_frame_t *frame)
{
    uint8_t payload[4];
    float ch_value;

    msg_wait_adc_boot_ready();
    ch_value = data_adc_raw_to_volt(ADC_get_ch1()) * ratio_ch1;
    msg_alarm_check("CH1", ch_value, limit_ch1);
    msg_write_float(payload, ch_value);
    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, frame->cmd, payload, 4U);
}

static msg_result_t msg_cmd_get_ch2(msg_frame_t *frame)
{
    uint8_t payload[4];
    float pt_res;
    float pt_temp;

    if (PT100_Read(&pt_res, &pt_temp) == 0U) {
        return msg_build_error();
    }
    msg_write_float(payload, pt_temp);
    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, frame->cmd, payload, 4U);
}

static msg_result_t msg_cmd_set_ratio_ch0(msg_frame_t *frame)
{
    data_cfg_t cfg;
    float value;

    if (frame->length != 4U) {
        return msg_build_error();
    }

    value = msg_read_float(frame->payload);
    if ((value < 0.0f) || (value > 100.0f)) {
        return msg_build_error();
    }

    ratio_ch0 = value;
    get_data_config(&cfg);
    cfg.ratio_ch0 = ratio_ch0;
    set_data_config(&cfg);

    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_set_ratio_ch1(msg_frame_t *frame)
{
    data_cfg_t cfg;
    float value;

    if (frame->length != 4U) {
        return msg_build_error();
    }

    value = msg_read_float(frame->payload);
    if ((value < 0.0f) || (value > 100.0f)) {
        return msg_build_error();
    }

    ratio_ch1 = value;
    get_data_config(&cfg);
    cfg.ratio_ch1 = ratio_ch1;
    set_data_config(&cfg);

    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_set_sample_cycle(msg_frame_t *frame)
{
    data_cfg_t cfg;
    uint32_t cycle;

    if (frame->length != 1U) {
        return msg_build_error();
    }

    switch (frame->payload[0]) {
        case 0x01U: cycle = 1000U;  break;
        case 0x02U: cycle = 3000U;  break;
        case 0x03U: cycle = 5000U;  break;
        default:    return msg_build_error();
    }

    adc_sample_cycle = cycle;
    get_data_config(&cfg);
    cfg.sample_cycle = adc_sample_cycle;
    set_data_config(&cfg);

    if (msg_auto_report_flag != 0U) {
        msg_auto_report_start = Gettim6Time();
    }

    return msg_build_ok(frame->cmd);
}

////////////////////////////外部调用//////////////////////////////
void msg_app_auto_report_poll(void)
{
    if (msg_auto_report_flag == 0U) {
        return;
    }

    if (tim6_timeoutcheck(&msg_auto_report_start, adc_sample_cycle) != 0U) {
        if (msg_build_auto_report() == MSG_OK) {
            msg_send_current();
            msg_app_alarm_print();
        }
    }
}

uint8_t msg_app_auto_report_is_active(void)
{
    return msg_auto_report_flag;
}

void msg_app_alarm_print(void)
{
    uint8_t i;

    for (i = 0U; i < msg_alarm_count; i++) {
        printf("%s | %s | %.2f | %.2f\r\n",
               unix_to_str(get_unix_time()), msg_alarm_channel[i], msg_alarm_limit[i], msg_alarm_value[i]);
    }
    msg_alarm_count = 0U;
}

void msg_app_reset_alarm_state(void)
{
    msg_alarm_count = 0U;
}

////////////////////////////其他命令//////////////////////////////
static msg_result_t msg_cmd_sleep(msg_frame_t *frame)
{
    if (frame->length != 0U)
    {
        return msg_build_error();
    }
    msg_auto_report_flag = 0U;
    function_sample_state_set(0U);
    msg_sleep_pending = 1U;

    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_get_limits(msg_frame_t *frame)
{
    uint8_t payload[8];

    msg_write_float(&payload[0], limit_ch0);
    msg_write_float(&payload[4], limit_ch1);
    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, frame->cmd, payload, 8U);
}

static msg_result_t msg_cmd_get_limit0(msg_frame_t *frame)
{
    uint8_t payload[4];

    msg_write_float(payload, limit_ch0);
    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, frame->cmd, payload, 4U);
}

static msg_result_t msg_cmd_get_limit1(msg_frame_t *frame)
{
    uint8_t payload[4];

    msg_write_float(payload, limit_ch1);
    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, frame->cmd, payload, 4U);
}

static msg_result_t msg_cmd_set_limit0(msg_frame_t *frame)
{
    data_cfg_t cfg;
    float value;

    if (frame->length != 4U) {
        return msg_build_error();
    }

    value = msg_read_float(frame->payload);
    if ((value < 0.0f) || (value > 500.0f)) {
        return msg_build_error();
    }

    limit_ch0 = value;
    get_data_config(&cfg);
    cfg.limit_ch0 = limit_ch0;
    set_data_config(&cfg);

    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_set_limit1(msg_frame_t *frame)
{
    data_cfg_t cfg;
    float value;

    if (frame->length != 4U) {
        return msg_build_error();
    }

    value = msg_read_float(frame->payload);
    if ((value < 0.0f) || (value > 500.0f)) {
        return msg_build_error();
    }

    limit_ch1 = value;
    get_data_config(&cfg);
    cfg.limit_ch1 = limit_ch1;
    set_data_config(&cfg);

    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_set_alarm_report(msg_frame_t *frame)
{
    data_cfg_t cfg;
    uint8_t mode;

    if (frame->length != 1U) {
        return msg_build_error();
    }

    mode = frame->payload[0];
    if ((mode != 1U) && (mode != 2U)) {
        return msg_build_error();
    }

    alarm_report_mode = mode;
    get_data_config(&cfg);
    cfg.alarm_report_mode = alarm_report_mode;
    set_data_config(&cfg);

    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_get_alarm_logs(msg_frame_t *frame)
{
    static uint8_t empty_str[] = "empty\r\n";

    if (frame->length != 0U) {
        return msg_build_error();
    }

    msg_send_string(empty_str, 7U);
    return MSG_OK;
}

static msg_result_t msg_cmd_clear_alarm_logs(msg_frame_t *frame)
{
    if (frame->length != 0U) {
        return msg_build_error();
    }

    clear_all_over_logs();
    msg_app_reset_alarm_state();
    overlimit_flag = 0U;
    delay_1ms(100U);
    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_enter_upgrade(msg_frame_t *frame)
{
    msg_result_t ret;

    if (frame->length != 0U) {
        return msg_build_error();
    }

    /* 1. Build OK response */
    ret = msg_build_ok(frame->cmd);
    if (ret != MSG_OK) {
        return ret;
    }

    /* 2. Send response immediately (synchronous, waits DMA+TC) */
    msg_send_current();

    /* 3. Allow RS485 transceiver and peer to settle */
    delay_1ms(50);

    /* 4. Write boot_flag = 0xA5 to parameter area (FMC page erase + program) */
    boot_param_set_flag(BOOT_FLAG_UPDATE);

    /* 5. Soft reset 鈫?Bootloader sees boot_flag=0xA5 鈫?upgrade console */
    delay_1ms(10);
    NVIC_SystemReset();

    /* Never reached */
    return MSG_OK;
}
