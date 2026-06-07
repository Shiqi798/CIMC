#include "app_param.h"
#include "gd32f4xx_fmc.h"

/************************* 参数默认值 *************************/
float ratio_ch0 = 1.0f;
float ratio_ch1 = 1.0f;
float limit_ch0 = 100.0f;
float limit_ch1 = 100.0f;
float dac_volt = 1.0f;
uint8_t alarm_report_mode = 2U;
uint8_t usart1_baud_mode = 0x13U;
volatile uint32_t adc_sample_cycle = 5000U;

/************************* boot参数区 *************************/
#define BOOT_PARAM_CHECKSUM_LEN 8U

void boot_param_set_flag(uint8_t flag)
{
    boot_param_t param;
    uint32_t i;
    uint32_t *src;

    memcpy(&param, (const void *)BOOT_PARAM_ADDR, sizeof(param));

    //参数区没初始化过就先补一个底
    if (param.magic != BOOT_CONTROL_MAGIC) {
        memset(&param, 0, sizeof(param));
        param.magic     = BOOT_CONTROL_MAGIC;
        param.boot_flag = BOOT_FLAG_NORMAL;
    }

    //升级前同步一下，bootloader也要认这个ID和波特率
    param.device_id = msg_device_id;
    param.baud_code = usart1_baud_mode;
    param.boot_flag = flag;
    param.checksum = (uint32_t)Calculate_CRC16((uint8_t *)&param, BOOT_PARAM_CHECKSUM_LEN);

    //内部flash参数页，整页擦再写
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    fmc_page_erase(BOOT_PARAM_ADDR);
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);

    src = (uint32_t *)&param;
    for (i = 0U; i < (sizeof(param) + 3U) / 4U; i++) {
        fmc_word_program(BOOT_PARAM_ADDR + (i * 4U), src[i]);
        fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    }
    fmc_lock();
}
