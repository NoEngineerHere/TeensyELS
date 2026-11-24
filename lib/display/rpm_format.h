#pragma once
#include <cmath>
#include <cstdio>
#include <cstring>
inline bool formatRpm(float rpm, char (&out)[8]) {
  std::memset(out, 0, sizeof(out));
  if (std::isfinite(rpm) && rpm >= -999 && rpm <= 9999) {
    const int size = std::snprintf(out, sizeof(out), "%4dRPM", static_cast<int>(rpm));
    if (size >= 0 && static_cast<size_t>(size) < sizeof(out)) return true;
  }
  std::memcpy(out, " ---RPM", sizeof(out));
  return false;
}
