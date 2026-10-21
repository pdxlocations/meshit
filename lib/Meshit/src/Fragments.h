#pragma once
#include "Types.h"
namespace meshit {
constexpr size_t FRAGMENT_BYTES=112, MAX_FRAGMENTS=7;
struct Reassembly {Id transfer=0;uint32_t at=0;uint8_t count=0;std::array<Bytes,MAX_FRAGMENTS> parts;bool relayed=false;};
inline std::vector<Bytes> fragment(Id transfer,const Bytes& payload){
  size_t count=(payload.size()+FRAGMENT_BYTES-1)/FRAGMENT_BYTES;
  if(!transfer||!count||count>MAX_FRAGMENTS)return {};
  std::vector<Bytes> out;
  for(size_t i=0;i<count;++i){Bytes b{'m',1};put(b,transfer,6);b.push_back(uint8_t(i));b.push_back(uint8_t(count));auto start=payload.begin()+i*FRAGMENT_BYTES;auto end=payload.begin()+std::min(payload.size(),(i+1)*FRAGMENT_BYTES);b.insert(b.end(),start,end);out.push_back(std::move(b));}return out;
}
class Assembler {
  std::array<Reassembly,4> slots{};
public:
  bool accept(const Bytes& b,uint32_t now,bool relayed,Bytes& out,bool& allRelayed){
    out.clear();if(b.size()<11||b.size()>10+FRAGMENT_BYTES||b[0]!='m'||b[1]!=1)return false;
    Id id=get(b.data()+2,6);uint8_t i=b[8],count=b[9];
    if(!id||!count||count>MAX_FRAGMENTS||i>=count||(i+1<count&&b.size()!=10+FRAGMENT_BYTES))return false;
    Reassembly* p=nullptr;
    for(auto& s:slots){if(s.transfer&&uint32_t(now-s.at)>600000)s={};if(s.transfer==id)p=&s;}
    if(!p){for(auto& s:slots)if(!s.transfer){p=&s;break;}if(!p)return false;*p={};p->transfer=id;p->count=count;p->at=now;p->relayed=true;}
    if(p->count!=count)return false;
    Bytes part(b.begin()+10,b.end());
    if(!p->parts[i].empty()&&p->parts[i]!=part)return false;
    p->parts[i]=std::move(part);p->relayed=p->relayed&&relayed;
    for(size_t n=0;n<count;++n)if(p->parts[n].empty())return false;
    for(size_t n=0;n<count;++n)out.insert(out.end(),p->parts[n].begin(),p->parts[n].end());allRelayed=p->relayed;*p={};return true;
  }
};
}
