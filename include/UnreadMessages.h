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
  size_t count()const{return ids.size();}
};
