#pragma once
#include <array>
#include <string>
#include <vector>
constexpr int SSD1306_SWITCHCAPVCC=2;
class OledWireMock {
public:
 int transmissions=0,failAt=-1;uint8_t failure=2;bool shortWrite=false;
 void beginTransmission(uint8_t){}
 size_t write(uint8_t){return shortWrite?0:1;}
 size_t write(const uint8_t*,size_t count){return shortWrite?0:count;}
 uint8_t endTransmission(){return ++transmissions==failAt?failure:0;}
 void setClock(uint32_t){}
};
static OledWireMock Wire;
class Adafruit_SSD1306 {
public:
 bool beginOK=true;
 std::array<uint8_t,1024> buffer{};
 std::vector<std::string> text;
 Adafruit_SSD1306()=default;
 Adafruit_SSD1306(int,int,OledWireMock*,int){}
 bool begin(int,int){return beginOK;}
 uint8_t* getBuffer(){return buffer.data();}
 void clearDisplay(){}
 void setCursor(int,int){}
 void setTextSize(int){}
 void setTextColor(int){}
 void print(const char* value){text.emplace_back(value);}
 void display(){}
 void drawLine(int,int,int,int,int){}
 void drawBitmap(int,int,const uint8_t*,int,int,int){}
 void fillRoundRect(int,int,int,int,int,int){}
};
