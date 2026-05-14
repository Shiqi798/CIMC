#include "HeaderFiles.h"
//************************* ?? *************************

#define  SFLASH_ID                     0xC84013
#define BUFFER_SIZE                    256
#define TX_BUFFER_SIZE                 BUFFER_SIZE
#define RX_BUFFER_SIZE                 BUFFER_SIZE
#define  FLASH_WRITE_ADDRESS           0x000000
#define  FLASH_READ_ADDRESS            FLASH_WRITE_ADDRESS

// ??
uint32_t flash_id = 0;
uint8_t  tx_buffer[TX_BUFFER_SIZE];
uint8_t  rx_buffer[TX_BUFFER_SIZE];
uint16_t i = 0, count, result = 0;
uint8_t  is_successful = 0;
char* DEVICE_ID = "Device_ID:2026-WUT-QRS-9\0"; // ??ID???64???

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
    systick_config();     // ?? systick
    tim6_functimer_init();
    
	USART1_Init();
    // USART1_DMA_All_Init(); // 已在 USART1_Init() 内部调用，移除此行避免重复初始化
    OLED_Init();
    OLED_Printf(0,0,16,"system idle");
    OLED_Refresh();
    LED_Init(); 
    ADC_port_init();       // ??? ADC
    RTC_Init();            // ??? ????
    spi_flash_init();      // ??? SPI Flash
    fal_init();
    fal_show_part_table();
    if (flashdb_kv_demo())
    {
        printf("[FDB DEMO] migrate ok\r\n");
    }
    else
    {
        printf("[FDB DEMO] migrate check failed\r\n");
    }
    Key_Init();      // ??? ??

    nvic_config();        // ?? NVIC
    adc_sample_cycle = spi_flash_sample_cycle_read(); // ? Flash ??????
    overlimit_flag = 0;                               // ??????
    hide_flag = 0;                                    // ??????
    if(spi_flash_device_id_write(DEVICE_ID)!=1) // ???ID?? Flash
    {
        printf("Device ID write failed!\r\n");
    }
    printf("\r\n====system init====\r\n");
    printf("%s\r\n", spi_flash_device_id_read()); // ? Flash ???????ID
    printf("====system ready====\r\n");

    if(is_power_on_reset())
    {
        spi_flash_power_count_update(); // ?? Flash ??????
    }

    power_count = spi_flash_power_count_read(); // ??????
    spi_ratio_limit_read(&ratio_ch0, &limit_ch0); // ? Flash ????/??
}

void sysFunction_loop(void)
{

    while (1)
    {
        sample_led_update(); // ??????????
        key_update();        // ????
        cmd_parse();         // ????
        dac_test_tick();    
    }
}

void sample_led_update(void)
{
    static uint32_t led1_turn_start = 0;
    static uint8_t last_sampling_flag=0;
    if (sampling_flag == 1)
    {
        data_calc_eng_volt();
        if (tim6_timeoutcheck(&led1_turn_start, led1_turn_time))
        {
            led1_turn();
            if(overlimit_flag == 1)
            {
                led2_on(); // ????? LED2
            }
            else
            {
                led2_off();
            }
            if (tim6_timeoutcheck(&adc_sample_start, adc_sample_cycle))
            {
                sample_result_show();
            }
            rtc_current_time_get(&rtc_initpara);

        }
    }
    else
    {
        led1_off();
        led2_off();
//        OLED_Printf(0, 0, 16, "system idle"); // ????
 //       OLED_Printf(0, 16, 16, "            ");
 //       OLED_Refresh();
    }
    if(last_sampling_flag==1 && sampling_flag==0) // 刚从采样状态切换到非采样状态
    {
        OLED_Printf(0, 0, 16, "system idle"); // ????
        OLED_Printf(0, 16, 16, "            ");
        OLED_Refresh();
    }
    last_sampling_flag = sampling_flag;
}

void key_update(void)
{
    if (Key_Check(sample_s, KEY_DOWN))
	{
		if (sampling_flag == 0)
		{
            cmd_parse_start(); // ????
		}
		else
		{
            cmd_parse_stop(); // ????
		}
	}
    /*
    // 临时调试：监控 PE11（sample_cycle2，对应 index 2）稳定电平变化
    {
        static uint8_t prev_level_cycle2 = KEY_UNPRESSED;
        uint8_t curr_level_cycle2 = Key_GetState(sample_cycle2);
        if (curr_level_cycle2 != prev_level_cycle2)
        {
            prev_level_cycle2 = curr_level_cycle2;
        }
    }
    */
	if (Key_Check(sample_cycle1, KEY_DOWN))
	{
        spi_flash_sample_cycle_update(1,5000); //5 ?????
	}
	if (Key_Check(sample_cycle2, KEY_DOWN))
	{
        spi_flash_sample_cycle_update(1,10000);
	}
	if (Key_Check(sample_cycle3, KEY_DOWN))
	{
        spi_flash_sample_cycle_update(1,15000);
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
    uint8_t ret = (rcu_flag_get(RCU_FLAG_PORRST)==SET )? 1 : 0;
    rcu_all_reset_flag_clear(); // ???????
    return ret;
}

/****************************End*****************************/
