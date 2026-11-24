#pragma once
#include <array>
#include <Wire.h>
#include "Adafruit_GFX.h"
#include "recording_text.h"
constexpr int SSD1306_SWITCHCAPVCC=2;
class Adafruit_SSD1306 : public RecordingText {
public:
 bool beginOK=true;
 std::array<uint8_t,1024> buffer{};
 TwoWire* const wire;
 int initAddress=-1;
 bool periphBegin=true;
 Adafruit_SSD1306(int,int,TwoWire* bus,int,uint32_t,uint32_t):wire(bus){}
 bool begin(int,int address,bool,bool periph){initAddress=address;periphBegin=periph;return beginOK;}
 uint8_t* getBuffer(){return buffer.data();}
 void clearDisplay(){}
 void setTextColor(int){}
 void drawLine(int,int,int,int,int){}
 void drawBitmap(int,int,const uint8_t*,int,int,int){}
 void fillRoundRect(int,int,int,int,int,int){}
};
