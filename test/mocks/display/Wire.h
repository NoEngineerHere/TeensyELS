#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

class TwoWire {
public:
  int sda=-1,scl=-1,begins=0,transmissions=0,failAt=-1;
  uint32_t clock=0;
  uint8_t failure=2;
  bool beginOK=true,shortWrite=false;
  size_t bytes=0;
  std::vector<uint8_t> addresses;
  void setSDA(int pin){sda=pin;}
  void setSCL(int pin){scl=pin;}
  void begin(){++begins;}
  bool begin(int data,int clk,uint32_t rate){sda=data;scl=clk;clock=rate;++begins;return beginOK;}
  void beginTransmission(uint8_t addr){addresses.push_back(addr);}
  size_t write(uint8_t){return shortWrite?0:1;}
  size_t write(const uint8_t*,size_t count){if(!shortWrite) bytes+=count;return shortWrite?0:count;}
  uint8_t endTransmission(){return ++transmissions==failAt?failure:0;}
  void setClock(uint32_t rate){clock=rate;}
};
inline TwoWire Wire, Wire1, Wire2;
