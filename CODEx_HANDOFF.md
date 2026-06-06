# Codex Handoff

This file is intentionally ASCII-only so it can be read safely in any shell or new chat.

## Project

Workspace:

`d:\desktop\GD32\Project\V02.2`

Active EIDE project:

`d:\desktop\GD32\Project\V02.2\vscode eide`

Target:

2026 CIMC embedded contest APP software. Bootloader is in another project and is not handled here yet.

## User Preferences

- Work step by step.
- Before larger features, explain what will be done and how, then wait for approval.
- Keep changes minimal.
- Match the current project style. It is okay to keep debug traces.
- Avoid over-polished comments. Comment only enough to understand.
- Do not over-abstract.
- Protocol code belongs in `Protocol/msg.c/h`.
- Business logic belongs in `Function/`.
- Old `sysFunction/` was moved to `Function/`.

## Important Files

- `Protocol/msg.c/h`: ASCII HEX protocol parse/build, command table, CRC.
- `Function/app_vars.c/h`: global app variables.
- `Function/myflashdb_data.c/h`: persistent config in FlashDB.
- `Function/myflashdb_log.c/h`: FlashDB logs, including alarm logs.
- `Function/Function.c`: init, main loop, OLED/LED status.
- `Driver/HardWare/USART/USART.c/h`: USART1 + RS485 + DMA.
- `Driver/System/ADC/ADC.c`: ADC CH0/CH1 setup.
- `Driver/System/myDMA/myDMA.c/h`: ADC DMA buffer.

## Protocol Format

Frames are built as bytes, then transmitted as ASCII hex characters.

Example: byte header `A5 B6` is sent as text `A5B6`.

Frame:

`A5B6 + DeviceID + Type + Cmd + Len + Ver(02) + Payload + CRC16-Modbus(big endian) + B6A5`

CRC range:

from header through payload, excluding CRC bytes and tail.

Some replies are direct strings, not protocol frames:

- alarm active report
- alarm query
- sleep wakeup
- Bootloader messages

## Implemented Commands

System:

- `FFFF`: heartbeat find, type `05`, broadcast only.
- `8888`: heartbeat, type `05`, sent once after APP starts.
- `0101`: reboot, reply OK then reset.
- `0104`: firmware version, currently `02 00 01 00`.
- `0105`: set UTC time.
- `0106`: get UTC time.
- `01A1`: set device ID, persisted.
- `0111`: get device ID.
- `0112`: get baud mode.
- `01A2`: set baud mode, persisted, reply OK then reset.

Data:

- `0201`: query CH0, returns float after ratio.
- `0202`: query CH1, returns float after ratio.
- `0221`: query PT100 temp.
- `0241`: set CH0 ratio.
- `0242`: set CH1 ratio.
- `0261`: set report interval.
  - Current implementation matches the PDF one-byte mapping: `01=1s`, `02=3s`, `03=5s`.

Control:

- `0301`: set DAC raw `0000~0FFF`, sets both DAC_OUT0 and DAC_OUT1.
- `0302`: start auto report. First reply is data frame, then periodic same frame.
- `0303`: stop auto report.
- `03AA`: deep sleep. Reply OK, RTC sleep 10s, wake prints `instrument wakeup`.

Params:

- `0400`: get CH0/CH1 thresholds.
- `0401`: get CH0 threshold.
- `0402`: get CH1 threshold.
- `0411`: set CH0 threshold.
- `0412`: set CH1 threshold.

Alarm:

- `0601`: set alarm mode. `01` active, `02` passive.
- `0602`: print latest 10 alarm logs as direct strings via `print_latest_over_logs(10)`.
- `0603`: clear alarm logs.

## Current Behavior Details

### Auto Report

`0302` sets:

- internal `msg_auto_report_flag = 1`
- public `msg_auto_sample_flag = 1`

`Function.c` uses:

`sampling_flag || msg_auto_sample_flag`

for OLED/LED status.

Auto report payload:

`UTC(4B) + CH0 float(4B) + CH1 float(4B)`

