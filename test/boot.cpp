#include <gtest/gtest.h>
#include <atomic>
#include <new>
#include <globalstate.h>
#include <leadscrew.h>
#include <boot_diagnostics.h>
#include "mocks/leadscrewio_mock.h"

namespace BootHW {
struct Screen {
 bool initOK=true, fault=false, commFailed=false;int banners=0,updates=0;
 bool init(){return initOK;}
 void showBootStatus(bool value){fault=value;++banners;}
 void update(){++updates;}
 bool failed() const {return commFailed;}
};
struct Keys {int calls=0;void handle(){++calls;}};
struct SerialPort {void begin(int){}};
struct Timer {bool ok=true;int starts=0;bool begin(void(*)(),int){++starts;return ok;}};
}
namespace TeensyBoot {
GlobalState* globalState=GlobalState::getInstance();
Spindle spindle;LeadscrewIOMock io;
Leadscrew leadscrew(&spindle,&io,1000,100,1200,2,1200);
BootHW::Screen display;BootHW::Keys keyPad;BootHW::SerialPort Serial;BootHW::Timer timer;
void initializeEnableOutput(){}
void initializeBoardControls(){}
#include "../src/main.cpp"
void reset(){
 setTestMicros(0);BootDiagnosticsTest::reset();globalState->setMotionMode(MM_DISABLED);globalState->clearOTA();
 spindle.~Spindle();new(&spindle)Spindle;
 io=LeadscrewIOMock{};leadscrew.~Leadscrew();new(&leadscrew)Leadscrew(&spindle,&io,1000,100,1200,2,1200);
 display=BootHW::Screen{};keyPad=BootHW::Keys{};timer=BootHW::Timer{};
 ready=false;bootFault=false;displayReady=false;bannerStarted=0;
}
}
namespace EspBoot {
GlobalState* globalState=GlobalState::getInstance();
Spindle spindle;LeadscrewIOMock io;
Leadscrew leadscrew(&spindle,&io,1000,100,1200,2,1200);
BootHW::Screen display;BootHW::Keys keyPad;BootHW::SerialPort Serial;
struct Comms {int calls=0;void loop(){++calls;}} commsManager;
using TickType_t=uint32_t;using TaskHandle_t=void*;
constexpr int portPRIVILEGE_BIT=0x800000,pdPASS=1;
int tasks=0,failTask=0;
struct TaskYield {};
bool stopAtYield=false;
TickType_t xTaskGetTickCount(){return 0;}
TickType_t pdMS_TO_TICKS(int ms){return ms;}
void vTaskDelayUntil(TickType_t*,TickType_t){}
void vTaskDelay(int){if(stopAtYield)throw TaskYield{};}
int xTaskCreatePinnedToCore(void(*)(void*),const char*,int,void*,int,TaskHandle_t* handle,int){
 ++tasks;if(tasks==failTask)return 0;*handle=reinterpret_cast<void*>(uintptr_t(tasks));return pdPASS;
}
void disableLoopWDT(){}
void* xTaskGetHandle(const char*){return nullptr;}
void esp_task_wdt_delete(void*){}
void initializeEnableOutput(){}
void initializeBoardControls(){}
#define ESP32
#include "../src/main.cpp"
#undef ESP32
void reset(){
 setTestMicros(0);BootDiagnosticsTest::reset();globalState->setMotionMode(MM_DISABLED);globalState->clearOTA();
 spindle.~Spindle();new(&spindle)Spindle;
 io=LeadscrewIOMock{};leadscrew.~Leadscrew();new(&leadscrew)Leadscrew(&spindle,&io,1000,100,1200,2,1200);
 display=BootHW::Screen{};keyPad=BootHW::Keys{};commsManager=Comms{};tasks=failTask=0;
 stopAtYield=false;
 ready=false;bootFault=false;displayReady=false;bannerStarted=0;
}
}
TEST(Boot, BannerRemainsLockedWhileDiagnosticsDrainWithoutUSBWait) {
 using namespace TeensyBoot;reset();setup();
 EXPECT_EQ(timer.starts,0);EXPECT_EQ(display.banners,1);EXPECT_FALSE(display.fault);
 EXPECT_EQ(globalState->getButtonLock(),LK_LOCKED);EXPECT_EQ(globalState->getMotionMode(),MM_DISABLED);
 for(uint32_t t=0;t<2000000;t+=10000){setTestMicros(t);loop();}
 EXPECT_EQ(timer.starts,0);EXPECT_EQ(display.updates,0);EXPECT_EQ(keyPad.calls,0);
 auto& port=BootDiagnosticsTest::port();port.connected=true;
 setTestMicros(2000000);loop();EXPECT_TRUE(ready);EXPECT_EQ(timer.starts,1);
 EXPECT_EQ(port.breadcrumb,4u);EXPECT_FALSE(port.output.empty());
 EXPECT_EQ(globalState->getMotionMode(),MM_DISABLED);EXPECT_EQ(io.completions,0);
}
TEST(Boot, OutputDisplayTimerAndThermalFailuresNeverStartMotionOrControls) {
 using namespace TeensyBoot;
 for(int failure:{0,1,2,3}){
  reset();if(failure==0)io.beginOK=false;if(failure==1)display.initOK=false;
  if(failure==2)timer.ok=false;if(failure==3)BootDiagnosticsTest::port().thermal=true;
  setup();setTestMicros(2000000);loop();EXPECT_TRUE(bootFault);EXPECT_FALSE(ready);
  EXPECT_EQ(timer.starts,failure==2?1:0);EXPECT_EQ(keyPad.calls,0);EXPECT_EQ(display.updates,0);
  if(failure!=1)EXPECT_TRUE(display.fault);
  auto& port=BootDiagnosticsTest::port();port.connected=true;loop();EXPECT_FALSE(port.output.empty());
  leadscrew.setFaultInhibit(false);globalState->setMotionMode(MM_ENABLED);spindle.incrementCurrentPosition(1200);
  for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);timerCallback();}
  EXPECT_EQ(io.completions,0);
  const char* reason=failure==0?"PULSE_IO_INIT_FAILED":failure==1?"DISPLAY_INIT_FAILED":failure==2?"TIMER_INIT_FAILED":"THERMAL_RESET";
  EXPECT_NE(std::string(BootDiagnosticsTest::report()).find(reason),std::string::npos);
 }
}
TEST(Boot, EitherTaskCreationFailureKeepsStartedWorkInhibited) {
 using namespace EspBoot;
 for(int failure:{1,2}){
  reset();failTask=failure;setup();setTestMicros(2000000);loop();
  EXPECT_TRUE(bootFault);EXPECT_FALSE(ready);EXPECT_EQ(tasks,failure);EXPECT_TRUE(display.fault);
  displayLoop();EXPECT_EQ(keyPad.calls,0);
  leadscrew.setFaultInhibit(false);globalState->setMotionMode(MM_ENABLED);spindle.incrementCurrentPosition(1200);
  for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);timerCallback();}
  EXPECT_EQ(io.completions,0);
  EXPECT_NE(std::string(BootDiagnosticsTest::report()).find(failure==1?"MOTION_TASK_INIT_FAILED":"DISPLAY_TASK_INIT_FAILED"),std::string::npos);
  stopAtYield=true;
  EXPECT_THROW(SpindleTask(nullptr),TaskYield);
  stopAtYield=false;
 }
}
TEST(Boot, OTAHandsOffOnlyAfterInflightCompletionAndNeverRearms) {
 using namespace EspBoot;reset();setup();setTestMicros(2000000);loop();ASSERT_TRUE(ready);
 globalState->setMotionMode(MM_ENABLED);timerCallback();spindle.incrementCurrentPosition(1200);
 for(int i=0;i<1000 && !io.step;++i){setTestMicros(elsMicros()+4);timerCallback();}
 ASSERT_EQ(io.step,1);globalState->setOTA();setTestMicros(elsMicros()+3);timerCallback();
 EXPECT_EQ(commsManager.calls,0);EXPECT_FALSE(leadscrew.isOutputIdle());
 setTestMicros(elsMicros()+1);timerCallback();EXPECT_EQ(commsManager.calls,1);EXPECT_EQ(io.completions,1);
 EXPECT_TRUE(leadscrew.isOutputIdle());globalState->clearOTA();leadscrew.setFaultInhibit(false);
 globalState->setMotionMode(MM_ENABLED);spindle.incrementCurrentPosition(1200);
 for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);timerCallback();}
 EXPECT_EQ(io.completions,1);
}
TEST(Boot, RuntimeOledFaultStopsControlsButTeensyServiceDrainsAcceptedPulse) {
 using namespace TeensyBoot;reset();setup();setTestMicros(2000000);loop();ASSERT_TRUE(ready);
 globalState->setMotionMode(MM_JOG_RIGHT);
 for(int i=0;i<1000&&!io.step;++i){setTestMicros(elsMicros()+4);timerCallback();}
 ASSERT_EQ(io.step,1);ASSERT_EQ(io.completions,0);
 display.commFailed=true;loop();ASSERT_FALSE(ready);ASSERT_TRUE(bootFault);
 const int controls=keyPad.calls;loop();EXPECT_EQ(keyPad.calls,controls);
 setTestMicros(elsMicros()+4);timerCallback();
 EXPECT_EQ(io.completions,1);EXPECT_EQ(io.step,0);EXPECT_EQ(leadscrew.getCurrentPosition(),1);
 leadscrew.setFaultInhibit(false);globalState->setMotionMode(MM_JOG_RIGHT);
 for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);timerCallback();}
 EXPECT_EQ(io.completions,1);
 EXPECT_NE(std::string(BootDiagnosticsTest::report()).find("DISPLAY_COMM_FAILED"),std::string::npos);
}
TEST(Boot, RuntimeOledFaultAllowsEspTaskToDrainBeforeIdleYield) {
 using namespace EspBoot;reset();setup();setTestMicros(2000000);loop();ASSERT_TRUE(ready);
 globalState->setMotionMode(MM_JOG_RIGHT);
 for(int i=0;i<1000&&!io.step;++i){setTestMicros(elsMicros()+4);timerCallback();}
 ASSERT_EQ(io.step,1);ASSERT_EQ(io.completions,0);
 display.commFailed=true;loop();ASSERT_FALSE(ready);ASSERT_TRUE(bootFault);
 stopAtYield=true;setTestMicros(elsMicros()+4);EXPECT_THROW(SpindleTask(nullptr),TaskYield);stopAtYield=false;
 EXPECT_EQ(io.completions,1);EXPECT_EQ(io.step,0);EXPECT_EQ(leadscrew.getCurrentPosition(),1);
 const int controls=keyPad.calls;displayLoop();EXPECT_EQ(keyPad.calls,controls);
 leadscrew.setFaultInhibit(false);globalState->setMotionMode(MM_JOG_RIGHT);
 for(int i=0;i<1000;++i){setTestMicros(elsMicros()+4);timerCallback();}
 EXPECT_EQ(io.completions,1);
}
