#ifndef __HEADERFILES_H
#define __HEADERFILES_H
#include <stdint.h>
#include <string.h>
#include <ctype.h>
extern uint8_t usart1_rx_flag;
extern uint16_t usart1_rx_len;
extern uint8_t usart1_rx_buffer[];
uint32_t get_unix_time(void);
void USART1_SendData(uint8_t *buf, uint16_t len);
void USART1_ClearRxBuf(void);
void delay_1ms(uint32_t count);
void NVIC_SystemReset(void);
#endif
