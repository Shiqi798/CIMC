#include "myflashdb_log.h"
//--------------底层数据库对象--------------------------------
struct fdb_tsdb sample_db;
struct fdb_tsdb over_db;
struct fdb_tsdb hide_db;   
struct fdb_tsdb normal_db; // 操作日志库

////////////////jiegouti//////////////////////////////

//采样
typedef struct {
    fdb_time_t time;
    sample_log_t data;
} sample_cache_t;

typedef struct {
    sample_cache_t *cache;
    int max_count;
    int count;
} sample_ctx_t;
//over
typedef struct {
    fdb_time_t time;
    over_log_t data;

} over_cache_t;

typedef struct {
    over_cache_t *cache;
    int max_count;
    int count;
} over_ctx_t;

//hide
typedef struct {
    fdb_time_t time;
    hide_log_t data;
} hide_cache_t;

typedef struct {
    hide_cache_t *cache;
    int max_count;
    int count;
} hide_ctx_t;

//normal
typedef struct {
    fdb_time_t time;
    char str_data[MAX_OP_STR_LEN];


} op_cache_t;

typedef struct {
    op_cache_t *cache;
    int max_count;
    int count;
} op_ctx_t;


//////////////////////////时间戳和获取/////////////////////////////////////
extern uint32_t get_unix_time(void);

static fdb_time_t get_current_time(void) {
    // 静态变量，记住上一次存入 FlashDB 的时间
    static fdb_time_t last_time = 0; 
    
    // 获取当前 RTC 真实时间
    fdb_time_t current_time = (fdb_time_t)get_unix_time(); 
    if (current_time <= last_time) {
        current_time = last_time + 1;
    }
    
    // 更新记忆
    last_time = current_time;
    
    return current_time; 
}

//初始化
int flash_log_init(void) {
    fdb_err_t err;

    err  = fdb_tsdb_init(&sample_db, "sample", "sample_log", get_current_time, sizeof(sample_log_t), NULL);
    err |= fdb_tsdb_init(&over_db,   "over",   "over_log",   get_current_time, sizeof(over_log_t),   NULL);
    err |= fdb_tsdb_init(&hide_db,   "hide",   "hide_log",   get_current_time, sizeof(hide_log_t),   NULL);
    
    //MAX_OP_STR_LEN-日志最大长度
    err |= fdb_tsdb_init(&normal_db, "normal", "op_log",     get_current_time, MAX_OP_STR_LEN,       NULL);

    if (err != FDB_NO_ERR) {
        printf("FlashDB 数据库初始化失败! code: %d\n", err);
        return -1;
    }
//    printf("FlashDB 4个日志数据库初始化成功!\n");
    return 0;
}


////////////////////////////////////写///////////////////////////////////////////////
void append_sample_log(float voltage, float ratio, uint16_t sample_cycle) {
    struct fdb_blob blob;
    sample_log_t log_data = {voltage, ratio, sample_cycle};
    fdb_blob_make(&blob, &log_data, sizeof(log_data));
    fdb_tsl_append(&sample_db, &blob);
}

void append_over_log(float voltage, float limit) {
    struct fdb_blob blob;
    over_log_t log_data = {voltage, limit};
    fdb_blob_make(&blob, &log_data, sizeof(log_data));
    fdb_tsl_append(&over_db, &blob);
}

void append_hide_log(float voltage, const char* hide_str) {
    struct fdb_blob blob;
    hide_log_t log_data;
    log_data.voltage = voltage;
    strncpy(log_data.hide_data, hide_str, sizeof(log_data.hide_data) - 1);
    log_data.hide_data[sizeof(log_data.hide_data) - 1] = '\0';
    fdb_blob_make(&blob, &log_data, sizeof(log_data));
    fdb_tsl_append(&hide_db, &blob);
}

void append_normal_log(const char *fmt, ...) 
{
    rtc_current_time_get(&rtc_initpara);
    char log_buf[MAX_OP_STR_LEN];
    struct fdb_blob blob;
    va_list args;
    
    va_start(args, fmt);
    vsnprintf(log_buf, MAX_OP_STR_LEN, fmt, args);
    va_end(args);

    log_buf[MAX_OP_STR_LEN - 1] = '\0'; 
    size_t len = strlen(log_buf) + 1;

    fdb_blob_make(&blob, log_buf, len);
    fdb_tsl_append(&normal_db, &blob);
}

//////////////////////////////////////读取打印///////////////////////////////////////

//暂存新日志，，反写
static bool query_latest_sample_cb(fdb_tsl_t tsl, void *arg) {
    sample_ctx_t *ctx = (sample_ctx_t *)arg;
    struct fdb_blob blob;

    if (tsl->status == FDB_TSL_WRITE) {
        ctx->cache[ctx->count].time = tsl->time;
        fdb_blob_make(&blob, &ctx->cache[ctx->count].data, sizeof(sample_log_t));
        fdb_blob_read((fdb_db_t)&sample_db, fdb_tsl_to_blob(tsl, &blob));
        
        ctx->count++; 
        if (ctx->count >= ctx->max_count) return true; 
    }
    return false; 
}
//采样
void print_latest_sample_logs(int count) {
    if (count <= 0) return;
    
    sample_cache_t buffer[count]; 
    sample_ctx_t ctx = {buffer, count, 0}; 
    fdb_tsl_iter_reverse(&sample_db, query_latest_sample_cb, &ctx);
    printf("\r\n");
//    printf("\r\n 最新 %d 条 [采样日志] (实际读出 %d 条)\r\n", count, ctx.count);
    for (int i = ctx.count - 1; i >= 0; i--) {
        printf("[%d]%s ch0=%.2fV ratio=%.2f %ds\r\n", ctx.count - i,
               unix_to_str(ctx.cache[i].time), 
               ctx.cache[i].data.voltage, 
               ctx.cache[i].data.ratio, 
               ctx.cache[i].data.sample_cycle/1000);
    }
}

