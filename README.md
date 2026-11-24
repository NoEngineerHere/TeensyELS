# Teensy electronic leadscrew

Electronic leadscrew firmware for Teensy 4.1 and classic ESP32. The same motion
controller handles both processors. The wiring header selects physical pins and
controls. The PlatformIO environment selects the processor and screen driver.
Lathe settings such as gear ratio, leadscrew pitch, encoder PPR and feed tables
stay in `lib/config/config.h`.

## Build and wiring

Use PlatformIO Core 6.2.0. The firmware environments pin their Arduino cores
and libraries. These are wiring examples, not bench qualification results.

| Environment | Processor and wiring | Controls | Screen |
|---|---|---|---|
| `teensy41`, `teensy41_debug` | Teensy 4.1 direct | Direct buttons | SSD1306 128×64 |
| `esp32dev` | LilyGO T-Display | Legacy matrix and encoder | ST7789 240×135 |
| `teensy41_pcb` | Teensy 4.1 on the repository PCB | Legacy matrix and encoder | SSD1306 128×64 |
| `esp32_halfnut` | ESP32 on the HalfNut Mk2 PCB | Mk2 matrix and encoder | ST7789 320×240, LVGL |
| `esp32_oled` | ESP32 with HalfNut input wiring and a custom OLED connection | Mk2 matrix and encoder | SSD1306 128×64 |
| `teensy41_tft` | Teensy 4.1 with custom TFT wiring | Direct buttons | ST7789 240×135 |
| `teensy41_lvgl` | Teensy 4.1 with custom TFT wiring | Direct buttons | ST7789 320×240, LVGL |

