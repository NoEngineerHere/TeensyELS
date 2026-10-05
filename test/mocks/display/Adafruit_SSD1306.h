#pragma once
#include <string>
#include <vector>
constexpr int SSD1306_SWITCHCAPVCC=2;
static int Wire;
class Adafruit_SSD1306 {
public:
 bool beginOK=true;int frames=0;
 std::vector<std::string> text;
 Adafruit_SSD1306()=default;
 Adafruit_SSD1306(int,int,int*,int){}
 bool begin(int,int){return beginOK;}
 void clearDisplay(){}
 void setCursor(int,int){}
 void setTextSize(int){}
 void setTextColor(int){}
 void print(const char* value){text.emplace_back(value);}
 void display(){++frames;}
 void drawLine(int,int,int,int,int){}
 void drawBitmap(int,int,const uint8_t*,int,int,int){}
 void fillRoundRect(int,int,int,int,int,int){}
};
