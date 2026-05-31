#ifndef __MSG_H
#define __MSG_H

#include "HeaderFiles.h"

#define MSG_MIN_LEN          8U
//偏移值
#define MSG_DEVICE_ID 0U
#define MSG_TYPE      2U
#define MSG_LENGTH   3U
#define MSG_VERSION   5U//版本号
#define MSG_PAYLOAD   6U//数据
//
#define MSG_CRC_LEN          2U
#define MSG_SAMPLE_PAYLOAD_LEN 16U
#define MSG_STATUS_PAYLOAD_LEN 2U
#define MSG_TX_BUF_LEN 256U

//type定义
#define MSG_TYPE_SET_OR_UPLOAD     0x01U
#define MSG_TYPE_GET_DEVICE_ID     0x02U
#define MSG_TYPE_SINGLE_SAMPLE     0x21U
#define MSG_TYPE_START_CONTINUOUS  0x22U
#define MSG_TYPE_STOP_CONTINUOUS   0x2FU

#define MSG_RESP_TYPE_DATA         0x01U
#define MSG_RESP_TYPE_ACK          0x02U
#define MSG_STATUS_OK              0x8000U
#define MSG_STATUS_FAIL            0x7000U

typedef enum {
    MSG_OK = 0,
    MSG_ERR_NULL = -1,
    MSG_ERR_TOO_SHORT = -2,
    MSG_ERR_LENGTH = -3,
    MSG_ERR_CRC = -4,
    MSG_ERR_TYPE = -5,
    MSG_ERR_BUF = -6
} msg_result_t;

///参数拆分
typedef struct {
    uint16_t device_id;
    uint8_t type;
    uint16_t length;
    uint8_t version;
    uint8_t *payload;
    uint16_t payload_len;
    uint16_t crc;
} msg_frame_t;


extern uint8_t msg_tx_buffer[MSG_TX_BUF_LEN];
extern uint16_t msg_tx_len;

//报文解析
msg_result_t msg_parse_basic(uint8_t *buf, uint16_t rx_len, msg_frame_t *frame);
//报文构建
uint16_t msg_build_frame(uint16_t device_id, uint8_t type, uint8_t version,uint8_t *payload,
                         uint16_t payload_len,uint8_t *tx_buf, uint16_t tx_buf_len);
//type匹配
msg_result_t msg_type_cmd(uint8_t *buf, uint16_t rx_len);

uint8_t msg_poll(void);



#endif
