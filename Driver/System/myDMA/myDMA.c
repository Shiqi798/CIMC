#include "myDMA.h"


///////////////////////////// 变量区 //////////////////////////
// 先256凑合
uint8_t usart1_rx_buffer[256];
uint8_t usart1_tx_buffer[256];
/* 预留4个通道 */
uint16_t adc_value[4];

// 初始化TX缓冲区
void mydma_init_buffers(void)
{
    memset(usart1_rx_buffer, 0, 256);
    memset(usart1_tx_buffer, 0, 256);
    memset(adc_value, 0, sizeof(adc_value));
} 

/////////////////////// 核心底层接口 ///////////////////////

/* 串口TX DMA通用配置，主要是配地址和增量模式 */
void dma_usart_tx_config(uint32_t dma_periph, dma_channel_enum channelx, uint32_t par, uint32_t mar)
{
    dma_single_data_parameter_struct dma_init_struct;

    // DMA0还是1全开了
    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMA1);

    dma_deinit(dma_periph, channelx);

    dma_init_struct.periph_addr = par;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory0_addr = mar;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE; /* TX不开循环！！！ */
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;          /* 内存搬到串口 */
    dma_init_struct.number = 0; 
    
    // 优先级过高，丢包
    // dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init_struct.priority = DMA_PRIORITY_MEDIUM;
    dma_single_data_mode_init(dma_periph, channelx, &dma_init_struct);
    
    // 默认配的SUBPERI4是给USART1用的，要是改USART0可能得换号
    dma_channel_subperipheral_select(dma_periph, channelx, DMA_SUBPERI4);
}


/* USART1专用RX DMA配置，DMA0, CH5, 默认开启循环模式 */
void dma_usart1_rx_config(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(RCU_DMA0);
    dma_deinit(DMA0, DMA_CH5);

    dma_init_struct.periph_addr = (uint32_t)&USART_DATA(USART1);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory0_addr = (uint32_t)usart1_rx_buffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_ENABLE; // RX这里循环
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    
    // 缓冲区大小
    dma_init_struct.number = 256;
    
    dma_init_struct.priority = DMA_PRIORITY_MEDIUM;

    dma_single_data_mode_init(DMA0, DMA_CH5, &dma_init_struct);
    dma_channel_subperipheral_select(DMA0, DMA_CH5, DMA_SUBPERI4);
    dma_channel_enable(DMA0, DMA_CH5); // RX常开
}

/* 开启/触发一次新的传输，主要用于TX发不定长数据 */
void dma_enable(uint32_t dma_periph, dma_channel_enum channelx, uint16_t ndtr)
{
    dma_channel_disable(dma_periph, channelx);
    
    // 清标志位
    dma_flag_clear(dma_periph, channelx, DMA_FLAG_FTF);
    
    dma_transfer_number_config(dma_periph, channelx, ndtr);
    dma_channel_enable(dma_periph, channelx);
}


/////////////////////// 接收辅助函数 ///////////////////////

/* 算一下当前收了多少个字节 */
// 配合IDLE中断用，直接读返回长度
uint16_t get_usart1_rx_len(void)
{
    return 256 - dma_transfer_number_get(DMA0, DMA_CH5);
}

/* 收到一包处理完后，重置一下RX DMA */
void reset_usart1_rx_dma(void)
{
    dma_channel_disable(DMA0, DMA_CH5);
    // 清一波标志位，之前没清偶尔会卡
    dma_flag_clear(DMA0, DMA_CH5, DMA_FLAG_FTF);
    dma_transfer_number_config(DMA0, DMA_CH5, 256);
    dma_channel_enable(DMA0, DMA_CH5);
}


//////////////////// 特定外设绑定的初始化 ////////////////////

/* ADC0 的 DMA 搬运初始化 (DMA1, CH0) */
void DMA_ADC_Init(void)
{
    dma_single_data_parameter_struct dma_init_struct;
    rcu_periph_clock_enable(RCU_DMA1);
    dma_deinit(DMA1, DMA_CH0);
    // ADC0数据寄存器地址
    dma_init_struct.periph_addr = (uint32_t)(&ADC_RDATA(ADC0));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory0_addr = (uint32_t)adc_value;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    // 这里偶尔拿错数据，注意ADC是半字16位的！！！
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_16BIT; 
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_ENABLE;
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    
    // 测试值4通道
    dma_init_struct.number = 4;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH0, &dma_init_struct);
    // ADC0固定通道
    dma_channel_subperipheral_select(DMA1, DMA_CH0, DMA_SUBPERI0);
    dma_channel_enable(DMA1, DMA_CH0);
}


//////////////////// 顶层接口 ////////////////////

void USART1_DMA_All_Init(void)
{
    // TX配给 DMA0 的 CH6，SUBPERI4
    dma_usart_tx_config(DMA0, DMA_CH6, (uint32_t)&USART_DATA(USART1), (uint32_t)usart1_tx_buffer);
    // RX配置
    dma_usart1_rx_config();

    DMA_ADC_Init();
}
