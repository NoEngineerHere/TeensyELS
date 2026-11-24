#pragma once
constexpr int VSPI=3, HSPI=2;
class SPIClass {
public:
  const int port;
  int begins=0,mosi=-1,miso=-1,sclk=-1;
  explicit SPIClass(int port=0):port(port){}
  void setMOSI(int pin){mosi=pin;}
  void setMISO(int pin){miso=pin;}
  void setSCK(int pin){sclk=pin;}
  void begin(){++begins;}
  void begin(int sck,int mi,int mo,int){sclk=sck;miso=mi;mosi=mo;++begins;}
};
inline SPIClass SPI, SPI1{1}, SPI2{2};
