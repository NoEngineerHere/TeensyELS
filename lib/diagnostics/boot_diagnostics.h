#pragma once
#include <cstdint>
// Foreground only. RAM retention lasts for this running boot, not power loss.
bool captureBootDiagnostics();
void serviceBootDiagnostics();
void recordBootStage(uint32_t stage);
void recordBootFailure(const char* reason);
#ifdef ELS_PROGRESS_TELEMETRY
// Foreground only; one bounded progress record after the retained boot report.
bool progressTelemetryDue();
void queueProgressTelemetry(uint32_t loop, uint32_t timer, uint32_t frames,
                            bool oledFault, bool ready);
#endif
#ifdef PIO_UNIT_TESTING
#include <cstddef>
#include <string>
namespace BootDiagnosticsTest {
struct Port {
  uint32_t resetFlags = 0, resetDetail = 0, breadcrumb = 0;
  bool thermal = false, crashPresent = false, connected = false;
  std::string crash, output, commands;
  size_t capacity = 64, writeLimit = 64, commandOffset = 0;
  unsigned renders = 0, breadcrumbWrites = 0, reads = 0;
};
Port& port();
void reset();
const char* report();
size_t reportSize();
bool truncated();
}
#endif
