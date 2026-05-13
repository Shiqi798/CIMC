#include "myflash.h"



uint8_t spi_flash_device_id_write(const char* device_id)
{
    // 防一下空指针
    if (device_id == NULL) {
        return 0; 
    }

    uint16_t safe_len = 0;
    
    spi_flash_sector_erase(FLASH_ADDR_DEVICE_ID); // 擦除之前存储的设备ID

    uint8_t buffer[FLASH_LEN_DEVICE_ID] = {0};

    while (device_id[safe_len] != '\0' && safe_len < (FLASH_LEN_DEVICE_ID - 1)) {
        safe_len++;
    }




    // 最多拷 safe_len，超长也不会越界
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
    flash_data_t data;


    flash_data_read_latest(&data);
    
    if (data.magic != FLASH_DATA_MAGIC) {
        data.magic = FLASH_DATA_MAGIC;
        data.power_count = 1;
        data.sample_cycle = 5000;
        data.ratio_ch0 = 1.0f;
        data.limit_ch0 = 100.0f;
        data.dac_volt = 0.0f;
        data.reserved = 0;

    } else {
        data.power_count += 1;
    }
    
    uint8_t ret = flash_data_append_write(&data);  // 先尝试直接追加
    if(ret == 0) {
        // 满了就 fold 一次，再写
        flash_data_fold();
        flash_data_append_write(&data);
    }
    


    power_count = data.power_count;

    return 1;
}

uint32_t spi_flash_power_count_read(void)
{
    flash_data_t data;
    
    if (flash_data_read_latest(&data)) {
        return data.power_count;
    }

    
    return 0; // 第一次上电
}

uint8_t spi_flash_sample_cycle_update(uint8_t isprintf,uint32_t cycle)
{
    flash_data_t data;
    flash_data_read_latest(&data);

    if (data.magic != FLASH_DATA_MAGIC) {

        data.magic = FLASH_DATA_MAGIC;
        data.power_count = 0;

        data.ratio_ch0 = 1.0f;
        data.limit_ch0 = 100.0f;
        data.dac_volt = 0.0f;
        data.reserved = 0;
    }
    data.sample_cycle = cycle;
    uint8_t ret = flash_data_append_write(&data);
    if(ret == 0) {
        flash_data_fold();
        flash_data_append_write(&data);
    }
    if (isprintf) {
        printf("\r\nsample cycle adjusted: %ds\r\n", cycle/1000);
        sample_result_show();
    }
    adc_sample_cycle = cycle;
    adc_sample_start = 0;
    return 1;
}

uint32_t spi_flash_sample_cycle_read(void)
{
    flash_data_t data;
    
    if (flash_data_read_latest(&data) && data.magic == FLASH_DATA_MAGIC) {
        return data.sample_cycle;
    }
    
    return 5000; // 默认 5s
}




uint8_t spi_ratio_limit_read(float *r, float *l)
{
    flash_data_t data;
    
    if (flash_data_read_latest(&data) && data.magic == FLASH_DATA_MAGIC) {
        *r = data.ratio_ch0;
        *l = data.limit_ch0;
        return 1;
    }
    
    *r = 1.0f;
    *l = 100.0f;
    
    data.magic = FLASH_DATA_MAGIC;
    data.power_count = 0;
    data.sample_cycle = 5000;
    data.ratio_ch0 = 1.0f;
    data.limit_ch0 = 100.0f;
    data.dac_volt = 0.0f;
    data.reserved = 0;
    
    flash_data_append_write(&data); // 顺手落一次默认值
    return 0;
}

uint8_t spi_ratio_limit_write(float r, float l)
{
    flash_data_t data;
    flash_data_read_latest(&data);
    
    if (data.magic != FLASH_DATA_MAGIC) {
        data.magic = FLASH_DATA_MAGIC;
        data.power_count = 0;
        data.sample_cycle = 5000;
        data.dac_volt = 0.0f;
        data.reserved = 0;
    }
    
    data.ratio_ch0 = r;
    data.limit_ch0 = l;
    
    uint8_t ret = flash_data_append_write(&data);
    if(ret == 0) {
        flash_data_fold();
        flash_data_append_write(&data);
    }
    
    return 1;
}

uint8_t spi_flash_dac_update(float dac_volt)
{
    flash_data_t data;
    flash_data_read_latest(&data);
    
    if (data.magic != FLASH_DATA_MAGIC) {
        data.magic = FLASH_DATA_MAGIC;
        data.power_count = 0;
        data.sample_cycle = 5000;
        data.ratio_ch0 = 1.0f;
        data.limit_ch0 = 100.0f;
        data.reserved = 0;
    }
    data.dac_volt = dac_volt;
    
    uint8_t ret = flash_data_append_write(&data);
    if(ret == 0) {
        flash_data_fold();
        flash_data_append_write(&data);
    }
    
    return 1;
}

float spi_flash_dac_volt_read(void)
{
    flash_data_t data;
    
    if (flash_data_read_latest(&data) && data.magic == FLASH_DATA_MAGIC) {
        return data.dac_volt;
    }
    
    return 0.0f; // 初始状态
}

