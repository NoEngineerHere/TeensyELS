// Native boot tests supply only the hardware boundary; this state machine is shared.
#ifndef PIO_UNIT_TESTING
#include <Arduino.h>
#include <atomic>
#include <globalstate.h>
#include <leadscrew.h>
#include <spindle.h>
#include <boot_diagnostics.h>
#include "ESPCommsManager.h"
#include "buttons.h"
#include "buttonpad.h"
#include "config.h"
#include "display.h"
#include "keyarray.h"
#ifdef ESP32
#include <esp_task_wdt.h>
#include <leadscrew_io_esp.h>
#else
#include <leadscrew_io_teensy.h>
IntervalTimer timer;
#endif

GlobalState* globalState = GlobalState::getInstance();
Spindle spindle(ELS_SPINDLE_ENCODER_A, ELS_SPINDLE_ENCODER_B);
#ifdef ESP32
LeadscrewIOESP leadscrewIOImpl;
#else
LeadscrewIOTeensy leadscrewIOImpl;
#endif
Leadscrew leadscrew(&spindle, &leadscrewIOImpl, ACCEL_PULSE_SEC,
    LEADSCREW_INITIAL_PULSE_DELAY_US, ELS_LEADSCREW_STEPPER_PPR * ELS_GEARBOX_RATIO,
    ELS_LEADSCREW_PITCH_MM, ELS_SPINDLE_ENCODER_PPR);
#ifdef ESP32
KeyArray keyArray(&leadscrew);
ButtonPad keyPad(&spindle, &leadscrew, &keyArray);
ESPCommsManager commsManager;
#else
ButtonHandler keyPad(&spindle, &leadscrew);
#endif
Display display(&spindle, &leadscrew);
static void initializeEnableOutput() {
  // Compatibility assertion only: LOW is not a claim of electrical disable.
  digitalWrite(ELS_STEPPER_ENA, LOW);
  pinMode(ELS_STEPPER_ENA, OUTPUT);
}
static void initializeBoardControls() {
  CHECK_BOUNDS(DEFAULT_METRIC_THREAD_PITCH_IDX, threadPitchMetric, "Invalid thread default");
  CHECK_BOUNDS(DEFAULT_METRIC_FEED_PITCH_IDX, feedPitchMetric, "Invalid feed default");
  CHECK_BOUNDS(DEFAULT_IMPERIAL_THREAD_PITCH_IDX, threadPitchImperial, "Invalid thread default");
  CHECK_BOUNDS(DEFAULT_IMPERIAL_FEED_PITCH_IDX, feedPitchImperial, "Invalid feed default");
#ifdef ELS_UI_ENCODER
  pinMode(ELS_IND_GREEN, OUTPUT);
  pinMode(ELS_IND_RED, OUTPUT);
#endif
#ifdef ELS_USE_BUTTON_ARRAY
  keyArray.initPad();
#else
  for (int pin : {ELS_RATE_INCREASE_BUTTON, ELS_RATE_DECREASE_BUTTON, ELS_MODE_CYCLE_BUTTON,
                 ELS_THREAD_SYNC_BUTTON, ELS_HALF_NUT_BUTTON, ELS_ENABLE_BUTTON,
                 ELS_LOCK_BUTTON, ELS_JOG_LEFT_BUTTON, ELS_JOG_RIGHT_BUTTON}) pinMode(pin, INPUT_PULLUP);
#endif
}
#endif

static std::atomic<bool> ready{false};
static bool bootFault = false, displayReady = false;
static uint32_t bannerStarted = 0;

void timerCallback() {
#ifdef ESP32
  if (globalState->hasOTA()) leadscrew.setFaultInhibit(true);
#endif
  spindle.update();
  leadscrew.update();
#ifdef ESP32
  if (globalState->hasOTA() && leadscrew.isOutputIdle()) commsManager.loop();
#endif
}
void displayLoop() {
  if (!ready.load()) return;
  keyPad.handle();
  display.update();
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
    if (ready.load()) timerCallback();
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
  globalState->setButtonLock(LK_LOCKED);
  globalState->setMotionMode(MM_DISABLED);
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
  leadscrew.setTargetPitchMM(globalState->getCurrentFeedPitch());
  bannerStarted = elsMicros();
}
void loop() {
  serviceBootDiagnostics();
  const uint32_t bannerElapsed = elsMicros() - bannerStarted;
  if (!bootFault && displayReady && !ready.load() && bannerElapsed >= 2000000) startMotionServices();
#ifdef ESP32
  vTaskDelay(1);
#else
  displayLoop();
#endif
}
