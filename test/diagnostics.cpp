#include <gtest/gtest.h>
#include <boot_diagnostics.h>
#include <clock.h>
#include <globalstate.h>
#include <cstring>
namespace D=BootDiagnosticsTest;
class Diagnostics : public testing::Test {
protected:
 void SetUp() override {
  D::reset();setTestMicros(1234);GlobalState::getInstance()->setMotionMode(MM_DISABLED);
 }
 void drain() {
  for(int i=0;i<5000;++i) {
   const size_t previous=D::port().output.size();serviceBootDiagnostics();
   ASSERT_LE(D::port().output.size()-previous,64u);
   if(D::port().output.size()==previous)return;
  }
  FAIL()<<"Report did not finish";
 }
};
TEST_F(Diagnostics, CapturePrecedesDestructiveRenderAndOnlyOccursOnce) {
 auto& p=D::port();p.resetFlags=0x1234;p.resetDetail=0x5678;p.breadcrumb=99;
 p.crashPresent=true;p.crash="retained crash and old breadcrumb 99\n";
 recordBootStage(1);EXPECT_EQ(p.breadcrumb,99u);
 ASSERT_TRUE(captureBootDiagnostics());
 EXPECT_EQ(p.resetFlags,0u);EXPECT_EQ(p.renders,1u);
 const std::string report=D::report();
 EXPECT_NE(report.find("0x00001234"),std::string::npos);EXPECT_NE(report.find("0x00005678"),std::string::npos);
 EXPECT_NE(report.find(p.crash),std::string::npos);
 recordBootStage(1);recordBootStage(2);EXPECT_EQ(p.breadcrumb,2u);
 p.crash="replacement";ASSERT_TRUE(captureBootDiagnostics());EXPECT_EQ(p.renders,1u);
 EXPECT_EQ(std::string(D::report()),report);
 p.connected=true;drain();EXPECT_EQ(p.output,report);
 p.output.clear();p.commands="Rr";serviceBootDiagnostics();EXPECT_EQ(p.reads,1u);
 p.output.clear();drain();EXPECT_EQ(p.reads,2u);EXPECT_EQ(p.output,report);
 EXPECT_EQ(GlobalState::getInstance()->getMotionMode(),MM_DISABLED);
}
TEST_F(Diagnostics, AbsentUSBZeroCapacityPartialWritesAndReconnectRetainReport) {
 ASSERT_TRUE(captureBootDiagnostics());const std::string report=D::report();auto& p=D::port();
 for(int i=0;i<1000;++i)serviceBootDiagnostics();EXPECT_TRUE(p.output.empty());
 p.connected=true;p.capacity=0;serviceBootDiagnostics();EXPECT_TRUE(p.output.empty());
 p.capacity=64;p.writeLimit=0;serviceBootDiagnostics();EXPECT_TRUE(p.output.empty());
 p.capacity=13;p.writeLimit=5;serviceBootDiagnostics();EXPECT_EQ(p.output,report.substr(0,5));
 p.connected=false;serviceBootDiagnostics();EXPECT_EQ(p.output,report.substr(0,5));
 p.connected=true;p.output.clear();drain();EXPECT_EQ(p.output,report);
 p.output.clear();p.commands="R";p.capacity=200;p.writeLimit=200;drain();EXPECT_EQ(p.output,report);
}
TEST_F(Diagnostics, OverflowRemainsTerminatedAndIncludesTruncationEvidence) {
 auto& p=D::port();p.crashPresent=true;p.crash.assign(10000,'x');
 ASSERT_TRUE(captureBootDiagnostics());EXPECT_TRUE(D::truncated());
 EXPECT_EQ(D::reportSize(),4095u);EXPECT_EQ(std::strlen(D::report()),4095u);
 EXPECT_EQ(D::report()[4095],0);
 EXPECT_NE(std::string(D::report()).find("\nTRUNCATED\n"),std::string::npos);
 p.connected=true;drain();EXPECT_EQ(p.output,std::string(D::report()));
}
TEST_F(Diagnostics, FullNonOverflowingSinkKeepsEveryAcceptedByte) {
 auto& p=D::port();p.crashPresent=true;ASSERT_TRUE(captureBootDiagnostics());
 const size_t prefix=D::reportSize();D::reset();p.crashPresent=true;p.crash.assign(4084-prefix,'x');
 ASSERT_TRUE(captureBootDiagnostics());EXPECT_FALSE(D::truncated());EXPECT_EQ(D::reportSize(),4084u);
 EXPECT_EQ(std::strlen(D::report()),4084u);
 recordBootFailure("DISPLAY_INIT_FAILED");EXPECT_TRUE(D::truncated());EXPECT_EQ(D::reportSize(),4095u);
}
TEST_F(Diagnostics, ThermalPreservesCoreRecordAndBreadcrumbs) {
 auto& p=D::port();p.thermal=true;p.resetFlags=0x100;p.resetDetail=0x55;p.breadcrumb=77;
 p.crashPresent=true;p.crash="must remain unconsumed";
 EXPECT_FALSE(captureBootDiagnostics());recordBootStage(1);recordBootStage(4);
 EXPECT_EQ(p.renders,0u);EXPECT_EQ(p.breadcrumbWrites,0u);EXPECT_EQ(p.breadcrumb,77u);
 EXPECT_EQ(p.resetFlags,0x100u);EXPECT_EQ(p.resetDetail,0x55u);
 EXPECT_NE(std::string(D::report()).find("THERMAL_RESET_REPORT_DEFERRED"),std::string::npos);
 EXPECT_NE(std::string(D::report()).find("BOOT_FAILURE: THERMAL_RESET"),std::string::npos);
 p.connected=true;drain();EXPECT_EQ(p.output,std::string(D::report()));
 EXPECT_FALSE(captureBootDiagnostics());EXPECT_EQ(p.renders,0u);
}
TEST_F(Diagnostics, NoCrashAndAppendedFailuresRemainReplayable) {
 ASSERT_TRUE(captureBootDiagnostics());recordBootFailure("TIMER_INIT_FAILED");
 EXPECT_NE(std::string(D::report()).find("No Crash Data To Report"),std::string::npos);
 D::port().connected=true;drain();EXPECT_EQ(D::port().output,std::string(D::report()));
 EXPECT_NE(D::port().output.find("TIMER_INIT_FAILED"),std::string::npos);
}
#ifdef ELS_PROGRESS_TELEMETRY
TEST_F(Diagnostics, ProgressRateSurvivesClockRolloverAndZeroCapacity) {
 setTestMicros(UINT32_MAX-400000);ASSERT_TRUE(captureBootDiagnostics());auto& p=D::port();
 p.connected=true;drain();p.output.clear();
 setTestMicros(elsMicros()+999999);EXPECT_FALSE(progressTelemetryDue());
 queueProgressTelemetry(1,2,3,false,true);drain();EXPECT_TRUE(p.output.empty());
 setTestMicros(elsMicros()+1);ASSERT_TRUE(progressTelemetryDue());
 queueProgressTelemetry(UINT32_MAX,UINT32_MAX-1,UINT32_MAX-2,true,false);
 p.capacity=0;for(int i=0;i<100;++i)serviceBootDiagnostics();EXPECT_TRUE(p.output.empty());
 p.capacity=7;p.writeLimit=3;drain();
 EXPECT_EQ(p.output.find("ELS_PROGRESS source="),0u);
 EXPECT_NE(p.output.find(" us="+std::to_string(elsMicros())+" "),std::string::npos);
 EXPECT_NE(p.output.find(" loop=4294967295 timer=4294967294 frames=4294967293 oled_fault=1 ready=0\n"),std::string::npos);
 EXPECT_FALSE(progressTelemetryDue());
 const std::string once=p.output;
 for(int i=0;i<100;++i){queueProgressTelemetry(1,2,3,false,true);serviceBootDiagnostics();}
 EXPECT_EQ(p.output,once);
}
TEST_F(Diagnostics, PartialProgressFinishesBeforeFailureReportReplay) {
 ASSERT_TRUE(captureBootDiagnostics());auto& p=D::port();p.connected=true;drain();p.output.clear();
 setTestMicros(elsMicros()+1000000);queueProgressTelemetry(10,20,30,false,true);
 p.capacity=7;p.writeLimit=3;serviceBootDiagnostics();
 ASSERT_EQ(p.output,"ELS");
 recordBootFailure("DISPLAY_COMM_FAILED");p.commands="R";drain();
 const size_t end=p.output.find('\n');ASSERT_NE(end,std::string::npos);
 EXPECT_NE(p.output.substr(0,end).find("loop=10 timer=20 frames=30 oled_fault=0 ready=1"),std::string::npos);
 EXPECT_EQ(p.output.substr(end+1),std::string(D::report()));
}
TEST_F(Diagnostics, ReconnectDiscardsOldPartialProgressAndReplaysBootFirst) {
 ASSERT_TRUE(captureBootDiagnostics());auto& p=D::port();p.connected=true;drain();p.output.clear();
 setTestMicros(elsMicros()+1000000);queueProgressTelemetry(10,20,30,false,true);
 p.capacity=7;p.writeLimit=3;serviceBootDiagnostics();ASSERT_EQ(p.output,"ELS");
 p.connected=false;serviceBootDiagnostics();p.connected=true;p.output.clear();drain();
 EXPECT_EQ(p.output,std::string(D::report()));EXPECT_FALSE(progressTelemetryDue());
 p.output.clear();setTestMicros(elsMicros()+1000000);queueProgressTelemetry(11,21,31,true,false);drain();
 EXPECT_EQ(p.output.find("ELS_PROGRESS source="),0u);
 EXPECT_NE(p.output.find("loop=11 timer=21 frames=31 oled_fault=1 ready=0\n"),std::string::npos);
}
#endif
