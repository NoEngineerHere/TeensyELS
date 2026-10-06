#include "boot_diagnostics.h"
#include <clock.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#ifndef PIO_UNIT_TESTING
#include <Arduino.h>
#ifdef ESP32
#include <esp_system.h>
#endif
#endif
#ifndef FIRMWARE_REVISION
#define FIRMWARE_REVISION "unknown"
#endif

namespace {
char report[4096]{};
size_t length = 0, offset = 0;
bool captured = false, thermal = false, overflow = false, wasConnected = false;
constexpr char marker[] = "\nTRUNCATED\n";
constexpr size_t limit = sizeof(report) - sizeof(marker);
#ifdef ELS_PROGRESS_TELEMETRY
char progress[256]{};
size_t progressLength = 0, progressOffset = 0;
uint32_t lastProgress = 0;
static_assert(sizeof(FIRMWARE_REVISION) <= 64, "Progress identity must fit one bounded record");
#endif
void appendByte(char value) {
  if (overflow) return;
  if (length < limit) {
    report[length++] = value;
    report[length] = 0;
  } else {
    std::memcpy(report + length, marker, sizeof(marker));
    length += sizeof(marker) - 1;
    overflow = true;
  }
}
void append(const char* text) { while (*text) appendByte(*text++); }
#ifdef PIO_UNIT_TESTING
BootDiagnosticsTest::Port testPort;
#else
class ReportPrint : public Print {
public:
  size_t write(uint8_t value) override { appendByte(static_cast<char>(value)); return 1; }
};
#endif
}

bool captureBootDiagnostics() {
  if (captured) return !thermal;
  // Capture raw state before Print can consume/reset it or breadcrumbs are written.
#if defined(CORE_TEENSY)
  const uint32_t resetFlags = SRC_SRSR;
  const uint32_t resetDetail = SRC_GPR5;
  thermal = (resetFlags & SRC_SRSR_TEMPSENSE_RST_B) != 0;
  const bool crashPresent = bool(CrashReport);
  const char* target = "teensy41";
#elif defined(ESP32)
  const uint32_t resetFlags = static_cast<uint32_t>(esp_reset_reason());
  const uint32_t resetDetail = 0;
  const bool crashPresent = false;
  const char* target = "esp32dev";
#else
  const uint32_t resetFlags = testPort.resetFlags;
  const uint32_t resetDetail = testPort.resetDetail;
  thermal = testPort.thermal;
  const bool crashPresent = testPort.crashPresent;
  const char* target = "native-test";
#endif
  const uint32_t uptime = elsMicros();
  captured = true;
  append("Current capturing firmware: " FIRMWARE_REVISION "\nBuild: " __DATE__ " " __TIME__ "\nTarget: ");
  append(target);
  char details[160];
  std::snprintf(details, sizeof(details), "\nReset flags/reason: 0x%08lx\nReset detail: 0x%08lx\nCrash present: %s\nCapture uptime us: %lu\n",
      static_cast<unsigned long>(resetFlags), static_cast<unsigned long>(resetDetail),
      crashPresent ? "yes" : "no", static_cast<unsigned long>(uptime));
  append(details);
  append("A retained crash may predate the current firmware flash.\n");
  if (thermal) {
    append("THERMAL_RESET_REPORT_DEFERRED\n");
    recordBootFailure("THERMAL_RESET");
    return false;
  }
#if defined(CORE_TEENSY)
  ReportPrint sink;
  CrashReport.printTo(sink); // Once only: consumes the core record, flags and old breadcrumbs.
#elif defined(ESP32)
  append("Teensy crash report: not applicable\n");
#else
  ++testPort.renders;
  append(crashPresent ? testPort.crash.c_str() : "No Crash Data To Report\n");
  testPort.resetFlags = testPort.resetDetail = testPort.breadcrumb = 0;
#endif
  return true;
}

