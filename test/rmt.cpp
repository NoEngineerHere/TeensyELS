#include <gtest/gtest.h>
#include <cstdint>
#define ELS_TEST_ESP_IO
#define ELS_LEADSCREW_STEP 25
#define ELS_LEADSCREW_DIR 26
#define ELS_STEP_HIGH_US 20
#define ELS_STEP_LOW_US 20
using gpio_num_t=int;
constexpr int GPIO_MODE_OUTPUT=1, RMT_CHANNEL_0=0, RMT_IDLE_LEVEL_LOW=0;
constexpr int ESP_OK=0, ESP_ERR_TIMEOUT=1;
struct rmt_item32_t { unsigned duration0=0,level0=0,duration1=0,level1=0; };
struct rmt_config_t { int clk_div; struct { bool carrier_en,loop_en,idle_output_en; int idle_level; } tx_config; };
#define RMT_DEFAULT_CONFIG_TX(pin,channel) rmt_config_t{}
static int configResult,installResult,sendResult,pollResult,polls,submissions;
static rmt_item32_t submitted;
static void gpio_set_level(int,int) {}
static void gpio_set_direction(int,int) {}
static int rmt_config(rmt_config_t*) {return configResult;}
static int rmt_driver_install(int,int,int) {return installResult;}
static int rmt_write_items(int,const rmt_item32_t* item,int count,bool wait) {
 EXPECT_EQ(count,1);EXPECT_FALSE(wait);submitted=*item;++submissions;return sendResult;
}
static int rmt_wait_tx_done(int,int timeout) {EXPECT_EQ(timeout,0);++polls;return pollResult;}
#include <leadscrew_io_esp.h>
class Rmt : public testing::Test {
 void SetUp() override {configResult=installResult=sendResult=polls=submissions=0;pollResult=ESP_ERR_TIMEOUT;}
};
TEST_F(Rmt, PendingCompleteIsOneSubmissionAndOneCompletion) {
 LeadscrewIOESP io;ASSERT_TRUE(io.begin());
 EXPECT_EQ(io.servicePulse(0),LeadscrewIO::PulseResult::IDLE);EXPECT_EQ(polls,0);
 ASSERT_TRUE(io.startPulse(0));EXPECT_FALSE(io.startPulse(1));
 EXPECT_EQ(io.servicePulse(1),LeadscrewIO::PulseResult::PENDING);
 EXPECT_EQ(io.servicePulse(20),LeadscrewIO::PulseResult::PENDING);
 pollResult=ESP_OK;
 EXPECT_EQ(io.servicePulse(40),LeadscrewIO::PulseResult::COMPLETE);
 const int completedPolls=polls;
 EXPECT_EQ(io.servicePulse(44),LeadscrewIO::PulseResult::IDLE);EXPECT_EQ(polls,completedPolls);
 EXPECT_EQ(submissions,1);EXPECT_EQ(submitted.duration0,20u);EXPECT_EQ(submitted.duration1,20u);
 EXPECT_EQ(submitted.level0,1u);EXPECT_EQ(submitted.level1,0u);
 EXPECT_TRUE(io.startPulse(100));
}
TEST_F(Rmt, BackendErrorsRemainFaulted) {
 for(int which:{0,1,2,3}) {
  configResult=installResult=sendResult=0;pollResult=ESP_ERR_TIMEOUT;
  if(which==0)configResult=-1;if(which==1)installResult=-1;
  LeadscrewIOESP io;EXPECT_EQ(io.begin(),which>=2);
  if(which==2)sendResult=-1;
  EXPECT_EQ(io.startPulse(0),which==3);
  if(which==3)pollResult=-1;
  EXPECT_EQ(io.servicePulse(40),LeadscrewIO::PulseResult::FAULT);
  configResult=installResult=sendResult=pollResult=0;
  EXPECT_FALSE(io.startPulse(100));EXPECT_FALSE(io.begin());
 }
}
