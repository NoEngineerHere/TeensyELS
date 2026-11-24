#pragma once
#include <cstdint>

enum class UpdatePhase { Idle, Draining, Waiting, Receiving, Failed, Complete };
struct UpdateStatus {
  UpdatePhase phase = UpdatePhase::Idle;
  char network[11]{};
  char key[33]{};
  char address[16]{};
  uint32_t received = 0, total = 0;
  const char* detail = nullptr;
};
