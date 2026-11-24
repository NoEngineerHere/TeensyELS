"""Check the OTA descriptor in a built ESP32 app against its compiled wiring."""

import argparse
import json
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ESP_ENVS = ("esp32dev", "esp32_halfnut", "esp32_oled")
ESP_DESC_OFFSET = 24 + 8 + 256


def descriptor(options):
    flags = [arg for line in options["build_flags"] for arg in shlex.split(line)
             if arg.startswith("-DELS_")]
    header = options["custom_hardware_config"]
    source = """
#include <cstdio>
#include <firmware_descriptor.h>
int main() {
  constexpr auto descriptor = makeFirmwareDescriptor(hardware, pulseTiming);
  return std::fwrite(descriptor.data(), 1, descriptor.size(), stdout) != descriptor.size();
}
"""
    with tempfile.TemporaryDirectory() as tmp:
        program = Path(tmp) / "descriptor"
        subprocess.run(
            ["c++", "-std=gnu++17", "-Iinclude", "-Ilib/board", "-Ilib/leadscrew", *flags,
             f'-DELS_HARDWARE_CONFIG="{header}"', "-x", "c++", "-", "-o", str(program)],
            input=source, text=True, cwd=ROOT, check=True,
        )
        return subprocess.check_output([program])


def verify(environment, options):
    directory = ROOT / ".pio" / "build" / environment
    data = (directory / "firmware.bin").read_bytes()
    expected = descriptor(options)
    end = ESP_DESC_OFFSET + len(expected)
    if (len(data) < end or data[0] != 0xe9 or data[12:14] != b"\0\0" or
            struct.unpack_from("<I", data, 32)[0] != 0xabcd5432):
        raise SystemExit(f"{environment}: not a complete classic ESP32 app prefix")
    segment_end = 32 + struct.unpack_from("<I", data, 28)[0]
    if not end <= segment_end <= len(data) or data[ESP_DESC_OFFSET:end] != expected:
        raise SystemExit(f"{environment}: wrong descriptor at app offset 0x{ESP_DESC_OFFSET:x}")
    partitions = (directory / "partitions.bin").read_bytes()
    app_sizes = []
    for offset in range(0, len(partitions) - 31, 32):
        magic, kind, subtype, _, size = struct.unpack_from("<HBBII", partitions, offset)
        if magic != 0x50aa:
            break
        if kind == 0 and subtype in (0x10, 0x11):
            app_sizes.append(size)
    if len(app_sizes) != 2 or len(data) > min(app_sizes):
        raise SystemExit(f"{environment}: image does not fit both OTA app slots")
    print(f"{environment}: descriptor at 0x{ESP_DESC_OFFSET:x}; {len(data)}/{min(app_sizes)} app bytes")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("environments", nargs="+", choices=ESP_ENVS)
    args = parser.parse_args()
    project = dict(json.loads(subprocess.check_output(["pio", "project", "config", "--json-output"], cwd=ROOT)))
    for environment in args.environments:
        verify(environment, dict(project[f"env:{environment}"]))


if __name__ == "__main__":
    main()
