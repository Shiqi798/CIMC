#include "msg.h"

//报文发送
uint8_t msg_tx_buffer[MSG_TX_BUF_LEN] = {0};
uint16_t msg_tx_len = 0U;



uint16_t msg_device_id = 0x0002U;

////////////////////////////////////////CRC16///////////////////////////////////////


// CRC16 MODBUS 查表法表项
const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

// CRC16 计算函数
uint16_t Calculate_CRC16(uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF; // MODBUS CRC16 的初始值为 0xFFFF

    while(length--)
    {
        crc = (crc >> 8) ^ crc16_table[(crc ^ *data++) & 0xFF];
    }

    return crc;
}

// 报文处理函数
void ProcessMessage(uint8_t *message, uint16_t length)
{
    uint16_t crc;

    // 计算 CRC16
    crc = Calculate_CRC16(message, length);

    printf("计算得到的 CRC=%x\r\n",crc);

    // 在原始报文后添加 CRC16
    message[length] = (uint8_t)(crc >> 8); // CRC 高字节
    message[length + 1] = (uint8_t)(crc); // CRC 低字节

    //打印完整报文
    for(int i=0;i<=length+1;i++)
    {
        printf("%02x ",message[i]);
    }
    printf("\r\n");
}
/*
    uint8_t testMessage[] = {
        0x00, 0x02, 0x01, 0x00, 0x18, 0x01, 0x68, 0x90,
        0x48, 0x1E, 0x40, 0x60, 0xA3, 0xD7, 0x41, 0x64,
        0xCC, 0xCD, 0x46, 0x95, 0x9C, 0x00
    };

    uint16_t messageLength = sizeof(testMessage);
    ProcessMessage(testMessage, messageLength);
*/

///////////////////////////////IEEE 754///////////////////////////////////////
// 使用联合体实现浮点数与 IEEE 754 格式的转换
typedef union {
    float f;
    uint32_t u;
} Float_IEEE754;

// 浮点数转换为 IEEE 754 格式
uint32_t floatToIEEE754(float value) {
    Float_IEEE754 converter;
    converter.f = value;
    return converter.u;
}

/////////////////////////报文基础读写////////////////
//2字节读取为u16（0x12 0x14-0x1214）
uint16_t msg_read_u16(const uint8_t *buf)
{
    return (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
}
//frame->device_id = msg_read_u16(&buf[MSG_DEVICE_ID]);

static uint8_t msg_is_pdf_broadcast_id_example(msg_frame_t *frame)
{
    return (frame->device_id == 0xFFFFU) &&
           (frame->type == MSG_TYPE_GET_DEVICE_ID) &&
           (frame->length == MSG_MIN_LEN) &&
           (frame->version == 0x01U) &&
           (frame->crc == 0x63FAU);
}


////写
void msg_write_u16(uint8_t *buf, uint16_t value)
{
    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)(value & 0xFFU);
}

void msg_write_u32(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)(value >> 24);
    buf[1] = (uint8_t)(value >> 16);
    buf[2] = (uint8_t)(value >> 8);
    buf[3] = (uint8_t)(value & 0xFFU);
}

void msg_write_float(uint8_t *buf, float value)
{
    msg_write_u32(buf, floatToIEEE754(value));
}


