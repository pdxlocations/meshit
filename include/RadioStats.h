#pragma once
#include <Core.h>
namespace meshit {
inline size_t outgoingCount(const Core& core){size_t n=0;for(const auto& p:core.pending)if(!p.ack&&!p.history&&(p.event.kind==Kind::Post||p.event.kind==Kind::Vote))++n;return n;}
inline size_t roomMessageCount(const Core& core,const std::string& room){return std::count_if(core.messages.begin(),core.messages.end(),[&](const Message& m){return m.event.room==room;});}
}
