# Append & Fold Flash 参数存储方案实现

## 概览

本实现采用工业级的"追加写入与折叠" (Append & Fold) 方案替代传统的乒乓擦除法，极大降低了 Flash 擦除频率（寿命延长 128 倍）。

## 核心改动

### 1. 地址分配变化（myflash.h）

```
旧方案:
- FLASH_ADDR_POWER_COUNT  0x00002000  (4B，每改写一次要擦除)
- FLASH_ADDR_SAMPLE_CYCLE 0x00003000  (4B，每改写一次要擦除)
- FLASH_ADDR_RATIO        0x00007000  (4B，每改写一次要擦除)
- FLASH_ADDR_LIMIT        0x00008000  (4B，每改写一次要擦除)
- 多个分散地址存储日志状态

新方案 (Append & Fold):
- FLASH_ADDR_FLASH_DATA   0x00001000  (4KB 扇区，追加写入)
- FLASH_ADDR_LOG_STATE    0x00002000  (4KB 扇区，追加写入)
```

### 2. 数据结构体设计（32 字节定长）

**flash_data_t** - 参数数据记录
```c
typedef struct {
    uint32_t magic;         // 0xA5A50001 - 数据有效性标记
    uint32_t power_count;   // 上电次数
    uint32_t sample_cycle;  // 采样周期(ms)
    float ratio_ch0;        // 比例系数
    float limit_ch0;        // 限值
    float dac_volt;         // DAC电压
    uint32_t reserved;      // 预留
} flash_data_t;             // 总计 32 字节
```

**log_record_t** - 日志状态记录
```c
typedef struct {
    uint32_t magic;         // 0xA5A5000X - 记录类型标记
    uint16_t line;          // 当前行数
    uint16_t reserved;      // 预留
    char filename[24];      // 文件名
} log_record_t;             // 总计 32 字节
```

### 3. 核心函数实现

**追加写入 (Append Write)**
```c
uint8_t flash_data_append_write(const flash_data_t *data);
uint8_t log_record_append_write(const log_record_t *record);
```
- 扫描扇区找到下一个 0xFF 空白地址
- 直接写入新记录，不进行擦除
- 返回 0 表示扇区满，需要折叠

**读取最新 (Read Latest)**
```c
uint8_t flash_data_read_latest(flash_data_t *data);
uint8_t log_record_read_latest(uint32_t magic, log_record_t *record);
```
- 从后向前扫描扇区
- 找到的第一条有效记录（magic 不是 0xFFFFFFFF）即为最新数据
- 上电时只需一次扫描即可获取所有参数和日志状态

**折叠 (Garbage Collection)**
```c
uint8_t flash_data_fold(void);
uint8_t log_record_fold(void);
```
- 扇区写满时（128 条 32 字节记录）执行一次
- 读出最新的有效记录
- 擦除整个 4KB 扇区
- 将最新记录重新写在扇区头
- 寿命收益：128 倍（4096 / 32 = 128）

### 4. 兼容性封装

所有原有的公开接口保持不变，内部实现已迁移到 Append & Fold 方案：

```c
// 上电次数 - 现在使用 Append & Fold
uint8_t spi_flash_power_count_update(void);
uint32_t spi_flash_power_count_read(void);

// 采样周期 - 现在使用 Append & Fold
uint8_t spi_flash_sample_cycle_update(uint32_t cycle);
uint32_t spi_flash_sample_cycle_read(void);

// 比例/限值 - 现在使用 Append & Fold
uint8_t spi_ratio_limit_read(float *r, float *l);
uint8_t spi_ratio_limit_write(float r, float l);

// 日志状态 - 现在使用 Append & Fold
void log_states_save_sample(void);
void log_states_save_over(void);
void log_states_save_hide(void);
void log_states_load_all(void);
```

## 工作流程

### 写流程
```
1. 读取最新参数 flash_data_read_latest(&data)
2. 修改需要更新的字段
3. 追加写入 flash_data_append_write(&data)
   ├─ 如果成功 → 返回 1，流程结束
   └─ 如果失败（扇区满） → 执行折叠后重试
      ├─ 折叠 flash_data_fold()
      └─ 重新追加写入 flash_data_append_write(&data)
```

### 读流程
```
1. 上电初始化时，调用一次 log_states_load_all()
2. 这会扫描 0x00002000 扇区，找到三种日志类型的最新记录
3. 所有参数都被缓存到内存全局变量中
4. 运行时直接使用内存中的缓存数据（极速）
5. 需要持久化时再写入 Flash
```

## 性能对比

| 指标 | 乒乓法 | Append & Fold | 提升 |
|------|--------|---------------|------|
| 每次参数修改的擦除次数 | 1 次 | 0 次（平均 1/128） | **128 倍** |
| 上电次数增加 | 1 次擦除 | 0 次（平均 1/128） | **128 倍** |
| 比例/限值更新 | 2 次擦除 | 0 次（平均 1/128） | **256 倍** | 
| Flash 寿命 (K 次擦除) | 100K | 12.8M | **128 倍** |
| 读取性能 | 随机读 | 顺序扫描 | 相当 |
| 写入性能 | 慢（需擦除） | 快（直接写） | **显著提升** |

## 折叠机制详解

**何时触发折叠：**
```c
if (flash_data_append_write(&data) == 0) {  // 返回 0 = 扇区满
    flash_data_fold();  // 自动触发折叠
    flash_data_append_write(&data);  // 重试写入
}
```

**折叠过程：**
```
1. 读取最新记录（从后向前扫描找到的第一条有效记录）
2. 擦除整个 4KB 扇区（仅此一次）
3. 写入最新记录到扇区头
4. 继续使用追加写入

结果：
- 128 次参数修改才触发 1 次扇区擦除
- 极大延长 Flash 使用寿命
```

## 内存到限制判断与回环

数据流回环示例：

```c
// 用户修改比例/限值参数
float new_ratio = 1.5f;
float new_limit = 120.0f;

// 1. 写入 Flash（Append & Fold）
spi_ratio_limit_write(new_ratio, new_limit);

// 2. 内部流程
//   a. 读取最新参数结构体
//   b. 修改 ratio_ch0 和 limit_ch0 字段
//   c. 追加写入新记录到 0x00001000 扇区
//   d. 返回成功

// 3. 限制判断回环
if (measured_value > limit_ch0) {  // 在限制判断处回环使用
    record_over_limit();
    spi_flash_power_count_update();  // 继续产生新的参数修改
}

// 4. 下次上电时
//   a. log_states_load_all() 从 0x00002000 读取最新日志
//   b. 扫描 0x00001000 读取最新参数
//   c. 所有数据一次性加载完成
```

## 最小改动说明

- **未修改**: 设备ID存储（0x00000000）、其他应用逻辑
- **已迁移**: power_count, sample_cycle, ratio/limit → 使用 Append & Fold
- **已迁移**: 日志状态（sample/overlimit/hidedata）→ 使用 Append & Fold
- **新增**: 6 个核心函数 + 2 个折叠函数 = 完整 Append & Fold 实现
- **移除**: 原有的乒乓擦除逻辑和分散地址访问

## 验证方式

1. **编译验证**: 所有函数签名保持兼容
2. **地址验证**: 新地址不与其他应用冲突
3. **功能验证**: 参数修改 → 数据持久化 → 上电恢复
4. **性能验证**: 修改参数多次 → 观察 Flash 擦除次数（应大幅降低）

## 后续优化建议

1. 在折叠过程中添加 CRC32 校验
2. 使用状态机管理复杂的参数依赖关系
3. 添加参数版本号支持固件升级时的参数迁移
4. 考虑双扇区备份增强可靠性
