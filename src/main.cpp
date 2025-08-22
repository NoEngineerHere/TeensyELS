// Libraries
#include <Arduino.h>
#include <SPI.h>
#include <globalstate.h>
#include "config.h"

// Factory and DI includes
#include "../lib/factory/system_factory.h"
#include "../lib/interfaces/system_interfaces.h"

//#define FULLMONITOR
#ifdef ESP32
#include <esp_task_wdt.h>
#else
IntervalTimer timer;
#endif

// Dependency injection container
std::unique_ptr<DependencyContainer> systemContainer;

// Component references (resolved from container)
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
  spindle = systemContainer->resolve<ISpindle>();
  leadscrew = systemContainer->resolve<ILeadscrew>();
  display = systemContainer->resolve<IDisplay>();
  buttonHandler = systemContainer->resolve<IButtonHandler>();
  
  // Safety checks for embedded systems (where resolve returns nullptr on failure)
#ifndef PIO_UNIT_TESTING
  if (!spindle || !leadscrew || !display || !buttonHandler) {
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

  // Hardware pin setup
#ifndef ELS_SPINDLE_DRIVEN
//  pinMode(ELS_SPINDLE_ENCODER_A, INPUT_PULLUP); // encoder pin 1
//  pinMode(ELS_SPINDLE_ENCODER_B, INPUT_PULLUP); // encoder pin 2
#endif

#ifdef ELS_USE_RMT
  rmt_obj_t* leadscreRMT = rmtInit(ELS_LEADSCREW_STEP, true, RMT_MEM_64);
  // Note: Would need to cast leadscrew to concrete type for setRMT, keeping for now
  // static_cast<Leadscrew*>(leadscrew)->setRMT(leadscreRMT);
  rmtSetTick(leadscreRMT, 2500);
#else
  pinMode(ELS_LEADSCREW_STEP, OUTPUT); // step output pin
#endif
  pinMode(ELS_LEADSCREW_DIR, OUTPUT);  // direction output pin

#ifdef ELS_UI_ENCODER
  //  pinMode(ELS_UI_ENCODER_A, INPUT); // encoder pin 1
  //  pinMode(ELS_UI_ENCODER_B, INPUT); // encoder pin 2

#ifdef ELS_IND_GREEN
  pinMode(ELS_IND_GREEN, OUTPUT);
  pinMode(ELS_IND_RED, OUTPUT);
#endif
#endif

  pinMode(ELS_STEPPER_ENA, OUTPUT);
  digitalWrite(ELS_STEPPER_ENA, 0);

#ifdef ELS_USE_BUTTON_ARRAY
  auto keyArray = systemContainer->resolve<IKeyArray>();
  keyArray->initPad();
#else
  pinMode(ELS_RATE_INCREASE_BUTTON, INPUT_PULLUP);  // rate Inc
  pinMode(ELS_RATE_DECREASE_BUTTON, INPUT_PULLUP);  // rate Dec
  pinMode(ELS_MODE_CYCLE_BUTTON, INPUT_PULLUP);     // mode cycle
  pinMode(ELS_THREAD_SYNC_BUTTON, INPUT_PULLUP);    // thread sync
  pinMode(ELS_HALF_NUT_BUTTON, INPUT_PULLUP);       // half nut
  pinMode(ELS_ENABLE_BUTTON, INPUT_PULLUP);         // enable toggle
  pinMode(ELS_LOCK_BUTTON, INPUT_PULLUP);           // lock toggle
  pinMode(ELS_JOG_LEFT_BUTTON, INPUT_PULLUP);       // jog left
  pinMode(ELS_JOG_RIGHT_BUTTON, INPUT_PULLUP);      // jog right
#endif

  // Component initialization
  display->init();
  leadscrew->setTargetPitchMM(GlobalState::getInstance()->getCurrentFeedPitch());
  display->update();

#ifdef ESP32

  TaskHandle_t spindleTask;
  TaskHandle_t displayTask;
  //TaskHandle_t commsTask;
  xTaskCreatePinnedToCore(SpindleTask, "Spindle", 2048, NULL, 24 | portPRIVILEGE_BIT, &spindleTask, 0);
  xTaskCreatePinnedToCore(DisplayTask, "Display", 8000, NULL, 1, &displayTask, 1);
  //xTaskCreatePinnedToCore(comms_loop, "Comms", 16000, NULL, 10, &commsTask, 1);
  disableLoopWDT();
  esp_task_wdt_delete(xTaskGetHandle("IDLE0"));
  esp_task_wdt_delete(xTaskGetHandle("IDLE1"));
  esp_task_wdt_delete(spindleTask);
  esp_task_wdt_delete(displayTask);
  //esp_task_wdt_delete(commsTask);

#else
  timer.begin(timerCallback, LEADSCREW_TIMER_US);
#endif

  delay(2000);

}

void loop() {
#ifdef ESP32  
  vTaskDelay(1000);
#else
  displayLoop();
#endif
}