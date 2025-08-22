// Libraries
#include <Arduino.h>
#include <SPI.h>
#include <globalstate.h>
#include "config.h"

// Factory and DI includes
#include "../lib/factory/system_factory.h"
#include "../lib/interfaces/system_interfaces.h"
#include "../lib/platform/platform_abstraction.h"

//#define FULLMONITOR
#ifdef ESP32
#include <esp_task_wdt.h>
#else
IntervalTimer timer;
#endif

// Dependency injection container
std::unique_ptr<DependencyContainer> systemContainer;

// Component references (resolved from container)
IPlatformAbstraction* platform = nullptr;
ISpindle* spindle = nullptr;
ILeadscrew* leadscrew = nullptr;
IDisplay* display = nullptr;
IButtonHandler* buttonHandler = nullptr;

#ifdef ESP32
ICommsManager* commsManager = nullptr;
#endif
int64_t lastcycle;
int cyclecount;
int finalcyclecount;


// have to handle the leadscrew updates in a timer callback so we can update the
// screen independently without losing pulses
void timerCallback() {
  if (GlobalState::getInstance()->hasOTA()) {
#ifdef ESP32
    commsManager->loop();
#endif
  } else {
    spindle->update();
    leadscrew->update();
  }
}


void displayLoop() {
  buttonHandler->handle();
  display->update();
}

#ifdef ESP32
void DisplayTask(void* parameter) {
  uint64_t m = 1;
  while (true) {
    displayLoop();
    esp_task_wdt_reset();
    uint64_t c = micros();
    uint64_t delay = (250000 - (c - m)) / 1000;
    if (delay > 0) {
      vTaskDelay((delay > 250 ? 250 : delay) / portTICK_PERIOD_MS);
    }
    m = c + 250000;
  }
}

void SpindleTask(void* parameter) {
  while (true) {
    timerCallback();
    esp_task_wdt_reset();
  }
}

void comms_loop(void* parameters) { commsManager->loop(); }

#endif


void setup() {
  Serial.begin(921600);

  // config - compile time checks for safety
  CHECK_BOUNDS(DEFAULT_METRIC_THREAD_PITCH_IDX, threadPitchMetric,
    "DEFAULT_METRIC_THREAD_PITCH_IDX out of bounds");
  CHECK_BOUNDS(DEFAULT_METRIC_FEED_PITCH_IDX, feedPitchMetric,
    "DEFAULT_METRIC_FEED_PITCH_IDX out of bounds");
  CHECK_BOUNDS(DEFAULT_IMPERIAL_THREAD_PITCH_IDX, threadPitchImperial,
    "DEFAULT_IMPERIAL_THREAD_PITCH_IDX out of bounds");
  CHECK_BOUNDS(DEFAULT_IMPERIAL_FEED_PITCH_IDX, feedPitchImperial,
    "DEFAULT_IMPERIAL_FEED_PITCH_IDX out of bounds");

  // Create system through factory
  systemContainer = SystemFactory::createSystem();
  
  // Resolve dependencies
  platform = systemContainer->resolve<IPlatformAbstraction>();
  spindle = systemContainer->resolve<ISpindle>();
  leadscrew = systemContainer->resolve<ILeadscrew>();
  display = systemContainer->resolve<IDisplay>();
  buttonHandler = systemContainer->resolve<IButtonHandler>();
  
  // Safety checks for embedded systems (where resolve returns nullptr on failure)
#ifndef PIO_UNIT_TESTING
  if (!platform || !spindle || !leadscrew || !display || !buttonHandler) {
    Serial.println("ERROR: Failed to resolve dependencies");
    while(1); // Halt system
  }
#endif
  
#ifdef ESP32
  commsManager = systemContainer->resolve<ICommsManager>();
#ifndef PIO_UNIT_TESTING
  if (!commsManager) {
    Serial.println("ERROR: Failed to resolve ESP32 communications manager");
    while(1); // Halt system
  }
#endif
#endif

  // Platform-specific hardware setup is now handled by platform abstraction
  // Additional initialization for ESP32 button matrix
#ifdef ESP32
  if (platform->getCapabilities().hasButtonMatrix) {
    auto keyArray = systemContainer->resolve<IKeyArray>();
    if (keyArray) {
      keyArray->initPad();
    }
  }
#endif

  // Component initialization
  display->init();
  leadscrew->setTargetPitchMM(GlobalState::getInstance()->getCurrentFeedPitch());
  display->update();

  // Platform-specific timer and task initialization
  if (platform->getCapabilities().hasDualCore) {
#ifdef ESP32
    // ESP32 dual-core task setup
    platform->initializeTaskScheduler();
    
    TaskHandle_t spindleTask;
    TaskHandle_t displayTask;
    xTaskCreatePinnedToCore(SpindleTask, "Spindle", 2048, NULL, 24 | portPRIVILEGE_BIT, &spindleTask, 0);
    xTaskCreatePinnedToCore(DisplayTask, "Display", 8000, NULL, 1, &displayTask, 1);
    
    esp_task_wdt_delete(spindleTask);
    esp_task_wdt_delete(displayTask);
#endif
  } else {
    // Single-core timer setup (Teensy)
    platform->initializeTimer(timerCallback, LEADSCREW_TIMER_US);
  }

  delay(2000);

}

void loop() {
  if (platform->getCapabilities().hasDualCore) {
    // ESP32: Tasks handle the work, main loop just yields
    platform->yieldProcessor();
  } else {
    // Teensy: Main loop handles display updates
    displayLoop();
  }
}