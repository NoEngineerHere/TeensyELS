#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#define PROGMEM
constexpr int TFT_BLACK=0, TFT_WHITE=1, TFT_RED=2, TFT_GREEN=3, TFT_YELLOW=4;
class TFT_eSPI {
public:
  struct Rect { int x,y,w,h; };
  std::vector<std::string> text;
  std::vector<Rect> cleared;
  void init() {}
  void setRotation(int) {}
  void fillScreen(int) {}
  void setCursor(int,int) {}
  void setTextSize(int) {}
  void setTextColor(int) {}
  void fillRect(int x,int y,int w,int h,int) { cleared.push_back({x,y,w,h}); }
  void print(const char* s) { text.emplace_back(s); }
  void drawRect(int,int,int,int,int) {}
  void drawLine(int,int,int,int,int) {}
  void drawBitmap(int,int,const uint8_t*,int,int,int) {}
  void fillRoundRect(int,int,int,int,int,int) {}
};
