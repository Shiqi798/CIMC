# 架构整理清单

## 赛题分层要求

本工程后续按赛题要求整理成三层：

- `Driver`：UART/IIC/SPI/OLED/Flash/RTC/ADC/DAC/LED/PT100底层驱动。
- `Protocol`：报文接收、ASCII HEX转换、CRC、帧解析、帧组装、应答发送。
- `Function`：采样、变比、阈值、告警、参数管理、业务判断、睡眠/升级触发。

Bootloader在另一个工程，本工程只处理APP侧。

## 当前工程编译文件

从 `project/TASK.uvprojx` 看，当前APP工程实际编译：

- `User`：`gd32f4xx_it.c`，`main.c`，`systick.c`
- `Driver/HardWare`：`KEY`，`LED`，`OLED`，`RTC`，`SPI_FLASH`，`USART`
- `Driver/System`：`AD3344`，`ADC`，`DAC`，`TIM6`，`myDMA`，`sleep_wake`
- `Driver/FlashDB`：`fal`，`flashdb`
- `Function`：`Function.c`，`cmd_parse.c`，`data_process.c`，`pt100.c`，`app_vars.c`，`myflashdb_data.c`，`myflashdb_log.c`
- `Protocol`：`msg.c`，`msg.h`

## Function目录旧文件

当前在 `Function` 目录但没有进入 `TASK.uvprojx`：

- `myflash.c`
- `myflash.h`
- `boot_param.c`
- `boot_param.h`

第一轮不删除、不移动。后续如果要删、迁移、加入工程，先单独出方案。

## msg.c分类

### 留在Protocol

这些函数只做协议层工作，后续保留在 `Protocol/msg.c`：

- `Calculate_CRC16`
- `msg_read_u16`
- `msg_write_u16`
- `msg_write_u32`
- `msg_read_u32`
- `msg_write_float`
- `msg_read_float`
- `msg_hex_to_nibble`
- `msg_nibble_to_hex`
- `msg_ascii_start_is_frame`
- `msg_ascii_to_raw`
- `msg_raw_to_ascii`
- `msg_parse_raw`
- `msg_addr_match`
- `msg_build_frame`
- `msg_build_ok`
- `msg_build_error`
- `msg_send_heartbeat`
- `msg_poll`

`msg_poll` 后续只保留接收、解析、地址过滤、发送、清缓冲这些流程；业务分发逐步迁到 `Function`。

### 迁到Function

这些函数属于业务层，后续迁到 `Function`，建议新文件名先用 `msg_app.c/.h`：

- `msg_alarm_check`
- `msg_alarm_print`
- `msg_build_auto_report`
- `msg_set_time_by_unix`
- `msg_wait_adc_boot_ready`
- `msg_auto_report_poll`
- `msg_cmd_reboot`
- `msg_cmd_version`
- `msg_cmd_set_time`
- `msg_cmd_get_time`
- `msg_cmd_set_id`
- `msg_cmd_get_id`
- `msg_cmd_get_baud`
- `msg_cmd_set_baud`
- `msg_cmd_set_dac`
- `msg_cmd_auto_start`
- `msg_cmd_auto_stop`
- `msg_cmd_sleep`
- `msg_cmd_get_ch0`
- `msg_cmd_get_ch1`
- `msg_cmd_get_ch2`
- `msg_cmd_set_ratio_ch0`
- `msg_cmd_set_ratio_ch1`
- `msg_cmd_set_sample_cycle`
- `msg_cmd_get_limits`
- `msg_cmd_get_limit0`
- `msg_cmd_get_limit1`
- `msg_cmd_set_limit0`
- `msg_cmd_set_limit1`
- `msg_cmd_set_alarm_report`
- `msg_cmd_get_alarm_logs`
- `msg_cmd_clear_alarm_logs`
- `msg_cmd_enter_upgrade`
- `msg_handle_cmd`

相关状态也应随业务迁走：

- `msg_reboot_pending`
- `msg_sleep_pending`
- `msg_auto_report_flag`
- `msg_adc_boot_ready`
- `msg_alarm_count`
- `msg_over_log_empty`
- `msg_alarm_channel`
- `msg_alarm_value`
- `msg_alarm_limit`
- `msg_auto_report_start`

