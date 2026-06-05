#ifndef __MSG_H
#define __MSG_H

#include "HeaderFiles.h"

/*------------------ 固定字段 ------------------*/
#define MSG_TX_BUF_LEN       256U
#define MSG_RAW_BUF_LEN      128U
#define MSG_MIN_RAW_LEN      13U
#define MSG_START_FLAG       0xA5B6U
#define MSG_END_FLAG         0xB6A5U
#define MSG_BROADCAST_ID     0xFFFFU
#define MSG_PROTO_VER        0x02U

/*------------------ Type定义 ------------------*/
#define MSG_TYPE_CMD         0x01U
#define MSG_TYPE_ACK         0x02U
#define MSG_TYPE_HEART       0x05U
#define MSG_TYPE_ERR         0xFFU

/*------------------ Cmd定义 ------------------*/
#define MSG_OK_BYTE          0xFFU
#define MSG_BAUD_19200       0x13U

//结果枚举
typedef enum {
    MSG_OK = 0,
    MSG_ERR_NULL = -1,
    MSG_ERR_TOO_SHORT = -2,
    MSG_ERR_LENGTH = -3,
    MSG_ERR_CRC = -4,
    MSG_ERR_TYPE = -5,
    MSG_ERR_BUF = -6,
    MSG_ERR_FORMAT = -7
} msg_result_t;
/*------------------ 帧结构体拆分 ------------------*/
///参数拆分
typedef struct {
    uint16_t device_id;
    uint8_t type;
    uint16_t cmd;
    uint8_t length;
    uint8_t version;
    uint8_t *payload;
    uint16_t crc;
} msg_frame_t;

extern uint8_t msg_tx_buffer[MSG_TX_BUF_LEN];
extern uint16_t msg_tx_len;
extern uint16_t msg_device_id;
/*------------------ 外部接口定义 ------------------*/
// CRC计算
uint16_t Calculate_CRC16(uint8_t *data, uint16_t length);
// 发送心跳包
void msg_send_heartbeat(void);
// id msg_send_heartbeat(void);
//报文处理
uint8_t msg_poll(void);

#endif
