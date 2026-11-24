#pragma once
#include <leadscrew_io.h>
#include <clock.h>
#include <vector>
inline constexpr PulseTiming testPulseTiming{4, 4, 4, 4};
class LeadscrewIOMock : public LeadscrewIO {
public:
  struct Edge { uint32_t time; int step; int dir; };
  std::vector<Edge> edges;
  bool beginOK = true, startOK = true, completionOK = true;
  uint8_t step = 0, dir = 0;
  uint32_t started = 0;
  int completions = 0;
  bool begin() override { step = 0; dir = 0; return beginOK; }
  void writeDirPin(uint8_t value) override { dir = value; edges.push_back({elsMicros(), step, dir}); }
  bool startPulse(uint32_t now) override {
    if (step || !startOK) return false;
    step = 1; started = now; edges.push_back({now, step, dir}); return true;
  }
  PulseStatus servicePulse(uint32_t now) override {
    if (!step) return {PulseResult::IDLE};
    if (!completionOK) return {PulseResult::FAULT};
    const uint32_t elapsed = now - started;
    if (elapsed < testPulseTiming.highUs) return {PulseResult::PENDING};
    step = 0; ++completions; edges.push_back({now, step, dir}); return {PulseResult::COMPLETE, now};
  }
  uint8_t readStepPin() const { return step; }
  uint8_t readDirPin() const { return dir; }
};
