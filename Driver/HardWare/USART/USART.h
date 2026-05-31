#ifndef __USART1_H
#define __USART1_H

/************************* 头文件 *************************/

#include "HeaderFiles.h"

/************************* 宏定义 *************************/

#define USART1_RX_BUF_LEN 128  // 接收缓冲区长度（适配赛题最长指令）
//#define USART1_CMD_END    '\n' // 指令结束符（赛题串口指令以回车/换行结尾）
#define RS485_DIR_PIN    GPIO_PIN_8
#define RS485_DIR_PORT   GPIOE
// 发送模式（PE8 高电平）
#define RS485_TX_MODE()  gpio_bit_set(RS485_DIR_PORT, RS485_DIR_PIN)
// 接收模式（PE8 低电平，默认）
#define RS485_RX_MODE()  gpio_bit_reset(RS485_DIR_PORT, RS485_DIR_PIN)


#define printf  rs485_printf

/************************ 全局变量 ************************/
extern uint8_t data_recv;                          // 单字节接收临时变量
extern uint8_t usart1_rx_buf[USART1_RX_BUF_LEN]; // 串口接收缓冲区（核心）
extern uint16_t usart1_rx_len;                     // 缓冲区已存储的字节数
extern uint8_t usart1_rx_flag;                     // 指令接收完成标志（收到\n置1）

/************************* 函数声明 *************************/
void rs485_printf(const char *fmt, ...);
/**
 * @brief 配置USART1串口
 */
void USART1_Init(void);

/**
 * @brief  清空USART1接收缓冲区
 */
void USART1_ClearRxBuf(void);


void USART1_SendData(uint8_t *buf, uint16_t len);
//void USART1_SendBytes(uint8_t *buf, uint16_t len);


#endif

/**************************** 文件结束 *****************************/
