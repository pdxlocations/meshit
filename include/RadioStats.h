#pragma once
#include <Core.h>
namespace meshit {
enum class RadioActivity {Standby,Rx,Tx};
inline const char* radioActivityName(RadioActivity state){return state==RadioActivity::Tx?"tx":state==RadioActivity::Rx?"rx":"standby";}
class RadioActivityClock {
  uint32_t lastRx=0;bool hasRx=false;
public:
  void received(uint32_t now){lastRx=now;hasRx=true;}
  RadioActivity state(bool ready,bool transmitting,uint32_t now)const{return !ready?RadioActivity::Standby:transmitting?RadioActivity::Tx:hasRx&&uint32_t(now-lastRx)<1500?RadioActivity::Rx:RadioActivity::Standby;}
};

inline size_t outgoingCount(const Core& core){size_t n=0;for(const auto& p:core.pending)if(!p.ack&&!p.history&&(p.event.kind==Kind::Post||p.event.kind==Kind::Vote))++n;return n;}
inline size_t roomMessageCount(const Core& core,const std::string& room){return std::count_if(core.messages.begin(),core.messages.end(),[&](const Message& m){return m.event.room==room;});}
}
