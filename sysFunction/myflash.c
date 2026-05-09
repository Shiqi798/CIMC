#include "myflash.h"

uint8_t spi_flash_device_id_write(const char* device_id)
{
    // ！！防空指针暴毙，传空地址直接打回
    if (device_id == NULL) {
        return 0; 
    }

    uint16_t safe_len = 0;
    

    spi_flash_sector_erase(FLASH_ADDR_DEVICE_ID); // 擦除之前存储的设备ID

    uint8_t buffer[FLASH_LEN_DEVICE_ID] = {0};
    
    // 复制数据，哪怕没\0结尾，或者超长，这里也只会复制 safe_len 个字节，绝对安全
    memcpy(buffer, device_id, safe_len); 
    
    spi_flash_page_write(buffer, FLASH_ADDR_DEVICE_ID, FLASH_LEN_DEVICE_ID); // 写入Flash
    
    return 1; // 写入成功
}


char* spi_flash_device_id_read(void)
{
    static uint8_t buffer[FLASH_LEN_DEVICE_ID];
    spi_flash_buffer_read(buffer, FLASH_ADDR_DEVICE_ID, FLASH_LEN_DEVICE_ID); // 从Flash读取设备ID
    buffer[FLASH_LEN_DEVICE_ID - 1] = '\0';
    return (char*)(buffer); // 返回设备ID字符串
}
uint32_t power_count = 0; // 定义全局变量存储上电次数
uint8_t spi_flash_power_count_update(void)
{
    uint32_t g_power_count = spi_flash_power_count_read(); // 读取当前上电次数
    if(g_power_count == 0xFFFFFFFF) {
        g_power_count = 0; // 如果是初始状态，重置为0
    }
    else
    {
        g_power_count+=1; // 上电次数加1
    }

    uint8_t buffer[4];
    memcpy(buffer, &g_power_count, sizeof(uint32_t)); // 将上电次数转换为字节数组
    spi_flash_sector_erase(FLASH_ADDR_POWER_COUNT); // 擦除之前存储的上电次数
    spi_flash_page_write(buffer, FLASH_ADDR_POWER_COUNT, sizeof(uint32_t)); // 写入Flash
    power_count = g_power_count; // 更新全局变量
    return 1; // 写入成功
}

uint32_t spi_flash_power_count_read(void)
{
    uint8_t buffer[4];
    spi_flash_buffer_read(buffer, FLASH_ADDR_POWER_COUNT, sizeof(uint32_t)); // 从Flash读取字节数组
    uint32_t r_power_count;
    memcpy(&r_power_count, buffer, sizeof(uint32_t)); // 将字节数组转换回uint32_t
    return r_power_count;
}

uint8_t spi_flash_sample_cycle_update(uint32_t cycle)
{

    uint8_t buffer[sizeof(uint32_t)];
    memcpy(buffer, &cycle, sizeof(uint32_t));
    spi_flash_sector_erase(FLASH_ADDR_SAMPLE_CYCLE); // 擦除之前存储的采样周期
    spi_flash_page_write(buffer, FLASH_ADDR_SAMPLE_CYCLE, sizeof(uint32_t)); // 写入Flash
    printf("\r\nsample cycle adjusted: %ds\r\n", cycle/1000);
    sample_result_show(); 
    adc_sample_cycle = cycle; // 更新全局变量以立即应用新的采样周期
    adc_sample_start = 0; // 重置采样计时器以立即应用新的采样周期
    return 1;                                                               // 写入成功
}

uint32_t spi_flash_sample_cycle_read(void)
{
    uint8_t cycle[sizeof(uint32_t)];
    uint8_t buffer[sizeof(uint32_t)];
    spi_flash_buffer_read(cycle, FLASH_ADDR_SAMPLE_CYCLE, sizeof(uint32_t)); // 从Flash读取采样周期
    uint32_t cycle_value;
    memcpy(&cycle_value, cycle, sizeof(uint32_t));
    if(cycle_value == 0xFFFFFFFF) {
        memcpy(buffer, &cycle_value, sizeof(uint32_t));
        spi_flash_sector_erase(FLASH_ADDR_SAMPLE_CYCLE);                         // 擦除之前存储的采样周期
        spi_flash_page_write(buffer, FLASH_ADDR_SAMPLE_CYCLE, sizeof(uint32_t)); // 写入Flash
        return 5000; // 如果是初始状态，返回默认采样周期5000ms
    }
    uint32_t result;
    memcpy(&result, cycle, sizeof(uint32_t));
    return result;
}

