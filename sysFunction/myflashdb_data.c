#include "myflashdb_data.h"

struct fdb_kvdb flashdata_db;

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
    fdb_kv_set(&flashdata_db, TEAM_NUM_KEY, team_str);
}

void get_team_number(char *out_str, int max_len) {
    strncpy(out_str, "DEFAULT_TEAM", max_len); 
    
    char *val = fdb_kv_get(&flashdata_db, TEAM_NUM_KEY);
    if (val != NULL) {
        strncpy(out_str, val, max_len);
    }
}


uint32_t get_power_count(void) {
    uint32_t count;
    struct fdb_blob blob;


    fdb_blob_make(&blob, &count, sizeof(count));
    fdb_kv_get_blob(&flashdata_db, POWER_CNT_KEY, &blob);

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


void get_data_config(data_cfg_t *out_cfg) {
    if (out_cfg == NULL) return;
    struct fdb_blob blob;
//    size_t read_len;
//初始化数据
    out_cfg->sample_cycle = 5000; 
    out_cfg->ratio_ch0    = 1.0f;     
    out_cfg->limit_ch0    = 10.0f;            
    out_cfg->dac_volt     = 0.0f;

//flash读取
    fdb_blob_make(&blob, out_cfg, sizeof(data_cfg_t));
    //read_len = fdb_kv_get_blob(&flashdata_db, DATA_CFG_KEY, &blob);
    fdb_kv_get_blob(&flashdata_db, DATA_CFG_KEY, &blob);
}

void set_data_config(data_cfg_t *cfg) {
    if (cfg == NULL) return;
    struct fdb_blob blob;
    fdb_blob_make(&blob, cfg, sizeof(data_cfg_t));
    fdb_kv_set_blob(&flashdata_db, DATA_CFG_KEY, &blob);
}

void flashdata_clear(void) {
    fdb_kv_set_default(&flashdata_db);
}