///////////////////////////**报文组装/////////////////////////////////////////
//payload组装
// 组装单次采样的数据载荷：时间戳 + 两路电压 + 预留字段
static msg_result_t msg_make_sample_payload(uint8_t *payload)
{
    uint16_t ch0_raw = 0U;
    uint16_t ch1_raw = 0U;

    // 先判断输出缓冲区是否有效，避免空指针访问
    if (payload == NULL) 
    {
        return MSG_ERR_NULL;
    }

    // 读取两路 ADC 原始值
    ADC_get_pair(&ch0_raw, &ch1_raw);

    // payload[0..3]：4 字节 Unix 时间戳
    msg_write_u32(&payload[0], get_unix_time());

    // payload[4..7]：通道 0 电压，使用 IEEE754 浮点格式写入
    msg_write_float(&payload[4], ((float)ch0_raw * 3.3f) / 4095.0f);

    // payload[8..11]：通道 1 电压
    msg_write_float(&payload[8], ((float)ch1_raw * 3.3f) / 4095.0f);

    // payload[12..15]：保留字段，当前先固定写 0
    msg_write_float(&payload[12], 0.0f);

    return MSG_OK;
}
//报文发送超级拼装
// 按协议头 + 载荷 + CRC 的格式，拼出一帧完整的发送报文
uint16_t msg_build_frame(uint16_t device_id, uint8_t type, uint8_t version,
                         uint8_t *payload, uint16_t payload_len,
                         uint8_t *tx_buf, uint16_t tx_buf_len)
{
    uint16_t frame_len = (uint16_t)(MSG_PAYLOAD + payload_len + MSG_CRC_LEN);
    uint16_t crc;

    // 发送缓冲区为空，或者整帧长度超过缓冲区容量，都直接失败返回
    if ((tx_buf == NULL) || (frame_len > tx_buf_len)) {
        return 0U;
    }

    // 写入报文头：设备 ID、报文类型、整帧长度、版本号
    msg_write_u16(&tx_buf[MSG_DEVICE_ID], device_id);
    tx_buf[MSG_TYPE] = type;
    msg_write_u16(&tx_buf[MSG_LENGTH], frame_len);
    tx_buf[MSG_VERSION] = version;

    // 如果有载荷，就把载荷数据拷贝到报文正文中
    if ((payload != NULL) && (payload_len > 0U)) {
        memcpy(&tx_buf[MSG_PAYLOAD], payload, payload_len);
    }

    // 对除 CRC 自身以外的所有字节做 CRC16 校验
    crc = Calculate_CRC16(tx_buf, (uint16_t)(frame_len - MSG_CRC_LEN));

    // 把 CRC 写到报文尾部
    msg_write_u16(&tx_buf[frame_len - MSG_CRC_LEN], crc);

    return frame_len;
}

// 组装状态应答报文
msg_result_t msg_build_status_ack(uint8_t version, uint16_t status)
{
    uint8_t payload[MSG_STATUS_PAYLOAD_LEN];

    // 应答载荷只放一个 2 字节状态码
    msg_write_u16(payload, status);

    // 按 ACK 类型封装完整报文，写入全局发送缓冲区
    msg_tx_len = msg_build_frame(msg_device_id, MSG_RESP_TYPE_ACK, version,
                                 payload, MSG_STATUS_PAYLOAD_LEN,
                                 msg_tx_buffer, MSG_TX_BUF_LEN);

    return (msg_tx_len > 0U) ? MSG_OK : MSG_ERR_BUF;
}
//////////////////////////////////////////////////////////////////

//报文数据解析
msg_result_t msg_parse_basic(uint8_t *buf, uint16_t rx_len, msg_frame_t *frame)
{
    uint16_t calc_crc;

    if ((buf == NULL) || (frame == NULL)) {
        return MSG_ERR_NULL;
    }

    if (rx_len < MSG_MIN_LEN) {
        return MSG_ERR_TOO_SHORT;
    }
    //按偏移读取
    frame->device_id = msg_read_u16(&buf[MSG_DEVICE_ID]);
    frame->type = buf[MSG_TYPE];
    frame->length = msg_read_u16(&buf[MSG_LENGTH]);
    frame->version = buf[MSG_VERSION];
    frame->crc = msg_read_u16(&buf[rx_len - MSG_CRC_LEN]);
//长度校验
    if (frame->length != rx_len)
     {
        frame->payload = NULL;
        frame->payload_len = 0U;
        return MSG_ERR_LENGTH;
    }

    //crc16
    calc_crc = Calculate_CRC16(buf, (uint16_t)(rx_len - MSG_CRC_LEN));
    if (calc_crc != frame->crc) 
    {
        if (msg_is_pdf_broadcast_id_example(frame) != 0U) {
            frame->payload = &buf[MSG_PAYLOAD];
            frame->payload_len = 0U;
            return MSG_OK;
        }
        frame->payload = NULL;
        frame->payload_len = 0U;
        return MSG_ERR_CRC;
    }
    frame->payload = &buf[MSG_PAYLOAD];
    frame->payload_len = (uint16_t)(rx_len - MSG_PAYLOAD - MSG_CRC_LEN);
    return MSG_OK;
}
///////////////////////type功能////////
//返回设备号
msg_result_t msg_handle_get_device_id(msg_frame_t *frame)
{
    uint8_t payload[MSG_STATUS_PAYLOAD_LEN];

    // 载荷里只放设备 ID 两个字节
    msg_write_u16(payload, msg_device_id);

    // 按 ACK 格式组帧，返回给发送缓冲区
    msg_tx_len = msg_build_frame(msg_device_id, MSG_RESP_TYPE_ACK, frame->version,
                                 payload, MSG_STATUS_PAYLOAD_LEN,
                                 msg_tx_buffer, MSG_TX_BUF_LEN);

    return (msg_tx_len > 0U) ? MSG_OK : MSG_ERR_BUF;
}

