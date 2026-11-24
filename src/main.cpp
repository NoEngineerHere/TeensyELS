// Native boot tests supply only the hardware boundary; this state machine is shared.
#ifndef PIO_UNIT_TESTING
#include <Arduino.h>
#include <atomic>
#include <globalstate.h>
#include <controls.h>
#include <leadscrew.h>
#include <spindle.h>
#include <board.h>
#include <board_indicators.h>
#include <pulse_timing.h>
#include <spindle_encoder.h>
#include <boot_diagnostics.h>
#include "ManualUpdateReceiver.h"
#include <panel_input.h>
#include "config.h"
#include "display.h"
#include "runtime.h"
#ifdef ESP32
#include <pulse_esp32.h>
#else
#include <pulse_teensy.h>
#endif

GlobalState globalState;
Spindle spindle;
SpindleEncoder spindleEncoder(hardware.motion.spindleA, hardware.motion.spindleB);
#ifdef ESP32
static_assert(pulseTiming.highUs > 0 && pulseTiming.highUs <= 32767, "RMT high duration out of range");
static_assert(pulseTiming.lowUs > 0 && pulseTiming.lowUs <= 32767, "RMT low duration out of range");
LeadscrewIOESP<hardware.motion.step, hardware.motion.dir> leadscrewIOImpl(pulseTiming);
#else
LeadscrewIOTeensy<hardware.motion.step, hardware.motion.dir> leadscrewIOImpl(pulseTiming);
#endif
Leadscrew leadscrew(globalState, &spindle, &leadscrewIOImpl, pulseTiming, ACCEL_PULSE_SEC,
    LEADSCREW_INITIAL_PULSE_DELAY_US, ELS_LEADSCREW_STEPPER_PPR * ELS_GEARBOX_RATIO,
    ELS_LEADSCREW_PITCH_MM, ELS_SPINDLE_ENCODER_PPR);
Controls controls(globalState, leadscrew);
#ifdef ESP32
ManualUpdateReceiver updateReceiver(globalState);
#endif
static bool maintenanceRequested() { return compiledProcessor == Processor::Esp32 && globalState.updateArmed(); }
PanelInput panel(controls);
#if ELS_INPUT_LAYOUT == ELS_INPUT_MK2_MATRIX
Display display(globalState, &spindle, &leadscrew, &panel.actions());
#else
Display display(globalState, &spindle, &leadscrew);
#endif
static void enableDisplayBacklight() {
#if ELS_SCREEN != ELS_SCREEN_SSD1306_128X64
  if constexpr (hardware.tft.backlight >= 0) {
    digitalWrite(hardware.tft.backlight, hardware.tft.backlightActiveHigh);
    pinMode(hardware.tft.backlight, OUTPUT);
    digitalWrite(hardware.tft.backlight, hardware.tft.backlightActiveHigh);
  }
#endif
}
static void initializeEnableOutput() {
  // Compatibility assertion only: LOW is not a claim of electrical disable.
  digitalWrite(hardware.motion.enable, LOW);
  pinMode(hardware.motion.enable, OUTPUT);
}
static bool initializeBoardControls() {
  CHECK_BOUNDS(DEFAULT_METRIC_THREAD_PITCH_IDX, threadPitchMetric, "Invalid thread default");
  CHECK_BOUNDS(DEFAULT_METRIC_FEED_PITCH_IDX, feedPitchMetric, "Invalid feed default");
  CHECK_BOUNDS(DEFAULT_IMPERIAL_THREAD_PITCH_IDX, threadPitchImperial, "Invalid thread default");
  CHECK_BOUNDS(DEFAULT_IMPERIAL_FEED_PITCH_IDX, feedPitchImperial, "Invalid feed default");
  initializeIndicators();
  return panel.begin();
}
static void initializeAlarmInput() {
  if constexpr (hardware.motion.alarm >= 0) pinMode(hardware.motion.alarm, INPUT);
}
static bool alarmAsserted() {
  if constexpr (hardware.motion.alarm >= 0)
    return digitalRead(hardware.motion.alarm) == (hardware.motion.alarmActiveLow ? LOW : HIGH);
  return false;
}
#endif

static std::atomic<bool> ready{false};
static std::atomic<const char*> faultReason{nullptr};
static bool displayReady = false, displayFaultShown = false, faultReported = false;
static bool runtimeStarted = false;
static std::atomic<bool> initialFrameComplete{false}, updateHandoff{false};
static uint32_t lastStatusMs = 0;
static uint32_t bannerStarted = 0;
#ifdef ELS_PROGRESS_TELEMETRY
static uint32_t foregroundLoops = 0;
static std::atomic<uint32_t> timerCompletions{0}, displayReturns{0};
#endif

static void latchFault(const char* reason) {
  if (faultReason.load(std::memory_order_acquire)) return;
  leadscrew.setFaultInhibit(true);
  const char* expected = nullptr;
  faultReason.compare_exchange_strong(expected, reason, std::memory_order_release);
  ready.store(false, std::memory_order_release);
}
static void sampleAlarm() {
  if (alarmAsserted()) latchFault("DRIVER_ALARM");
}
bool motionServiceNeeded() {
  return ready.load(std::memory_order_acquire) || leadscrew.needsPulseService();
}
static void reportFault() {
  if (const char* reason = faultReason.load(std::memory_order_acquire); reason && !faultReported) {
    recordBootFailure(reason);
    faultReported = true;
  }
}

