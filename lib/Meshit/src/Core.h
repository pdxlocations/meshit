#pragma once
#include "Types.h"
namespace meshit {
struct Message {Event event;uint32_t order=0;bool mine=false,acked=false,sent=false;};
struct Vote {Id event=0,target=0;uint32_t voter=0;bool up=false;};
struct Pending {Event event;uint8_t attempts=0;uint32_t due=0;bool ack=false,history=false;};
struct Peer {uint32_t id=0;uint16_t high=0;};
struct Missing {Id id=0;uint32_t due=0;uint8_t attempts=0;};
class Core {
  uint32_t clock=0;
  std::vector<Id> seen,dead;
  std::vector<Event> recent;
  std::vector<Peer> peers;
  std::vector<Missing> missing;
  std::vector<Vote> deferredVotes;
  template<class T> static void bounded(std::vector<T>& v,size_t n){if(v.size()>n)v.erase(v.begin(),v.begin()+(v.size()-n));}
  bool contains(const std::vector<Id>& list,Id id)const{return std::find(list.begin(),list.end(),id)!=list.end();}
  void remember(const Event& e){seen.push_back(e.id);bounded(seen,512);if(e.kind==Kind::Vote){recent.push_back(e);bounded(recent,32);}}
  void want(Id id,uint32_t now){if(find(id)||contains(dead,id)||contains(seen,id))return;for(auto& m:missing)if(m.id==id)return;if(missing.size()<16)missing.push_back({id,now+500,0});}
  void wantThread(Id id,uint32_t now){
    for(size_t depth=0;id&&depth<32;++depth){
      if(contains(dead,id))return;
      auto m=find(id);if(!m){want(id,now);return;}
      id=m->event.ref;
    }
  }
  void wantVotedThreads(uint32_t now){
    for(const auto& m:messages)if(score(m.event.id)>0)wantThread(m.event.id,now);
  }
  void noteGap(const Event& e,uint32_t now){
    if(e.kind==Kind::History)return;
    auto p=std::find_if(peers.begin(),peers.end(),[&](const Peer& p){return p.id==sender(e.id);});
    uint16_t n=sequence(e.id);
    if(p==peers.end()){if(peers.size()==64)peers.erase(peers.begin());peers.push_back({sender(e.id),n});return;}
    if(n>p->high){for(uint32_t k=std::max(uint32_t(p->high)+1,n>16?uint32_t(n)-16:1);k<n;++k)want((Id(p->id)<<16)|k,now);p->high=n;}
  }
  void applyVote(const Event& e){
    auto& list=find(e.ref)?votes:deferredVotes;
    auto p=std::find_if(list.begin(),list.end(),[&](const Vote& v){return v.target==e.ref&&v.voter==sender(e.id);});
    if(p!=list.end()){if(e.id>p->event){p->event=e.id;p->up=e.up;}}
    else {list.push_back({e.id,e.ref,sender(e.id),e.up});bounded(list,&list==&votes?512:32);}
  }
  void removeTree(Id id){
    std::vector<Id> removed{id};
    for(size_t i=0;i<removed.size();++i)for(const auto& m:messages)if(m.event.ref==removed[i]&&!contains(removed,m.event.id))removed.push_back(m.event.id);
    for(Id r:removed){dead.push_back(r);bounded(dead,512);}
    messages.erase(std::remove_if(messages.begin(),messages.end(),[&](const Message& m){return contains(removed,m.event.id);}),messages.end());
    votes.erase(std::remove_if(votes.begin(),votes.end(),[&](const Vote& v){return contains(removed,v.target);}),votes.end());
    pending.erase(std::remove_if(pending.begin(),pending.end(),[&](const Pending& p){return contains(removed,p.event.id)||contains(removed,p.event.ref);}),pending.end());
    recent.erase(std::remove_if(recent.begin(),recent.end(),[&](const Event& e){return contains(removed,e.id)||contains(removed,e.ref);}),recent.end());
  }
  void trim(){while(messages.size()>MAX_MESSAGES){auto p=std::min_element(messages.begin(),messages.end(),[&](const Message& a,const Message& b){auto x=score(a.event.id),y=score(b.event.id);return x!=y?x<y:a.order<b.order;});removeTree(p->event.id);}}
public:
  Core(){messages.reserve(MAX_MESSAGES+1);}
  std::vector<Message> messages;
  std::vector<Vote> votes;
  std::vector<Pending> pending;
  std::function<Id()> allocate;
  std::function<uint32_t()> jitter=[](){return 0;};
  Message* find(Id id){for(auto& m:messages)if(m.event.id==id)return &m;return nullptr;}
  const Message* find(Id id)const{for(const auto& m:messages)if(m.event.id==id)return &m;return nullptr;}
  int score(Id id)const{int n=0;for(const auto& v:votes)if(v.target==id&&v.up)++n;return n;}
  const std::vector<Id>& tombstones()const{return dead;}
  void restoreDead(Id id){if(id&&!contains(dead,id)){dead.push_back(id);bounded(dead,512);}}
  bool voted(Id id,uint32_t who)const{for(const auto& v:votes)if(v.target==id&&v.voter==who)return v.up;return false;}
  void restore(Message m){if(valid(m.event)&&!find(m.event.id)){clock=std::max(clock,m.order);messages.push_back(std::move(m));trim();}}
  bool submit(Event e,uint32_t now){
    if(pending.size()>=12||!allocate)return false;
    if(e.kind==Kind::Post&&e.ref){auto p=find(e.ref);if(!p||p->event.room!=e.room)return false;}
    e.id=(Id(1)<<16)|(e.ref==((Id(1)<<16)|1)?2:1);if(!valid(e))return false;e.id=allocate();if(!valid(e))return false;
    receive(e,now,false,true);if(e.kind==Kind::Post&&!find(e.id))return false;
    pending.push_back({e,0,now+100+jitter()%500,false,false});return true;
  }
  bool resend(Id id,uint32_t now){
    auto m=find(id);if(!m||!m->mine||m->acked||pending.size()>=12)return false;
    for(const auto& p:pending)if(p.event.id==id&&!p.history)return false;
    pending.push_back({m->event,0,now+100+jitter()%500,false,false});return true;
  }
  void receive(const Event& e,uint32_t now,bool relayed,bool local=false){
    if(!valid(e))return;
    if(auto m=find(e.id))if(m->mine&&m->sent&&relayed&&e.kind==Kind::Post&&encode(m->event)==encode(e))m->acked=true;
    // An echo only acknowledges an originated event, after a successful transmission.
    for(auto& p:pending)if(p.event.id==e.id&&p.attempts&&relayed&&!p.history&&encode(p.event)==encode(e)){p.ack=true;if(auto m=find(e.id))m->acked=true;}
    // Hearing an answer suppresses our scheduled answer, even when already stored.
    if(e.kind==Kind::Post||e.kind==Kind::History||e.kind==Kind::Vote){
      pending.erase(std::remove_if(pending.begin(),pending.end(),[&](const Pending& p){return p.history&&p.event.id==e.id;}),pending.end());
      missing.erase(std::remove_if(missing.begin(),missing.end(),[&](const Missing& m){return m.id==e.id;}),missing.end());
    }
    if(contains(dead,e.id))return;
    if(e.kind!=Kind::History&&contains(seen,e.id))return;
    if(!local)noteGap(e,now);
    if(e.kind!=Kind::History)remember(e);
    if(e.kind==Kind::Post||e.kind==Kind::History){
      if(find(e.id))return;
      if(e.ref){if(contains(dead,e.ref))return;auto p=find(e.ref);if(p&&p->event.room!=e.room)return;
        Id ancestor=e.ref;for(size_t depth=0;ancestor;++depth){if(ancestor==e.id||depth>=32)return;auto parent=find(ancestor);if(!parent)break;ancestor=parent->event.ref;}
      }
      Event post=e;post.kind=Kind::Post;messages.push_back({post,++clock,local,false});
      for(auto it=deferredVotes.begin();it!=deferredVotes.end();)if(it->target==e.id){votes.push_back(*it);it=deferredVotes.erase(it);}else ++it;
      trim();wantVotedThreads(now);
    }else if(e.kind==Kind::Vote){
      if(contains(dead,e.ref))return;
      applyVote(e);if(e.up)wantThread(e.ref,now);
    }else if(e.kind==Kind::Request){
      if(pending.size()>=12||contains(dead,e.ref))return;
      Event answer;bool found=false;
      if(auto m=find(e.ref)){answer=m->event;answer.kind=Kind::History;found=true;}
      else for(const auto& r:recent)if(r.id==e.ref&&r.kind==Kind::Vote){answer=r;found=true;break;}
      if(!found)for(const auto& v:votes)if(v.event==e.ref){answer.kind=Kind::Vote;answer.id=v.event;answer.ref=v.target;answer.up=v.up;found=true;break;}
      if(found){for(const auto& p:pending)if(p.history&&p.event.id==answer.id)return;pending.push_back({answer,0,now+1000+jitter()%5000,false,true});}
    }
  }
  // send() must return false for channel-busy or failed transmission: no retry consumed.
  void tick(uint32_t now,const std::function<bool(const Event&,bool)>& send){
    for(auto it=pending.begin();it!=pending.end();){
      if(it->ack){it=pending.erase(it);continue;}
      if(int32_t(now-it->due)<0){++it;continue;}
      if(it->attempts>=(it->history?1:4)){it=pending.erase(it);continue;}
      if(send(it->event,it->history)){++it->attempts;if(auto m=find(it->event.id))m->sent=true;it->due=now+45000+(it->attempts-1)*15000+jitter()%5000;}
      else it->due=now+1000+jitter()%2000;
      break; // At most one event per loop.
    }
    for(auto it=missing.begin();it!=missing.end();){
      if(find(it->id)||contains(dead,it->id)||it->attempts>=3){it=missing.erase(it);continue;}
      if(int32_t(now-it->due)>=0&&pending.size()<12&&allocate){Event e;e.kind=Kind::Request;e.ref=it->id;e.id=allocate();if(send(e,false)){remember(e);++it->attempts;it->due=now+30000+jitter()%15000;}else it->due=now+3000;break;}++it;
    }
  }
};
}
