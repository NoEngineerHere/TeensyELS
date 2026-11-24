# Hardware and setup

Pick the PlatformIO environment for your board and screen. These presets describe wiring. They have not all been bench-tested.

| Environment | Board or wiring | Controls | Screen |
|---|---|---|---|
| `teensy41`, `teensy41_debug` | Teensy 4.1 | Direct buttons | SSD1306 128×64 |
| `esp32dev` | LilyGO T-Display | Legacy matrix and encoder | ST7789 240×135 |
| `teensy41_pcb` | Teensy 4.1 on the project PCB | Legacy matrix and encoder | SSD1306 128×64 |
| `esp32_halfnut` | HalfNut Mk2 PCB | Mk2 matrix and encoder | ST7789 320×240, LVGL |
| `esp32_oled` | ESP32 with HalfNut controls and separate OLED wiring | Mk2 matrix and encoder | SSD1306 128×64 |
| `teensy41_tft` | Teensy 4.1 with custom TFT wiring | Direct buttons | ST7789 240×135 |
| `teensy41_lvgl` | Teensy 4.1 with custom TFT wiring | Direct buttons | ST7789 320×240, LVGL |

The Teensy PCB preset matches the [two-layer](../kicad/2Layer) and [four-layer](../kicad/4Layer) boards. The Mk2 preset follows Martin Long's [board header](https://github.com/martinlong1978/HalfNutELS/blob/be12236846e6f1342a6e1437ec513952e00375c/lib/config/board.h) and [build settings](https://github.com/martinlong1978/HalfNutELS/blob/be12236846e6f1342a6e1437ec513952e00375c/platformio.ini). It runs TeensyELS, not the HalfNutELS interface. The OLED example does not use an OLED connector on the Mk2 PCB.

## Custom wiring

Copy a header from [`include/els/presets/`](../include/els/presets/) to `include/my_hardware.h`. Edit the pins in `makeHardwareConfig()` and choose direct, legacy matrix or Mk2 matrix controls with `ELS_INPUT_LAYOUT`. Add an environment to [`platformio.ini`](../platformio.ini) that inherits the processor and screen you want:

```ini
[env:my_teensy_tft]
extends = env:teensy41_tft
custom_hardware_config = my_hardware.h
```

Build with `pio run -e my_teensy_tft`. The parent environment supplies the screen driver and its libraries, so don't define `ELS_SCREEN` in the wiring header.

Use `-1` for an absent reset, backlight, alarm or indicator. Set both UI encoder pins or neither. The motion pins and pins for your selected controls and screen are required, apart from the optional pins above and ESP32 SPI MISO. Unused groups don't reserve pins. Teensy SPI requires a valid MISO pin even if you don't connect it to a write-only screen. The TFT layouts use landscape rotation 1 or 3.

| Teensy 4.1 bus | Pins |
|---|---|
| I2C 0 (Wire) | SDA 18, SCL 19 |
| I2C 1 (Wire1) | SDA 17 or 44, SCL 16 or 45 |
| I2C 2 (Wire2) | SDA 25, SCL 24 |
| SPI 0 | MOSI 11, SCK 13, MISO 12 |
| SPI 1 | MOSI 26, SCK 27, MISO 1 or 39 |
| SPI 2 | MOSI 43 or 50, SCK 45 or 49, MISO 42 or 54 |

ESP32 supports I2C buses 0 and 1. SPI bus 0 is VSPI and bus 1 is HSPI. Don't use GPIO 6 through 11, which are wired to flash. GPIO 34 through 39 are input-only and don't have internal pulls. The ESP32 encoder presets need external bias. Check the boot strap levels with the keypad and other circuits connected.

## Mk2 controls

| Key | Action |
|---|---|
| MODE | Click to cycle Feed, Thread and Reverse while disabled. Hold to change units. |
| RATE | Open pitch and jog controls. Use the encoder to change pitch. |
| STOPS | Use the arrows or encoder to select a stop. Press OK to set or clear it. |
| LEFT / RIGHT | In Rate, click to jog or hold to set or clear a stop. In Menu, move through the entries. |
| OK | In Rate, click to sync or hold to reset the display. In Menu, select the current entry. |
| MENU | Open or close the menu. Use the arrows or encoder to select an entry. |
| ENABLE | Toggle motion when unlocked and not jogging. |
| HALT | Stop on press, even when locked. Does not cut power. |

Lock and Unlock are available from the menu. The matrix scanner debounces keys for 10 ms and emits one hold after one second. Pressing several keys together cannot start motion.

## Updating over Wi-Fi

Install the first ESP32 image over USB. To update, disable motion and unlock the panel, then choose Update and press OK on Mk2 or hold the half-nut button on legacy or direct panels. The update network starts only after motion output is idle. Teensy updates always use a cable.

Join the `ELS-…` network shown on the display. Combine the two halves of the password. The ESP32 allows one connection and uses a new password each time. Set `ELS_ENV` to the matching build environment and `ELS_AP` to the IP on the display. Send `firmware.bin`, not a merged flash image:

```sh
curl --fail --upload-file ".pio/build/$ELS_ENV/firmware.bin" \
  -H 'Content-Type: application/octet-stream' "http://$ELS_AP/firmware"
```

The network closes when the update finishes or fails, after five minutes of waiting, or if an upload stalls for ten seconds. A successful update starts the new image locked and disabled. On failure, motion stays disabled until you reboot locally. The new image must match the processor, wiring, controls, screen and pulse timing. Use a cable if those change. This check prevents accidental mismatches. It does not authenticate firmware.

## Bench checklist

With motor power disconnected and an independent way to inhibit the drive:

1. Check pin levels at power-on and reset, encoder direction and detents, every button and hold action, screen rotation, LEDs and alarm handling. Check buttons during a screen refresh. A full TFT refresh can delay polling on Teensy.
2. Check the driver's ENA polarity and which edge it counts. Firmware leaves ENA low. Pause does not remove holding torque, and firmware cannot control the pin levels before startup.
3. Measure STEP high and low times and DIR setup and hold times under display and input load. Repeat for reversal, pause during a pulse and faults. A healthy output finishes a pulse already in progress and starts no more after disable. A pulse output fault can leave the carriage position unknown and prevent updates.
4. Check both stops and repeat thread pickup in both directions before cutting. With a fresh speed reading and room for the ramp within half a turn, the carriage can start accelerating before the saved spindle phase. It returns to the same thread path as it gets up to speed. Run-up depends on pitch, speed and acceleration. Reversing the spindle while synced disables the cut.
5. If you use Wi-Fi updates, check that the wrong password cannot join, a mismatched image leaves the old app selected, and a matching image restarts locked and disabled.

`ELS_STEP_HIGH_US` and `ELS_STEP_LOW_US` default to 4 µs on Teensy and 20 µs on ESP32. `ELS_DIR_SETUP_US` and `ELS_DIR_HOLD_US` default to 4 µs. Set them to meet the connected driver's minimums, rounded up to the timer resolution.

On the HalfNut preset, a low level on GPIO 27 inhibits motion until reset, even if it clears during startup. A disconnected alarm cable reads healthy. The Teensy PCB preset does not use its GPIO 39 alarm, and the legacy presets don't provide alarm protection.

The OLED checks frame writes. A failed write stops new pulses and reports `DISPLAY_COMM_FAILED`. Successful writes do not prove that the panel displayed the frame. The TFT driver cannot detect whether a panel is connected.
