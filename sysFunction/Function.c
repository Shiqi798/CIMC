#include "HeaderFiles.h"
//************************* ?? *************************

#define SFLASH_ID 0xC84013
#define BUFFER_SIZE 256
#define TX_BUFFER_SIZE BUFFER_SIZE
#define RX_BUFFER_SIZE BUFFER_SIZE
#define FLASH_WRITE_ADDRESS 0x000000
#define FLASH_READ_ADDRESS FLASH_WRITE_ADDRESS

uint8_t oled_idle_refresh_flag=0; 
uint16_t oled_idle_time = 0;
// ??
uint32_t flash_id = 0;
uint8_t tx_buffer[TX_BUFFER_SIZE];
uint8_t rx_buffer[TX_BUFFER_SIZE];
uint16_t i = 0, count, result = 0;
uint8_t is_successful = 0;
char *DEVICE_ID = "Device_ID:2026-WUT-QRS-9\0"; // ??ID???64???

uint8_t sampling_flag = 0;  // 0: ???1: ???
uint8_t overlimit_flag = 0; // 0: ???1: ??
uint8_t hide_flag = 0;      // 0: ???1: ??

// ????
void nvic_config(void);
void sample_led_update(void);
void key_update(void);
uint8_t is_power_on_reset(void);

void sysFunction_Init(void)
{
    	
    SCB->VTOR = FLASH_BASE | 0x8000; 
	__enable_irq(); 
    SystemInit();
    systick_config(); // ?? systick
    tim6_functimer_init();

    USART1_Init();
    // USART1_DMA_All_Init();
    OLED_Init();
    OLED_Printf(0, 0, 16, "system idle");
    OLED_Refresh();
    LED_Init();
    ADC_port_init(); // ??? ADC
    RTC_Init();      // ??? ????
    DAC_Init();

   spi_flash_init(); // ??? SPI Flash
/* 
    printf("Erasing Flash... Please wait 5-20 seconds...\r\n");
    spi_flash_bulk_erase(); //flash全擦除
   printf("Flash Erase Done!\r\n");
*/
    fal_init();
    flashdata_init();
    flash_log_init();



    //    fal_show_part_table();
/*
    if (flashdata_init() == 0 && flash_log_init() == 0)
    {
        printf("[FlashDB] Env and Log DB init success!\r\n");
    }
    else
    {
        printf("[FlashDB] DB init failed!\r\n");
    }
*/
    Key_Init(); // ??? ??

    nvic_config(); // ?? NVIC

    data_cfg_t sys_cfg;
    get_data_config(&sys_cfg);
    adc_sample_cycle = sys_cfg.sample_cycle;
    ratio_ch0 = sys_cfg.ratio_ch0;
    limit_ch0 = sys_cfg.limit_ch0;
    dac_volt = sys_cfg.dac_volt;

    overlimit_flag = 0;
    hide_flag = 0;                                 // ??????

    set_team_number(DEVICE_ID); 
    printf("\r\n====system init====\r\n");
    char current_dev_id[64];
    get_team_number(current_dev_id, sizeof(current_dev_id));
    printf("\r\n%s\r\n", current_dev_id); 
    printf("Boot Mode:APP\r\n");
    printf("====system ready====\r\n");
    if (is_power_on_reset())
    {
        set_power_count();
    }
    DAC_SetVoltage(0, 0.0f);
    DAC_SetVoltage(1, 0.0f);
    uint32_t power_count = get_power_count();   // ??????
 //   printf("SystemCoreClock = %ld\r\n", SystemCoreClock);

}

void sysFunction_loop(void)
{

    while (1)
    {
        sample_led_update(); // ??????????
        key_update();        // ????
        cmd_parse();      // ????
        dac_test_tick();
        oled_idle_refresh(); // OLED?????
    }
}

void oled_idle_refresh(void)
{
    if (oled_idle_refresh_flag == 1)
    {
        OLED_Printf(0, 0, 16, "system idle  ");
        OLED_Printf(0, 16, 16, "        ");
        OLED_Refresh();
        oled_idle_refresh_flag = 0;
    }
}

void sample_led_update(void)
{
    static uint32_t led1_turn_start = 0;
    static uint32_t rtc_refresh_start = 0; // ?新增：用于专门给时间和屏幕分配 1s 更新周期
    static uint8_t last_sampling_flag = 0;
    
    if (sampling_flag == 1)
    {
        if (tim6_timeoutcheck(&rtc_refresh_start, 1000))
        {
            rtc_current_time_get(&rtc_initpara);
            OLED_Printf(0, 0, 16, "%02d:%02d:%02d    ",
                    BCD2BYTE(rtc_initpara.hour),
                    BCD2BYTE(rtc_initpara.minute),
                    BCD2BYTE(rtc_initpara.second));
            OLED_Refresh();
        }

        // 实时计算电压
        data_calc_eng_volt();

        // LED 闪烁及超限灯逻辑
        if (tim6_timeoutcheck(&led1_turn_start, led1_turn_time))
        {
            led1_turn();
            if (overlimit_flag == 1)
            {
                led2_on(); // 超限点亮 LED2
            }
            else
            {
                led2_off();
            }
        }
        if (tim6_timeoutcheck(&adc_sample_start, adc_sample_cycle))
        {
            sample_result_show();
        }
    }
    else
    {
        led1_off();
        led2_off();
    }
    
    // 刚从采样状态切换到非采样状态，清空屏幕并显示 idle
    if (last_sampling_flag == 1 && sampling_flag == 0) 
    {
        OLED_Printf(0, 0, 16, "system idle     "); 
        OLED_Printf(0, 16, 16, "                ");
        OLED_Refresh();
    }
    last_sampling_flag = sampling_flag;
}
        //        OLED_Printf(0, 0, 16, "system idle"); // ????
        //       OLED_Printf(0, 16, 16, "            ");
        //       OLED_Refresh();
    
void key_update(void)
{
    if (Key_Check(sample_s, KEY_DOWN))
    {
        if (sampling_flag == 0)
        {
            cmd_parse_start(); 
            append_normal_log("Periodic Sampling START(Key)");
        }
        else
        {
            cmd_parse_stop(); 
            append_normal_log("Periodic Sampling STOP(Key)");
        }
    }
    if (Key_Check(sample_cycle1, KEY_DOWN))
    {
        update_sample_cycle(5000);
        append_normal_log("Sample cycle set to 5s");
    }
    if (Key_Check(sample_cycle2, KEY_DOWN))
    {
        update_sample_cycle(10000);
        append_normal_log("Sample cycle set to 10s");
    }
    if (Key_Check(sample_cycle3, KEY_DOWN))
    {
        update_sample_cycle(15000);
        append_normal_log("Sample cycle set to 15s");
    }
}

/**
 * @brief ?? NVIC
 */
void nvic_config(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3); // set priority grouping
    nvic_irq_enable(SDIO_IRQn, 0, 0);                 // enable SDIO IRQ
}

uint8_t is_power_on_reset(void)
{
    uint8_t ret = (rcu_flag_get(RCU_FLAG_PORRST) == SET) ? 1 : 0;
    rcu_all_reset_flag_clear(); // ???????
    return ret;
}
void update_sample_cycle(uint32_t new_cycle)
{
    data_cfg_t temp_cfg;
    
    get_data_config(&temp_cfg);

    temp_cfg.sample_cycle = new_cycle;
    set_data_config(&temp_cfg);

    printf("\r\nsample cycle adjusted: %ds\r\n", new_cycle / 1000);

    sample_result_show();
    
    adc_sample_cycle = new_cycle;
    
    adc_sample_start = 0; 
}

/****************************End*****************************/
