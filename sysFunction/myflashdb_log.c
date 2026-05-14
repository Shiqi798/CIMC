#include "myflashdb_log.h"
/* ================= 1. 底层数据库对象 ================= */

struct fdb_tsdb sample_db; // 采样日志库
struct fdb_tsdb over_db;   // 超限日志库
struct fdb_tsdb hide_db;   // 隐藏日志库
struct fdb_tsdb normal_db; // 操作日志库

/* ================= 2. 缓存结构体与上下文定义 ================= */

/* 2.1 采样日志缓存 */
typedef struct {
    fdb_time_t time;
    sample_log_t data;
} sample_cache_t;

typedef struct {
    sample_cache_t *cache;
    int max_count;
    int count;
} sample_ctx_t;

/* 2.2 超限日志缓存 */
typedef struct {
    fdb_time_t time;
    over_log_t data;
} over_cache_t;

typedef struct {
    over_cache_t *cache;
    int max_count;
    int count;
} over_ctx_t;

/* 2.3 隐藏日志缓存 */
typedef struct {
    fdb_time_t time;
    hide_log_t data;
} hide_cache_t;

typedef struct {
    hide_cache_t *cache;
    int max_count;
    int count;
} hide_ctx_t;

/* 2.4 操作日志缓存 (字符串专版) */
typedef struct {
    fdb_time_t time;
    char str_data[MAX_OP_STR_LEN];
} op_cache_t;

typedef struct {
    op_cache_t *cache;
    int max_count;
    int count;
} op_ctx_t;


/* 请确保你的工程中实现了这个函数以提供 Unix 时间戳 */
extern uint32_t get_unix_time(void);

static fdb_time_t get_current_time(void) {
    return (fdb_time_t)get_unix_time(); 
}

/* ================= 4. 初始化业务 ================= */

int flash_log_init(void) {
    fdb_err_t err;

    err  = fdb_tsdb_init(&sample_db, "sample", "fdb_tsdb1", get_current_time, sizeof(sample_log_t), NULL);
    err |= fdb_tsdb_init(&over_db,   "over",   "fdb_tsdb2", get_current_time, sizeof(over_log_t),   NULL);
    err |= fdb_tsdb_init(&hide_db,   "hide",   "fdb_tsdb3", get_current_time, sizeof(hide_log_t),   NULL);
    
    /* 注意：操作日志的 max_len 传入的是定义的字符串最大长度 */
    err |= fdb_tsdb_init(&normal_db, "normal", "fdb_tsdb4", get_current_time, MAX_OP_STR_LEN,       NULL);

    if (err != FDB_NO_ERR) {
        printf("FlashDB 数据库初始化失败!\n");
        return -1;
    }
    printf("FlashDB 4个日志数据库初始化成功!\n");
    return 0;
}

/* ================= 5. 写日志接口 ================= */

void append_sample_log(float voltage, float ratio, float sample_cycle) {
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

void append_normal_log(const char *fmt, ...) {
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

/* ================= 6. 读取与打印接口 ================= */

/* ---------- 6.1 采样日志 ---------- */
static bool query_latest_sample_cb(fdb_tsl_t tsl, void *arg) {
    sample_ctx_t *ctx = (sample_ctx_t *)arg;
    struct fdb_blob blob;

    if (tsl->status == FDB_TSL_USER_STATUS1) {
        ctx->cache[ctx->count].time = tsl->time;
        fdb_blob_make(&blob, &ctx->cache[ctx->count].data, sizeof(sample_log_t));
        fdb_blob_read((fdb_db_t)&sample_db, fdb_tsl_to_blob(tsl, &blob));
        
        ctx->count++; 
        if (ctx->count >= ctx->max_count) return true; 
    }
    return false; 
}

void print_latest_sample_logs(int count) {
    if (count <= 0) return;
    
    sample_cache_t buffer[count]; 
    sample_ctx_t ctx = {buffer, count, 0}; 

    fdb_tsl_iter_reverse(&sample_db, query_latest_sample_cb, &ctx);

    printf("--- 最新 %d 条 [采样日志] (实际读出 %d 条) ---\n", count, ctx.count);
    for (int i = ctx.count - 1; i >= 0; i--) {
        printf("[时间戳: %u] 电压: %.2fV, 比例: %.2f, 周期: %.2fms\n", 
               ctx.cache[i].time, 
               ctx.cache[i].data.voltage, 
               ctx.cache[i].data.ratio, 
               ctx.cache[i].data.sample_cycle);
    }
}

/* ---------- 6.2 超限日志 ---------- */
static bool query_latest_over_cb(fdb_tsl_t tsl, void *arg) {
    over_ctx_t *ctx = (over_ctx_t *)arg;
    struct fdb_blob blob;

    if (tsl->status == FDB_TSL_USER_STATUS1) {
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

    printf("--- 最新 %d 条 [超限日志] (实际读出 %d 条) ---\n", count, ctx.count);
    for (int i = ctx.count - 1; i >= 0; i--) {
        printf("[时间戳: %u] 超限电压: %.2fV, 设定的阈值: %.2fV\n", 
               ctx.cache[i].time, 
               ctx.cache[i].data.voltage, 
               ctx.cache[i].data.limit);
    }
}

/* ---------- 6.3 隐藏日志 ---------- */
static bool query_latest_hide_cb(fdb_tsl_t tsl, void *arg) {
    hide_ctx_t *ctx = (hide_ctx_t *)arg;
    struct fdb_blob blob;

    if (tsl->status == FDB_TSL_USER_STATUS1) {
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

    printf("--- 最新 %d 条 [隐藏日志] (实际读出 %d 条) ---\n", count, ctx.count);
    for (int i = ctx.count - 1; i >= 0; i--) {
        printf("[时间戳: %u] 电压: %.2fV, 隐藏数据: %s\n", 
               ctx.cache[i].time, 
               ctx.cache[i].data.voltage, 
               ctx.cache[i].data.hide_data);
    }
}

/* ---------- 6.4 操作日志 (字符串专版) ---------- */
static bool query_latest_op_cb(fdb_tsl_t tsl, void *arg) {
    op_ctx_t *ctx = (op_ctx_t *)arg;
    struct fdb_blob blob;

    if (tsl->status == FDB_TSL_USER_STATUS1) {
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

    printf("--- 最新 %d 条 [操作日志] (实际读出 %d 条) ---\n", count, ctx.count);
    for (int i = ctx.count - 1; i >= 0; i--) {
        printf("[时间戳: %u] %s\n", 
               ctx.cache[i].time, 
               ctx.cache[i].str_data);
    }
}

/*************************清空********************************* */

void clear_all_sample_logs(void) 
{ 
    fdb_tsl_clean(&sample_db);
//     printf("采样日志已清空!\n"); 
}
void clear_all_over_logs(void)   
{ 
    fdb_tsl_clean(&over_db);   
//    printf("超限日志已清空!\n"); 
}
void clear_all_hide_logs(void)   
{ 
    fdb_tsl_clean(&hide_db);  
//     printf("隐藏日志已清空!\n"); 
}
void clear_all_normal_logs(void) 
{ 
    fdb_tsl_clean(&normal_db); 
 //   printf("操作日志已清空!\n"); 
}