#include "msg.h"

uint8_t msg_tx_buffer[MSG_TX_BUF_LEN] = {0};
uint16_t msg_tx_len = 0U;
uint16_t msg_device_id = 0x0001U;
uint8_t msg_auto_sample_flag = 0U;

static uint8_t msg_raw_buffer[MSG_RAW_BUF_LEN] = {0};
static uint8_t msg_reboot_pending = 0U;
static uint8_t msg_sleep_pending = 0U;
static uint8_t msg_boot_heartbeat_sent = 0U;
static uint8_t msg_auto_report_flag = 0U;
static uint8_t msg_alarm_count = 0U;
static char msg_alarm_channel[2][4] = {0};
static float msg_alarm_value[2] = {0.0f};
static float msg_alarm_limit[2] = {0.0f};
static uint32_t msg_auto_report_start = 0U;

#define MSG_CMD_HEART_FIND   0xFFFFU
#define MSG_CMD_HEART_BEAT   0x8888U
#define MSG_CMD_ERR          0xEEEEU

typedef msg_result_t (*msg_handler_t)(msg_frame_t *frame);

typedef struct
{
    uint16_t cmd;
    msg_handler_t handler;
} msg_cmd_entry_t;

static msg_result_t msg_cmd_reboot(msg_frame_t *frame);
static msg_result_t msg_cmd_version(msg_frame_t *frame);
static msg_result_t msg_cmd_set_time(msg_frame_t *frame);
static msg_result_t msg_cmd_get_time(msg_frame_t *frame);
static msg_result_t msg_cmd_get_id(msg_frame_t *frame);
static msg_result_t msg_cmd_get_baud(msg_frame_t *frame);
static msg_result_t msg_cmd_set_id(msg_frame_t *frame);
static msg_result_t msg_cmd_set_baud(msg_frame_t *frame);
static msg_result_t msg_cmd_set_dac(msg_frame_t *frame);
static msg_result_t msg_cmd_auto_start(msg_frame_t *frame);
static msg_result_t msg_cmd_auto_stop(msg_frame_t *frame);
static msg_result_t msg_cmd_sleep(msg_frame_t *frame);
static msg_result_t msg_cmd_get_ch0(msg_frame_t *frame);
static msg_result_t msg_cmd_get_ch1(msg_frame_t *frame);
static msg_result_t msg_cmd_get_ch2(msg_frame_t *frame);
static msg_result_t msg_cmd_set_ratio_ch0(msg_frame_t *frame);
static msg_result_t msg_cmd_set_ratio_ch1(msg_frame_t *frame);
static msg_result_t msg_cmd_set_sample_cycle(msg_frame_t *frame);
static msg_result_t msg_cmd_get_limits(msg_frame_t *frame);
static msg_result_t msg_cmd_get_limit0(msg_frame_t *frame);
static msg_result_t msg_cmd_get_limit1(msg_frame_t *frame);
static msg_result_t msg_cmd_set_limit0(msg_frame_t *frame);
static msg_result_t msg_cmd_set_limit1(msg_frame_t *frame);
static msg_result_t msg_cmd_set_alarm_report(msg_frame_t *frame);
static msg_result_t msg_cmd_get_alarm_logs(msg_frame_t *frame);
static msg_result_t msg_cmd_clear_alarm_logs(msg_frame_t *frame);

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
    {0x0601U, msg_cmd_set_alarm_report},
    {0x0602U, msg_cmd_get_alarm_logs},
    {0x0603U, msg_cmd_clear_alarm_logs},
};

