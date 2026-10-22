#pragma once
#include <cstdint>
#include <string>

namespace meshit {
// The factory MAC suffix is independent of the rotating message sender ID.
inline std::string setupRadioName(uint16_t macSuffix){
  const char* digits="0123456789abcdef";
  std::string name="mesit-";
  for(int shift=12;shift>=0;shift-=4)name+=digits[(macSuffix>>shift)&15];
  return name;
}
// Reuse established numeric credentials even if display detection changes.
// New display-equipped radios use a repeatable, easy four-digit PIN twice.
inline std::string setupPassword(bool displayReady,const std::string& saved,uint32_t hardwareSeed){
  bool reusable=saved.size()==8;
  for(char c:saved)if(c<'0'||c>'9')reusable=false;
  if(reusable)return saved;
  if(!displayReady)return "12345678";
  const std::string pin=std::to_string(1000+hardwareSeed%9000);
  return pin+pin;
}
}