The Teensy PCB preset follows `kicad/2Layer` and `kicad/4Layer`. The Mk2 wiring
follows Martin Long's [board header](https://github.com/martinlong1978/HalfNutELS/blob/be12236846e6f1342a6e1437ec513952e00375c/lib/config/board.h)
and [build configuration](https://github.com/martinlong1978/HalfNutELS/blob/be12236846e6f1342a6e1437ec513952e00375c/platformio.ini).
The Mk2 keys and screen operate TeensyELS. This firmware does not run HalfNutELS
or use its assets. The OLED example does not imply an OLED connector on that PCB.

### Custom wiring

Copy the closest header from `include/els/presets/` to `include/my_hardware.h`.
For example, copy `teensy_tft.h`, edit `makeHardwareConfig()` and
`ELS_INPUT_LAYOUT`, and add this environment to `platformio.ini`:

```ini
[env:my_teensy_tft]
extends = env:teensy41_tft
custom_hardware_config = my_hardware.h
```

Run `pio run -e my_teensy_tft`. To change the screen, inherit an environment
for the same processor and the desired screen. The environment supplies the
screen selector and its libraries. Do not set `ELS_SCREEN` in the wiring
header or edit a driver to set pin numbers. You can choose direct, legacy
matrix or Mk2 matrix controls with any screen, subject to valid GPIO and bus
routes. Matrix rows and columns are GPIOs. The action assigned to a matrix
position comes from the selected layout.

Use `-1` for an absent reset, backlight, alarm or indicator. The UI encoder
requires both pins or neither. Motion pins and the selected input and screen
pins are required, except ESP32 SPI MISO may be absent. Inactive input and screen
groups do not reserve pins. Teensy SPI still reserves MISO for a write-only TFT,
though you do not need to wire that pin to the screen. TFT layouts accept
landscape rotations 1 and 3.

| Teensy 4.1 bus | Routes |
|---|---|
| I2C 0 (Wire) | SDA 18, SCL 19 |
| I2C 1 (Wire1) | SDA 17 or 44, SCL 16 or 45 |
| I2C 2 (Wire2) | SDA 25, SCL 24 |
| SPI 0 | MOSI 11, SCK 13, MISO 12 |
| SPI 1 | MOSI 26, SCK 27, MISO 1 or 39 |
| SPI 2 | MOSI 43 or 50, SCK 45 or 49, MISO 42 or 54 |

ESP32 I2C supports buses 0 and 1. ESP32 SPI bus 0 is VSPI and bus 1 is HSPI.
Route valid pins through the GPIO matrix. GPIO 6–11 are reserved for flash.
GPIO 34–39 are input-only and need external bias where pull resistors are
required. The shipped ESP32 spindle and UI encoders require external bias.
GPIOs used for boot straps must meet the ESP32 reset-level requirements; check
those levels with the keypad and external circuits connected.

### Controls and display faults

Direct buttons keep AbleButtons timing and existing click and hold actions.
The [portable matrix scanner](https://github.com/NoEngineerHere/TeensyELS/issues/40)
uses 10 ms debounce and a one-shot hold at one second. Ambiguous chords cannot
start motion. On Mk2, HALT stops on a debounced press in every view, even when
locked. It is a software stop, not an emergency stop or power isolation.

Mk2 MODE clicks change feed/thread and holds change units. RATE returns to pitch
and jog controls. STOPS gives the arrows and encoder control of stop selection;
OK sets or clears the selected stop. In Rate, OK syncs and a hold resets the
display. MENU opens or leaves the menu. Navigate with the arrows or encoder and
press OK to apply the selected entry. Lock and Unlock are available while
locked. ENABLE uses the normal motion and lock rules in every view.

The SSD1306 uses the configured bus and checks every frame command and data
chunk. A failed transfer latches inhibit and reports `DISPLAY_COMM_FAILED`.
Any accepted pulse still finishes once if the backend is healthy. Initial
frame transfer must finish before startup releases inhibit. The SSD1306
library cannot report every initialization command failure. A successful
transfer cannot prove pixels changed. ST7789 initialization does not detect
panel presence. Input polling and LVGL service target 5 ms; the status refresh
is 250 ms. A full TFT refresh can delay cooperative input polling on Teensy.

The carriage readout uses the left stop when set, the right stop otherwise,
then machine origin if neither is set. Its sign follows the machine coordinate.
The display does not change stop positions or thread phase.

### Local firmware update (ESP32)

Install the first image over a cable. For later updates, unlock the panel and
disable motion. Select **Update** in the Mk2 menu and press OK, or hold the
half-nut button on a legacy or direct panel. No receiver runs before local arm
and confirmed output idle. Teensy updates remain wired.

Join the `ELS-…` network shown on the screen. Enter the two displayed halves
of the WPA2 key as one password. The receiver allows one station, uses a new
key for each arm, and never connects to a LAN. Send the app binary, not a
merged flash image. Set `ELS_ENV` to the matching environment and `ELS_AP` to
the displayed IP:

```sh
curl --fail --upload-file ".pio/build/$ELS_ENV/firmware.bin" \
  -H 'Content-Type: application/octet-stream' "http://$ELS_AP/firmware"
```

The receiver closes after five minutes, or after a transfer stalls for ten
seconds. It also closes after a result. A failed update leaves motion inhibited
until a local reboot. A valid image reaches the inactive app slot and restarts
locked and disabled. The image must match the existing processor, wiring,
input layout, screen and pulse timing. Change those over a cable. The match
prevents accidental wrong-image uploads; it is not a firmware signature.

## Software checks

```sh
pio test -e native
pio test -e native_sanitized
pio test -e native_lvgl
pio test -e native_hardware
pio run -e teensy41 -e teensy41_debug -e esp32dev -e teensy41_pcb -e esp32_halfnut -e esp32_oled -e teensy41_tft -e teensy41_lvgl
python3 scripts/verify_esp_descriptor.py esp32dev esp32_halfnut esp32_oled
```

Native tests exercise production motion, panel and startup logic with simulated
peripherals. `native_lvgl` renders with LVGL into an offscreen SPI target.
`native_hardware` compiles real wiring headers and checks alternate bus routing.
No build or native test qualifies an external driver, display, radio or machine.


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
clearing the boot inhibit never requests motion. A terminal backend fault leaves
position uncertain and blocks update handoff.

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

On each board, first disconnect motor power. Verify cold-start pin levels,
encoder direction and detents, each key and hold, scan timing under screen
refresh, screen rotation, indicator polarity, and alarm and fault handling.
Then measure STEP high/low and DIR setup/hold under display and input load. Check
that inhibit drains one accepted pulse before trying mechanical motion.

HalfNut uses the active-low alarm on GPIO 27. A sampled assertion latches
inhibit until reset even if it clears during the boot banner. A disconnected
alarm cable reads healthy, so this is not failsafe. The Teensy PCB alarm on
GPIO 39 is not used; legacy presets do not provide alarm protection.

For ESP32 updates on an isolated bench, verify that the wrong WPA2 password
cannot join, a mismatched image leaves the old app selected, and a matching
image restarts locked. Host tests do not prove RF or flash behavior.

RPM display accepts finite values from -999 through 9999, truncating toward
zero. Out-of-range/nonfinite samples display ` ---RPM` in the same bounded
seven-character field. Native tests compile the production TFT rendering/cache
against a recording screen; this is not physical panel verification.

## Reset evidence and startup

Startup initializes inhibited outputs before Serial, captures the old reset
evidence, and shows `BOOT / LOCKED` for two seconds without blocking diagnostic
service. Output stays inhibited until the banner interval, initial OLED frame,
encoder attachment, display bus and required timer/tasks have passed their
available checks. Successful startup remains locked and disabled. A fault shows
`BOOT / FAULT` if the display can still render it; diagnostics continue without
accepting controls.

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