void spi_flash_erase(void)
{
    systick_config();     // 时钟初始化
    spi_flash_init();     // SPI FLASH初始化
    spi_flash_bulk_erase(); // 擦除整个Flash
}

/* --------------//append/fold 底层 -------------------- */

static uint16_t flash_find_next_write_offset(uint32_t addr, uint16_t record_size)
{
    uint8_t buf[4];
    uint32_t magic;
    
    for (uint16_t offset = 0; offset < SPI_FLASH_SECTOR_SIZE; offset += record_size) {
        spi_flash_buffer_read(buf, addr + offset, 4);
        memcpy(&magic, buf, 4);
        
        if (magic == FLASH_EMPTY_MAGIC) {
            return offset;
        }
    }
    
    return SPI_FLASH_SECTOR_SIZE;
}

static int16_t flash_find_last_valid_offset(uint32_t addr, uint16_t record_size, uint32_t expected_magic)
{
    uint8_t buf[4];
    uint32_t magic;
    
    for (int16_t offset = SPI_FLASH_SECTOR_SIZE - record_size; offset >= 0; offset -= record_size) {
        spi_flash_buffer_read(buf, addr + offset, 4);
        memcpy(&magic, buf, 4);
        
        if (magic == expected_magic) {
            return offset;
        }
    }
    
    return -1;
}

/* flash_data 参数区（0x1000） */
uint8_t flash_data_append_write(const flash_data_t *data)
{
    if (data == NULL) return 0;
    
    uint16_t offset = flash_find_next_write_offset(FLASH_ADDR_FLASH_DATA, SPI_FLASH_RECORD_SIZE);
    
    if (offset >= SPI_FLASH_SECTOR_SIZE) {
        return 0;
    }
    
    uint8_t buf[SPI_FLASH_RECORD_SIZE];
    memset(buf, 0xFF, SPI_FLASH_RECORD_SIZE);
    memcpy(buf, data, sizeof(flash_data_t));
    
    spi_flash_page_write(buf, FLASH_ADDR_FLASH_DATA + offset, SPI_FLASH_RECORD_SIZE);
    
    return 1;
}

uint8_t flash_data_read_latest(flash_data_t *data)
{
    if (data == NULL) return 0;
    
    int16_t offset = flash_find_last_valid_offset(FLASH_ADDR_FLASH_DATA, SPI_FLASH_RECORD_SIZE, FLASH_DATA_MAGIC);
    
    if (offset < 0) {
        memset(data, 0, sizeof(flash_data_t));
        return 0; // 全是FF
    }
    
    uint8_t buf[SPI_FLASH_RECORD_SIZE];
    spi_flash_buffer_read(buf, FLASH_ADDR_FLASH_DATA + offset, SPI_FLASH_RECORD_SIZE);
    memcpy(data, buf, sizeof(flash_data_t));
    
    return 1;
}

uint8_t flash_data_fold(void)
{
    flash_data_t latest_data;
    
    if (!flash_data_read_latest(&latest_data)) {
        spi_flash_sector_erase(FLASH_ADDR_FLASH_DATA);
        return 1;
    }
    
    spi_flash_sector_erase(FLASH_ADDR_FLASH_DATA); // 整扇区清掉
    
    uint8_t buf[SPI_FLASH_RECORD_SIZE];
    memset(buf, 0xFF, SPI_FLASH_RECORD_SIZE);
    memcpy(buf, &latest_data, sizeof(flash_data_t));
    
    spi_flash_page_write(buf, FLASH_ADDR_FLASH_DATA, SPI_FLASH_RECORD_SIZE);
    
    return 1;
}
////////////////////////////////////////////////////////log//////////////////////////////////////////////////////////
/* 这些变量在 file_mgr.c 里定义，这里只声明 */
extern uint16_t g_sample_line;
extern char g_sample_file[64];
extern uint16_t g_over_line;
extern char g_over_file[64];
extern uint16_t g_hide_line;
extern char g_hide_file[64];
//log 状态区（0x2000）
uint8_t log_record_append_write(const log_record_t *record)
{
    if (record == NULL) return 0;
    
    uint16_t offset = flash_find_next_write_offset(FLASH_ADDR_LOG_STATE, SPI_FLASH_RECORD_SIZE);
    
    if (offset >= SPI_FLASH_SECTOR_SIZE) {
        return 0;
    }
    
    uint8_t buf[SPI_FLASH_RECORD_SIZE];
    memset(buf, 0xFF, SPI_FLASH_RECORD_SIZE);
    memcpy(buf, record, sizeof(log_record_t));
    
    spi_flash_page_write(buf, FLASH_ADDR_LOG_STATE + offset, SPI_FLASH_RECORD_SIZE);
    
    return 1;
}

