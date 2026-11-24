from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def compiler(header, processor, screen, *extra):
    cmd = ["c++", "-std=gnu++17", "-Iinclude", "-Ilib/board", "-Itest/fixtures",
           f'-DELS_HARDWARE_CONFIG="{header}"',
           f"-DELS_NATIVE_PROCESSOR=Processor::{processor}", *extra]
    if screen is not None:
        cmd.append(f"-DELS_SCREEN={screen}")
    return [*cmd, "-x", "c++", "-"]


def check(header, processor, screen, error=None, *extra):
    result = subprocess.run(
        [*compiler(header, processor, screen, *extra), "-fsyntax-only"],
        input="#include <board.h>\n", cwd=ROOT, text=True, capture_output=True,
    )
    if error is None and result.returncode:
        raise AssertionError(f"{header}: {result.stderr}")
    if error and (not result.returncode or error not in result.stderr):
        raise AssertionError(f"{header}: expected {error!r}\n{result.stderr}")


for preset, processor, screen in (
    ("teensy_direct", "Teensy41", 1), ("teensy_pcb", "Teensy41", 1),
    ("lilygo", "Esp32", 2), ("halfnut", "Esp32", 3), ("esp32_oled", "Esp32", 1),
    ("teensy_tft", "Teensy41", 2), ("teensy_lvgl", "Teensy41", 3),
):
    check(f"els/presets/{preset}.h", processor, screen)

check("els/presets/teensy_direct.h", "Esp32", 1, "Hardware processor mismatch")
check("els/presets/teensy_direct.h", "Teensy41", None, "Missing or unknown ELS_SCREEN selector")
check("invalid_hardware_probe.h", "Esp32", 1, "Invalid GPIO direction or pin", "-DPROBE_ESP_OUTPUT")
check("invalid_hardware_probe.h", "Teensy41", 2, "Invalid display bus routing")
check("invalid_hardware_probe.h", "Teensy41", 2, "Hardware pin collision", "-DPROBE_COLLISION")

# Compile the production bus adapter twice. Change only a user header, not the
# adapter, selected screen or processor, and read back actual transport calls.
source = """
#include <cstdio>
#include "display_bus_teensy.cpp"
int main() {
  if (!initializeDisplayBus()) return 1;
  const auto& bus = displayI2cBus();
  std::printf("%d %d %d %d", Wire.begins, Wire1.begins, bus.sda, bus.scl);
}
"""
with tempfile.TemporaryDirectory() as d:
    name = "my_hardware.h"
    header, program = Path(d) / name, Path(d) / "probe"
    for bus, sda, scl in ((0, 18, 19), (1, 44, 45)):
        preset = ROOT / "include/els/presets/teensy_direct.h"
        if bus == 0:
            header.write_text(preset.read_text(encoding="utf-8"), encoding="utf-8")
        else:
            header.write_text(f'''#include <els/presets/teensy_direct_wiring.h>
#define ELS_INPUT_LAYOUT ELS_INPUT_DIRECT
constexpr HardwareConfig makeHardwareConfig() {{
  auto c = teensyDirectWiring();
  c.oled = {{{bus}, {sda}, {scl}}};
  return c;
}}
''', encoding="utf-8")
        subprocess.run(
            [*compiler(name, "Teensy41", 1, "-I", d, "-Itest/mocks/display", "-DELS_TEST_TEENSY_BUS"),
             "-o", str(program)], input=source, cwd=ROOT, text=True, check=True,
        )
        readback = subprocess.check_output([program], text=True)
        expected = f"{int(bus == 0)} {int(bus == 1)} {sda} {scl}"
        if readback != expected:
            raise AssertionError(f"custom Wire route: expected {expected!r}, got {readback!r}")
