#include "myflashdb_data.h"

struct fdb_kvdb flashdata_db;
uint8_t exflash_erase_flag = 0;

static void data_config_default(data_cfg_t *cfg)
{
    if (cfg == NULL) return;

    cfg->sample_cycle = 5000U;
    cfg->device_id    = 0x0001U;
    cfg->ratio_ch0    = 1.0f;
    cfg->ratio_ch1    = 1.0f;
    cfg->limit_ch0    = 100.0f;
    cfg->limit_ch1    = 100.0f;
    cfg->dac_volt     = 1.0f;
}


//上下限合理性检查
static void data_config_check(data_cfg_t *cfg)
{
    if (cfg == NULL) return;

    if (cfg->sample_cycle < 1000U || cfg->sample_cycle > 600000U) {
        cfg->sample_cycle = 5000U;
    }
    if ((cfg->device_id == 0U) || (cfg->device_id == 0xFFFFU)) {
        cfg->device_id = 0x0001U;
    }
    if (cfg->ratio_ch0 < 0.0f || cfg->ratio_ch0 > 100.0f) {
        cfg->ratio_ch0 = 1.0f;
    }
    if (cfg->limit_ch0 < 0.0f || cfg->limit_ch0 > 500.0f) {
        cfg->limit_ch0 = 100.0f;
    }
    if (cfg->ratio_ch1 < 0.0f || cfg->ratio_ch1 > 100.0f) {
        cfg->ratio_ch1 = 1.0f;
    }
    if (cfg->limit_ch1 < 0.0f || cfg->limit_ch1 > 500.0f) {
        cfg->limit_ch1 = 100.0f;
    }
    if (cfg->dac_volt < 0.0f || cfg->dac_volt > 3.0f) {
        cfg->dac_volt = 1.0f;
    }
}

int flashdata_init(void)
{
    fdb_err_t err;

    err = fdb_kvdb_init(&flashdata_db, "env_param", "fdb_kvdb1", NULL, NULL);

    if (err != FDB_NO_ERR) {
        printf("[Error] Param KVDB init failed! code=%d\n", err);
        return -1;
    }
    return 0;
}


void set_team_number(const char *team_str) {
    if (team_str == NULL) return;

    // 捕获返回值并打印
    fdb_err_t err = fdb_kv_set(&flashdata_db, TEAM_NUM_KEY, team_str);
    if (err != FDB_NO_ERR) {
        printf("[FDB_ERR] fdb_kv_set failed! Error code: %d\r\n", err);
    }
}

void get_team_number(char *out_str, int max_len) {
    if (out_str == NULL || max_len <= 0) return;

    strncpy(out_str, "DEFAULT_TEAM", max_len - 1);
    out_str[max_len - 1] = '\0';

    char *val = fdb_kv_get(&flashdata_db, TEAM_NUM_KEY);
    if (val != NULL) {
        strncpy(out_str, val, max_len - 1);
        out_str[max_len - 1] = '\0';
    }
}


uint32_t get_power_count(void) {
    uint32_t count = 0;
    struct fdb_blob blob;
    size_t read_len;


    fdb_blob_make(&blob, &count, sizeof(count));
    read_len = fdb_kv_get_blob(&flashdata_db, POWER_CNT_KEY, &blob);
    if (read_len != sizeof(count)) {
        count = 0;
    }

    return count;
}

void set_power_count(void)
{
    uint32_t current = get_power_count();
    if (current == 0xFFFFFFFF)
    {
        current = 0;
    }
    else
    {
        current++;
    }
    struct fdb_blob blob;
    fdb_blob_make(&blob, &current, sizeof(current));
    fdb_kv_set_blob(&flashdata_db, POWER_CNT_KEY, &blob);
}


void get_data_config(data_cfg_t *out_cfg)
{
    if (out_cfg == NULL) return;
    struct fdb_blob blob;
    size_t read_len;
//初始化数据
    data_config_default(out_cfg);

//flash读取
    fdb_blob_make(&blob, out_cfg, sizeof(data_cfg_t));
    read_len = fdb_kv_get_blob(&flashdata_db, DATA_CFG_KEY, &blob);
    if (read_len != sizeof(data_cfg_t)) {
        data_config_default(out_cfg);
        return;
    }
    data_config_check(out_cfg);
}

void set_data_config(data_cfg_t *cfg) {
    if (cfg == NULL) return;
    struct fdb_blob blob;
    data_config_check(cfg);
    fdb_blob_make(&blob, cfg, sizeof(data_cfg_t));
    fdb_kv_set_blob(&flashdata_db, DATA_CFG_KEY, &blob);
}

void flashdata_clear(void) {
    fdb_kv_set_default(&flashdata_db);
}
