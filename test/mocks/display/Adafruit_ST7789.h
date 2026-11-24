#pragma once
#include "Adafruit_GFX.h"
#include "recording_text.h"
#include <SPI.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
constexpr int ST77XX_BLACK=0, ST77XX_WHITE=0xffff, ST77XX_RED=0xf800,
  ST77XX_GREEN=0x07e0, ST77XX_YELLOW=0xffe0;
class Adafruit_ST7789 : public RecordingText {
public:
  struct Rect { int x,y,w,h; };
  SPIClass* const spi;
  const int cs,dc,reset;
  int width=0,height=0,rotation=0;
  bool inverted=false;
  uint32_t frequency=0, maxFlushPixels=0;
  std::vector<Rect> cleared;
  std::vector<uint16_t> pixels;
  Rect window{};
  Adafruit_ST7789(SPIClass* bus,int select,int data,int rst):spi(bus),cs(select),dc(data),reset(rst){}
  void init(int w,int h){width=w;height=h;spi->begin();}
  void setRotation(int r){rotation=r;if(r%2)std::swap(width,height);pixels.resize(width*height);}
  void setSPISpeed(uint32_t hz){frequency=hz;}
  void invertDisplay(bool invert){inverted=invert;}
  void fillScreen(int colour){std::fill(pixels.begin(),pixels.end(),colour);}
  void setTextColor(int,int=0){}
  void fillRect(int x,int y,int w,int h,int){cleared.push_back({x,y,w,h});}
  void drawLine(int,int,int,int,int){}
  void drawBitmap(int,int,const uint8_t*,int,int,int){}
  void fillRoundRect(int,int,int,int,int,int){}
  void startWrite(){}
  void setAddrWindow(int x,int y,int w,int h){window={x,y,w,h};}
  void writePixels(uint16_t* values,uint32_t len,bool){
    maxFlushPixels=std::max(maxFlushPixels,len);
    for(int row=0;row<window.h;++row)
      std::copy_n(values+window.w*row,window.w,pixels.begin()+width*(window.y+row)+window.x);
  }
  void endWrite(){}
};