### 暂不优先动

这些地方虽然不干净，但已经配合上位机跑通过，迁移时要最后处理：

- `0602` 当前直接发送 `empty\r\n`
- `0603` 清告警后延时
- `0501` 升级前先发OK再写boot flag
- 睡眠唤醒后的OLED刷新
- 波特率/设备ID设置后的重启时序

## 后续小切口建议

1. 先只开放 `Protocol` 的组帧接口，不迁业务函数。
2. 再把命令表和 `msg_handle_cmd` 迁到 `Function/msg_app.c`。
3. 再迁 `010x` 系统类命令。
4. 再迁 `020x/030x` 采样和DAC命令。
5. 最后迁 `060x/03AA/0501` 这些带日志、睡眠、升级时序的命令。

每一步都先出方案，不夹带删除和工程文件迁移。

## 任务执行要求

- 每一步动手前先给方案书，确认本步改什么、不改什么、怎么检查。
- 优先最小改动，能小步拆就不一次大搬迁。
- 删除、移动、工程排除、加入旧文件编译前，必须提前单独说明。
- 代码风格参考当前工程，尤其 `cmd_parse.c`、`Function.c` 的写法。
- 注释尽量少写，只写必要说明，不搞太整齐的“新工程风格”。
- 注释按 `comment_writing_guide.md`：中文为主，模块分区可以明显，小功能短句说明。
- 不写太像模板的注释，少用 `Step x`、`@brief` 这类统一格式，除非确实复杂。
- 关键逻辑、硬件操作、FlashDB、RTC、ADC/DAC、OLED、USART 时序这些地方可以多写一点原因。
- 简单赋值、普通返回、自增不强行注释。
- 可以保留少量口语化和现场调试痕迹，能说明之前踩过的坑更好。
- 可以保留调试痕迹和当前工程已有的粗糙感，先保证赛题功能稳定。
- 不过度封装，不为了好看新增一堆小函数。
- `Protocol` 只逐步保留协议解析、CRC、组帧、发送流程。
- `Function` 逐步承接命令分发、采样、参数、告警、睡眠、升级触发等业务。
- `Driver` 只放底层外设驱动，不把业务判断塞回驱动层。
- 每完成一步，都在本文档 `进度记录` 里补充状态、已做内容、检查结果、下一步建议。
- 如果命令行无法编译，就明确写“待EIDE编译确认”，不假装已经验证。
- 每次改完优先做静态检查：搜索残留符号、确认工程文件、确认没有夹带不相关改动。

## 进度记录

### Step 0：建立架构清单

状态：已完成

已做：

- 新增本文件，记录赛题分层要求、当前工程编译文件、旧文件清单、`msg.c` 函数分类。
- 没有改源码、没删文件、没移动文件、没改工程文件。

检查：

- 对照 `TASK.uvprojx` 确认当前编译文件清单。
- 标出 `myflash.c/.h`、`boot_param.c/.h` 当前没有进入工程，后续处理前需要单独出方案。

### Step 1：开放Protocol组帧接口

状态：已完成

已做：

- `Protocol/msg.c` 中 `msg_read_*`、`msg_write_*`、`msg_build_*`、`msg_adc_raw_to_volt` 去掉 `static`。
- `Protocol/msg.h` 增加对应声明。
- 没有改协议行为，没有新增 `.c`，没有改 `TASK.uvprojx`。

检查：

- 静态检查确认这些接口不再是 `static`。
- `msg.h` 能搜到所有声明。
- 命令行环境没有 `eide/armcc/uv4`，编译需要在 EIDE 里确认。

### Step 2：迁出命令表和分发器

状态：已完成，待EIDE编译确认

已做：

- 新增 `Function/msg_app.c`、`Function/msg_app.h`。
- `msg_cmd_table` 和 `msg_app_handle_cmd()` 放到 `Function/msg_app.c`。
- `Protocol/msg.c` 删除旧的 `msg_handle_cmd()`，`msg_poll()` 改为调用 `msg_app_handle_cmd(&frame)`。
- `Protocol/msg.h` 暂时声明 `msg_cmd_*`，并放出 `MSG_CMD_HEART_FIND / MSG_CMD_HEART_BEAT / MSG_CMD_ERR`。
- `TASK.uvprojx` 已加入 `Function/msg_app.c/.h`。

