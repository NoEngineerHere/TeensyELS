#include <gtest/gtest.h>
#include <firmware_upload.h>
#include <vector>
#include <pulse_timing.h>

#undef ELS_HARDWARE_CONFIG
#define ELS_HARDWARE_CONFIG "native_oled.h"
namespace UploadConfig {
#include <firmware_descriptor.h>
}

namespace {
using namespace FirmwareUpload;
using UploadConfig::hardware;
using UploadConfig::makeFirmwareDescriptor;
constexpr auto descriptor = makeFirmwareDescriptor(hardware, PulseTiming{20,20,4,4});
constexpr size_t prefix = descriptorOffset + descriptor.size();
void putWord(std::vector<uint8_t>& bytes, size_t offset, uint32_t value) {
  for (unsigned n=0; n<4; ++n) bytes[offset+n] = uint8_t(value >> (8*n));
}
std::vector<uint8_t> application() {
  std::vector<uint8_t> bytes(prefix+2051,0);
  bytes[0] = imageMagic;
  bytes[1] = 2;
  putWord(bytes,segmentSizeOffset,appDescSize+descriptor.size()+64);
  putWord(bytes,appDescOffset,appDescMagic);
  std::copy(descriptor.begin(),descriptor.end(),bytes.begin()+descriptorOffset);
  for (size_t i=prefix;i<bytes.size();++i) bytes[i]=uint8_t(i);
  return bytes;
}
struct Body {
  std::vector<uint8_t> bytes = application();
  size_t readAt=0, interruptedAt=SIZE_MAX, received=0, chunk=17;
  bool permitted=true;
  bool active() const { return permitted; }
  int read(uint8_t* target,size_t count) {
    count=std::min({count,bytes.size()-readAt,interruptedAt-readAt,chunk});
    std::copy_n(bytes.data()+readAt,count,target);
    readAt+=count;
    return count;
  }
  void progress(size_t n) { EXPECT_GT(n,received); received=n; }
};
struct Attempts {
  unsigned begin=0, finish=0, select=0, abort=0;
  bool valid=true, selected=true, writeOK=true;
  std::vector<uint8_t> flash;
};
struct Writer {
  Attempts& a;
  bool live=false;
  ~Writer() { if(live) ++a.abort; }
  bool begin(size_t) { ++a.begin; live=true; return true; }
  bool write(const uint8_t* bytes,size_t count) {
    if(!a.writeOK) return false;
    a.flash.insert(a.flash.end(),bytes,bytes+count);
    return true;
  }
  bool finish() { live=false; ++a.finish; return a.valid; }
  bool select() { ++a.select; return a.selected; }
};
Result upload(Body& body, Attempts& flash) {
  Writer writer{flash};
  return install(body,writer,body.bytes.size(),descriptor);
}
}

TEST(FirmwareUpload, SizeMustBeFiniteDecimalAndFitExactlyInTheInactiveSlot) {
  EXPECT_EQ(contentLength("1048576",1048576,1310720),1048576u);
  for(const char* bad:{"", "0", "-2", "+512", "512x", "512,512", " 512", "99999999999999999999999999"})
    EXPECT_EQ(contentLength(bad,512,1310720),0u);
  EXPECT_EQ(contentLength("511",512,1310720),0u);
  EXPECT_EQ(contentLength("1310721",1310721,1310720),0u);
}

TEST(FirmwareUpload, WrongStepAndMissingDescriptorNeverOpenFlash) {
  Body body;Attempts flash;
  auto incompatible=hardware;
  incompatible.motion.step=23;
  const auto changed=makeFirmwareDescriptor(incompatible,PulseTiming{20,20,4,4});
  std::copy(changed.begin(),changed.end(),body.bytes.begin()+descriptorOffset);
  EXPECT_EQ(upload(body,flash),Result::WrongHardware);
  body={};std::fill_n(body.bytes.begin()+descriptorOffset,descriptor.size(),0);
  EXPECT_EQ(upload(body,flash),Result::WrongHardware);
  EXPECT_EQ(flash.begin,0u);EXPECT_EQ(flash.select,0u);
}

TEST(FirmwareUpload, RejectsMalformedImageBoundsAndChipBeforeErase) {
  Attempts flash;
  for(int fault:{0,1,2,3,4}) {
    Body body;
    if(fault==0) body.bytes[0]=0;
    if(fault==1) body.bytes[chipOffset]=9;
    if(fault==2) putWord(body.bytes,segmentSizeOffset,body.bytes.size());
    if(fault==3) putWord(body.bytes,segmentSizeOffset,appDescSize);
    if(fault==4) putWord(body.bytes,appDescOffset,0);
    EXPECT_EQ(upload(body,flash),Result::WrongImage);
  }
  EXPECT_EQ(flash.begin,0u);EXPECT_EQ(flash.select,0u);
}

TEST(FirmwareUpload, InterruptedBodyOrFlashErrorNeverSelectsAndAbortsLiveWrites) {
  for (int fault:{0,1,2,3,4}) {
    Body body;Attempts flash;
    if(fault==0) body.interruptedAt=prefix-1;
    if(fault==1) body.interruptedAt=prefix+13;
    if(fault==2) flash.writeOK=false;
    if(fault==3) flash.valid=false;
    if(fault==4) body.permitted=false;
    const auto expected = fault==2 ? Result::WriteFailed : fault==3 ? Result::VerifyFailed :
      fault==4 ? Result::Cancelled : Result::TransferFailed;
    EXPECT_EQ(upload(body,flash),expected);
    EXPECT_EQ(flash.select,0u);
    EXPECT_EQ(flash.abort,fault==1||fault==2?1u:0u);
  }
}

TEST(FirmwareUpload, CompleteFragmentedImageSelectsExactlyOnceAfterVerification) {
  for (bool available : {false,true}) {
    Body body;Attempts flash;flash.selected=available;
    EXPECT_EQ(upload(body,flash),available?Result::Installed:Result::SelectFailed);
    EXPECT_EQ(body.received,body.bytes.size());EXPECT_EQ(flash.flash,body.bytes);
    EXPECT_EQ(flash.begin,1u);EXPECT_EQ(flash.finish,1u);EXPECT_EQ(flash.select,1u);
    EXPECT_EQ(flash.abort,0u);
  }
}
