#include <gtest/gtest.h>
#include <cstdlib>

TEST(HardwareConfig, CompilesRealHeadersAndRoutesCustomDisplayBus) {
  EXPECT_EQ(std::system("python3 test/test_hardware/probes.py"),0);
}
