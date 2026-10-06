# Teensy Electronic leadscrew
This is an implementation of an electronic leadscrew using a Teensy microcrontroller.

# Architecture
We use [https://platformio.org/](platformio) as the infrastructure to build this project.

# Installation
You will currently need to setup platformio to build this, it can also handle installation to your teensy as well.
NOTE: you will have to modify some of the configuration to get this set up for your particular lathe, instructions coming soon.

# Testing
We aim to have the business logic of this testable locally without having to program any hardware or run tests on hardware (for the most part)
To test you just need to run the `pio test --environment native` command to run what tests we have written

### Reproducible software gates

Use PlatformIO Core 6.2.0. Dependencies and both Arduino cores are pinned in
`platformio.ini`; the ESP32 build deliberately retains Arduino 2.0.17.
Native tests use production spindle/controller math with a controlled 32-bit
`elsMicros()` clock. Button tests also compile production `ButtonHandler` and
real AbleButtons0.4 against simulated GPIO/time; OLED tests simulate Wire errors.

```sh
pio test -e native
ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 pio test -e native_sanitized
pio run -e teensy41 -e teensy41_debug
cp src/SECRETS-SAMPLE.h src/SECRETS.h # isolated build only; empty compile fixtures
pio run -e esp32dev
python3 -m pip install -r requirements-capture.txt
python3 -m unittest discover -s tests -p 'test_event_capture.py' -v
```

CI builds without uploading. Teensy uses SSD1306 at I2C address `0x3C`, reset
`-1`, default Wire SDA/SCL 18/19; ESP32 uses ST7789. These build profiles do
not establish installed wiring or qualify an external motor driver. Do not
flash a live coupled machine to verify these fixes.

### Button hold semantics

AbleButtons0.4's effective timing is preserved: debounce50ms, held1000ms,
double-click window500ms. The old `Button setHeldTime(50)` and
`Button setClickTime(200)` declarations constructed unused local buttons; they
never configured timing and are removed, not replaced with a50ms hold.
A short mode-button click changes feed/thread once. A long hold changes units
once and its release does not also click. Each jog button independently toggles
its stop once per hold. Debounced release rearms that button's latch; a press
observed while locked (or jog-blocked by enabled motion) is consumed until release.
ESP32's separate keypad already consumes its held event once; it is not retuned.

### OLED communication faults

The SSD1306 wrapper retains the vendor renderer/buffer but checks each frame
window-command and data transfer through Wire, including short writes and
nonzero `endTransmission` results. Data chunks fit a32-byte Wire buffer.
Allocation or initial checked-frame failure inhibits startup; runtime failure
latches inhibit, reports `DISPLAY_COMM_FAILED`, and stops controls/rearm.
Existing accepted pulses still finish and are accounted exactly once before
idle-low; ESP32's service task does not strand an in-flight completion merely
because controls are no longer ready. No retry, reset or bus recovery is added;
the pinned Wire implementation's existing bounded timeout/recovery still applies.

**Limit:** vendor `begin()` initialization commands retain internally ignored
transfer statuses. Successful subsequent frame commands/data do not prove that
all initial panel configuration succeeded, or that physical pixels changed.
ST7789 has no equivalent communication-status check here. Native transfer/pulse
tests are simulations, not electrical, optical or powered-machine qualification.



## Debugging
To test this application, you will be limited to setting breakpoints on unit tests only as the Teensy series of boards do not support hardware debuggers directly.

The "blessed" way to debug is to:
- Use vscode
- Set your environment to "native" (this is the default)
- Ensure that 'gdb' is installed
- Set your breakpoints in your code to test
    - note: if you want to debug a single test, set the test name in `test_debug` in platformio.ini
- go to the debug panel in vscode and run the "PIO debug" target
- ???
- Profit!

## Timing contract

Controller and spindle service must execute at least once per `2^32` microseconds.
Unsigned elapsed-time arithmetic supports rollover and arbitrarily long idle
periods with periodic service. Multiple wraps without execution cannot be
reconstructed; a stalled application requires reset/re-arm and separate
liveness investigation. Velocity expires after one second and resumes only
from a fresh measurement window. Unrepresentable spindle deltas latch a motion
fault until reset.

Synchronization uses an explicitly established stop anchor, never an unset
stop's sentinel coordinate. Setting the first stop at the current tool position
registers spindle phase; editing/removing that anchor extrapolates before
changing its coordinate. Invalid phase arithmetic inhibits registration.
An explicit enable without an anchor follows relative spindle movement but
does not claim thread registration. Invalid motion ratios remain reset-latched.

## Output and machine qualification

Only the controller services STEP/DIR. Disable finishes an already-started pulse
once, then stays LOW, discards spindle backlog, and resets the acceleration
profile. Position counts completed pulses; the external driver's counted edge
must be measured. Re-enable requires a fresh explicit request. Initialization,
arithmetic, pulse IO and explicit fault inhibits remain latched until reset;
clearing the boot inhibit never requests motion. OTA waits for output idle.

`ELS_STEP_HIGH_US` / `ELS_STEP_LOW_US` default to 4/4 µs on Teensy and 20/20 µs
on ESP32. `ELS_DIR_SETUP_US` / `ELS_DIR_HOLD_US` default to 4/4 µs. These are
unqualified software defaults, not driver specifications. Override upward to
the driver's published minima rounded to the timer resolution. ESP32 submits
one persistent RMT item and polls completion without blocking.

ENA retains its static LOW compatibility level; pause does not remove holding
torque. LOW is not asserted to be electrically disabled. Before deployment,
record board/display, driver/configuration, ENA polarity, counted STEP edge,
high/low and DIR timing minima, and reset pull states. With motor power isolated
and an independent inhibit available, scope power-on/reset, pause in each pulse
phase, reversal, initialization failures and controller faults. Qualify the
hardware safety chain separately; firmware cannot control pre-setup reset pins.

RPM display accepts finite values from -999 through 9999, truncating toward
zero. Out-of-range/nonfinite samples display ` ---RPM` in the same bounded
seven-character field. Native tests compile the production TFT rendering/cache
against a recording screen; this is not physical panel verification.

## Reset evidence and startup

Startup initializes inhibited outputs before Serial, captures the old reset
evidence, and shows `BOOT / LOCKED` for two seconds without blocking diagnostic
service. Motion timers/tasks start only after that interval; all creation
results are checked while still inhibited. Successful startup remains locked
and disabled. Any initialization failure shows `BOOT / FAULT` when the display
is available and leaves diagnostic service running without accepting controls.

A single 4096-byte NUL-terminated RAM report retains the current boot's capture.
It contains the current capturing firmware revision, build target/date/time,
reset flags/reason, reset detail, crash presence and capture uptime. On Teensy,
flags/detail are saved `SRC_SRSR`/`SRC_GPR5`; CrashReport is rendered once into
RAM before writing new breadcrumbs. Thermal reset skips destructive core
printing/clearing and all breadcrumbs, reports `THERMAL_RESET_REPORT_DEFERRED`,
and inhibits startup. ESP32 captures `esp_reset_reason()`, not a Teensy report.

Connect Serial after boot, or send `R`/`r` to replay the same retained report.
Each foreground service consumes at most one command byte and transmits at
most 64 bytes, limited by writer capacity and actual bytes written. Disconnect
and backpressure do not discard evidence; overflow is marked `TRUNCATED`.
Teensy USB connection detection restarts replay. ESP32's UART Serial API does
not expose USB-bridge host connection state: use `R` after connecting there.
Retention is RAM for the running boot only, not persistent crash history.

### Opt-in progress and durable receive-only capture

`ELS_PROGRESS_TELEMETRY` adds bounded foreground progress output. It is enabled
in `teensy41_debug` and native test profiles, **off in normal release/ESP32
profiles** unless explicitly defined. It does not enable motion or add recovery.
No formatting or serial output runs in the motion ISR. After boot-report drain,
at most one progress record is queued per second in a256-byte buffer. The shared
writer sends at most64 bytes per call according to available capacity/actual
writes; partial records finish before replay/failure text, without waiting for
host capacity. Reconnect drops an old partial progress record and replays boot
evidence first. Backpressure can delay a queued snapshot.

```text
ELS_PROGRESS source=<FIRMWARE_REVISION> us=<uint32> loop=<uint32> timer=<uint32> frames=<uint32> oled_fault=<0|1> ready=<0|1>
```

`loop` increments before main diagnostic service; `timer` counts completed
callbacks; `frames` counts runtime display-call returns, including a returning
failed attempt—not successful physical frames. Fields are independently
observed, not an atomic safety-state snapshot. `ready` is controls/service
readiness, not a powered-stage or motion-inhibition measurement. All counters
wrap at32 bits; `us` wraps approximately every71.6minutes. A blocked foreground
can prevent output even while interrupts run; USB presence alone proves no
application progress. The original intermittent reset cause remains unknown.

Install optional pyserial with `python3 -m pip install -r requirements-capture.txt`.
For a separately authorized receive-only observation, set `ELS_PORT` to the
explicit serial path and `ELS_SOURCE` to the **exact installed image's emitted
source token**, not merely the revision of a newly compiled artifact:

```sh
python3 scripts/els_event_capture.py \
  --port "$ELS_PORT" --expected-source "$ELS_SOURCE" \
  --output .validation/capture-session --duration 3600 \
  --stall 3 --grace 5 --max-bytes 1048576 --files 4 --fsync-interval 1
```

Default baud is921600. The recorder also explicitly supports installed legacy
`ELS_LEGACY_TEST_20261006_C` and its pinned full source SHA, using `ms`, `fg`,
`timer_done`, `frames` and inhibited-state fields. That support does not equate
C with this PR firmware or the unknown original installed image. Boot-report
text alone never establishes ongoing identity/progress; ordinary release
firmware without progress output cannot satisfy unattended stall monitoring.

Raw bytes and monotonic/UTC JSONL events append and flush during capture.
Each stream retains at most `--files` files of `--max-bytes` bytes; older data
is deliberately rotated away. Use a dedicated output directory and archive a
fault window before retention overwrites it. `--fsync-interval` bounds the
unsynced loss window; rotation/close also sync. Source/state validation restarts
per connection, with bounded `--reconnects`/`--reconnect-delay`. Alerts distinguish
disconnect, missing heartbeat, stalled counters, wraps and reset candidates;
they do not diagnose power/EMI or prove a physical display failure.
Exit0 means valid ready samples and no observed fault; exit1 means an observed
fault/missing progress/invalid input; exit2 means an I/O/setup/durability failure.
Ending a bounded capture mid-line is an audited truncation, not a device fault.

The recorder sends no commands, uploads or autonomous resets and performs no
implicit device discovery. **Opening a port may change DTR/RTS** through OS/
pyserial defaults; receive-only is not a hardware no-reset guarantee. Host
unit/fake-port/PTY tests are simulations, not live PR firmware validation.


`scripts/build_identity.py` supplies git's 12-character revision plus `-dirty`
when applicable; source exports report `unknown`. Identity labels the capturing
firmware: retained crash data may predate the last flash. CI archives the ELF,
firmware image, identity file and resolved package manifest. Retain those exact
artifacts for each bench test; do not diagnose a crash with a different ELF.

Physical acceptance remains required: on an isolated bench, verify cold boot,
controlled software reset, a deliberately faulting diagnostic build, delayed
Serial connection/replay, reset reason and boot banners, and no motion
restoration. Compare STEP/DIR timing with Serial absent, backpressured and
draining. No controlled crash/reset tests on a live coupled machine. Software
tests establish neither a particular reset cause nor EMI immunity.