void motionTick() {
  sampleAlarm();
  const bool maintenance = maintenanceRequested();
  if (maintenance) ready.store(false, std::memory_order_release);
  spindleEncoder.sample(spindle);
  leadscrew.update();
  if (leadscrew.hasOutputFault()) latchFault("PULSE_OUTPUT_FAULT");
  else if (spindle.hasMotionFault()) latchFault("SPINDLE_COUNT_FAULT");
  if (maintenance && !faultReason.load(std::memory_order_acquire) &&
      !display.failed() && leadscrew.isOutputIdle())
    updateHandoff.store(true, std::memory_order_release);
#ifdef ELS_PROGRESS_TELEMETRY
  timerCompletions.fetch_add(1, std::memory_order_relaxed);
#endif
}
void inputTick() {
  const uint32_t now = millis();
  if (ready.load(std::memory_order_acquire) && !display.failed() && !maintenanceRequested()) panel.poll(now);
  updateIndicators(globalState, now);
}
void displayTick() {
  if (!displayReady) return;
  const uint32_t now = millis();
  const bool frameComplete = display.service(now);
  if (display.failed()) { latchFault("DISPLAY_COMM_FAILED"); return; }
  if (!frameComplete) return;
  if (!initialFrameComplete.load(std::memory_order_relaxed))
    initialFrameComplete.store(true, std::memory_order_release);
  if (faultReason.load(std::memory_order_acquire)) {
    if (!displayFaultShown) {
      display.showBootStatus(true);
      displayFaultShown = true;
    }
    return;
  }
  if (!ready.load(std::memory_order_acquire) && !maintenanceRequested()) return;
  if (now - lastStatusMs < 250) return;
  lastStatusMs = now;
  display.update();
  if (display.failed()) { latchFault("DISPLAY_COMM_FAILED"); return; }
#ifdef ELS_PROGRESS_TELEMETRY
  displayReturns.fetch_add(1, std::memory_order_relaxed);
#endif
}
static void startMotionServices() {
  const auto result = startRuntime();
  switch (result) {
    case RuntimeStartResult::MotionFailed: latchFault("MOTION_SERVICE_INIT_FAILED"); return;
    case RuntimeStartResult::InputFailed: latchFault("INPUT_TASK_INIT_FAILED"); return;
    case RuntimeStartResult::DisplayFailed: latchFault("DISPLAY_TASK_INIT_FAILED"); return;
    case RuntimeStartResult::Started: break;
  }
  runtimeStarted = true;
  sampleAlarm();
  if (faultReason.load(std::memory_order_acquire)) return;
  recordBootStage(4);
  leadscrew.setFaultInhibit(false);
  ready.store(true, std::memory_order_release);
}
void setup() {
  globalState.setButtonLock(LK_LOCKED);
  globalState.setMotionMode(MM_DISABLED);
  const bool outputsReady = leadscrew.initializeOutputs();
  initializeEnableOutput();
  Serial.begin(921600);
  const bool captureOK = captureBootDiagnostics();
  // Old breadcrumbs must be consumed before recording this boot's first stage.
  recordBootStage(1);
  recordBootStage(2);
  if (!captureOK) { latchFault("THERMAL_RESET"); faultReported = true; }
  if (!outputsReady) latchFault("PULSE_IO_INIT_FAILED");
  initializeAlarmInput();
  sampleAlarm();
  if (!spindleEncoder.begin()) latchFault("SPINDLE_ENCODER_INIT_FAILED");
  if (!initializeBoardControls()) latchFault("UI_ENCODER_INIT_FAILED");
  if (!initializeDisplayBus()) { latchFault("DISPLAY_BUS_INIT_FAILED"); reportFault(); return; }
  displayReady = display.init();
  if (!displayReady) { latchFault("DISPLAY_INIT_FAILED"); reportFault(); return; }
  enableDisplayBacklight();
  displayFaultShown = faultReason.load(std::memory_order_acquire) != nullptr;
  display.showBootStatus(displayFaultShown);
  recordBootStage(3);
  leadscrew.setTargetPitchMM(globalState.getCurrentFeedPitch());
  bannerStarted = elsMicros();
}
void loop() {
#ifdef ELS_PROGRESS_TELEMETRY
  ++foregroundLoops;
#endif
  serviceBootDiagnostics();
  serviceRuntimeForeground();
#ifdef ELS_PROGRESS_TELEMETRY
  if (progressTelemetryDue()) {
    queueProgressTelemetry(foregroundLoops, timerCompletions.load(std::memory_order_relaxed),
        displayReturns.load(std::memory_order_relaxed), display.failed(), ready.load());
  }
#endif
  if (!runtimeStarted && !faultReason.load(std::memory_order_acquire) && displayReady) {
    sampleAlarm();
    if (!faultReason.load(std::memory_order_acquire) && initialFrameComplete.load(std::memory_order_acquire) &&
        elsMicros() - bannerStarted >= 2000000)
      startMotionServices();
  }
  reportFault();
}

#if defined(ESP32) || defined(ELS_TEST_ESP_RUNTIME)
void serviceUpdateReceiver() {
  if (!maintenanceRequested()) return;
  sampleAlarm();
  const bool healthy = !faultReason.load(std::memory_order_acquire) && !display.failed();
  if (updateHandoff.exchange(false, std::memory_order_acquire) && healthy) updateReceiver.arm();
  updateReceiver.poll(healthy);
}
#endif
