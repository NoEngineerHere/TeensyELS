// Native boot tests supply only the hardware boundary; this state machine is shared.
#ifndef PIO_UNIT_TESTING
#include <Arduino.h>
#include <atomic>
#include <globalstate.h>
#include <controls.h>
#include <leadscrew.h>
#include <spindle.h>
#include <board.h>
#include <pulse_timing.h>
#include <spindle_encoder.h>
#include <boot_diagnostics.h>
#include "ESPCommsManager.h"
#include "buttons.h"
#include "buttonpad.h"
#include "config.h"
#include "display.h"
#include "keyarray.h"
#ifdef ESP32
#include <esp_task_wdt.h>
#include <pulse_esp32.h>
#else
#include <pulse_teensy.h>
IntervalTimer timer;
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
ESPCommsManager commsManager(globalState);
#endif
#if ELS_INPUT_LAYOUT == ELS_INPUT_LEGACY_MATRIX
KeyArray keyArray;
ButtonPad keyPad(controls, keyArray);
#else
ButtonHandler keyPad(controls);
#endif
Display display(globalState, &spindle, &leadscrew);
static void initializeEnableOutput() {
  // Compatibility assertion only: LOW is not a claim of electrical disable.
  digitalWrite(hardware.motion.enable, LOW);
  pinMode(hardware.motion.enable, OUTPUT);
}
static void initializeBoardControls() {
  CHECK_BOUNDS(DEFAULT_METRIC_THREAD_PITCH_IDX, threadPitchMetric, "Invalid thread default");
  CHECK_BOUNDS(DEFAULT_METRIC_FEED_PITCH_IDX, feedPitchMetric, "Invalid feed default");
  CHECK_BOUNDS(DEFAULT_IMPERIAL_THREAD_PITCH_IDX, threadPitchImperial, "Invalid thread default");
  CHECK_BOUNDS(DEFAULT_IMPERIAL_FEED_PITCH_IDX, feedPitchImperial, "Invalid feed default");
  if constexpr (hardware.indicators.green >= 0) pinMode(hardware.indicators.green, OUTPUT);
  if constexpr (hardware.indicators.red >= 0) pinMode(hardware.indicators.red, OUTPUT);
#if ELS_INPUT_LAYOUT == ELS_INPUT_LEGACY_MATRIX
  keyArray.initPad();
#else
  for (int pin : {hardware.direct.increase, hardware.direct.decrease, hardware.direct.mode,
                 hardware.direct.sync, hardware.direct.halfNut, hardware.direct.enable,
                 hardware.direct.lock, hardware.direct.left, hardware.direct.right}) pinMode(pin, INPUT_PULLUP);
#endif
}
#endif

static std::atomic<bool> ready{false};
static bool bootFault = false, displayReady = false;
static uint32_t bannerStarted = 0;
#ifdef ELS_PROGRESS_TELEMETRY
static uint32_t foregroundLoops = 0;
static std::atomic<uint32_t> timerCompletions{0}, displayReturns{0};
#endif

void timerCallback() {
#ifdef ESP32
  if (globalState.hasOTA()) leadscrew.setFaultInhibit(true);
#endif
  spindleEncoder.sample(spindle);
  leadscrew.update();
#ifdef ESP32
  if (globalState.hasOTA() && leadscrew.isOutputIdle()) commsManager.loop();
#endif
#ifdef ELS_PROGRESS_TELEMETRY
  timerCompletions.fetch_add(1, std::memory_order_relaxed);
#endif
}
void displayLoop() {
  if (!ready.load() || display.failed()) return;
  keyPad.handle();
  display.update();
#ifdef ELS_PROGRESS_TELEMETRY
  displayReturns.fetch_add(1, std::memory_order_relaxed);
#endif
}
#ifdef ESP32
void DisplayTask(void*) {
  TickType_t lastWake = xTaskGetTickCount();
  while (true) {
    displayLoop();
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(250));
  }
}
void SpindleTask(void*) {
  while (true) {
    if (ready.load() || !leadscrew.isOutputIdle()) timerCallback();
    else vTaskDelay(1); // A later task-creation failure must not starve IDLE0.
  }
}
#endif
static void failBoot(const char* reason) {
  ready.store(false);
  bootFault = true;
  leadscrew.setFaultInhibit(true);
  recordBootFailure(reason);
  if (displayReady) display.showBootStatus(true);
}
static void startMotionServices() {
#ifdef ESP32
  TaskHandle_t spindleTask = nullptr, displayTask = nullptr;
  if (xTaskCreatePinnedToCore(SpindleTask, "Spindle", 2048, nullptr, 24 | portPRIVILEGE_BIT, &spindleTask, 0) != pdPASS) {
    failBoot("MOTION_TASK_INIT_FAILED"); return;
  }
  if (xTaskCreatePinnedToCore(DisplayTask, "Display", 8000, nullptr, 1, &displayTask, 1) != pdPASS) {
    failBoot("DISPLAY_TASK_INIT_FAILED"); return;
  }
  disableLoopWDT();
  esp_task_wdt_delete(xTaskGetHandle("IDLE0"));
  esp_task_wdt_delete(xTaskGetHandle("IDLE1"));
#else
  if (!timer.begin(timerCallback, LEADSCREW_TIMER_US)) { failBoot("TIMER_INIT_FAILED"); return; }
#endif
  recordBootStage(4);
  leadscrew.setFaultInhibit(false);
  ready.store(true);
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
  if (!captureOK) { bootFault = true; leadscrew.setFaultInhibit(true); }
  if (!outputsReady) failBoot("PULSE_IO_INIT_FAILED");
  initializeBoardControls();
  displayReady = display.init();
  if (!displayReady) { failBoot("DISPLAY_INIT_FAILED"); return; }
  display.showBootStatus(bootFault);
  recordBootStage(3);
  leadscrew.setTargetPitchMM(globalState.getCurrentFeedPitch());
  bannerStarted = elsMicros();
}
void loop() {
#ifdef ELS_PROGRESS_TELEMETRY
  ++foregroundLoops;
#endif
  serviceBootDiagnostics();
  if (displayReady && display.failed() && !bootFault) failBoot("DISPLAY_COMM_FAILED");
#ifdef ELS_PROGRESS_TELEMETRY
  if (progressTelemetryDue()) {
    queueProgressTelemetry(foregroundLoops, timerCompletions.load(std::memory_order_relaxed),
        displayReturns.load(std::memory_order_relaxed), display.failed(), ready.load());
  }
#endif
  const uint32_t bannerElapsed = elsMicros() - bannerStarted;
  if (!bootFault && displayReady && !ready.load() && bannerElapsed >= 2000000) startMotionServices();
#ifdef ESP32
  vTaskDelay(1);
#else
  displayLoop();
#endif
}
