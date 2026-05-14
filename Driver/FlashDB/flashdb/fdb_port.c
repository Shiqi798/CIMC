#include <flashdb.h>
#include <fdb_def.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

/*
 * 裸机单线程环境下，FlashDB 操作不需要全局关中断。
 * 若关闭中断，串口打印/延时等依赖时基的流程可能阻塞，表现为“卡死”。
 */
void fdb_port_lock(fdb_db_t db) {
    (void)db;
}

void fdb_port_unlock(fdb_db_t db) {
    (void)db;
}

/* 获取当前时间戳 (TSDB 需要记录每条日志的时间) */
fdb_time_t fdb_port_get_time(void) {
    /* 赛题有要求 UNIX 时间戳。
       如果你目前还没有写好 RTC 代码，可以先暂时返回 0，或者返回系统的运行节拍（如 HAL_GetTick() / 1000）让它跑通。
       等你把 RTC 搞定后，在这里返回真正的 UNIX 时间戳即可。*/
    return 0; 
}

/* 打印输出接口 */
void fdb_print(const char *format, ...) {
    /* 如果你的单片机 printf 已经重定向好了，这里直接透传即可 */
    va_list args;
    va_start(args, format);
    vprintf(format, args); 
    va_end(args);
}
