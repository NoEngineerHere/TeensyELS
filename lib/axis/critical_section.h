#pragma once
#include <cstdint>
class ElsCriticalSection {
#ifdef CORE_TEENSY
  uint32_t saved;
#endif
public:
  ElsCriticalSection();
  ~ElsCriticalSection();
  ElsCriticalSection(const ElsCriticalSection&) = delete;
  ElsCriticalSection& operator=(const ElsCriticalSection&) = delete;
};
