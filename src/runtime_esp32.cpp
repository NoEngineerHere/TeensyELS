#if defined(ESP32) || defined(ELS_TEST_ESP_RUNTIME)
#ifndef PIO_UNIT_TESTING
#include <Arduino.h>
#include <esp_task_wdt.h>
#include "runtime.h"
#endif

static bool displayTaskStarted = false;
static uint32_t lastBootDisplayMs = 0;
static constexpr TickType_t uiTicks = pdMS_TO_TICKS(5) ? pdMS_TO_TICKS(5) : 1;

static void motionTask(void*) {
  while (true) {
    if (motionServiceNeeded()) motionTick();
    else vTaskDelay(1);
  }
}
static void inputTask(void*) {
  TickType_t wake = xTaskGetTickCount();
  while (true) {
    inputTick();
    vTaskDelayUntil(&wake, uiTicks);
  }
}
static void displayTask(void*) {
  TickType_t wake = xTaskGetTickCount();
  while (true) {
    displayTick();
    vTaskDelayUntil(&wake, uiTicks);
  }
}
RuntimeStartResult startRuntime() {
  if (xTaskCreatePinnedToCore(motionTask, "Motion", 2048, nullptr,
                            24 | portPRIVILEGE_BIT, nullptr, 0) != pdPASS)
    return RuntimeStartResult::MotionFailed;
  if (xTaskCreatePinnedToCore(inputTask, "Input", 4096, nullptr, 2, nullptr, 1) != pdPASS)
    return RuntimeStartResult::InputFailed;
  if (xTaskCreatePinnedToCore(displayTask, "Display", 8000, nullptr, 1, nullptr, 1) != pdPASS)
    return RuntimeStartResult::DisplayFailed;
  displayTaskStarted = true;
  disableLoopWDT();
  esp_task_wdt_delete(xTaskGetHandle("IDLE0"));
  esp_task_wdt_delete(xTaskGetHandle("IDLE1"));
  return RuntimeStartResult::Started;
}
void serviceRuntimeForeground() {
  if (!displayTaskStarted) {
    const uint32_t now = millis();
    if (now - lastBootDisplayMs >= 5) {
      lastBootDisplayMs = now;
      displayTick();
    }
  }
  vTaskDelay(1);
}
#endif
