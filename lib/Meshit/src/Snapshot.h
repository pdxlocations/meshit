#pragma once
#include "Core.h"
namespace meshit {
// Streaming binary snapshots avoid a second, unbounded JSON copy of the history.
// Reader/Writer supply read/write(uint8_t*, size_t), returning byte count.
template<class Stream> bool writeSnapshot(Stream& stream,const Core& core){
  bool ok=true;
  auto number=[&](uint64_t value,size_t len){uint8_t b[8];for(size_t i=0;i<len;++i)b[i]=value>>(i*8);if(stream.write(b,len)!=len)ok=false;};
  auto event=[&](const Event& e){auto bytes=encode(e);number(bytes.size(),2);if(stream.write(bytes.data(),bytes.size())!=bytes.size())ok=false;};
  number(0x3254494d,4);number(core.messages.size(),2);number(core.votes.size(),2);number(core.tombstones().size(),2);
  size_t count=0;for(const auto& p:core.pending)if(!p.history&&!p.ack&&p.event.kind!=Kind::Request)++count;number(count,1);
  for(const auto& m:core.messages){event(m.event);number(m.order,4);number((m.mine?1:0)|(m.acked?2:0)|(m.sent?4:0),1);}
  for(const auto& v:core.votes){number(v.event,6);number(v.target,6);number(v.voter,4);number(v.down?2:v.up?1:0,1);}
  for(Id id:core.tombstones())number(id,6);
  for(const auto& p:core.pending)if(!p.history&&!p.ack&&p.event.kind!=Kind::Request){event(p.event);number(p.attempts,1);}
  number(0x454e4421,4);return ok;
}
template<class Stream> bool readSnapshot(Stream& stream,Core& core,uint32_t now){
  bool ok=true;
  auto number=[&](size_t len){uint8_t b[8]={};if(stream.read(b,len)!=int(len))ok=false;return get(b,len);};
  auto event=[&](Event& e){size_t len=number(2);if(!ok||len<13||len>MAX_EVENT)return false;Bytes bytes(len);if(stream.read(bytes.data(),len)!=int(len))return false;return decode(bytes,e);};
  const auto version=number(4);if(version!=0x3154494d&&version!=0x3254494d)return false;
  size_t messages=number(2),votes=number(2),dead=number(2),pending=number(1);
  if(!ok||messages>200||votes>512||dead>512||pending>12)return false;
  for(size_t i=0;i<messages;++i){Message m;if(!event(m.event)||m.event.kind!=Kind::Post)return false;m.order=number(4);uint8_t flags=number(1);m.mine=flags&1;m.acked=flags&2;m.sent=flags&4;core.restore(std::move(m));}
  for(size_t i=0;i<votes;++i){Vote v;v.event=number(6);v.target=number(6);v.voter=number(4);const auto value=number(1);if(value>(version==0x3154494d?1:2))return false;v.up=value==1;v.down=value==2;if(core.find(v.target))core.votes.push_back(v);}
  for(size_t i=0;i<dead;++i)core.restoreDead(number(6));
  for(size_t i=0;i<pending;++i){Event e;if(!event(e))return false;uint8_t attempts=number(1);if(attempts<4&&(e.kind==Kind::Post||e.kind==Kind::Vote))core.pending.push_back({e,attempts,now+30000,false,false});}
  return number(4)==0x454e4421&&ok;
}
}
