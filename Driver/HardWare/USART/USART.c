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
/*    if(exflash_erase_flag==1)
    {
        return;
    }  
*/ 
    char buf[512];
    va_list ap;
    int n;
    static uint8_t print_started = 0;

    // 拼接整段字符串
    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n < 0) {
        return;
    }

    uint16_t len = (uint16_t)strlen(buf);
    if (len >= sizeof(usart1_tx_buffer)) {
        len = sizeof(usart1_tx_buffer) - 1;
    }
    if (len == 0) {
        return;  // 字符串为空，直接返回
    }

    // 帧间隔
    if (print_started != 0U) {
        delay_1ms(4);
    }
    print_started = 1U;

    RS485_TX_MODE();
 
    // 先拷到DMA专用的TX buffer，免得局部变量buf出了作用域被覆写
    memcpy(usart1_tx_buffer, buf, len);
    usart_flag_clear(USART1, USART_FLAG_TC);
    dma_enable(DMA0, DMA_CH6, len);
    


    // 等待DMA完成
    while (dma_flag_get(DMA0, DMA_CH6, DMA_FLAG_FTF) == RESET);
    
    // 等待USART最后一个bit真正发完
    while (usart_flag_get(USART1, USART_FLAG_TC) == RESET);
    

    RS485_RX_MODE();
}

/**
 * @brief  清空USART1接收缓冲区
 */
void USART1_ClearRxBuf(void)
{
    // 改成清DMA的数组
    memset(usart1_rx_buffer, 0, USART1_RX_BUF_LEN);
    usart1_rx_len = 0;
    usart1_rx_flag = 0;
}

/**
 * @brief  USART1初始化配置
 */
void USART1_Init(void)
{
    uint32_t baud = 19200U;

    if (usart1_baud_mode == 0x11U) {
        baud = 4800U;
    } else if (usart1_baud_mode == 0x12U) {
        baud = 9600U;
    } else if (usart1_baud_mode == 0x14U) {
        baud = 115200U;
    }

    // 开时钟
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);  // 添加GPIOE时钟（RS485方向脚）
    rcu_periph_clock_enable(RCU_USART1);
    //PE8 0接受，1发送
    gpio_mode_set(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_8);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
    gpio_bit_reset(GPIOE, GPIO_PIN_8); //开机接收
    
    //PD5 TX
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_5);
    gpio_af_set(GPIOD, GPIO_AF_7, GPIO_PIN_5);

    //PD6 RX
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_6);
    gpio_af_set(GPIOD, GPIO_AF_7, GPIO_PIN_6);
    //初始化DMA
    USART1_DMA_All_Init();
    usart_deinit(USART1);
    usart_baudrate_set(USART1, baud);
    usart_word_length_set(USART1, USART_WL_8BIT);
    usart_stop_bit_set(USART1, USART_STB_1BIT);
    usart_parity_config(USART1, USART_PM_NONE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    
    //DMA
    usart_dma_transmit_config(USART1, USART_TRANSMIT_DMA_ENABLE);
    usart_dma_receive_config(USART1, USART_RECEIVE_DMA_ENABLE);
    // 中断
    nvic_irq_enable(USART1_IRQn, 2, 0);  // 开USART1中断
    usart_interrupt_enable(USART1, USART_INT_IDLE);    /* 使用空闲中断接整包 */

    usart_enable(USART1);
    USART1_ClearRxBuf();
    data_recv = 0;
}

/**
 * @brief  USART1发送数据
 */
/**
void USART1_SendData(uint16_t *buf, uint16_t len)
{
    if (len > sizeof(usart1_tx_buffer)) {
        len = sizeof(usart1_tx_buffer);
    }
    for(uint16_t i=0; i<len; i++){
        usart1_tx_buffer[i] = (uint8_t)buf[i];
    }
    
    RS485_TX_MODE();
    dma_enable(DMA0, DMA_CH6, len);
    while(dma_flag_get(DMA0, DMA_CH6, DMA_FLAG_FTF) == RESET); 
    while(usart_flag_get(USART1, USART_FLAG_TC) == RESET);
    RS485_RX_MODE();
}
*/
void USART1_SendData(uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U)) {
        return;
    }

    if (len > sizeof(usart1_tx_buffer)) {
        len = sizeof(usart1_tx_buffer);
    }

    memcpy(usart1_tx_buffer, buf, len);

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

        // 防止缓冲区溢出
        if (usart1_rx_len >= USART1_RX_BUF_LEN) {
            usart1_rx_len = USART1_RX_BUF_LEN - 1U;
        }

        // Keep raw bytes for binary frames; append '\0' only for legacy text commands.
        usart1_rx_buffer[usart1_rx_len] = '\0';
        
        usart1_rx_flag = 1;  // 标志置1，通知主循环去解包
        reset_usart1_rx_dma();
    }
}