/*------------------ CRC16 ------------------*/
// 计算16位CRC校验码 (MODBUS)
uint16_t Calculate_CRC16(uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFFU;
    uint8_t i;

    if (data == NULL) {
        return 0U;
    }

    while (length-- > 0U) {
        crc ^= *data++;
        for (i = 0U; i < 8U; i++) {
            if ((crc & 0x0001U) != 0U) {
                crc = (uint16_t)((crc >> 1) ^ 0xA001U);
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/*------------------ 基础读写 ------------------*/
// 读取2字节为u16 (高位在前)
static uint16_t msg_read_u16(const uint8_t *buf)
{
    return (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
}

// 写入u16 (高位在前)
static void msg_write_u16(uint8_t *buf, uint16_t value)
{
    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)(value & 0xFFU);
}

// 写入u32 (高位在前)
static void msg_write_u32(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)(value >> 24);
    buf[1] = (uint8_t)(value >> 16);
    buf[2] = (uint8_t)(value >> 8);
    buf[3] = (uint8_t)(value & 0xFFU);
}

static uint32_t msg_read_u32(const uint8_t *buf)
{
    return (((uint32_t)buf[0] << 24) |
            ((uint32_t)buf[1] << 16) |
            ((uint32_t)buf[2] << 8) |
            (uint32_t)buf[3]);
}

static void msg_write_float(uint8_t *buf, float value)
{
    union {
        float f;
        uint32_t u;
    } data;

    data.f = value;
    msg_write_u32(buf, data.u);
}

static float msg_read_float(const uint8_t *buf)
{
    union {
        float f;
        uint32_t u;
    } data;

    data.u = msg_read_u32(buf);
    return data.f;
}

// ASCII字符转半字节
static uint8_t msg_hex_to_nibble(uint8_t ch, uint8_t *value)
{
    if ((ch >= '0') && (ch <= '9')) {
        *value = (uint8_t)(ch - '0');
        return 1U;
    }
    if ((ch >= 'A') && (ch <= 'F')) {
        *value = (uint8_t)(ch - 'A' + 10U);
        return 1U;
    }
    if ((ch >= 'a') && (ch <= 'f')) {
        *value = (uint8_t)(ch - 'a' + 10U);
        return 1U;
    }
    return 0U;
}

// 半字节转ASCII字符
static uint8_t msg_nibble_to_hex(uint8_t value)
{
    value &= 0x0FU;
    return (value < 10U) ? (uint8_t)('0' + value) : (uint8_t)('A' + value - 10U);
}

/*------------------ ASCII HEX转换 ------------------*/
// 检查是否以ASCII帧头 A5B6 开始
static uint8_t msg_ascii_start_is_frame(uint8_t *buf, uint16_t len)
{
    uint16_t i = 0U;

    while ((i < len) && isspace((unsigned char)buf[i])) {
        i++;
    }

    if ((uint16_t)(len - i) < 4U) {
        return 0U;
    }

    return ((buf[i] == 'A') || (buf[i] == 'a')) &&
           (buf[i + 1U] == '5') &&
           ((buf[i + 2U] == 'B') || (buf[i + 2U] == 'b')) &&
           (buf[i + 3U] == '6');
}

// ASCII格式转原始二进制
static msg_result_t msg_ascii_to_raw(uint8_t *ascii, uint16_t ascii_len,
                                     uint8_t *raw, uint16_t *raw_len)
{
    uint16_t i;
    uint16_t count = 0U;
    uint8_t high;
    uint8_t low;

    if ((ascii == NULL) || (raw == NULL) || (raw_len == NULL)) {
        return MSG_ERR_NULL;
    }

    for (i = 0U; i < ascii_len; i++) {
        if (isspace((unsigned char)ascii[i])) {
            continue;
        }
        if (msg_hex_to_nibble(ascii[i], &high) == 0U) {
            return MSG_ERR_FORMAT;
        }

        i++;
        while ((i < ascii_len) && isspace((unsigned char)ascii[i])) {
            i++;
        }
        if ((i >= ascii_len) || (msg_hex_to_nibble(ascii[i], &low) == 0U)) {
            return MSG_ERR_FORMAT;
        }
        if (count >= MSG_RAW_BUF_LEN) {
            return MSG_ERR_BUF;
        }
        raw[count++] = (uint8_t)((high << 4) | low);
    }

    *raw_len = count;
    return MSG_OK;
}

// 原始二进制转ASCII
static uint16_t msg_raw_to_ascii(uint8_t *raw, uint16_t raw_len,
                                 uint8_t *ascii, uint16_t ascii_len)
{
    uint16_t i;

    if ((raw == NULL) || (ascii == NULL) || (ascii_len < (uint16_t)(raw_len * 2U))) {
        return 0U;
    }

    for (i = 0U; i < raw_len; i++) {
        ascii[i * 2U] = msg_nibble_to_hex((uint8_t)(raw[i] >> 4));
        ascii[i * 2U + 1U] = msg_nibble_to_hex(raw[i]);
    }

    return (uint16_t)(raw_len * 2U);
}

/*------------------ 报文解析 ------------------*/
// 解析并校验报文
static msg_result_t msg_parse_raw(uint8_t *raw, uint16_t raw_len, msg_frame_t *frame)
{
    uint16_t calc_crc;
    uint16_t crc_pos;

    if ((raw == NULL) || (frame == NULL)) {
        return MSG_ERR_NULL;
    }
    if (raw_len < MSG_MIN_RAW_LEN) {
        return MSG_ERR_TOO_SHORT;
    }
    if ((msg_read_u16(&raw[0]) != MSG_START_FLAG) ||
        (msg_read_u16(&raw[raw_len - 2U]) != MSG_END_FLAG)) {
        return MSG_ERR_FORMAT;
    }

    frame->device_id = msg_read_u16(&raw[2]);
    frame->type = raw[4];
    frame->cmd = msg_read_u16(&raw[5]);
    frame->length = raw[7];
    frame->version = raw[8];

    // 长度校验
    if (raw_len != (uint16_t)(MSG_MIN_RAW_LEN + frame->length)) {
        return MSG_ERR_LENGTH;
    }

    crc_pos = (uint16_t)(9U + frame->length);
    frame->payload = &raw[9];
    frame->crc = msg_read_u16(&raw[crc_pos]);

    calc_crc = Calculate_CRC16(raw, crc_pos);
    if (calc_crc != frame->crc) {
        return MSG_ERR_CRC;
    }
    if (frame->version != MSG_PROTO_VER) {
        return MSG_ERR_TYPE;
    }

    return MSG_OK;
}

// 地址匹配
static uint8_t msg_addr_match(uint16_t device_id)
{
    return (device_id == msg_device_id) || (device_id == MSG_BROADCAST_ID);
}

/*------------------ 报文组装 ------------------*/
static msg_result_t msg_build_frame(uint16_t device_id, uint8_t type, uint16_t cmd,
                                    uint8_t *payload, uint8_t payload_len)
{
    uint8_t raw[MSG_RAW_BUF_LEN];
    uint16_t raw_len;
    uint16_t crc_pos;
    uint16_t crc;

    raw_len = (uint16_t)(MSG_MIN_RAW_LEN + payload_len);
    if ((raw_len > sizeof(raw)) || ((uint16_t)(raw_len * 2U) > MSG_TX_BUF_LEN)) {
        return MSG_ERR_BUF;
    }

    msg_write_u16(&raw[0], MSG_START_FLAG);
    msg_write_u16(&raw[2], device_id);
    raw[4] = type;
    msg_write_u16(&raw[5], cmd);
    raw[7] = payload_len;
    raw[8] = MSG_PROTO_VER;

    if ((payload != NULL) && (payload_len > 0U)) {
        memcpy(&raw[9], payload, payload_len);
    }

    crc_pos = (uint16_t)(9U + payload_len);
    crc = Calculate_CRC16(raw, crc_pos);
    msg_write_u16(&raw[crc_pos], crc);
    msg_write_u16(&raw[crc_pos + 2U], MSG_END_FLAG);

    msg_tx_len = msg_raw_to_ascii(raw, raw_len, msg_tx_buffer, MSG_TX_BUF_LEN);
    return (msg_tx_len > 0U) ? MSG_OK : MSG_ERR_BUF;
}

/*------------------ 应答 ------------------*/
static msg_result_t msg_build_ok(uint16_t cmd)
{
    uint8_t payload = MSG_OK_BYTE;
    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, cmd, &payload, 1U);
}

static msg_result_t msg_build_error(void)
{
    return msg_build_frame(msg_device_id, MSG_TYPE_ERR, MSG_CMD_ERR, NULL, 0U);
}

static float msg_adc_raw_to_volt(uint16_t raw)
{
    return ((float)raw * 3.3f) / 4095.0f;
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

static void msg_alarm_print(void)
{
    uint8_t i;

    for (i = 0U; i < msg_alarm_count; i++) {
        printf("%s | %s | %.2f | %.2f\r\n",
               unix_to_str(get_unix_time()), msg_alarm_channel[i], msg_alarm_limit[i], msg_alarm_value[i]);
    }
    msg_alarm_count = 0U;
}

static msg_result_t msg_build_auto_report(void)
{
    uint8_t payload[12];
    float ch0_value;
    float ch1_value;

    ch0_value = msg_adc_raw_to_volt(ADC_get()) * ratio_ch0;
    ch1_value = msg_adc_raw_to_volt(ADC_get_ch1()) * ratio_ch1;
    msg_alarm_check("CH0", ch0_value, limit_ch0);
    msg_alarm_check("CH1", ch1_value, limit_ch1);
    overlimit_flag = ((ch0_value > limit_ch0) || (ch1_value > limit_ch1)) ? 1U : 0U;

    msg_write_u32(&payload[0], get_unix_time());
    msg_write_float(&payload[4], ch0_value);
    msg_write_float(&payload[8], ch1_value);

    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, 0x0302U, payload, 12U);
}

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

void msg_send_heartbeat(void)
{
    if (msg_build_frame(msg_device_id, MSG_TYPE_HEART, MSG_CMD_HEART_BEAT, NULL, 0U) == MSG_OK) {
        USART1_SendData(msg_tx_buffer, msg_tx_len);
    }
}

static void msg_auto_report_poll(void)
{
    if (msg_auto_report_flag == 0U) {
        return;
    }

    if (tim6_timeoutcheck(&msg_auto_report_start, adc_sample_cycle) != 0U) {
        if (msg_build_auto_report() == MSG_OK) {
            USART1_SendData(msg_tx_buffer, msg_tx_len);
            msg_alarm_print();
        }
    }
}

/*------------------ 命令处理 ------------------*/
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

    dac_volt = msg_adc_raw_to_volt(dac_raw);
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
    msg_auto_sample_flag = 1U;
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
    msg_auto_sample_flag = 0U;
    overlimit_flag = 0U;
    msg_auto_report_start = 0U;

    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_sleep(msg_frame_t *frame)
{
    /*
    if (frame->length != 0U) 
    {
        return msg_build_error();
    }
*/
    msg_auto_report_flag = 0U;
    msg_auto_sample_flag = 0U;
    msg_sleep_pending = 1U;

    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_cmd_get_ch0(msg_frame_t *frame)
{
    uint8_t payload[4];
    float ch_value;

    ch_value = msg_adc_raw_to_volt(ADC_get()) * ratio_ch0;
    msg_alarm_check("CH0", ch_value, limit_ch0);
    msg_write_float(payload, ch_value);
    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, frame->cmd, payload, 4U);
}

static msg_result_t msg_cmd_get_ch1(msg_frame_t *frame)
{
    uint8_t payload[4];
    float ch_value;

    ch_value = msg_adc_raw_to_volt(ADC_get_ch1()) * ratio_ch1;
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

    if (frame->length != 4U) {
        return msg_build_error();
    }

    cycle = msg_read_u32(frame->payload);
    if ((cycle < 1000U) || (cycle > 600000U)) {
        return msg_build_error();
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
    if (frame->length != 0U) {
        return msg_build_error();
    }

    print_latest_over_logs(10);
    return MSG_OK;
}

static msg_result_t msg_cmd_clear_alarm_logs(msg_frame_t *frame)
{
    if (frame->length != 0U) {
        return msg_build_error();
    }

    clear_all_over_logs();
    return msg_build_ok(frame->cmd);
}

static msg_result_t msg_handle_cmd(msg_frame_t *frame)
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

//外部调用接口
uint8_t msg_poll(void)
{
    msg_frame_t frame;
    uint16_t raw_len = 0U;
    msg_result_t result;
    uint16_t target_id = MSG_BROADCAST_ID;

    if (msg_boot_heartbeat_sent == 0U) {
        msg_boot_heartbeat_sent = 1U;
        msg_send_heartbeat();
    }

    if (usart1_rx_flag == 0U) {
        msg_auto_report_poll();
        return 0U;
    }
    if (msg_ascii_start_is_frame(usart1_rx_buffer, usart1_rx_len) == 0U) {
        if (msg_auto_report_flag != 0U) {
            USART1_ClearRxBuf();
            return 1U;
        }
        return 0U;
    }

    msg_tx_len = 0U;
    result = msg_ascii_to_raw(usart1_rx_buffer, usart1_rx_len, msg_raw_buffer, &raw_len);

    if ((result == MSG_OK) && (raw_len >= 4U)) {
        target_id = msg_read_u16(&msg_raw_buffer[2]);
    } else if (raw_len >= 4U) {
        target_id = msg_read_u16(&msg_raw_buffer[2]);
    }

    if ((raw_len >= 4U) && (msg_addr_match(target_id) == 0U)) {
        USART1_ClearRxBuf();
        return 1U;
    }

    if (result == MSG_OK) {
        result = msg_parse_raw(msg_raw_buffer, raw_len, &frame);
    }

    if ((result == MSG_OK) && (msg_auto_report_flag != 0U) && (frame.cmd != 0x0303U)) {
        USART1_ClearRxBuf();
        return 1U;
    }

    if ((result == MSG_OK) && (msg_addr_match(frame.device_id) != 0U)) {
        result = msg_handle_cmd(&frame);
    } else if (result != MSG_OK) {
        result = msg_build_error();
    }

    if ((result == MSG_OK) && (msg_tx_len > 0U)) {
        USART1_SendData(msg_tx_buffer, msg_tx_len);
    }
    msg_alarm_print();

    USART1_ClearRxBuf();

    if (msg_reboot_pending != 0U) {
        delay_1ms(20U);
        NVIC_SystemReset();
    }

    if (msg_sleep_pending != 0U) {
        msg_sleep_pending = 0U;
        delay_1ms(20U);
        OLED_Printf(0, 0, 16, "Sleep Mode   ");
        OLED_Refresh();
        RTC_SetWakeup(10U);
        pmu_flag_clear(PMU_FLAG_RESET_WAKEUP);
        pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);
        SystemInit();
        USART1_Init();
        printf("instrument wakeup\r\n");
        OLED_Printf(0, 0, 16, "wake up ok    ");
        OLED_Refresh();
        oled_idle_time = 2000;
    }
    return 1U;
}
