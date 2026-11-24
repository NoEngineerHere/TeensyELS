#pragma once
#include <cstdint>
class GlobalState;
void initializeIndicators();
void updateIndicators(GlobalState& state, uint32_t nowMs);
