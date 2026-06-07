#include "msg.h"
#include "msg_app.h"

static uint8_t msg_tx_buffer[MSG_TX_BUF_LEN] = {0};
static uint16_t msg_tx_len = 0U;
uint16_t msg_device_id = 0x0001U;

static uint8_t msg_raw_buffer[MSG_RAW_BUF_LEN] = {0};
static uint8_t msg_boot_heartbeat_sent = 0U;

/*------------------ CRC16 ------------------*/
// 璁＄畻16浣岰RC鏍￠獙鐮?(MODBUS)
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

/*------------------ 鍩虹璇诲啓 ------------------*/
// 璇诲彇2瀛楄妭涓簎16 (楂樹綅鍦ㄥ墠)
uint16_t msg_read_u16(const uint8_t *buf)
{
    return (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
}

// 鍐欏叆u16 (楂樹綅鍦ㄥ墠)
void msg_write_u16(uint8_t *buf, uint16_t value)
{
    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)(value & 0xFFU);
}

// 鍐欏叆u32 (楂樹綅鍦ㄥ墠)
void msg_write_u32(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)(value >> 24);
    buf[1] = (uint8_t)(value >> 16);
    buf[2] = (uint8_t)(value >> 8);
    buf[3] = (uint8_t)(value & 0xFFU);
}

uint32_t msg_read_u32(const uint8_t *buf)
{
    return (((uint32_t)buf[0] << 24) |
            ((uint32_t)buf[1] << 16) |
            ((uint32_t)buf[2] << 8) |
            (uint32_t)buf[3]);
}

void msg_write_float(uint8_t *buf, float value)
{
    union {
        float f;
        uint32_t u;
    } data;

    data.f = value;
    msg_write_u32(buf, data.u);
}

float msg_read_float(const uint8_t *buf)
{
    union {
        float f;
        uint32_t u;
    } data;

    data.u = msg_read_u32(buf);
    return data.f;
}

// ASCII瀛楃杞崐瀛楄妭
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

// 鍗婂瓧鑺傝浆ASCII瀛楃
static uint8_t msg_nibble_to_hex(uint8_t value)
{
    value &= 0x0FU;
    return (value < 10U) ? (uint8_t)('0' + value) : (uint8_t)('A' + value - 10U);
}

/*------------------ ASCII HEX杞崲 ------------------*/
// 妫€鏌ユ槸鍚︿互ASCII甯уご A5B6 寮€濮?
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

// ASCII鏍煎紡杞師濮嬩簩杩涘埗
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

// 鍘熷浜岃繘鍒惰浆ASCII
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

/*------------------ 鎶ユ枃瑙ｆ瀽 ------------------*/
// 瑙ｆ瀽骞舵牎楠屾姤鏂?
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

    // 闀垮害鏍￠獙
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

// 鍦板潃鍖归厤
static uint8_t msg_addr_match(uint16_t device_id)
{
    return (device_id == msg_device_id) || (device_id == MSG_BROADCAST_ID);
}

/*------------------ 鎶ユ枃缁勮 ------------------*/
msg_result_t msg_build_frame(uint16_t device_id, uint8_t type, uint16_t cmd,
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

/*------------------ 搴旂瓟 ------------------*/
msg_result_t msg_build_ok(uint16_t cmd)
{
    uint8_t payload = MSG_OK_BYTE;
    return msg_build_frame(msg_device_id, MSG_TYPE_ACK, cmd, &payload, 1U);
}

msg_result_t msg_build_error(void)
{
    return msg_build_frame(msg_device_id, MSG_TYPE_ERR, MSG_CMD_ERR, NULL, 0U);
}

void msg_send_heartbeat(void)
{
    if (msg_build_frame(msg_device_id, MSG_TYPE_HEART, MSG_CMD_HEART_BEAT, NULL, 0U) == MSG_OK) {
        msg_send_current();
    }
}

void msg_send_current(void)
{
    if (msg_tx_len > 0U) {
        USART1_SendData(msg_tx_buffer, msg_tx_len);
    }
}

void msg_send_string(uint8_t *str, uint16_t len)
{
    if ((str != NULL) && (len > 0U)) {
        USART1_SendData(str, len);
    }
}

//澶栭儴璋冪敤鎺ュ彛
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
        msg_app_auto_report_poll();
        return 0U;
    }
    if (msg_ascii_start_is_frame(usart1_rx_buffer, usart1_rx_len) == 0U) {
        if (msg_app_auto_report_is_active() != 0U) {
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

    if ((result == MSG_OK) && (msg_app_auto_report_is_active() != 0U) && (frame.cmd != 0x0303U)) {
        USART1_ClearRxBuf();
        return 1U;
    }

    if ((result == MSG_OK) && (msg_addr_match(frame.device_id) != 0U)) {
        result = msg_app_handle_cmd(&frame);
    } else if (result != MSG_OK) {
        result = msg_build_error();
    }

    if (result == MSG_OK) {
        msg_send_current();
    }
    msg_app_alarm_print();

    USART1_ClearRxBuf();

    msg_app_after_send();
    return 1U;
}
