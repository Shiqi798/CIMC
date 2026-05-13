# Append & Fold 实现验证清单与使用指南

## 文件改动清单

### ✅ 已修改文件

#### 1. `sysFunction/myflash.h` - 头文件
- [x] 重定义 Flash 地址分配
  - 新增 `FLASH_ADDR_FLASH_DATA   0x00001000` (参数数据)
  - 新增 `FLASH_ADDR_LOG_STATE    0x00002000` (日志状态)
  - 移除旧的分散地址 (POWER_COUNT, SAMPLE_CYCLE, RATIO, LIMIT)

- [x] 定义数据结构体
  - 优化 `flash_data_t` 为 32 字节
  - 创建 `log_record_t` 为 32 字节
  - 移除旧的 `log_state_t`

- [x] 定义魔数标记
  - `FLASH_DATA_MAGIC 0xA5A50001` - 参数数据
  - `LOG_SAMPLE_MAGIC 0xA5A50002` - Sample 日志
  - `LOG_OVERLIMIT_MAGIC 0xA5A50003` - Overlimit 日志
  - `LOG_HIDEDATA_MAGIC 0xA5A50004` - Hidedata 日志
  - `FLASH_EMPTY_MAGIC 0xFFFFFFFF` - 空白标记

- [x] 新增函数声明
  ```c
  // Append & Fold 核心函数
  uint8_t flash_data_append_write(const flash_data_t *data);
  uint8_t flash_data_read_latest(flash_data_t *data);
  uint8_t flash_data_fold(void);
  
  uint8_t log_record_append_write(const log_record_t *record);
  uint8_t log_record_read_latest(uint32_t magic, log_record_t *record);
  uint8_t log_record_fold(void);
  ```

#### 2. `sysFunction/myflash.c` - 实现文件
- [x] 添加外部变量声明
  ```c
  extern uint16_t g_sample_line;
  extern char g_sample_file[64];
  extern uint16_t g_over_line;
  extern char g_over_file[64];
  extern uint16_t g_hide_line;
  extern char g_hide_file[64];
  ```

- [x] 实现辅助函数
  - `flash_find_next_write_offset()` - 扫描下一个可写位置
  - `flash_find_last_valid_offset()` - 反向扫描最新记录

- [x] 实现 flash_data_t Append & Fold (共 3 个函数)
  - `flash_data_append_write()` - 追加写入参数
  - `flash_data_read_latest()` - 读最新参数
  - `flash_data_fold()` - 参数折叠

- [x] 实现 log_record_t Append & Fold (共 3 个函数)
  - `log_record_append_write()` - 追加写入日志
  - `log_record_read_latest()` - 读指定类型最新日志
  - `log_record_fold()` - 日志折叠

- [x] 修改包装函数
  - `spi_flash_power_count_update()` - 使用新方案
  - `spi_flash_power_count_read()` - 使用新方案
  - `spi_flash_sample_cycle_update()` - 使用新方案
  - `spi_flash_sample_cycle_read()` - 使用新方案
  - `spi_ratio_limit_write()` - 使用新方案
  - `spi_ratio_limit_read()` - 使用新方案
  - `log_states_save_all()` - 使用新方案
  - `log_states_save_sample()` - 使用新方案
  - `log_states_save_over()` - 使用新方案
  - `log_states_save_hide()` - 使用新方案
  - `log_states_load_all()` - 使用新方案

- [x] 清理旧代码
  - 移除 `save_state()` 辅助函数
  - 移除 `load_state()` 辅助函数
  - 更新 `spi_ratio_limit_erase()`

## 使用示例

### 写入参数示例

```c
// 示例 1: 修改上电次数（自动触发）
spi_flash_power_count_update();

// 示例 2: 修改采样周期
spi_flash_sample_cycle_update(10000);  // 10 秒

// 示例 3: 修改比例/限值
spi_ratio_limit_write(1.5f, 120.0f);

// 示例 4: 手动保存日志状态（在写数据到 SD 卡后）
// ... 写数据到 SD ...
log_states_save_sample();  // 只保存 sample 状态
```