检查：

- `msg_cmd_table` 只在 `Function/msg_app.c`。
- `Protocol/msg.c` 已无旧 `msg_handle_cmd`。
- `TASK.uvprojx` XML 解析正常。
- `git diff --check` 未发现空白错误。
- 命令行环境没有 `eide/armcc/uv4`，编译需要在 EIDE 里确认。

下一步建议：

- Step 3 已继续迁移 `010x/01Ax` 系统类命令。

### Step 3：迁移010x/01Ax系统类命令

状态：已完成，待EIDE编译确认

已做：

- `msg_set_time_by_unix` 迁到 `Function/msg_app.c`。
- `0101/0104/0105/0106/0111/0112/01A1/01A2` 对应处理函数迁到 `Function/msg_app.c`。
- `msg_reboot_pending` 迁到 `Function/msg_app.c`。
- 新增 `msg_app_after_send()`，`Protocol/msg.c` 的 `msg_poll()` 在发送和清缓冲后调用它处理重启。
- `Protocol/msg.h` 删除已迁走的系统类命令声明，保留尚未迁移的命令声明。
- 没有改 `TASK.uvprojx`，没有新增工程文件。

检查：

- `Protocol/msg.c` 已不再包含 `010x/01Ax` 系统类命令函数。
- `msg_reboot_pending` 只在 `Function/msg_app.c`。
- `msg_poll()` 已调用 `msg_app_after_send()`。
- 采样、DAC、告警、睡眠、升级命令暂未迁移。
- 命令行环境没有 `eide/armcc/uv4`，编译需要在 EIDE 里确认。

下一步建议：

- EIDE 构建通过后，迁移 `020x/030x` 采样和DAC命令。

### Step 4：迁移020x/030x采样和DAC命令

状态：已完成，待EIDE编译确认

已做：

- `msg_cmd_get_ch0` / `get_ch1` / `get_ch2` / `set_ratio_ch0` / `set_ratio_ch1` / `set_sample_cycle` 迁到 `Function/msg_app.c`。
- `msg_cmd_set_dac` / `auto_start` / `auto_stop` 迁到 `Function/msg_app.c`。
- 静态辅助函数 `msg_wait_adc_boot_ready` / `msg_alarm_check` / `msg_build_auto_report` 一并迁入。
- 相关静态变量（`msg_auto_report_flag`、`msg_adc_boot_ready`、`msg_alarm_*`、`msg_auto_report_start`）迁入 `msg_app.c`。
- `msg_auto_sample_flag` 定义从 `msg.c` 迁到 `msg_app.c`。
- `msg_poll()` 改为调用 `msg_app_auto_report_poll()` / `msg_app_auto_report_is_active()` / `msg_app_alarm_print()`。
- `msg_cmd_clear_alarm_logs` 改为调用 `msg_app_reset_alarm_state()`。
- `Protocol/msg.h` 删除已迁走的 9 个 extern 声明。
- `msg_cmd_sleep`(0x03AA) 暂留 `msg.c`，等 Step 5 迁移。
- `msg_auto_report_flag` 暂时非 static（被 msg_cmd_sleep 交叉引用），Step 5 后可改回 static。

检查：

- `Protocol/msg.c` 无已迁函数残留引用。
- `Protocol/msg.h` 已删除 02xx/03xx 声明，保留 `msg_cmd_sleep`。
- `msg_poll()` 改用 3 个 msg_app 接口函数。
- `msg_cmd_clear_alarm_logs` 改用 `msg_app_reset_alarm_state()`。
- 命令行环境没有 `eide/armcc/uv4`，编译需要在 EIDE 里确认。

下一步建议：

- EIDE 构建通过后，迁移 `04xx/05xx/06xx` 命令和 `msg_cmd_sleep`。

### Step 5：迁移剩余全部命令（04xx/05xx/06xx/sleep）

状态：代码已迁移，待EIDE编译确认

已做：

