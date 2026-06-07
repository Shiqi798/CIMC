#include "boot_param.h"
#include "gd32f4xx_fmc.h"

/* CRC16 覆盖区域：magic(4) + device_id(2) + baud_code(1) + boot_flag(1) = 8 bytes */
#define BOOT_PARAM_CHECKSUM_LEN  8U

void boot_param_set_flag(uint8_t flag)
{
    boot_param_t param;
    uint32_t i;
    uint32_t *src;

    /* 1. Read current parameter block from flash */
    memcpy(&param, (const void *)BOOT_PARAM_ADDR, sizeof(param));

    /* 2. Validate magic; reset to defaults if corrupt */
    if (param.magic != BOOT_CONTROL_MAGIC) {
        memset(&param, 0, sizeof(param));
        param.magic      = BOOT_CONTROL_MAGIC;
        param.boot_flag  = BOOT_FLAG_NORMAL;
    }

    /* 3. Always sync APP's current device_id and baud_code to parameter block.
     *    These may have been changed via 0x01A1 / 0x01A2 after bootloader wrote
     *    its defaults. Without this, the bootloader will reject upgrade frames
     *    because of device ID mismatch. */
    param.device_id  = msg_device_id;
    param.baud_code  = usart1_baud_mode;

    /* 4. Update boot_flag */
    param.boot_flag = flag;

    /* 5. Calculate CRC16 checksum (low 16 bits of checksum field) */
    param.checksum = (uint32_t)Calculate_CRC16((uint8_t *)&param, BOOT_PARAM_CHECKSUM_LEN);

    /* 6. Erase parameter page (4K, GD32F470 fmc_page_erase) */
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    fmc_page_erase(BOOT_PARAM_ADDR);
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);

    /* 7. Write parameter block word by word */
    src = (uint32_t *)&param;
    for (i = 0U; i < (sizeof(param) + 3U) / 4U; i++) {
        fmc_word_program(BOOT_PARAM_ADDR + (i * 4U), src[i]);
        fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    }
    fmc_lock();
}
