#pragma once
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

class RecordingText {
  int x=0,y=0,size=1;
public:
  struct Text { int x,y,size; std::string value; };
  std::vector<std::string> text;
  std::vector<Text> printed;
  void setCursor(int px,int py){x=px;y=py;}
  void setTextSize(int s){size=s;}
  void print(const char* s){text.emplace_back(s);printed.push_back({x,y,size,s});x+=text.back().size()*6*size;}
  void capture(const char* name) const {
    const char* dir=std::getenv("ELS_RENDER_DIR");
    if(!dir)return;
    std::ofstream out(std::string(dir)+"/"+name+".tsv");
    for(const auto& row:printed) out<<row.x<<'\t'<<row.y<<'\t'<<row.size<<'\t'<<row.value<<'\n';
  }
};