Auto report checks CH0/CH1 threshold every sample and stores alarm logs with:

`append_over_log_ch()`

During auto report:

- protocol frames other than `0303` are ignored
- legacy text commands are cleared and ignored

### Alarm Logs

`over_log_t` currently has:

```c
char channel[4];
float voltage;
float limit;
```

Write APIs:

- `append_over_log_ch("CH0", value, limit)`
- `append_over_log_ch("CH1", value, limit)`
- old `append_over_log(value, limit)` is kept and maps to CH0

Query output:

`YYYY-MM-DD HH:MM:SS | CHx | limit | value`

If no logs:

`empty`

Important:

Because `over_log_t` changed size, old FlashDB over logs may decode badly. After flashing this version, send `0603` once before alarm tests.

### OLED / LED

Current intended behavior:

- OLED line 1: team ID `2026639584`.
- OLED line 2: `IDLE` or `AutoSample`.
- LED1: toggles every 1s while APP runs.
- LED2: on during sampling or protocol auto report, off otherwise.

OLED uses `OLED_ShowString()` directly (not `OLED_Printf()`) because the embedded
`vsprintf` inside `OLED_Printf` drops the first character with `%-16.16s` format.

Font table note:
`asc2_1608[33]` (uppercase `A`) was fixed on 2026-06-06. Only the 16 bytes for
`A` were changed in `Driver/HardWare/OLED/OLEDfont.h`.

### Baud Rate

Global:

`uint8_t usart1_baud_mode`

Mappings:

- `0x11`: 4800
- `0x12`: 9600
- `0x13`: 19200
- `0x14`: 115200

Default:

`0x13`

`01A2` saves `baud_mode`, replies OK, then resets.

On boot, `Function.c` reads FlashDB config, sets `usart1_baud_mode`, and calls `USART1_Init()` again so the saved baud is used.

## Persistent Config

`data_cfg_t` currently contains:

```c
uint32_t sample_cycle;
uint16_t device_id;
float ratio_ch0;
float ratio_ch1;
float limit_ch0;
float limit_ch1;
float dac_volt;
uint8_t alarm_report_mode;
uint8_t baud_mode;
```

Defaults:

- `sample_cycle = 5000`
- `device_id = 0x0001`
- `ratio_ch0 = 1.0`
- `ratio_ch1 = 1.0`
- `limit_ch0 = 100.0`
- `limit_ch1 = 100.0`
- `dac_volt = 1.0`
- `alarm_report_mode = 2`
- `baud_mode = 0x13`

Note:

Adding fields changes struct size. Existing FlashDB config may reset to defaults if read length differs.

## ADC / DAC Notes

CH0:

- board potentiometer
- `ADC_get()`

CH1:

- DAC feedback, PA4 jump to PC1
- `ADC_get_ch1()`

ADC uses scan mode plus DMA buffer of 2:

- `adc_value[0]`: CH0, PC0, ADC_CHANNEL_10
- `adc_value[1]`: CH1, PC1, ADC_CHANNEL_11

CH1 instability was fixed by using the same ADC hardware oversampling / scan DMA path as CH0.

DAC set command `0301` was hardware-tested:

- raw `0800` gives about `1.6495V`
- user measured DAC output and CH1 both OK

## Known Caveats / Review Items

1. `USART1_Init()` is called before FlashDB read, then again after config read.
   - This is intentional: boot default 19200 first, then switch to persisted baud.
2. `Function.c` old unused `power_count` is commented out.
3. Several `Function/*.c/h` files are not UTF-8. `apply_patch` may fail. Use very small byte-level replacements if needed.
4. Avoid editing old `Function/myflash.c`; it has stale duplicate log code and should not be compiled in the active EIDE project.
5. CH2 threshold commands `0x0403` / `0x0413` are not implemented in command table.

## Recent Changes (session 2026-06-06)

- `0261` set report interval: changed from 4-byte `uint32_t` ms to 1-byte mapping
  (`01=1s`, `02=3s`, `03=5s`). Test frames updated in this file.