- `msg_cmd_sleep`(0x03AA) 迁到 `Function/msg_app.c`。
- `msg_cmd_get_limits` / `get_limit0` / `get_limit1` / `set_limit0` / `set_limit1`(04xx) 迁到 `Function/msg_app.c`。
- `msg_cmd_enter_upgrade`(0x0501) 迁到 `Function/msg_app.c`。
- `msg_cmd_set_alarm_report` / `get_alarm_logs` / `clear_alarm_logs`(06xx) 迁到 `Function/msg_app.c`。
- `msg_sleep_pending` 从 `Protocol/msg.c` 迁到 `Function/msg_app.c`（非 static，extern 在 msg.h 供 msg_poll 读取）。
- `msg_auto_sample_flag` 定义从 `Protocol/msg.c` 迁到 `Function/msg_app.c`，当前 `msg.h` 仍保留 extern，后面确认引用后再决定是否收掉。
- `msg_auto_report_flag` 留在 `msg_app.c` 内部使用。
- `Protocol/msg.h` 删除全部 10 个已迁命令的 extern 声明。
- `Protocol/msg.c` 全部命令处理函数归零，仅保留纯协议层代码。
- `Protocol/msg.c` / `Function/msg_app.c` 零改动表项（msg_cmd_table 表已完整）。

检查：

- `Protocol/msg.c` 无任何 `msg_cmd_*` 函数定义或前向声明残留。
- `Protocol/msg.h` 当前声明协议层函数（CRC、读写、组帧、msg_poll）和 5 个 extern 变量（tx_buffer、tx_len、device_id、msg_sleep_pending、msg_auto_sample_flag）。
- `Function/msg_app.c` 命令表包含全部 01xx~06xx 条目，所有 handler 为 static。
- `msg_poll()` 第 369 行 `msg_sleep_pending` 通过 msg.h 的 extern 访问。
- `Function/msg_app.c` 里还有 `Step 4/Step 5` 这类偏模板注释，后续整理注释时再按注释指导改自然。
- `CODEx_HANDOFF.md` 当前在 git 状态里显示删除，本轮不恢复、不删除、不提交，等确认。
- 命令行环境没有 `eide/armcc/uv4`，编译需要在 EIDE 里确认。

下一步建议：

- EIDE 构建确认后，再出旧文件处理方案；在此之前不删 `myflash.c/.h`、不移动或加入 `boot_param.c/.h`。

### Step 6：收口核查和日志修正

状态：已完成

已做：

- 把 `comment_writing_guide.md` 中的注释风格要求补进“任务执行要求”。
- 修正 Step 5 中 `msg_auto_sample_flag` 的记录，按当前代码状态写成 `msg.h` 仍有 extern。
- Step 5 状态改为“代码已迁移，待EIDE编译确认”，不写成完全验收。
- 记录 `CODEx_HANDOFF.md` 当前删除状态，本步不处理这个文件。

检查：

- `rg "msg_cmd_" Protocol/msg.c` 只剩迁移说明注释，没有命令函数定义。
- `rg "extern" Protocol/msg.h` 当前有 `msg_tx_buffer`、`msg_tx_len`、`msg_device_id`、`msg_sleep_pending`、`msg_auto_sample_flag`。
- `rg "Step 4|Step 5" Function/msg_app.c` 能搜到偏模板注释，后续可单独自然化。
- `git status --short` 显示 `Function/msg_app.c/.h` 和本文件未跟踪，`CODEx_HANDOFF.md` 删除，工程和构建产物有改动。

下一步建议：

- 先让 EIDE 编译确认 Step 2~5 的迁移结果。
- 编译通过后，再出一个“旧文件处理方案”，只讨论 `myflash.c/.h`、`boot_param.c/.h` 是否删除、保留、加入工程或移到备份目录。

### Step 7：数据处理归位

状态：已完成，待EIDE编译确认

已做：

- 在 `Function/data_process.c/.h` 增加 `data_adc_raw_to_volt()`。
- `Function/msg_app.c` 中 CH0/CH1 查询、自动上报、DAC raw 转电压都改用 `data_adc_raw_to_volt()`。
- 删除 `Protocol/msg.c` 中旧的 `msg_adc_raw_to_volt()`。
- 删除 `Protocol/msg.h` 中旧声明。
- 本步没有迁睡眠流程，没有改协议帧，没有改命令逻辑。

