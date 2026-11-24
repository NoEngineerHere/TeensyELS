# Teensy electronic leadscrew

TeensyELS is firmware for a lathe's electronic leadscrew. It reads the spindle
encoder and drives the carriage for feeding and threading. It runs on Teensy 4.1
and classic ESP32 boards.

## Architecture

[PlatformIO](https://platformio.org/) builds the firmware. Both processors use
the same motion controller. The build environment selects the processor and
screen. A wiring header specifies the pins and controls, and
[`lib/config/config.h`](lib/config/config.h) holds the lathe settings.

## Installation

1. Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html).
2. Set the leadscrew pitch, gearbox ratio and encoder counts for your lathe in
   [`lib/config/config.h`](lib/config/config.h).
3. Pick an environment in the [wiring guide](docs/hardware.md). Build with
   `pio run -e teensy41`, replacing `teensy41` if you use another board.
4. Connect the board over USB and upload with `pio run -e teensy41 -t upload`.
   For later updates on ESP32, see [updating over Wi-Fi](docs/hardware.md#updating-over-wi-fi).

Disconnect motor power before checking a new setup. Follow the
[bench checklist](docs/hardware.md#bench-checklist) before trying motion.
The panel's HALT key stops software motion. It does not cut motor power.

## Using the leadscrew

Feed follows the spindle. Thread cuts toward the left stop. Reverse cuts toward
the right stop with the same spindle rotation. Both threading modes need a
saved spindle phase and a stop ahead of the carriage. Setting the first stop
at the current carriage position saves the phase. Disable threading before
changing pitch or units.

The position readout measures from the left stop if set, then the right stop,
then the machine origin. It does not change either stop.

See the [Mk2 controls](docs/hardware.md#mk2-controls) for the panel keys, or
[serial diagnostics](docs/diagnostics.md) for boot reports and motion logs.

## Testing

Run `pio test -e native` to test motion, startup and controls without hardware.
The optional `native_sanitized` build checks for memory and undefined behavior
errors. `native_lvgl` tests the display offscreen. `native_hardware` checks
valid and invalid wiring headers.

For ESP32 builds, run
`python3 scripts/verify_esp_descriptor.py esp32dev esp32_halfnut esp32_oled`
after building those three environments to check their update descriptors.
These checks do not replace the bench tests.

### Debugging

To step through a host test, select `native` in VS Code, install GDB, and use
PlatformIO's debug configuration. For a fault on the board, keep the ELF from
the build you flashed and follow the [serial diagnostics guide](docs/diagnostics.md).

## License

See [GNU General Public License version 3](LICENSE).
Third-party dependencies retain their own licenses.
Martin Long's [HalfNutELS](https://github.com/martinlong1978/HalfNutELS) is the
reference for the Mk2 wiring.
