#pragma once
#include <cstdint>

enum class puType { none, up, down };
namespace EncoderHardwareTest {
inline int attaches = 0, reads = 0;
inline bool attachSucceeds = true;
inline int64_t count = 0;
}
class ESP32Encoder {
  bool attached = false;
public:
  static inline puType useInternalWeakPullResistors = puType::down;
  void attachFullQuad(int,int) { ++EncoderHardwareTest::attaches; attached=EncoderHardwareTest::attachSucceeds; }
  void attachSingleEdge(int a,int b) { attachFullQuad(a,b); }
  bool isAttached() const { return attached; }
  int64_t getCount() { ++EncoderHardwareTest::reads; return EncoderHardwareTest::count; }
  void setFilter(int) {}
};