检查：

- `Protocol` 不再提供 ADC raw 转电压函数。
- `Function/msg_app.c` 中采样业务仍按原公式换算，数值逻辑不变。
- 手动命令行采样的 `data_calc_eng_volt()` 未改，避免影响去年命令路径。

下一步建议：

- EIDE 编译确认后，继续迁 `Protocol/msg.c` 里剩下的睡眠执行流程，把 `RTC/PMU/OLED/SystemInit/USART1_Init` 从协议层移到 Function 层。

### Step 8：应答后的业务动作归位

状态：已完成，待EIDE编译确认

已做：

- 把 `Protocol/msg.c` 中睡眠执行流程移到 `Function/msg_app.c` 的 `msg_app_after_send()`。
- `msg_app_after_send()` 现在统一处理应答发完后的重启和睡眠。
- `msg_sleep_pending` 改为 `Function/msg_app.c` 内部 static，`Protocol/msg.h` 不再暴露它。
- 删除 `Protocol/msg.c` 中迁移过程留下的 `moved to Function` 注释。
- `Function/msg_app.c` 中 `Step 4/Step 5` 模板注释改成自然分区注释。

检查：

- `Protocol/msg.c` 不再直接调用 `RTC_SetWakeup`、`pmu_to_deepsleepmode`、`SystemInit`、`USART1_Init`、`oled_idle_refresh`。
- `Protocol/msg.c/.h` 不再访问 `msg_sleep_pending`。
- 睡眠仍保持先回 OK，再进入低功耗，唤醒后刷新 OLED idle。
- 命令帧格式、命令表、各命令返回内容未改。

下一步建议：

- EIDE 编译确认后，继续处理 `Protocol/msg.c` 中剩余的发送耦合：评估是否把 `USART1_SendData` 包成协议层发送接口，或者先整理 `msg_app.c` 的告警/日志业务边界。

### Step 9：串口发送权收回Protocol

状态：已完成，待EIDE编译确认

已做：

- `Protocol/msg.c/.h` 增加 `msg_send_current()`，发送当前已组好的协议帧。
- `Protocol/msg.c/.h` 增加 `msg_send_string()`，给 `0602 empty` 这种特殊字符串用。
- `msg_send_heartbeat()` 和 `msg_poll()` 内部发送改走 `msg_send_current()`。
- `Function/msg_app.c` 中自动上报、`0602 empty`、`0501` 升级 OK 立即发送都改走 Protocol 发送接口。
- `msg_tx_buffer` 和 `msg_tx_len` 改成 `Protocol/msg.c` 内部 static，`Protocol/msg.h` 不再暴露。

检查：

- `Function/msg_app.c` 不再直接调用 `USART1_SendData`。
- `Function/msg_app.c` 不再直接访问 `msg_tx_buffer` / `msg_tx_len`。
- `Protocol/msg.h` 不再声明发送缓冲区 extern。
- 协议帧格式、命令表、业务判断未改。

下一步建议：

- EIDE 编译确认后，继续整理 `msg_app.c` 业务边界：把参数保存重复代码收成少量本地 helper，或者把告警/日志相关函数拆到单独 Function 文件。

### Step 10：重写Function主控层

状态：已完成，待EIDE编译确认

已做：

- 旧 `Function.c/.h` 已移到桌面备份目录：`D:\desktop\V02.2_old_function_backup`。
- 新版 `Function.c/.h` 作为本次赛题 APP 主控层，负责初始化、主循环、LED/OLED 状态和采样状态接口。
- `sysFunction_Init()` 保留外设初始化、FlashDB 初始化、参数恢复、DAC/USART 恢复、TIM6 启动。
- `sysFunction_loop()` 现在只跑 `function_led_update()`、`msg_poll()`、`oled_idle_refresh()`。
- 按键调试入口和旧串口命令入口仍保留在 `Function.c`，但默认注释掉。
- `msg_auto_sample_flag` 已从 `msg_app.c/msg.h` 移走，改为 `Function.c` 内部状态。
- `msg_app.c` 改用 `function_sample_state_set()` 和 `function_idle_refresh_request()`，不再直接维护自动采样状态和 idle 刷新标志。
- `Function.h` 不再声明 `update_sample_cycle()`，外部也不再暴露 `oled_idle_refresh_flag`。