- Team number: `DEVICE_ID` changed from `"Device_ID:2026-WUT-QRS-9"` to
  `"2026639584"`. OLED startup now reads from FlashDB, with fallback to literal.
- OLED '2' issue: `OLED_Printf` with `%-16.16s` drops first char. Fixed by using
  `OLED_ShowString()` directly. All three OLED refresh paths updated.
- OLED 'A' issue: `asc2_1608[33]` fixed with an 8x16 bitmap. `OLEDfont.h` is
  non-UTF8/GBK, so future edits should avoid rewriting the whole file.
- Heartbeat frame truncated: `USART1_SendData()` was missing
  `usart_flag_clear(USART1, USART_FLAG_TC)` before starting DMA. Fixed.
- Sleep `0x03AA`: restored commented-out length validation (`frame->length != 0`).
- Debug prints (`====system init====`, etc.) in `Function.c` commented out.
- `cmd_parse()` call in main loop commented out — was triggering
  `[ERROR] Unknown Command` on protocol frames.
- Auto report stop (`0303`): now sets `oled_idle_time = 10` so OLED refreshes to
  `IDLE` immediately.
- `LED.c` comment corrected: PB12/PB14 instead of wrong PB4/PB6.
- Error frames (CRC error, length mismatch, unknown command) all tested OK,
  returning `A5B6 xx FF EEEE 00 02 CRC B6A5`.

## Useful Test Frames

Assume device ID `0001`.

Heartbeat find:

`A5B6FFFF05FFFF00021C10B6A5`

Get ID broadcast:

`A5B6FFFF010111000201B0B6A5`

Get baud:

`A5B600010101120002D05AB6A5`

Set baud 115200:

`A5B600010101A20102143427B6A5`

Set baud 19200:

`A5B600010101A2010213F666B6A5`

Start auto report:

`A5B600010103020002AD5AB6A5`

Stop auto report:

`A5B6000101030300026D0BB6A5`

Set alarm active:

`A5B600010106010102017F71B6A5`

Set alarm passive:

`A5B600010106010102027E31B6A5`

Query alarm logs:

`A5B600010106020002615AB6A5`

Clear alarm logs:

`A5B600010106030002A10BB6A5`

Sleep:

`A5B600010103AA00024DDBB6A5`

Set DAC raw `0800`:

`A5B600010103010202080065B9B6A5`

Set CH1 ratio `1.0`:

`A5B6000101024204023F800000490FB6A5`

Set CH0 threshold `10.5`:

`A5B60001010411040241280000AE75B6A5`

Set CH1 threshold `10.5`:

`A5B60001010412040241280000AE46B6A5`

Set report interval 1s (1-byte mapping):

`A5B6000101026101010201BF9EB6A5`

Set report interval 3s (1-byte mapping):

`A5B6000101026101010202BEDEB6A5`

Set report interval 5s (1-byte mapping):

`A5B60001010261010102037E1FB6A5`

## Suggested Next Steps

1. Hardware-test `01A2`:
   - query baud at 19200
   - set 115200
   - observe OK at 19200
   - switch serial tool to 115200 after reboot
   - query baud returns `14`
   - set back 19200
2. Test `0261` report interval:
   - send 1s / 3s / 5s mapping frames
   - start `0302`
   - confirm report period changes
3. Test auto report plus alarm:
   - send `0603`
   - set low threshold
   - send `0601 02`
   - send `0302`
   - wait samples
   - send `0303`
   - send `0602`, should print records
4. Test active alarm:
   - send `0601 01`
   - send `0302`
   - data frame should be followed by direct string alarm if over limit
5. After APP protocol is stable, move to Bootloader / OTA in separate project.

## Last Local Checks

Used temporary stub `HeaderFiles.h` to run syntax checks:

- `gcc -fsyntax-only` on `Protocol/msg.c`
- `gcc -fsyntax-only` on `Function/Function.c`
- `gcc -fsyntax-only` on `Driver/HardWare/USART/USART.c`

All passed after baud changes.

Remaining known warning:

- old unused `power_count` in `Function.c`
