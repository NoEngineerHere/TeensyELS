#include <gtest/gtest.h>
#include <atomic>
#include <new>
#include <globalstate.h>
#include <leadscrew.h>
#include <boot_diagnostics.h>
#include "../mocks/leadscrewio_mock.h"
#include "../../src/runtime.h"
static uint32_t millis() { return elsMicros() / 1000; }

namespace BootHW {
struct Screen {
 bool initOK=true, fault=false, commFailed=false;int banners=0,updates=0;
 bool init(){return initOK;}
 int frameChunks=0, pending=0, transferred=0, failAt=-1;
 void showBootStatus(bool value){fault=value;++banners;pending=frameChunks;}
 bool service(uint32_t){
  if (commFailed) return false;
  if (pending && ++transferred == failAt) { commFailed=true; return false; }
  return !pending || --pending == 0;
 }
 void update(){++updates;}
 bool failed() const {return commFailed;}
};
struct Encoder {
 bool ok=true;
 bool begin(){return ok;}
 void sample(Spindle& spindle){spindle.incrementCurrentPosition(0);}
};
struct Keys {int calls=0;void poll(uint32_t){++calls;}};
struct SerialPort {void begin(int){}};
struct Timer {bool ok=true;int starts=0;bool begin(void(*)(),int){++starts;return ok;}};
}
namespace TeensyBoot {
GlobalState globalState;
Spindle spindle;BootHW::Encoder spindleEncoder;LeadscrewIOMock io;
Leadscrew leadscrew(globalState, &spindle,&io,testPulseTiming,1000,100,1200,2,1200);
BootHW::Screen display;BootHW::Keys panel;BootHW::SerialPort Serial;BootHW::Timer timer;
void initializeEnableOutput(){}
bool inputOK = true, alarmLine = false;
void initializeAlarmInput(){}
bool alarmAsserted(){return alarmLine;}
bool initializeBoardControls(){return inputOK;}
bool busOK=true;
bool initializeDisplayBus(){return busOK;}
void enableDisplayBacklight(){}
void updateIndicators(GlobalState&,uint32_t){}
bool maintenanceRequested(){return false;}
RuntimeStartResult startRuntime();
void serviceRuntimeForeground();
#include "../../src/main.cpp"
#define ELS_TEST_TEENSY_RUNTIME
#include "../../src/runtime_teensy.cpp"
#undef ELS_TEST_TEENSY_RUNTIME
void timerCallback() { motionInterrupt(); }
void reset(){
 setTestMicros(0);BootDiagnosticsTest::reset();globalState.~GlobalState();new(&globalState)GlobalState;
 spindle.~Spindle();new(&spindle)Spindle;
 io=LeadscrewIOMock{};leadscrew.~Leadscrew();new(&leadscrew)Leadscrew(globalState, &spindle,&io,testPulseTiming,1000,100,1200,2,1200);
 display=BootHW::Screen{};panel=BootHW::Keys{};timer=BootHW::Timer{};
 ready=false;faultReason=nullptr;displayReady=displayFaultShown=faultReported=false;bannerStarted=0;
 runtimeStarted=initialFrameComplete=updateHandoff=false;lastStatusMs=lastInputMs=0;
 inputOK=busOK=true;alarmLine=false;spindleEncoder={};
}
}
namespace EspBoot {
GlobalState globalState;
Spindle spindle;BootHW::Encoder spindleEncoder;LeadscrewIOMock io;
Leadscrew leadscrew(globalState, &spindle,&io,testPulseTiming,1000,100,1200,2,1200);
BootHW::Screen display;BootHW::Keys panel;BootHW::SerialPort Serial;
struct Receiver {int calls=0;void arm(){++calls;} void poll(bool){}} updateReceiver;
using TickType_t=uint32_t;using TaskHandle_t=void*;
constexpr int portPRIVILEGE_BIT=0x800000,pdPASS=1;
int tasks=0,failTask=0;
struct TaskYield {};
bool stopAtYield=false;
TickType_t xTaskGetTickCount(){return 0;}
constexpr TickType_t pdMS_TO_TICKS(int ms){return ms;}
void vTaskDelayUntil(TickType_t*,TickType_t){}
void vTaskDelay(int){if(stopAtYield)throw TaskYield{};}
int xTaskCreatePinnedToCore(void(*)(void*),const char*,int,void*,int,TaskHandle_t* handle,int){
 ++tasks;if(tasks==failTask)return 0;if(handle)*handle=reinterpret_cast<void*>(uintptr_t(tasks));return pdPASS;
}
void disableLoopWDT(){}
void* xTaskGetHandle(const char*){return nullptr;}
void esp_task_wdt_delete(void*){}
void initializeEnableOutput(){}
bool inputOK = true, alarmLine = false;
void initializeAlarmInput(){}
bool alarmAsserted(){return alarmLine;}
bool initializeBoardControls(){return inputOK;}
bool busOK=true;
bool initializeDisplayBus(){return busOK;}
void enableDisplayBacklight(){}
void updateIndicators(GlobalState&,uint32_t){}
bool maintenanceRequested(){return globalState.updateArmed();}

RuntimeStartResult startRuntime();
void serviceRuntimeForeground();
#define ELS_TEST_ESP_RUNTIME
#include "../../src/main.cpp"
#include "../../src/runtime_esp32.cpp"
#undef ELS_TEST_ESP_RUNTIME
void timerCallback() { if (motionServiceNeeded()) motionTick(); }
void reset(){
 setTestMicros(0);BootDiagnosticsTest::reset();globalState.~GlobalState();new(&globalState)GlobalState;
 spindle.~Spindle();new(&spindle)Spindle;
 io=LeadscrewIOMock{};leadscrew.~Leadscrew();new(&leadscrew)Leadscrew(globalState, &spindle,&io,testPulseTiming,1000,100,1200,2,1200);
 display=BootHW::Screen{};panel=BootHW::Keys{};updateReceiver={};tasks=failTask=0;
 displayTaskStarted=false;
 stopAtYield=false;
 ready=false;faultReason=nullptr;displayReady=displayFaultShown=faultReported=false;bannerStarted=0;
 runtimeStarted=initialFrameComplete=updateHandoff=false;lastStatusMs=lastBootDisplayMs=0;
 inputOK=busOK=true;alarmLine=false;spindleEncoder={};
}
}
TEST(Boot, BannerRemainsLockedWhileDiagnosticsDrainWithoutUSBWait) {
 using namespace TeensyBoot;reset();setup();
 EXPECT_EQ(timer.starts,0);EXPECT_EQ(display.banners,1);EXPECT_FALSE(display.fault);
 EXPECT_EQ(globalState.getButtonLock(),LK_LOCKED);EXPECT_EQ(globalState.getMotionMode(),MM_DISABLED);
 for(uint32_t t=0;t<2000000;t+=10000){setTestMicros(t);loop();}
 EXPECT_EQ(timer.starts,0);EXPECT_EQ(display.updates,0);EXPECT_EQ(panel.calls,0);
 auto& port=BootDiagnosticsTest::port();port.connected=true;
 setTestMicros(2000000);loop();EXPECT_TRUE(ready);EXPECT_EQ(timer.starts,1);
 EXPECT_EQ(port.breadcrumb,4u);EXPECT_FALSE(port.output.empty());
 EXPECT_EQ(globalState.getMotionMode(),MM_DISABLED);EXPECT_EQ(io.completions,0);
}
TEST(Boot, OutputDisplayTimerAndThermalFailuresNeverStartMotionOrControls) {
 using namespace TeensyBoot;
 for(int failure:{0,1,2,3,4,5,6,7}){
  reset();if(failure==0)io.beginOK=false;if(failure==1)display.initOK=false;
  if(failure==2)timer.ok=false;if(failure==3)BootDiagnosticsTest::port().thermal=true;
  if(failure==4)spindleEncoder.ok=false;if(failure==5)inputOK=false;if(failure==6)alarmLine=true;if(failure==7)busOK=false;
  setup();alarmLine=false;setTestMicros(2000000);loop();EXPECT_NE(faultReason.load(), nullptr);EXPECT_FALSE(ready);
  loop();
  EXPECT_EQ(timer.starts,failure==2?1:0);EXPECT_EQ(panel.calls,0);EXPECT_EQ(display.updates,0);
  if(failure!=1 && failure!=7)EXPECT_TRUE(display.fault);
  auto& port=BootDiagnosticsTest::port();port.connected=true;loop();EXPECT_FALSE(port.output.empty());
  leadscrew.setFaultInhibit(false);globalState.setMotionMode(MM_ENABLED);spindle.incrementCurrentPosition(1200);
  for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);timerCallback();}
  EXPECT_EQ(io.completions,0);
  const char* reasons[] = {"PULSE_IO_INIT_FAILED", "DISPLAY_INIT_FAILED", "MOTION_SERVICE_INIT_FAILED", "THERMAL_RESET",
                           "SPINDLE_ENCODER_INIT_FAILED", "UI_ENCODER_INIT_FAILED", "DRIVER_ALARM", "DISPLAY_BUS_INIT_FAILED"};
  const char* reason = reasons[failure];
  EXPECT_NE(std::string(BootDiagnosticsTest::report()).find(reason),std::string::npos);
 }
}
TEST(Boot, EveryTaskCreationFailureKeepsStartedWorkInhibited) {
 using namespace EspBoot;
 for(int failure:{1,2,3}){
  reset();failTask=failure;setup();setTestMicros(2000000);loop();
  EspBoot::displayTick();
  EXPECT_NE(faultReason.load(), nullptr);EXPECT_FALSE(ready);EXPECT_EQ(tasks,failure);EXPECT_TRUE(display.fault);
  EspBoot::inputTick();EXPECT_EQ(panel.calls,0);
  leadscrew.setFaultInhibit(false);globalState.setMotionMode(MM_ENABLED);spindle.incrementCurrentPosition(1200);
  for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);timerCallback();}
  EXPECT_EQ(io.completions,0);
  const char* reasons[] = {"MOTION_SERVICE_INIT_FAILED", "INPUT_TASK_INIT_FAILED", "DISPLAY_TASK_INIT_FAILED"};
  EXPECT_NE(std::string(BootDiagnosticsTest::report()).find(reasons[failure-1]),std::string::npos);
  stopAtYield=true;
  EXPECT_THROW(motionTask(nullptr),TaskYield);
  stopAtYield=false;
 }
}
TEST(Boot, OTAHandsOffOnlyAfterInflightCompletionAndNeverRearms) {
 using namespace EspBoot;reset();setup();setTestMicros(2000000);loop();ASSERT_TRUE(ready);
 globalState.setMotionMode(MM_ENABLED);timerCallback();spindle.incrementCurrentPosition(1200);
 for(int i=0;i<1000 && !io.step;++i){setTestMicros(elsMicros()+4);timerCallback();}
 ASSERT_EQ(io.step,1);EXPECT_FALSE(leadscrew.requestMaintenance());
 globalState.setButtonLock(LK_UNLOCKED);globalState.setMotionMode(MM_DISABLED);
 ASSERT_TRUE(leadscrew.requestMaintenance());
 setTestMicros(elsMicros()+3);timerCallback();loop();
 EXPECT_EQ(updateReceiver.calls,0);EXPECT_FALSE(leadscrew.isOutputIdle());
 setTestMicros(elsMicros()+1);timerCallback();EXPECT_EQ(updateReceiver.calls,0);
 loop();EXPECT_EQ(updateReceiver.calls,1);EXPECT_EQ(io.completions,1);
 EXPECT_TRUE(leadscrew.isOutputIdle());leadscrew.setFaultInhibit(false);
 globalState.setMotionMode(MM_ENABLED);spindle.incrementCurrentPosition(1200);
 for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);timerCallback();}
 EXPECT_EQ(io.completions,1);
}
TEST(Boot, RuntimeOledFaultStopsControlsButTeensyServiceDrainsAcceptedPulse) {
 using namespace TeensyBoot;reset();setup();setTestMicros(2000000);loop();ASSERT_TRUE(ready);
 globalState.setMotionMode(MM_JOG_RIGHT);
 for(int i=0;i<1000&&!io.step;++i){setTestMicros(elsMicros()+4);timerCallback();}
 ASSERT_EQ(io.step,1);ASSERT_EQ(io.completions,0);
 display.commFailed=true;loop();ASSERT_FALSE(ready);ASSERT_NE(faultReason.load(), nullptr);
 const int controls=panel.calls;loop();EXPECT_EQ(panel.calls,controls);
 setTestMicros(elsMicros()+4);timerCallback();
 EXPECT_EQ(io.completions,1);EXPECT_EQ(io.step,0);EXPECT_EQ(leadscrew.getCurrentPosition(),1);
 leadscrew.setFaultInhibit(false);globalState.setMotionMode(MM_JOG_RIGHT);
 for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);timerCallback();}
 EXPECT_EQ(io.completions,1);
 EXPECT_NE(std::string(BootDiagnosticsTest::report()).find("DISPLAY_COMM_FAILED"),std::string::npos);
}
TEST(Boot, RuntimeOledFaultAllowsEspTaskToDrainBeforeIdleYield) {
 using namespace EspBoot;reset();setup();setTestMicros(2000000);loop();ASSERT_TRUE(ready);
 globalState.setMotionMode(MM_JOG_RIGHT);
 for(int i=0;i<1000&&!io.step;++i){setTestMicros(elsMicros()+4);timerCallback();}
 ASSERT_EQ(io.step,1);ASSERT_EQ(io.completions,0);
 display.commFailed=true;EspBoot::displayTick();loop();ASSERT_FALSE(ready);ASSERT_NE(faultReason.load(), nullptr);
 stopAtYield=true;setTestMicros(elsMicros()+4);EXPECT_THROW(motionTask(nullptr),TaskYield);stopAtYield=false;
 EXPECT_EQ(io.completions,1);EXPECT_EQ(io.step,0);EXPECT_EQ(leadscrew.getCurrentPosition(),1);
 const int controls=panel.calls;EspBoot::inputTick();EXPECT_EQ(panel.calls,controls);
 leadscrew.setFaultInhibit(false);globalState.setMotionMode(MM_JOG_RIGHT);
 for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);timerCallback();}
 EXPECT_EQ(io.completions,1);
}

