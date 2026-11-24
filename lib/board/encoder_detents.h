#pragma once
#include <cstdint>

class EncoderDetents {
  const int64_t countsPerDetent;
  int64_t residual = 0;
public:
  explicit EncoderDetents(int64_t counts) : countsPerDetent(counts) {}
  int64_t consume(int64_t counts) {
    const int64_t partial = counts % countsPerDetent + residual;
    int64_t detents = counts / countsPerDetent + partial / countsPerDetent;
    residual = partial % countsPerDetent;
    if (detents > 0 && residual < 0) { --detents; residual += countsPerDetent; }
    if (detents < 0 && residual > 0) { ++detents; residual -= countsPerDetent; }
    return detents;
  }
};
