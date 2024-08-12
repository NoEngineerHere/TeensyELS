#include <axis.h>
#include <gmock/gmock.h>
#pragma once

class AxisMock : public Axis {
 public:
  MOCK_METHOD(int, getCurrentPosition, (), (override));
  MOCK_METHOD(void, resetCurrentPosition, (), (override));
  MOCK_METHOD(void, setCurrentPosition, (int position), (override));
  MOCK_METHOD(void, incrementCurrentPosition, (int amount), (override));
};
