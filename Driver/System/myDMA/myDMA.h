#ifndef __MYDMA_H
#define __MYDMA_H

#include "HeaderFiles.h"

#ifndef USART1_RX_BUF_LEN
#define USART1_RX_BUF_LEN 256U
#endif

#ifndef USART1_TX_BUF_LEN
#define USART1_TX_BUF_LEN 256U
#endif

extern uint8_t usart1_rx_buffer[USART1_RX_BUF_LEN];
extern uint8_t usart1_tx_buffer[USART1_TX_BUF_LEN];

extern volatile uint16_t adc_value[2];

// 初始化缓冲区
void mydma_init_buffers(void);

// --- 函数原型声明 ---

/* 串口TX DMA通用配置函数 */
void dma_usart_tx_config(uint32_t dma_periph, dma_channel_enum channelx, uint32_t par, uint32_t mar);

/* USART1专用RX DMA配置函数 (DMA0, CH5) */
void dma_usart1_rx_config(void);
/*触发一次新的DMA传输 */
void dma_enable(uint32_t dma_periph, dma_channel_enum channelx, uint16_t ndtr);

/* ADC DMA初始化 (ADC0 -> DMA1, CH0) */
void DMA_ADC_Init(void);
void DMA_ADJ_Init(void);

/* 初始化USART1与ADC的DMA */
void USART1_DMA_All_Init(void);

uint16_t get_usart1_rx_len(void);
void reset_usart1_rx_dma(void);

#endif
