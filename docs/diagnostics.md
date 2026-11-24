# Serial diagnostics

## Boot report

The firmware starts with motion disabled and shows `BOOT / LOCKED` for two seconds. It checks encoder setup, the display bus, required timers and tasks, and the first OLED frame before allowing controls. It stays locked and disabled after a successful boot. If a check fails, it stays disabled and shows `BOOT / FAULT` if the screen still works.

The report records the firmware revision, build target and time, reset reason, crash status and capture uptime. Teensy saves `SRC_SRSR`, `SRC_GPR5` and the available `CrashReport` before writing new reset information. After a thermal reset it leaves the crash evidence untouched, reports `THERMAL_RESET_REPORT_DEFERRED` and keeps motion disabled. ESP32 records `esp_reset_reason()`.

The report lives in RAM for this boot only. Teensy replays it on a USB serial connection. On ESP32, send `R` or `r` after connecting to replay it. A report that exceeded its buffer ends with `TRUNCATED`.

## Progress and motion logs

`teensy41_debug` enables `ELS_PROGRESS_TELEMETRY` and `ELS_MOTION_TRACE`. Normal release builds do not log either unless enabled at build time. Trace logging requires progress logging. Neither option enables motion. The motion loop does not format or write serial messages.

Once the boot report has gone out, the firmware queues up to one progress line a second:

```text
ELS_PROGRESS source=<revision> us=<uptime> loop=<foreground count> timer=<motion count> frames=<display count> oled_fault=<0|1> ready=<0|1>
```

`timer` counts finished motion callbacks. `frames` counts display calls that returned, including failed writes. `ready` means the controls and services can run. It does not report driver power or whether motion is enabled. Fields are sampled separately. The counters wrap at 32 bits, so `us` wraps about every 72 minutes. Serial congestion can delay a line.

The firmware samples motion at most once per 10 ms. Each line gives the callback duration in microseconds, the sampled encoder delta in counts, and the following error in step pulses. The queue holds 16 samples and counts drops when the writer falls behind. The recorder saves these as `motion_trace` events. They don't count as progress heartbeats.

## Record a session

Install pyserial with `python3 -m pip install -r requirements-capture.txt`.
Set `ELS_PORT` to the serial device and `ELS_SOURCE` to the source label printed by the installed firmware. A local build may have a different label.

```sh
python3 scripts/els_event_capture.py \
  --port "$ELS_PORT" --expected-source "$ELS_SOURCE" \
  --output .validation/capture-session --duration 3600 \
  --stall 3 --grace 5 --max-bytes 1048576 --files 4 --fsync-interval 1
```

The default baud rate is 921600. The recorder also recognizes the pinned legacy `ELS_LEGACY_TEST_20261006_C` image. It cannot check for stalls if the firmware sends only a boot report. The recorder sends no commands and doesn't search for ports. Opening a port can still change DTR or RTS and may reset the board.

Use a dedicated output directory. The recorder appends raw bytes and timestamped JSONL events and rotates both files. Save the files you need before rotation deletes them. `--fsync-interval` controls how often the recorder syncs to disk. After reconnecting, it checks the source and progress again. A capture ending mid-line is recorded separately from a bad line sent by the firmware.

The recorder exits with `0` if it saw valid progress and no faults, `1` for bad input, missing progress or an observed fault, and `2` for setup or I/O errors. An alert can show a serial disconnect or stalled counter. It cannot tell you whether power, interference or the screen caused it.

## Investigate a reset

Save the firmware image, ELF, build identity and package manifest from the exact build on the board. The revision in the report identifies the firmware that captured it, but saved crash data can come from an earlier flash.

Check boot, software reset, crash reporting and delayed serial replay on an isolated bench. Confirm that motion stays disabled after reset. Compare STEP and DIR timing with serial disconnected, blocked and reading normally. Don't force a crash on a coupled machine.