uint8_t log_record_read_latest(uint32_t magic, log_record_t *record)
{
    if (record == NULL) return 0;
    
    uint8_t buf[4];
    uint32_t read_magic;
    
    for (int16_t offset = SPI_FLASH_SECTOR_SIZE - SPI_FLASH_RECORD_SIZE; offset >= 0; offset -= SPI_FLASH_RECORD_SIZE) {
        spi_flash_buffer_read(buf, FLASH_ADDR_LOG_STATE + offset, 4);
        memcpy(&read_magic, buf, 4);
        
        if (read_magic == magic) {
            spi_flash_buffer_read(buf, FLASH_ADDR_LOG_STATE + offset, SPI_FLASH_RECORD_SIZE);
            memcpy(record, buf, sizeof(log_record_t));
            return 1;
        }
    }
    
    memset(record, 0, sizeof(log_record_t));
    return 0; // 这类记录还没有
}

uint8_t log_record_fold(void)
{
    log_record_t sample_rec, overlimit_rec, hidedata_rec;
    uint8_t has_sample = log_record_read_latest(LOG_SAMPLE_MAGIC, &sample_rec);
    uint8_t has_overlimit = log_record_read_latest(LOG_OVERLIMIT_MAGIC, &overlimit_rec);
    uint8_t has_hidedata = log_record_read_latest(LOG_HIDEDATA_MAGIC, &hidedata_rec);
    
    spi_flash_sector_erase(FLASH_ADDR_LOG_STATE); // 先擦，再把最后状态写回去
    
    uint16_t offset = 0;
    uint8_t buf[SPI_FLASH_RECORD_SIZE];
    
    if (has_sample) {
        memset(buf, 0xFF, SPI_FLASH_RECORD_SIZE);
        memcpy(buf, &sample_rec, sizeof(log_record_t));
        spi_flash_page_write(buf, FLASH_ADDR_LOG_STATE + offset, SPI_FLASH_RECORD_SIZE);
        offset += SPI_FLASH_RECORD_SIZE;
    }
    if (has_overlimit) {
        memset(buf, 0xFF, SPI_FLASH_RECORD_SIZE);
        memcpy(buf, &overlimit_rec, sizeof(log_record_t));
        spi_flash_page_write(buf, FLASH_ADDR_LOG_STATE + offset, SPI_FLASH_RECORD_SIZE);
        offset += SPI_FLASH_RECORD_SIZE;
    }
    if (has_hidedata) {
        memset(buf, 0xFF, SPI_FLASH_RECORD_SIZE);
        memcpy(buf, &hidedata_rec, sizeof(log_record_t));
        spi_flash_page_write(buf, FLASH_ADDR_LOG_STATE + offset, SPI_FLASH_RECORD_SIZE);
        offset += SPI_FLASH_RECORD_SIZE;
    }
    return 1;
}

//日志状态保存
void log_states_save_all(void)
{
    log_states_save_sample();
    log_states_save_over();
    log_states_save_hide();
}

void log_states_save_sample(void)
{
    log_record_t record;
    record.magic = LOG_SAMPLE_MAGIC;
    record.line = g_sample_line;
    record.reserved = 0;
    strncpy(record.filename, g_sample_file, 24);
    record.filename[23] = '\0';
    
    uint8_t ret = log_record_append_write(&record);
    if(ret == 0) {
        log_record_fold();
        log_record_append_write(&record);
    }
}

void log_states_save_over(void)
{
    log_record_t record;
    record.magic = LOG_OVERLIMIT_MAGIC;
    record.line = g_over_line;
    record.reserved = 0;
    strncpy(record.filename, g_over_file, 24);
    record.filename[23] = '\0';
    
    uint8_t ret = log_record_append_write(&record);
    if(ret == 0) {
        log_record_fold();
        log_record_append_write(&record);
    }
}

void log_states_save_hide(void)
{
    log_record_t record;
    record.magic = LOG_HIDEDATA_MAGIC;
    record.line = g_hide_line;
    record.reserved = 0;
    strncpy(record.filename, g_hide_file, 24);
    record.filename[23] = '\0';
    
    uint8_t ret = log_record_append_write(&record);
    if(ret == 0) {
        log_record_fold();
        log_record_append_write(&record);
    }
}

void log_states_load_all(void)
{
    log_record_t record;
    
    if (log_record_read_latest(LOG_SAMPLE_MAGIC, &record)) {
        g_sample_line = record.line;
        strncpy(g_sample_file, record.filename, 64);
        g_sample_file[63] = '\0';
    } else {
        g_sample_line = 0;
        memset(g_sample_file, 0, 64);
    }
    
    if (log_record_read_latest(LOG_OVERLIMIT_MAGIC, &record)) {
        g_over_line = record.line;
        strncpy(g_over_file, record.filename, 64);
        g_over_file[63] = '\0';
    } else {
        g_over_line = 0;
        memset(g_over_file, 0, 64);
    }
    
    if (log_record_read_latest(LOG_HIDEDATA_MAGIC, &record)) {
        g_hide_line = record.line;
        strncpy(g_hide_file, record.filename, 64);
        g_hide_file[63] = '\0';
    } else {
        g_hide_line = 0;
        memset(g_hide_file, 0, 64);
    }
}

