#include "myflash.h"
#include <flashdb.h>
#include <string.h>

/*
[I/FAL] ==================== FAL partition table ====================
[I/FAL] | name       | flash_dev  |   offset   |    length  |
[I/FAL] -------------------------------------------------------------
[I/FAL] | fdb_kvdb1  | nor_flash0 | 0x00000000 | 0x00008000 |
[I/FAL] | op_log     | nor_flash0 | 0x00008000 | 0x00010000 |
[I/FAL] | sample_log | nor_flash0 | 0x00018000 | 0x00030000 |
[I/FAL] | over_log   | nor_flash0 | 0x00048000 | 0x00020000 |
[I/FAL] | hide_log   | nor_flash0 | 0x00068000 | 0x00018000 |
[I/FAL] =============================================================
[I/FAL] RT-Thread Flash Abstraction Layer (V0.5.0) initialize success.
*/

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

uint8_t flashdb_kv_demo(void)
{
    static struct fdb_kvdb kvdb;
    static uint8_t inited = 0;
    const char *key = "port_demo";
    const char *expect = "flashdb_ok";
    char *value;
    fdb_err_t err;

    /* 使用与参数区分离的分区做演示，避免覆盖 0x000000~0x002000 的自定义数据 */
    if (!inited)
    {
        err = fdb_kvdb_init(&kvdb, "kv_demo", "op_log", NULL, NULL);
        if (err != FDB_NO_ERR)
        {
            printf("[FDB DEMO] init failed: %d\r\n", err);
            return 0;
        }
        inited = 1;
        printf("[FDB DEMO] init ok\r\n");
    }

    err = fdb_kv_set(&kvdb, key, expect);
    if (err != FDB_NO_ERR)
    {
        printf("[FDB DEMO] set failed: %d\r\n", err);
        return 0;
    }

    value = fdb_kv_get(&kvdb, key);
    if (value && strcmp(value, expect) == 0)
    {
        printf("[FDB DEMO] PASS key=%s val=%s\r\n", key, value);
        return 1;
    }

    printf("[FDB DEMO] FAIL readback\r\n");
    return 0;
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

//////////////////////////////////////////////////////////////////////日志区///////////////////////////////////////////////////////////////////////
#define MAX_PRINT_LOGS 20  // 定义最多读取的最新日志条数

/* ================= 1. 数据结构定义 ================= */

/* 定义 4 个底层数据库对象 */
struct fdb_tsdb sample_db; // 采样日志库
struct fdb_tsdb over_db;   // 超限日志库
struct fdb_tsdb hide_db;   // 隐藏日志库
struct fdb_tsdb normal_db; // 操作日志库

/* 纯数据结构体 (注意：不需要再定义 time，底层会加时间戳) */
typedef struct {
    float voltage;
    float ratio;
    float sample_cycle;
} sample_log_t;

typedef struct {
    float voltage;
    float limit;
} over_log_t;

typedef struct {
    float voltage;
    char hide_data[18]; // 18字节隐藏数据
} hide_log_t;

typedef struct {
    int log_flag; 
} normal_log_t;

/* ================= 2. 缓存结构体定义 (用于读取最新N条数据) ================= */

/* 2.1 采样日志的缓存结构 */
typedef struct {
    fdb_time_t time;
    sample_log_t data;
} sample_cache_t;

typedef struct {
    sample_cache_t cache[MAX_PRINT_LOGS];
    int count;
} sample_ctx_t;

/* 2.2 超限日志的缓存结构 */
typedef struct {
    fdb_time_t time;
    over_log_t data;
} over_cache_t;

typedef struct {
    over_cache_t cache[MAX_PRINT_LOGS];
    int count;
} over_ctx_t;

/* 2.3 隐藏日志的缓存结构 */
typedef struct {
    fdb_time_t time;
    hide_log_t data;
} hide_cache_t;

typedef struct {
    hide_cache_t cache[MAX_PRINT_LOGS];
    int count;
} hide_ctx_t;

/* 2.4 操作日志的缓存结构 */
typedef struct {
    fdb_time_t time;
    normal_log_t data;
} normal_cache_t;

typedef struct {
    normal_cache_t cache[MAX_PRINT_LOGS];
    int count;
} normal_ctx_t;

/* ================= 3. 底层时间接口 ================= */

/* 假设你外部有这个函数，如果没有，请按照之前的讨论用 RTC+mktime 实现 */
extern uint32_t get_unix_time(void);

static fdb_time_t get_current_time(void) {
    return (fdb_time_t)get_unix_time(); 
}

/* ================= 4. 核心业务：初始化 ================= */

/* 
 * 上层函数: 初始化所有日志数据库
 * 必须在系统启动时调用一次
 */
int init_all_log_apps(void) {
    fdb_err_t err;

    /* 
     * 参数 2: 数据库名称 (给上层看的，不能重名)
     * 参数 3: FAL 分区名称 (底层物理分区，必须在 fal_cfg.h 中对应 4 个不同分区!)
     */
    err  = fdb_tsdb_init(&sample_db, "sample", "fdb_tsdb1", get_current_time, sizeof(sample_log_t), NULL);
    err |= fdb_tsdb_init(&over_db,   "over",   "fdb_tsdb2", get_current_time, sizeof(over_log_t),   NULL);
    err |= fdb_tsdb_init(&hide_db,   "hide",   "fdb_tsdb3", get_current_time, sizeof(hide_log_t),   NULL);
    err |= fdb_tsdb_init(&normal_db, "normal", "fdb_tsdb4", get_current_time, sizeof(normal_log_t), NULL);

    if (err != FDB_NO_ERR) {
        printf("FlashDB 数据库初始化失败!\n");
        return -1;
    }
    printf("FlashDB 4个日志数据库初始化成功!\n");
    return 0;
}

/* ================= 5. 核心业务：写日志接口 ================= */

/* 写入：采样日志 */
void append_sample_log(float voltage, float ratio, float sample_cycle) {
    struct fdb_blob blob;
    sample_log_t log_data = {voltage, ratio, sample_cycle};

    fdb_blob_make(&blob, &log_data, sizeof(log_data));
    if (fdb_tsl_append(&sample_db, &blob) != FDB_NO_ERR) {
        printf("采样日志写入失败!\n");
    }
}

/* 写入：超限日志 */
void append_over_log(float voltage, float limit) {
    struct fdb_blob blob;
    over_log_t log_data = {voltage, limit};

    fdb_blob_make(&blob, &log_data, sizeof(log_data));
    if (fdb_tsl_append(&over_db, &blob) != FDB_NO_ERR) {
        printf("超限日志写入失败!\n");
    }
}

/* 写入：隐藏日志 */
void append_hide_log(float voltage, const char* hide_str) {
    struct fdb_blob blob;
    hide_log_t log_data;
    
    log_data.voltage = voltage;
    strncpy(log_data.hide_data, hide_str, sizeof(log_data.hide_data) - 1);
    log_data.hide_data[sizeof(log_data.hide_data) - 1] = '\0';

    fdb_blob_make(&blob, &log_data, sizeof(log_data));
    if (fdb_tsl_append(&hide_db, &blob) != FDB_NO_ERR) {
        printf("隐藏日志写入失败!\n");
    }
}

/* 写入：操作日志 */
void append_normal_log(int flag) {
    struct fdb_blob blob;
    normal_log_t log_data = {flag};

    fdb_blob_make(&blob, &log_data, sizeof(log_data));
    if (fdb_tsl_append(&normal_db, &blob) != FDB_NO_ERR) {
        printf("操作日志写入失败!\n");
    }
}

/* ================= 6. 核心业务：高效读取最新 20 条日志 ================= */

/* ---------- 6.1 采样日志的读取与打印 ---------- */
static bool query_latest_sample_cb(fdb_tsl_t tsl, void *arg) {
    sample_ctx_t *ctx = (sample_ctx_t *)arg;
    struct fdb_blob blob;

    if (tsl->status == FDB_TSL_USER_STATUS1) {
        ctx->cache[ctx->count].time = tsl->time;
        fdb_blob_make(&blob, &ctx->cache[ctx->count].data, sizeof(sample_log_t));
        fdb_blob_read((fdb_db_t)&sample_db, fdb_tsl_to_blob(tsl, &blob));
        
        ctx->count++; 
        if (ctx->count >= MAX_PRINT_LOGS) return true; // 抓满立刻停止
    }
    return false; 
}

void read_latest_sample_logs(void) {
    sample_ctx_t ctx = { .count = 0 }; 

    fdb_tsl_iter_reverse(&sample_db, query_latest_sample_cb, &ctx);

    printf("--- 最新 %d 条 [采样日志] (按时间正序) ---\n", ctx.count);
    for (int i = ctx.count - 1; i >= 0; i--) {
        printf("[时间戳: %u] 电压: %.2fV, 比例: %.2f, 周期: %.2fms\n", 
               ctx.cache[i].time, 
               ctx.cache[i].data.voltage, 
               ctx.cache[i].data.ratio, 
               ctx.cache[i].data.sample_cycle);
    }
}

/* ---------- 6.2 超限日志的读取与打印 ---------- */
static bool query_latest_over_cb(fdb_tsl_t tsl, void *arg) 
{
    over_ctx_t *ctx = (over_ctx_t *)arg;
    struct fdb_blob blob;

    if (tsl->status == FDB_TSL_USER_STATUS1) {
        ctx->cache[ctx->count].time = tsl->time;
        fdb_blob_make(&blob, &ctx->cache[ctx->count].data, sizeof(over_log_t));
        fdb_blob_read((fdb_db_t)&over_db, fdb_tsl_to_blob(tsl, &blob));
        
        ctx->count++; 
        if (ctx->count >= MAX_PRINT_LOGS) return true; 
    }
    return false; 
}

void read_latest_over_logs(void) {
    over_ctx_t ctx = { .count = 0 }; 

    fdb_tsl_iter_reverse(&over_db, query_latest_over_cb, &ctx);

    printf("--- 最新 %d 条 [超限日志] (按时间正序) ---\n", ctx.count);
    for (int i = ctx.count - 1; i >= 0; i--) {
        printf("[时间戳: %u] 超限电压: %.2fV, 设定的阈值: %.2fV\n", 
               ctx.cache[i].time, 
               ctx.cache[i].data.voltage, 
               ctx.cache[i].data.limit);
    }
}

/* ---------- 6.3 隐藏日志的读取与打印 ---------- */
static bool query_latest_hide_cb(fdb_tsl_t tsl, void *arg) {
    hide_ctx_t *ctx = (hide_ctx_t *)arg;
    struct fdb_blob blob;

    if (tsl->status == FDB_TSL_USER_STATUS1) {
        ctx->cache[ctx->count].time = tsl->time;
        fdb_blob_make(&blob, &ctx->cache[ctx->count].data, sizeof(hide_log_t));
        fdb_blob_read((fdb_db_t)&hide_db, fdb_tsl_to_blob(tsl, &blob));
        
        ctx->count++; 
        if (ctx->count >= MAX_PRINT_LOGS) return true; 
    }
    return false; 
}

void read_latest_hide_logs(void) {
    hide_ctx_t ctx = { .count = 0 }; 

    fdb_tsl_iter_reverse(&hide_db, query_latest_hide_cb, &ctx);

    printf("--- 最新 %d 条 [隐藏日志] (按时间正序) ---\n", ctx.count);
    for (int i = ctx.count - 1; i >= 0; i--) {
        printf("[时间戳: %u] 电压: %.2fV, 隐藏数据: %s\n", 
               ctx.cache[i].time, 
               ctx.cache[i].data.voltage, 
               ctx.cache[i].data.hide_data);
    }
}

/* ---------- 6.4 操作日志的读取与打印 ---------- */
static bool query_latest_normal_cb(fdb_tsl_t tsl, void *arg) {
    normal_ctx_t *ctx = (normal_ctx_t *)arg;
    struct fdb_blob blob;

    if (tsl->status == FDB_TSL_USER_STATUS1) {
        ctx->cache[ctx->count].time = tsl->time;
        fdb_blob_make(&blob, &ctx->cache[ctx->count].data, sizeof(normal_log_t));
        fdb_blob_read((fdb_db_t)&normal_db, fdb_tsl_to_blob(tsl, &blob));
        
        ctx->count++; 
        if (ctx->count >= MAX_PRINT_LOGS) return true; 
    }
    return false; 
}

void read_latest_normal_logs(void) {
    normal_ctx_t ctx = { .count = 0 }; 

    fdb_tsl_iter_reverse(&normal_db, query_latest_normal_cb, &ctx);

    printf("--- 最新 %d 条 [操作日志] (按时间正序) ---\n", ctx.count);
    for (int i = ctx.count - 1; i >= 0; i--) {
        printf("[时间戳: %u] 操作标志 (Flag): %d\n", 
               ctx.cache[i].time, 
               ctx.cache[i].data.log_flag);
    }
}

/* ================= 7. 核心业务：清空日志 ================= */

/* 
 * 上层函数: 清空指定的日志区
 */
void clear_all_sample_logs(void) { fdb_tsl_clean(&sample_db); printf("采样日志已清空!\n"); }
void clear_all_over_logs(void)   { fdb_tsl_clean(&over_db);   printf("超限日志已清空!\n"); }
void clear_all_hide_logs(void)   { fdb_tsl_clean(&hide_db);   printf("隐藏日志已清空!\n"); }
void clear_all_normal_logs(void) { fdb_tsl_clean(&normal_db); printf("操作日志已清空!\n"); }