检查：

- `rg "msg_auto_sample_flag" Function Protocol` 无结果。
- `rg "update_sample_cycle" Function User Protocol` 无结果。
- `oled_idle_refresh_flag` 只在 `Function.c` 内部出现。
- `function_key_update()` 只在主循环注释行保留，默认不参与比赛流程。
- `Function_old.c/.h` 没有进入 `TASK.uvprojx`。
- `Function/msg_app.c` 仍没有直接调用 `USART1_SendData`，也没有直接访问 `msg_tx_buffer/msg_tx_len`。
- `git diff --check` 只有 CRLF/LF 换行提示，没有空白错误。
- EIDE 首次编译发现 `tim6_functimer.c` 还直接写 `oled_idle_refresh_flag`，已改为调用 `function_idle_refresh_request()`。
- 修正后 `oled_idle_refresh_flag` 只在 `Function.c` 内部出现。

下一步建议：

- 先用 EIDE 编译确认新版 `Function.c/.h`。
- 编译过后，继续拆 `msg_app.c`：优先把告警/日志相关逻辑拆到单独业务文件，或者把参数保存重复代码收成本地 helper。

### Step 11：移走旧命令行cmd_parse

状态：已完成，待EIDE编译确认

已做：

- `Function/cmd_parse.c/.h` 已移到桌面备份目录，改名为 `cmd_parse_old.c/.h`。
- `TASK.uvprojx` 已删除 `cmd_parse.c/.h` 工程条目。
- `HeaderFiles.h` 已删除 `#include "cmd_parse.h"`。
- `Function.c` 主循环只保留比赛路径：LED 状态、协议轮询、OLED idle。
- `Function.c` 有效代码里删除旧按键启停和 DAC test 依赖。
- `TIM6` 中 DAC test 计数段已删除，不再依赖 `dac_test_*` 变量。
- 旧按键启停、DAC test 变量、TIM6 计数方式和 `dac_test_tick()` 参考代码，以 `/* ... */` 注释块保留在 `Function.c` 末尾。

检查：

- `project/TASK.uvprojx` 中无 `cmd_parse.c/.h`。
- `Driver/Function/Protocol/User` 中无 `#include "cmd_parse.h"`。
- `Driver/Function/Protocol/User` 中无 `cmd_parse` 真实代码残留。
- `dac_test_*` 只剩 `Function.c` 末尾注释块。
- `D:\desktop\V02.2_old_function_backup\cmd_parse_old.c/.h` 存在。
- `git diff --check` 只有 CRLF/LF 换行提示，没有空白错误。

下一步建议：

- 先用 EIDE 编译确认移除 `cmd_parse` 后的工程状态。
- 编译通过后，再单独评估 `boot_param.c/.h`、`myflash.c/.h` 这些旧文件是否保留、迁移或排除。

### Step 12：清理当前编译warning

状态：已完成，待EIDE编译确认

已做：

- `AD3344_cmd()` 删除注释掉的平均值流程留下的 4 个未使用变量。
- `msg_app.c` 删除只 set 不 read 的 `msg_over_log_empty`。
- `pt100.c` 给未启用的温度算法函数加条件编译，保留备用算法代码但不让未启用函数参与编译。
- `pt100_snap_resistance()` 和温度表保持原行为，继续用于电阻吸附，不跟温度算法一起关掉。

检查：

- `git diff --check` 只有 CRLF/LF 换行提示，没有空白错误。
- 命令行环境没有可直接调用的 `eide/armcc/uv4`，需要用 EIDE 再编译确认 warning 是否清掉。

下一步建议：

- EIDE 编译确认 warning 清理结果。
- 如果还有旧文件 warning，再按“先判断是否仍被使用，再移走或条件编译”的方式继续收。

### Step 13：合并app_vars和boot_param