TEST(Boot, AssertedAlarmAtReadinessKeepsOutputsInhibited) {
 using namespace TeensyBoot;reset();setup();
 alarmLine=true;setTestMicros(2000000);loop();
 EXPECT_FALSE(ready);EXPECT_EQ(timer.starts,0);
 EXPECT_STREQ(faultReason.load(),"DRIVER_ALARM");
 alarmLine=false;loop();EXPECT_FALSE(ready);
}
TEST(Boot, SuccessfulBannerReleasesStartupInhibit) {
 using namespace TeensyBoot;reset();setup();setTestMicros(2000000);loop();ASSERT_TRUE(ready);
 globalState.setButtonLock(LK_UNLOCKED);globalState.setMotionMode(MM_ENABLED);
 timerCallback();spindle.incrementCurrentPosition(1200);
 for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);timerCallback();}
 EXPECT_EQ(globalState.getMotionMode(),MM_ENABLED);EXPECT_GT(io.completions,0);
}
TEST(Boot, AlarmDrainsPendingPulseButTerminalFaultParksWithoutCompletion) {
 using namespace EspBoot;
 for (bool terminal : {false,true}) {
  reset();setup();setTestMicros(2000000);loop();ASSERT_TRUE(ready);
  globalState.setMotionMode(MM_JOG_RIGHT);
  for(int i=0;i<1000&&!io.step;++i){setTestMicros(elsMicros()+4);timerCallback();}
  ASSERT_EQ(io.step,1);
  globalState.setButtonLock(LK_UNLOCKED);globalState.setMotionMode(MM_DISABLED);
  ASSERT_TRUE(leadscrew.requestMaintenance());
  alarmLine=!terminal;io.completionOK=!terminal;setTestMicros(elsMicros()+4);timerCallback();
  EXPECT_FALSE(ready);EXPECT_FALSE(EspBoot::motionServiceNeeded());
  EXPECT_EQ(io.completions,terminal?0:1);EXPECT_EQ(leadscrew.getCurrentPosition(),terminal?0:1);
  EXPECT_EQ(leadscrew.isOutputIdle(),!terminal);
  EXPECT_STREQ(faultReason.load(),terminal?"PULSE_OUTPUT_FAULT":"DRIVER_ALARM");
  EXPECT_FALSE(leadscrew.requestMaintenance());alarmLine=false;timerCallback();loop();EXPECT_EQ(updateReceiver.calls,0);
  stopAtYield=true;EXPECT_THROW(motionTask(nullptr),TaskYield);stopAtYield=false;
 }
}

TEST(Boot, LastOledChunkMustSucceedBeforeStartingServices) {
 using namespace EspBoot;
 for(bool fail : {false,true}) {
  reset();display.frameChunks=35;display.failAt=fail?35:-1;setup();
  for(int n=0;n<34;++n){setTestMicros(2000000+5000*n);loop();ASSERT_FALSE(ready);}
  EXPECT_EQ(tasks,0);
  setTestMicros(2170000);loop();
  EXPECT_EQ(ready,!fail);EXPECT_EQ(tasks,fail?0:3);
  if(fail) EXPECT_STREQ(faultReason.load(),"DISPLAY_COMM_FAILED");
 }
}
