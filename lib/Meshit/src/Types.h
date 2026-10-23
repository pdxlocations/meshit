#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace meshit {
using Bytes = std::vector<uint8_t>;
using Id = uint64_t; // Only low 48 bits are transmitted.
constexpr size_t MAX_MESSAGES=200, MAX_TEXT=640, MAX_CHARS=160;
constexpr size_t MAX_ROOM=16, MAX_NICK=16, MAX_EVENT=700;
inline uint32_t sender(Id id){return uint32_t(id>>16);}
inline uint16_t sequence(Id id){return uint16_t(id);}
inline std::string hex(Id id){char b[13];snprintf(b,sizeof(b),"%012llx",(unsigned long long)id);return b;}
inline Id fromHex(const std::string& s){
  if(s.size()!=12)return 0; Id n=0;
  for(char c:s){int v=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;if(v<0)return 0;n=(n<<4)|v;}return n;
}
inline void put(Bytes& b,uint64_t n,size_t len){for(size_t i=0;i<len;++i)b.push_back(uint8_t(n>>(8*i)));}
inline uint64_t get(const uint8_t* b,size_t len){uint64_t n=0;for(size_t i=0;i<len;++i)n|=uint64_t(b[i])<<(i*8);return n;}
// Strict UTF-8 scalar validation; reject overlong encodings, surrogates and controls.
inline bool textValid(const std::string& s,size_t maxBytes,size_t maxChars,bool empty=false){
  if(s.size()>maxBytes||(!empty&&s.empty()))return false;
  size_t count=0;
  for(size_t i=0;i<s.size();){
    uint8_t a=s[i++];uint32_t c=a;size_t n=0;uint32_t min=0;
    if(a>=0xc2&&a<=0xdf){c=a&31;n=1;min=0x80;}
    else if(a>=0xe0&&a<=0xef){c=a&15;n=2;min=0x800;}
    else if(a>=0xf0&&a<=0xf4){c=a&7;n=3;min=0x10000;}
    else if(a>=0x80)return false;
    if(i+n>s.size())return false;
    for(size_t j=0;j<n;++j){uint8_t t=s[i++];if((t&0xc0)!=0x80)return false;c=(c<<6)|(t&63);}
    if(c<min||c>0x10ffff||(c>=0xd800&&c<=0xdfff)||(c<32&&c!=10)||c==127)return false;
    if(++count>maxChars)return false;
  }return true;
}
enum class Kind:uint8_t{Post=1,Vote=2,Request=3,History=4,Ack=5};
struct Event {Kind kind=Kind::Post;Id id=0,ref=0;std::string room,nick,text;bool up=true,down=false;};
inline bool valid(const Event& e){
  if(!sender(e.id)||!sequence(e.id))return false;
  if(e.kind==Kind::Post||e.kind==Kind::History)
    return e.ref!=e.id&&(!e.ref||(sender(e.ref)&&sequence(e.ref)))&&textValid(e.room,MAX_ROOM,MAX_ROOM)&&e.room.find('\n')==std::string::npos&&textValid(e.nick,MAX_NICK,MAX_NICK,true)&&e.nick.find('\n')==std::string::npos&&textValid(e.text,MAX_TEXT,MAX_CHARS);
  if(e.kind==Kind::Ack)return sender(e.ref)&&sequence(e.ref)&&sender(e.id)!=sender(e.ref);
  if(e.kind==Kind::Vote&&e.up&&e.down)return false;
  if(e.kind==Kind::Vote||e.kind==Kind::Request)return sender(e.ref)&&sequence(e.ref);
  return false;
}
inline Bytes encode(const Event& e){
  if(!valid(e))return {};
  Bytes b{uint8_t(e.kind)};put(b,e.id,6);put(b,e.ref,6);
  if(e.kind==Kind::Vote)b.push_back(e.down?2:e.up?1:0);
  if(e.kind==Kind::Post||e.kind==Kind::History){
    b.push_back(uint8_t(e.room.size()));b.push_back(uint8_t(e.nick.size()));put(b,e.text.size(),2);
    b.insert(b.end(),e.room.begin(),e.room.end());b.insert(b.end(),e.nick.begin(),e.nick.end());b.insert(b.end(),e.text.begin(),e.text.end());
  }return b;
}
inline bool decode(const Bytes& b,Event& e){
  if(b.size()<13||b.size()>MAX_EVENT)return false;
  e={};e.kind=Kind(b[0]);e.id=get(b.data()+1,6);e.ref=get(b.data()+7,6);
  if(e.kind==Kind::Post||e.kind==Kind::History){
    if(b.size()<17)return false;
    size_t r=b[13],n=b[14],t=get(b.data()+15,2);
    if(17+r+n+t!=b.size())return false;
    e.room.assign((const char*)b.data()+17,r);e.nick.assign((const char*)b.data()+17+r,n);e.text.assign((const char*)b.data()+17+r+n,t);
  }else if(e.kind==Kind::Vote){if(b.size()!=14||b[13]>2)return false;e.up=b[13]==1;e.down=b[13]==2;}
  else if(e.kind==Kind::Request||e.kind==Kind::Ack){if(b.size()!=13)return false;}
  else return false;
  return valid(e);
}
}