void recordBootFailure(const char* reason) {
  if (!captured) return;
  append("BOOT_FAILURE: "); append(reason); append("\n");
}
void recordBootStage(uint32_t stage) {
  if (!captured || thermal) return;
#ifdef CORE_TEENSY
  CrashReport.breadcrumb(1, stage);
#elif defined(PIO_UNIT_TESTING)
  testPort.breadcrumb = stage;
  ++testPort.breadcrumbWrites;
#else
  (void)stage;
#endif
}
#ifdef ELS_PROGRESS_TELEMETRY
bool progressTelemetryDue() {
  return captured && wasConnected && offset == length && !progressLength &&
      uint32_t(elsMicros() - lastProgress) >= 1000000;
}
void queueProgressTelemetry(uint32_t loop, uint32_t timer, uint32_t frames,
                            bool oledFault, bool ready) {
  if (!progressTelemetryDue()) return;
  const uint32_t now = elsMicros();
  lastProgress = now;
  const int size = std::snprintf(progress, sizeof(progress),
      "ELS_PROGRESS source=%s us=%lu loop=%lu timer=%lu frames=%lu oled_fault=%u ready=%u\n",
      FIRMWARE_REVISION, static_cast<unsigned long>(now), static_cast<unsigned long>(loop),
      static_cast<unsigned long>(timer), static_cast<unsigned long>(frames),
      unsigned(oledFault), unsigned(ready));
  if (size > 0 && size_t(size) < sizeof(progress)) {
    progressLength = size_t(size);
    progressOffset = 0;
  } else {
    recordBootFailure("PROGRESS_FORMAT_FAILED");
  }
}
#endif
void serviceBootDiagnostics() {
  if (!captured) return;
#ifdef PIO_UNIT_TESTING
  const bool connected = testPort.connected;
#else
  const bool connected = bool(Serial);
#endif
  if (connected && !wasConnected) {
    offset = 0;
#ifdef ELS_PROGRESS_TELEMETRY
    progressLength = progressOffset = 0;
    lastProgress = elsMicros();
#endif
  }
  wasConnected = connected;
  if (!connected) return;
#ifdef PIO_UNIT_TESTING
  if (testPort.commandOffset < testPort.commands.size()) {
    const char command = testPort.commands[testPort.commandOffset++];
    ++testPort.reads;
#else
  if (Serial.available() > 0) {
    const int command = Serial.read();
#endif
    if (command == 'r' || command == 'R') offset = 0;
  }
#ifdef PIO_UNIT_TESTING
  const size_t capacity = testPort.capacity;
#else
  const int available = Serial.availableForWrite();
  const size_t capacity = available > 0 ? static_cast<size_t>(available) : 0;
#endif
  const char* bytes = report;
  size_t* cursor = &offset;
  size_t remaining = length - offset;
#ifdef ELS_PROGRESS_TELEMETRY
  if (progressLength) {
    bytes = progress;
    cursor = &progressOffset;
    remaining = progressLength - progressOffset;
  }
#endif
  const size_t count = std::min<size_t>(64, std::min(capacity, remaining));
  if (!count) return;
#ifdef PIO_UNIT_TESTING
  const size_t written = std::min(count, testPort.writeLimit);
  testPort.output.append(bytes + *cursor, written);
#else
  const size_t written = Serial.write(reinterpret_cast<const uint8_t*>(bytes + *cursor), count);
#endif
  *cursor += std::min(written, count);
#ifdef ELS_PROGRESS_TELEMETRY
  if (progressLength && progressOffset == progressLength) progressLength = progressOffset = 0;
#endif
}
#ifdef PIO_UNIT_TESTING
namespace BootDiagnosticsTest {
Port& port() { return testPort; }
void reset() {
  testPort = Port{};
  std::memset(::report, 0, sizeof(::report));
  length = offset = 0;
  captured = thermal = overflow = wasConnected = false;
#ifdef ELS_PROGRESS_TELEMETRY
  progressLength = progressOffset = 0;
  lastProgress = 0;
#endif
}
const char* report() { return ::report; }
size_t reportSize() { return length; }
bool truncated() { return overflow; }
}
#endif
