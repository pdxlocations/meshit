#pragma once
#include "Types.h"
#include <ChaChaPoly.h>
#include <SHA256.h>

namespace meshit {
// Public, built-in application key: obscures broker contents, not secret from users.
inline std::array<uint8_t,32> appKey(){
  std::array<uint8_t,32> key{};SHA256 h;const char label[]="Meshit application encryption v1";h.update(label,sizeof(label)-1);h.finalize(key.data(),key.size());return key;
}
inline Bytes seal(const Bytes& plain,const std::array<uint8_t,12>& nonce){
  if(plain.empty()||plain.size()>MAX_EVENT)return {};
  Bytes b{'M','I',1};b.insert(b.end(),nonce.begin(),nonce.end());b.resize(15+plain.size()+16);
  ChaChaPoly c;auto k=appKey();c.setKey(k.data(),k.size());c.setIV(nonce.data(),nonce.size());c.addAuthData(b.data(),15);
  c.encrypt(b.data()+15,plain.data(),plain.size());c.computeTag(b.data()+15+plain.size(),16);return b;
}
inline bool open(const Bytes& b,Bytes& plain){
  plain.clear();if(b.size()<32||b.size()>MAX_EVENT+31||b[0]!='M'||b[1]!='I'||b[2]!=1)return false;
  Bytes out(b.size()-31);ChaChaPoly c;auto k=appKey();c.setKey(k.data(),k.size());c.setIV(b.data()+3,12);c.addAuthData(b.data(),15);
  c.decrypt(out.data(),b.data()+15,out.size());if(!c.checkTag(b.data()+15+out.size(),16))return false;
  plain=std::move(out);return true;
}
}
