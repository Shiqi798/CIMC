#include "USART.h"


////////////////////////////// 全局变量 //////////////////////////////
uint8_t data_recv = 0;                          // 暂时留着清状态用
//usart1_rx_buffer--mydma.c
uint16_t usart1_rx_len = 0;                     // 缓冲区已存储的字节数
uint8_t usart1_rx_flag = 0;


//////////////////////// 核心发送接收逻辑 ////////////////////////

/**
 * @brief  重定向C库printf函数到USART（配合DMA改版）
 */
void rs485_printf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    // 拼接整段字符串
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    RS485_TX_MODE();

    uint16_t len = strlen(buf);
    // 先拷到DMA专用的TX buffer，免得局部变量buf出了作用域被覆写
    memcpy(usart1_tx_buffer, buf, len);
    dma_enable(DMA0, DMA_CH6, len);
    
    // 等待搬运完
    while(dma_flag_get(DMA0, DMA_CH6, DMA_FLAG_FTF) == RESET); 
    while(usart_flag_get(USART1, USART_FLAG_TC) == RESET);   

    RS485_RX_MODE();
}

/**
 * @brief  清空USART1接收缓冲区
 */
void USART1_ClearRxBuf(void)
{
    // 改成清DMA的数组
    memset(usart1_rx_buffer, 0, 256);
    usart1_rx_len = 0;
    usart1_rx_flag = 0;
}

/**
 * @brief  USART1初始化配置
 */
void USART1_Init(void)
{
    // 开时钟
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART1);

    // ========== PA1 方向脚，默认 低电平(接收) ==========
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_1);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
    gpio_bit_reset(GPIOA, GPIO_PIN_1); //开机接收
    // ========== PA2 TX ==========
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2);
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_2);

    // ========== PA3 RX ==========
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_3);
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_3);

    // ========== USART1基本参数配置 ==========
    usart_deinit(USART1);
    usart_baudrate_set(USART1, 115200U);
    usart_word_length_set(USART1, USART_WL_8BIT);
    usart_stop_bit_set(USART1, USART_STB_1BIT);
    usart_parity_config(USART1, USART_PM_NONE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    
    //DMA的配置函数
    usart_dma_transmit_config(USART1, USART_TRANSMIT_DMA_ENABLE);
    usart_dma_receive_config(USART1, USART_RECEIVE_DMA_ENABLE);
    
    nvic_irq_enable(USART1_IRQn, 2, 0);  // 开USART1中断
    // usart_interrupt_enable(USART1, USART_INT_RBNE); // 单字节中断
    usart_interrupt_enable(USART1, USART_INT_IDLE);    /* 换成空闲中断接整包 */
    usart_enable(USART1);
    USART1_DMA_All_Init();
    // 初始化时清空缓冲区
    USART1_ClearRxBuf();
    data_recv = 0;
}

/**
 * @brief  USART1发送数据
 */
void USART1_SendData(uint16_t *buf, uint16_t len)
{
    for(uint16_t i=0; i<len; i++){
        usart1_tx_buffer[i] = (uint8_t)buf[i];
    }
    
    RS485_TX_MODE();
    dma_enable(DMA0, DMA_CH6, len);
    while(dma_flag_get(DMA0, DMA_CH6, DMA_FLAG_FTF) == RESET); 
    while(usart_flag_get(USART1, USART_FLAG_TC) == RESET);
    RS485_RX_MODE();
}


//////////////////////////////////// USART1中断服务函数 ////////////////////////////////////
void USART1_IRQHandler(void)
{
    // 查空闲中断标志位
    if (RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_IDLE))
    {
        data_recv = usart_data_receive(USART1);
        usart1_rx_len = get_usart1_rx_len();

        for(int i = 0; i < usart1_rx_len; i++)
        {
            if(usart1_rx_buffer[i] == '\n' || usart1_rx_buffer[i] == '\r')
            {
                usart1_rx_buffer[i] = '\0';
                break;
            }
        }
        
        // 防溢出
        usart1_rx_buffer[usart1_rx_len] = '\0'; 
        usart1_rx_flag = 1;  // 标志置1，通知主循环去解包
        reset_usart1_rx_dma();
    }
}
