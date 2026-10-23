#pragma once
#include <Core.h>
#include <algorithm>

// Only retained incoming posts can be unread. Viewing one room leaves others alone.
class UnreadMessages {
  std::vector<meshit::Id> ids;
public:
  void prune(const meshit::Core& core){
    ids.erase(std::remove_if(ids.begin(),ids.end(),[&](meshit::Id id){return !core.find(id);}),ids.end());
  }
  void add(meshit::Id id,const meshit::Core& core){
    prune(core);
    if(core.find(id)&&std::find(ids.begin(),ids.end(),id)==ids.end())ids.push_back(id);
  }
  bool viewed(const std::string& room,uint32_t seenRevision,uint32_t revision,const meshit::Core& core){
    prune(core);
    if(seenRevision!=revision)return false;
    ids.erase(std::remove_if(ids.begin(),ids.end(),[&](meshit::Id id){return core.find(id)->event.room==room;}),ids.end());
    return true;
  }
  void viewedIds(const std::string& room,const std::vector<meshit::Id>& viewed,const meshit::Core& core){
    prune(core);ids.erase(std::remove_if(ids.begin(),ids.end(),[&](meshit::Id id){const auto* m=core.find(id);return m&&m->event.room==room&&std::find(viewed.begin(),viewed.end(),id)!=viewed.end();}),ids.end());
  }
  size_t count(const std::string& room,const meshit::Core& core)const{size_t n=0;for(auto id:ids){const auto* m=core.find(id);if(m&&m->event.room==room)++n;}return n;}
  size_t count()const{return ids.size();}
};