状态：已完成，待EIDE编译确认

已做：

- 新增 `Function/app_param.c/.h`。
- `app_vars.c` 中运行参数定义已迁入 `app_param.c`。
- `boot_param.c/.h` 中 bootloader 参数区宏、结构体和 `boot_param_set_flag()` 已迁入 `app_param.c/.h`。
- `HeaderFiles.h` 已改为 include `app_param.h`，并删除重复的 `app_vars.h` 和 `boot_param.h` 引用。
- `TASK.uvprojx` 和 `.eide/eide.yml` 都已改为包含 `app_param.c/.h`。
- 旧 `app_vars.c/.h`、`boot_param.c/.h` 已移到桌面备份目录。

检查：

- `app_param.c/.h` 在 `TASK.uvprojx` 和 `.eide/eide.yml` 中都能搜到。
- `app_vars` 旧名在工程源码和配置中已无残留。
- `boot_param_set_flag()`、`BOOT_FLAG_UPDATE`、`ratio_ch0`、`dac_volt`、`usart1_baud_mode` 仍能正常从新模块找到。
- `D:\desktop\V02.2_old_function_backup\app_vars_old.c/.h` 和 `boot_param_old.c/.h` 存在。
- `git diff --check` 只有 CRLF/LF 换行提示，没有空白错误。

下一步建议：

- 用 EIDE 编译确认合并后的参数模块。
- 编译通过后再做 Step 14：继续瘦身 `msg_app.c`，优先拆告警/日志或参数保存 helper。

### Step 14：逻辑层注释风格整理

状态：已完成，待EIDE编译确认

已做：

- 按 `comment_writing_guide.md` 把最近新增文件里的英文分区注释改成中文短注释。
- `app_param.c/.h` 的参数区、boot参数区注释改成更自然的中文写法，`define` 不再刻意排成很整齐的一列。
- `Function.c/.h` 补了主控层关键说明，旧调试块改成中文说明。
- `msg_app.c` 只补关键时序注释：UTC转RTC、应答后复位/睡眠、ADC首帧等待、告警缓存、DAC输出、升级前发OK。
- `data_process.c/.h` 顺手清掉明显乱码和太啰嗦的注释。
- 没改命令号、结构体布局、协议返回值、FlashDB字段。

检查：

- `APP PARAM / BOOT PARAM / Step 4 / Step 5` 这类重构痕迹已清掉。
- `brief / param / retval` 这类模板注释未新增；搜索到的 `boot_param` 是保留接口名，不是注释模板。
- `Func` 搜索会命中 `sysFunction_*` 函数名，属于误报，不是分区注释残留。
- `git diff --check` 只有 CRLF/LF 换行提示，没有空白错误。

下一步建议：

- EIDE 编译确认注释整理没有带来编码问题。
- 之后再做 Step 15：把 `msg_app.c` 里告警/日志或参数保存 helper 拆出去，让业务文件继续瘦身。

### Step 15：调整Function手写感和编码

状态：已完成，待EIDE编译确认

已做：

- `Function.c` 重新保存为合法 UTF-8，无 BOM。
- 修掉 `Function.c` 中文注释里的坏字节和乱码。
- `Function.c` 函数之间和函数内部不再保持完全一样的空行节奏，短状态接口贴近一点。
- 主循环、参数恢复、LED刷新这些逻辑顺序未改，只调整注释和局部排版。
- `Function.h` 的注释改短，不写成模板分组。
- `app_param.h` 的分区和 define 轻微松了一下，不再刻意排得很规整。

检查：

- Python `decode('utf-8')` 检查 `Function.c/.h/app_param.h` 成功。
- 三个文件都无 UTF-8 BOM。
- `rg "�|\\?\\?" Function.c/Function.h/app_param.h` 无结果。
- `sysFunction_Init`、`function_param_load`、`function_sample_state_*` 仍在。
- `git diff --check` 只有 CRLF/LF 换行提示，没有空白错误。

下一步建议：

- 用 EIDE 编译确认编码和排版调整没有引入问题。
- 之后再继续处理 `msg_app.c` 太长的问题，优先从告警/日志或参数保存 helper 下手。
