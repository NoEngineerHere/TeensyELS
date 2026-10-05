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