///////over/////////////////
static bool query_latest_over_cb(fdb_tsl_t tsl, void *arg) {
    over_ctx_t *ctx = (over_ctx_t *)arg;
    struct fdb_blob blob;

    if (tsl->status == FDB_TSL_WRITE) {
        ctx->cache[ctx->count].time = tsl->time;
        fdb_blob_make(&blob, &ctx->cache[ctx->count].data, sizeof(over_log_t));
        fdb_blob_read((fdb_db_t)&over_db, fdb_tsl_to_blob(tsl, &blob));
        
        ctx->count++; 
        if (ctx->count >= ctx->max_count) return true; 
    }
    return false; 
}
void print_latest_over_logs(int count) {
    if (count <= 0) return;
    
    over_cache_t buffer[count];
    over_ctx_t ctx = {buffer, count, 0}; 

    fdb_tsl_iter_reverse(&over_db, query_latest_over_cb, &ctx);
    printf("\r\n");
//    printf("--- 最新 %d 条 [超限日志] (实际读出 %d 条) ---\n", count, ctx.count);
    printf("\r\n");
    for (int i = ctx.count - 1; i >= 0; i--) {
        printf("[%d]%s ch0=%.2fV limit=%.2fV\r\n", ctx.count - i,
               unix_to_str(ctx.cache[i].time),ctx.cache[i].data.voltage, ctx.cache[i].data.limit);
    }
}

//hide
static bool query_latest_hide_cb(fdb_tsl_t tsl, void *arg) {
    hide_ctx_t *ctx = (hide_ctx_t *)arg;
    struct fdb_blob blob;

    if (tsl->status == FDB_TSL_WRITE) {
        ctx->cache[ctx->count].time = tsl->time;
        fdb_blob_make(&blob, &ctx->cache[ctx->count].data, sizeof(hide_log_t));
        fdb_blob_read((fdb_db_t)&hide_db, fdb_tsl_to_blob(tsl, &blob));
        
        ctx->count++; 
        if (ctx->count >= ctx->max_count) return true; 
    }
    return false; 
}

void print_latest_hide_logs(int count) {
    if (count <= 0) return;
    
    hide_cache_t buffer[count];
    hide_ctx_t ctx = {buffer, count, 0}; 

    fdb_tsl_iter_reverse(&hide_db, query_latest_hide_cb, &ctx);
    printf("\r\n");
    for (int i = ctx.count - 1; i >= 0; i--) {
        printf("[%d]%s ch0=%.2fV hide_data=%s\r\n", ctx.count - i,
               unix_to_str(ctx.cache[i].time),ctx.cache[i].data.voltage, ctx.cache[i].data.hide_data);
    }
}

//操作日志
static bool query_latest_op_cb(fdb_tsl_t tsl, void *arg) {
    op_ctx_t *ctx = (op_ctx_t *)arg;
    struct fdb_blob blob;

    if (tsl->status == FDB_TSL_WRITE) {
        ctx->cache[ctx->count].time = tsl->time;
        
        fdb_blob_make(&blob, ctx->cache[ctx->count].str_data, MAX_OP_STR_LEN);
        fdb_blob_read((fdb_db_t)&normal_db, fdb_tsl_to_blob(tsl, &blob));
        
        ctx->cache[ctx->count].str_data[MAX_OP_STR_LEN - 1] = '\0';
        ctx->count++; 
        if (ctx->count >= ctx->max_count) return true; 
    }
    return false; 
}

void print_latest_normal_logs(int count) {
    if (count <= 0) return;
    
    op_cache_t buffer[count];
    op_ctx_t ctx = {buffer, count, 0}; 

    fdb_tsl_iter_reverse(&normal_db, query_latest_op_cb, &ctx);
//    printf("读出 %d 条\r\n", ctx.count);
    printf("\r\n");
    for (int i = ctx.count - 1; i >= 0; i--) {
        printf("[%d]%s %s\r\n", ctx.count - i,
               unix_to_str(ctx.cache[i].time),
               ctx.cache[i].str_data);
    }
}

/*************************清空********************************* */

void clear_all_sample_logs(void) 
{ 
    fdb_tsl_clean(&sample_db);
//     printf("采样日志已清空!\r\n"); 
}
void clear_all_over_logs(void)   
{ 
    fdb_tsl_clean(&over_db);   
}
void clear_all_hide_logs(void)   
{ 
    fdb_tsl_clean(&hide_db);  
}
void clear_all_normal_logs(void) 
{ 
    fdb_tsl_clean(&normal_db); 
}

void clear_all_logs(void)
{
    clear_all_sample_logs();
    clear_all_over_logs();
    clear_all_hide_logs();
    clear_all_normal_logs();
}