### 读取参数示例

```c
// 上电时一次性加载所有参数
void system_init(void) {
    // 1. 加载日志状态
    log_states_load_all();
    
    // 2. 读取采样周期
    uint32_t cycle = spi_flash_sample_cycle_read();
    
    // 3. 读取上电次数
    uint32_t power = spi_flash_power_count_read();
    
    // 4. 读取比例/限值
    float ratio, limit;
    spi_ratio_limit_read(&ratio, &limit);
    
    // 现在所有数据都在内存中，极速访问
}
```

### 隐式折叠示例

```c
// 系统会自动处理折叠
for (int i = 0; i < 200; i++) {
    // 修改比例 200 次
    // 前 128 次：直接追加写入
    // 第 129 次：返回 0（扇区满）→ 触发自动折叠 → 重新追加
    // 结果：仅擦除 2 次 (vs 原来的 200 次)
    float new_ratio = 1.0f + i * 0.001f;
    spi_ratio_limit_write(new_ratio, 100.0f);
}
```

## 内存布局

### 参数数据扇区 (0x00001000, 4KB)

```
偏移地址  | 内容             | 状态
---------|------------------|-------
0x0000   | 记录 0           | 0xA5A50001 (有效) 
0x0020   | 记录 1           | 0xA5A50001 (有效)
...      | ...              | ...
0x0FE0   | 记录 127         | 0xA5A50001 (有效)
0x1000   | 空白             | 0xFFFFFFFF
```

### 日志状态扇区 (0x00002000, 4KB)

```
偏移地址  | 内容             | 状态
---------|------------------|-------
0x0000   | Sample 记录      | 0xA5A50002 (有效)
0x0020   | OverLimit 记录   | 0xA5A50003 (有效)
0x0040   | HideData 记录    | 0xA5A50004 (有效)
0x0060   | Sample 记录 v2   | 0xA5A50002 (有效)
...      | ...              | ...
0x1000   | 空白             | 0xFFFFFFFF
```

## 故障排查

### 问题 1: 参数在上电后不恢复

**原因**: `log_states_load_all()` 未在系统初始化时调用

**解决**: 在 `main()` 早期添加:
```c
int main(void) {
    system_init();
    log_states_load_all();  // <-- 添加这行
    // ...
}
```

### 问题 2: Flash 扇区频繁擦除

**原因**: 可能回退到了旧的乒乓法实现

**检查**: 确认以下函数使用的是新 API
- `spi_flash_power_count_update()` 应调用 `flash_data_append_write()`
- `spi_ratio_limit_write()` 应调用 `flash_data_append_write()`

### 问题 3: 折叠不工作

**原因**: 扇区满时返回 0，但没有处理折叠

**检查**: 包装函数中应有:
```c
uint8_t ret = flash_data_append_write(&data);
if (ret == 0) {  // <-- 必须检查返回值
    flash_data_fold();
    flash_data_append_write(&data);
}
```

## 性能指标

| 操作 | 旧方案 | 新方案 | 改进倍数 |
|------|--------|--------|---------|
| 修改 power_count | 1 次擦除 | 0 次 (1/128) | **128×** |
| 修改 ratio/limit | 2 次擦除 | 0 次 (2/128) | **128×** |
| 128 次参数修改 | 128 次擦除 | 1 次擦除 | **128×** |
| Flash 寿命估算 | 100K 次 | 12.8M 次 | **128×** |

## 检查清单

部署前请确认:

- [ ] 头文件 `myflash.h` 已更新地址和结构体定义
- [ ] 源文件 `myflash.c` 已实现所有新函数
- [ ] 编译无错误和无未定义符号警告
- [ ] 所有旧函数调用已保持兼容（API 未变）
- [ ] `log_states_load_all()` 在系统初始化时被调用
- [ ] 参数修改后能成功持久化到 Flash
- [ ] 上电后参数能正确恢复

## 文档

详见 `APPEND_FOLD_IMPLEMENTATION.md`
