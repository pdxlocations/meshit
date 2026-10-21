#pragma once
#include "Types.h"
#include <AES.h>
#include <SHA256.h>
namespace meshit {
enum class Network:uint8_t{Meshtastic,MeshCore};
struct Received {Bytes payload;bool relayed=false;};
inline void aesCtr(Bytes& data,uint32_t origin,uint32_t packet){
  const uint8_t key[]={0xd4,0xf1,0xbb,0x3a,0x20,0x29,0x07,0x59,0xf0,0xbc,0xff,0xab,0xcf,0x4e,0x69,0x01};
  AES128 aes;aes.setKey(key,16);uint8_t iv[16]={},stream[16];
  for(unsigned i=0;i<4;++i){iv[i]=packet>>(i*8);iv[8+i]=origin>>(i*8);}
  for(size_t at=0;at<data.size();at+=16){aes.encryptBlock(stream,iv);for(size_t i=0;i<16&&at+i<data.size();++i)data[at+i]^=stream[i];for(int i=15;i>=0;--i)if(++iv[i])break;}
}
inline uint8_t channelHash(const std::string& name){
  uint8_t v=0;for(char c:name)v^=uint8_t(c);
  const uint8_t key[]={0xd4,0xf1,0xbb,0x3a,0x20,0x29,0x07,0x59,0xf0,0xbc,0xff,0xab,0xcf,0x4e,0x69,0x01};for(auto b:key)v^=b;return v;
}
inline void varint(Bytes& b,uint32_t n){while(n>127){b.push_back(uint8_t(n)|128);n>>=7;}b.push_back(uint8_t(n));}
inline bool readVar(const Bytes& b,size_t& p,uint32_t& n){n=0;for(unsigned i=0;i<5&&p<b.size();++i){uint8_t c=b[p++];if(i==4&&(c&0xf0))return false;n|=uint32_t(c&127)<<(i*7);if(!(c&128))return true;}return false;}
inline bool protobufData(const Bytes& b,Bytes& payload){
  size_t p=0;uint32_t port=0;bool got=false;
  while(p<b.size()){
    uint32_t key=0,n=0;if(!readVar(b,p,key)||!(key>>3))return false;
    if((key&7)==0){if(!readVar(b,p,n))return false;if((key>>3)==1)port=n;}
    else if((key&7)==2){if(!readVar(b,p,n)||n>b.size()-p)return false;if((key>>3)==2){payload.assign(b.begin()+p,b.begin()+p+n);got=true;}p+=n;}
    else if((key&7)==5){if(b.size()-p<4)return false;p+=4;}
    else if((key&7)==1){if(b.size()-p<8)return false;p+=8;}
    else return false;
  }return got&&port==256;
}
inline Bytes meshtasticPacket(const Bytes& payload,uint32_t origin,uint32_t packet,const std::string& channel,uint8_t hops=3){
  if(payload.empty()||payload.size()>230||!origin||!packet||hops>7)return {};
  Bytes data{8};varint(data,256);data.push_back(18);varint(data,payload.size());data.insert(data.end(),payload.begin(),payload.end());
  if(data.size()>239)return {};aesCtr(data,origin,packet);
  Bytes b;put(b,0xffffffff,4);put(b,origin,4);put(b,packet,4);b.push_back(hops|(hops<<5));b.push_back(channelHash(channel));b.push_back(0);b.push_back(origin&255);b.insert(b.end(),data.begin(),data.end());return b;
}
inline bool parseMeshtastic(const Bytes& b,const std::string& channel,Received& out){
  if(b.size()<20||b.size()>255||get(b.data(),4)!=0xffffffff||b[13]!=channelHash(channel))return false;
  uint8_t start=b[12]>>5,hops=b[12]&7;if(hops>start)return false;
  Bytes data(b.begin()+16,b.end());aesCtr(data,get(b.data()+4,4),get(b.data()+8,4));
  out={};if(!protobufData(data,out.payload))return false;out.relayed=hops<start;return true;
}
inline std::array<uint8_t,32> groupKey(){
  std::array<uint8_t,32> key{};SHA256 h;const char label[]="Meshit group transport v1";h.update(label,sizeof(label)-1);h.finalize(key.data(),key.size());return key;
}
inline uint8_t groupHash(){auto k=groupKey();uint8_t hash[32];SHA256 h;h.update(k.data(),k.size());h.finalize(hash,32);return hash[0];}
inline Bytes meshcorePacket(const Bytes& payload){
  // Private development datagram type 0xFF4D; all rooms use this single group.
  if(payload.empty()||payload.size()>157)return {};
  Bytes p{0x4d,0xff,uint8_t(payload.size())};p.insert(p.end(),payload.begin(),payload.end());p.resize((p.size()+15)&~size_t(15),0);
  Bytes b{0x19,0,groupHash(),0,0}; // flood, group data, v1, empty path, hash, MAC
  auto key=groupKey();AES128 aes;aes.setKey(key.data(),16);size_t offset=b.size();b.resize(offset+p.size());
  for(size_t at=0;at<p.size();at+=16)aes.encryptBlock(b.data()+offset+at,p.data()+at);
  SHA256 h;h.resetHMAC(key.data(),32);h.update(b.data()+offset,p.size());h.finalizeHMAC(key.data(),32,b.data()+3,2);return b;
}
inline bool parseMeshcore(const Bytes& b,Received& out){
  if(b.size()<21||b.size()>255||b[0]!=0x19)return false;
  uint8_t count=b[1]&63,sz=(b[1]>>6)+1;if(sz>3||count*sz>64)return false;
  size_t at=2+count*sz;if(at+3>=b.size()||b[at]!=groupHash())return false;
  size_t len=b.size()-at-3;if(!len||len>176||len%16)return false;
  auto key=groupKey();uint8_t mac[2];SHA256 h;h.resetHMAC(key.data(),32);h.update(b.data()+at+3,len);h.finalizeHMAC(key.data(),32,mac,2);
  if(mac[0]!=b[at+1]||mac[1]!=b[at+2])return false;
  Bytes p(len);AES128 aes;aes.setKey(key.data(),16);for(size_t i=0;i<len;i+=16)aes.decryptBlock(p.data()+i,b.data()+at+3+i);
  if(p[0]!=0x4d||p[1]!=0xff||size_t(p[2])+3>p.size())return false;
  for(size_t i=3+p[2];i<p.size();++i)if(p[i])return false;
  out.payload.assign(p.begin()+3,p.begin()+3+p[2]);out.relayed=count>0;return true;
}
}