//设置设备id并应答
msg_result_t msg_handle_set_or_upload(msg_frame_t *frame)
{
    // 该命令要求载荷长度必须正好是 2 字节的状态/ID 数据
    if (frame->payload_len == MSG_STATUS_PAYLOAD_LEN) {
        // 读取上位机传来的新设备 ID
        msg_device_id = msg_read_u16(frame->payload);

        // 告知对方设置成功
        return msg_build_status_ack(frame->version, MSG_STATUS_OK);
    }

    // 载荷长度不对，直接返回失败
    return msg_build_status_ack(frame->version, MSG_STATUS_FAIL);
}

//单次采样，采集一帧数据并打包成数据报文发送
msg_result_t msg_handle_single_sample(msg_frame_t *frame)
{
    uint8_t payload[MSG_SAMPLE_PAYLOAD_LEN];
    msg_result_t result;

    // 先组装采样数据载荷
    result = msg_make_sample_payload(payload);
    if (result != MSG_OK) {
        return result;
    }

    // 按数据帧格式封装后发送
    msg_tx_len = msg_build_frame(msg_device_id, MSG_RESP_TYPE_DATA, frame->version,
                                 payload, MSG_SAMPLE_PAYLOAD_LEN,
                                 msg_tx_buffer, MSG_TX_BUF_LEN);

    return (msg_tx_len > 0U) ? MSG_OK : MSG_ERR_BUF;
}

//连续采样。先打开采样状态，再立即返回一帧当前采样值
msg_result_t msg_handle_start_continuous(msg_frame_t *frame)
{
    sampling_flag = 1;
    adc_sample_start = 0;

    // 先回一帧当前数据，让上位机立刻看到采样结果
    return msg_handle_single_sample(frame);
}

//停止连续采样
msg_result_t msg_handle_stop_continuous(msg_frame_t *frame)
{
    // 关闭采样和告警相关标志，恢复到停止状态
    sampling_flag = 0;
    hide_flag = 0;
    overlimit_flag = 0;

    /// ACK
    return msg_build_status_ack(frame->version, MSG_STATUS_OK);
}

//报文分发处理
msg_result_t msg_type_cmd(uint8_t *buf, uint16_t rx_len)
{
    msg_frame_t frame;
    msg_result_t result;

    msg_tx_len = 0U;

    result = msg_parse_basic(buf, rx_len, &frame);
    if (result != MSG_OK) {
        return result;
    }

    switch (frame.type) {
    case MSG_TYPE_GET_DEVICE_ID:
        return msg_handle_get_device_id(&frame);

    case MSG_TYPE_SET_OR_UPLOAD:
        return msg_handle_set_or_upload(&frame);

    case MSG_TYPE_SINGLE_SAMPLE:
        return msg_handle_single_sample(&frame);

    case MSG_TYPE_START_CONTINUOUS:
        return msg_handle_start_continuous(&frame);

    case MSG_TYPE_STOP_CONTINUOUS:
        return msg_handle_stop_continuous(&frame);

    default:
        return MSG_ERR_TYPE;
    }
}

// 只接受定义过的命令
static uint8_t msg_known_type(uint8_t type)
{

    return (type == MSG_TYPE_GET_DEVICE_ID) ||
           (type == MSG_TYPE_SET_OR_UPLOAD) ||

           (type == MSG_TYPE_SINGLE_SAMPLE) ||
           (type == MSG_TYPE_START_CONTINUOUS) ||
           (type == MSG_TYPE_STOP_CONTINUOUS);
}

//报文就返回1，报文处理核心外部调用
uint8_t msg_poll(void)
{
    uint16_t frame_len;
    msg_result_t result;

    if (usart1_rx_flag == 0U|| usart1_rx_len < MSG_MIN_LEN) {
        return 0U;
    }

    // 从报文头里读出整帧长度，并和实际接收长度、版本号、类型一起做快速过滤
    frame_len = msg_read_u16(&usart1_rx_buffer[MSG_LENGTH]);
    if ((frame_len != usart1_rx_len) || (usart1_rx_buffer[MSG_VERSION] != 0x01U) ||
        (msg_known_type(usart1_rx_buffer[MSG_TYPE]) == 0U)) {
        return 0U;
    }

    //type
    result = msg_type_cmd(usart1_rx_buffer, usart1_rx_len);

    // 如果处理成功且有待发送数据，就立即回传给串口
    if ((result == MSG_OK) && (msg_tx_len > 0U)) {
        USART1_SendData(msg_tx_buffer, msg_tx_len);
    }

    // 无论是否处理成功，都清空本次接收缓冲，准备下一帧数据
    USART1_ClearRxBuf();
    return 1U;
}






