#include <gmock/gmock.h>

#pragma once

class LeadscrewIOMock : public LeadscrewIO {
 public:
  MOCK_METHOD(void, writeStepPin, (uint8_t val), (override));
  MOCK_METHOD(uint8_t, readStepPin, (), (override));
  MOCK_METHOD(void, writeDirPin, (uint8_t val), (override));
  MOCK_METHOD(uint8_t, readDirPin, (), (override));
};