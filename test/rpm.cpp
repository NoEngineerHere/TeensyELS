#include <gtest/gtest.h>
#include <rpm_format.h>
#include <limits>
TEST(Rpm, ExactBoundedRepresentations) {
  struct Sample {float input; const char* output;};
  for(const auto sample : {Sample{0,"   0RPM"}, Sample{12.9f,"  12RPM"}, Sample{-12.9f," -12RPM"}, Sample{-999,"-999RPM"}, Sample{9999,"9999RPM"}}) {
    struct {char before='a';char out[8];char after='z';} buffer;
    ASSERT_TRUE(formatRpm(sample.input,buffer.out));
    EXPECT_STREQ(buffer.out,sample.output);EXPECT_EQ(buffer.out[7],0);
    EXPECT_EQ(buffer.before,'a');EXPECT_EQ(buffer.after,'z');
  }
}
TEST(Rpm, InvalidValuesAreRepresentableWithoutOverflow) {
 for(float value:{-1000.0f,10000.0f,1000000.0f,float(INT32_MAX),float(INT32_MIN),
     std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()}) {
  struct {char before='a';char out[8];char after='z';} buffer;
  EXPECT_FALSE(formatRpm(value,buffer.out));EXPECT_STREQ(buffer.out," ---RPM");
  EXPECT_EQ(buffer.out[7],0);EXPECT_EQ(buffer.before,'a');EXPECT_EQ(buffer.after,'z');
 }
}