//擦除整个flash
void spi_flash_erase(void)
{
    systick_config();     // 时钟初始化
    spi_flash_init();  // SPI FLASH初始化
    spi_flash_bulk_erase();
}
/**
 * @brief 从指定SPI Flash地址加载行数和文件名
 * @param addr Flash地址
 * @param line 输出：行数
 * @param filename 输出：文件名缓冲区（至少64字节）
 * @retval 0成功，-1失败（未初始化）
 */
int load_state(uint32_t addr, uint16_t *line, char *filename)
{
    uint8_t buf[66]; // 2字节行数 + 64字节文件名
	
    spi_flash_buffer_read(buf, addr, sizeof(buf));

    uint16_t stored_line = (buf[1] << 8) | buf[0];
    if (stored_line == 0xFFFF)
    {
        *line = 0;
        return 0; // 初始状态，行数为0，文件名保持空
    }
    else
    {
        *line = stored_line;
        memcpy(filename, buf + 2, 64);
        filename[63] = '\0'; // 确保终止
        return 1;
    }
}

/**
 * @brief 保存行数和文件名到指定SPI Flash地址（自动擦除所在扇区）
 * @param addr Flash地址
 * @param line 行数
 * @param filename 文件名
 */
static void save_state(uint32_t addr, uint16_t line, const char *filename)
{

    spi_flash_sector_erase(addr);

    // 准备数据：2字节行数（小端） + 64字节文件名（不足补0）
    uint8_t buf[66];
    buf[0] = line & 0xFF;
    buf[1] = (line >> 8) & 0xFF;
    memset(buf + 2, 0, 64);
    strncpy((char*)buf + 2, filename, 64);

    // 写入Flash
    spi_flash_page_write(buf, addr, sizeof(buf));
}

/**
 * @brief 保存所有日志状态到SPI Flash
 */
void log_states_save_all(void)
{
    save_state(FLASH_ADDR_SAMPLE,   g_sample_line, g_sample_file);
    save_state(FLASH_ADDR_OVERLIMIT, g_over_line,  g_over_file);
    save_state(FLASH_ADDR_HIDEDATA,  g_hide_line,  g_hide_file);
}

void log_states_save_sample(void)
{
    save_state(FLASH_ADDR_SAMPLE,   g_sample_line, g_sample_file);
}

void log_states_save_over(void)
{
    save_state(FLASH_ADDR_OVERLIMIT, g_over_line,  g_over_file);
}
void log_states_save_hide(void)
{
    save_state(FLASH_ADDR_HIDEDATA,  g_hide_line,  g_hide_file);
}

void log_states_load_all(void)
{
    load_state(FLASH_ADDR_SAMPLE,   &g_sample_line, g_sample_file);
    load_state(FLASH_ADDR_OVERLIMIT, &g_over_line,  g_over_file);
    load_state(FLASH_ADDR_HIDEDATA,  &g_hide_line,  g_hide_file);
}


uint8_t spi_ratio_limit_read(float *r, float *l)
{
    uint8_t ratio_buf[4];
    uint8_t limit_buf[4];
    spi_flash_buffer_read(ratio_buf, FLASH_ADDR_RATIO, sizeof(ratio_buf));
    spi_flash_buffer_read(limit_buf, FLASH_ADDR_LIMIT, sizeof(limit_buf));

    uint32_t ratio_raw, limit_raw;
    memcpy(&ratio_raw, ratio_buf, sizeof(ratio_raw));
    memcpy(&limit_raw, limit_buf, sizeof(limit_raw));

    if (ratio_raw == 0xFFFFFFFF || limit_raw == 0xFFFFFFFF) {
        *r = 1.0f; // 默认比例
        *l = 100.0f; // 默认限值
        spi_ratio_limit_write(*r, *l); // 写入默认值到Flash
        return 0; // 初始状态，返回默认值
    }

    memcpy(r, &ratio_raw, sizeof(float));
    memcpy(l, &limit_raw, sizeof(float));
    return 1; // 成功读取
}
uint8_t spi_ratio_limit_write(float r, float l)
{
    uint8_t buf[8];
    memcpy(buf, &r, sizeof(float));
    memcpy(buf + 4, &l, sizeof(float));

    spi_ratio_limit_erase(); 
    spi_flash_page_write(buf, FLASH_ADDR_RATIO, sizeof(float)); // 写入Flash
    spi_flash_page_write(buf + 4, FLASH_ADDR_LIMIT, sizeof(float)); // 写入限值
    return 1; // 写入成功
}

void spi_ratio_limit_erase(void)
{
    spi_flash_sector_erase(FLASH_ADDR_RATIO); // 擦除比例所在扇区
    spi_flash_sector_erase(FLASH_ADDR_LIMIT); // 擦除限值所在扇区   
}

