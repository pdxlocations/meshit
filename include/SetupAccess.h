#pragma once
#include <cstdint>
#include <string>

namespace meshit {
// WPA requires at least eight characters. Repeating a four-digit PIN keeps it
// easy to read and type. Headless radios always use the documented fallback.
inline std::string setupPassword(bool displayReady,const std::string& saved,uint32_t random){
  if(!displayReady)return "12345678";
  bool reusable=saved.size()==8&&saved[0]>='1'&&saved[0]<='9';
  for(char c:saved)if(c<'0'||c>'9')reusable=false;
  if(reusable&&saved.substr(0,4)==saved.substr(4))return saved;
  const std::string pin=std::to_string(1000+random%9000);
  return pin+pin;
}
}
