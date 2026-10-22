#include <Core.h>
#include <CryptoBox.h>
#include <Fragments.h>
#include <Carriers.h>
#include <Snapshot.h>
#include "../include/SetupAccess.h"
#include "../include/BatteryGauge.h"
#include <cassert>
#include <iostream>
using namespace meshit;
Id id(uint32_t who,uint16_t n){return (Id(who)<<16)|n;}
Event post(uint32_t who,uint16_t n,Id parent=0){Event e;e.id=id(who,n);e.room="general";e.text="A lightweight conversation";e.ref=parent;return e;}
Event vote(uint32_t who,uint16_t n,Id target,bool up=true){Event e;e.id=id(who,n);e.kind=Kind::Vote;e.ref=target;e.up=up;return e;}
Core client(uint32_t who){Core c;c.allocate=[who,n=uint16_t(0)]()mutable{return id(who,++n);};return c;}
struct Memory {
  Bytes bytes;size_t at=0;
  size_t write(const uint8_t* b,size_t n){bytes.insert(bytes.end(),b,b+n);return n;}
  int read(uint8_t* b,size_t n){size_t len=std::min(n,bytes.size()-at);memcpy(b,bytes.data()+at,len);at+=len;return len;}
};
std::string hexBytes(const Bytes& b){std::string s;char x[3];for(auto c:b){snprintf(x,3,"%02x",c);s+=x;}return s;}
int main(){
  assert(!plausibleBatteryVoltage(0));assert(!plausibleBatteryVoltage(5000));
  assert(batteryBars(3400)==0&&batteryBars(3600)==1&&batteryBars(3800)==2&&batteryBars(4000)==3&&batteryBars(4200)==4);

  assert(setupPassword(false,"",0)=="12345678");
  assert(setupPassword(false,"48214821",99)=="12345678");
  assert(setupPassword(true,"48214821",99)=="48214821");
  for(const auto& saved:{"","abcdefghijkl","12345678","1234123x","00000000","123412341234","１２３４１２３４"}){
    auto pass=setupPassword(true,saved,0);assert(pass=="10001000");
  }
  for(uint32_t random:{0u,1u,8999u,9000u,UINT32_MAX}){
    auto pass=setupPassword(true,"",random);assert(pass.size()==8&&pass.substr(0,4)==pass.substr(4));
    for(char c:pass)assert(c>='0'&&c<='9');
    assert(setupPassword(true,pass,42)==pass);
  }

  assert(hex(id(0xabcdef01,0x1234))=="abcdef011234");assert(fromHex("abcdef011234")==id(0xabcdef01,0x1234));assert(!fromHex("gggggggggggg"));
  auto e=post(1,1);assert(valid(e));auto bytes=encode(e);Event decoded;assert(decode(bytes,decoded));assert(encode(decoded)==bytes);bytes.push_back(0);assert(!decode(bytes,decoded));
  e.text=std::string(160,'a');assert(valid(e));e.text+='a';assert(!valid(e));e.text="";for(int i=0;i<160;++i)e.text+="🙂";assert(valid(e));assert(e.text.size()==640);e.text+="a";assert(!valid(e));e.text="\xc0\x80";assert(!valid(e));e.text="\xed\xa0\x80";assert(!valid(e));e.text="\x01";assert(!valid(e));
  auto c=client(11);assert(c.submit(post(1,1),0));auto own=c.messages[0].event;int sends=0;
  c.receive(own,50,true);assert(!c.messages[0].acked); // No TX yet.
  c.tick(200,[&](const Event&,bool){++sends;return true;});assert(sends==1);
  c.receive(own,300,false);assert(!c.messages[0].acked);auto wrong=own;wrong.text="forged";c.receive(wrong,350,true);assert(!c.messages[0].acked);
  c.receive(own,400,true);assert(c.messages[0].acked);c.tick(500,[](const Event&,bool){assert(false);return true;});assert(c.pending.empty());
  auto r=client(12);r.submit(post(1,1),0);sends=0;for(uint32_t t=0;t<400000;t+=1000)r.tick(t,[&](const Event&,bool){++sends;return true;});assert(sends==4);assert(r.pending.empty());
  auto retryId=r.messages[0].event.id;auto original=encode(r.messages[0].event);
  assert(r.resend(retryId,400000));assert(!r.resend(retryId,400001));assert(r.messages.size()==1);
  assert(encode(r.pending[0].event)==original);assert(!r.resend(id(99,1),400002));
  r.tick(401000,[](const Event&,bool){return true;});r.receive(r.messages[0].event,402000,true);
  assert(r.messages[0].acked);assert(!r.resend(retryId,402001));
  auto foreign=client(15);foreign.receive(post(20,1),0,false);assert(!foreign.resend(id(20,1),1));
  r.receive(r.messages[0].event,401000,true);assert(r.messages[0].acked); // Late echo still earns a check.
  auto busy=client(13);busy.submit(post(1,1),0);for(uint32_t t=0;t<100000;t+=1000)busy.tick(t,[](const Event&,bool){return false;});assert(busy.pending[0].attempts==0);
  auto quiet=client(14);quiet.receive(post(20,50),0,false);assert(quiet.pending.empty());sends=0;quiet.tick(60000,[&](const Event&,bool){++sends;return true;});assert(!sends); // First hearing skips old history.
  quiet.receive(post(20,52),61000,false);std::vector<Event> emitted;
  quiet.tick(62000,[&](const Event& out,bool history){assert(!history);emitted.push_back(out);return true;});assert(emitted.size()==1&&emitted[0].kind==Kind::Request&&emitted[0].ref==id(20,51));
  auto source=client(20);source.receive(post(20,51),0,false);source.receive(emitted[0],62000,false);source.tick(70000,[&](const Event& out,bool history){assert(history&&out.kind==Kind::History);quiet.receive(out,70000,false);return true;});assert(quiet.find(id(20,51)));
  auto old=client(30);old.receive(vote(22,1,id(90,1)),0,false);old.tick(1000,[&](const Event& out,bool){assert(out.kind==Kind::Request&&out.ref==id(90,1));return true;});auto recovered=post(90,1);recovered.kind=Kind::History;old.receive(recovered,2000,false);assert(old.score(id(90,1))==1);old.receive(vote(22,2,id(90,1),false),3000,false);assert(old.score(id(90,1))==0);old.receive(vote(22,1,id(90,1)),4000,false);assert(old.score(id(90,1))==0);
  // A live orphan alone does not backfill history; an upvote recovers its parent.
  auto thread=client(31);auto threadRoot=post(90,1),threadReply=post(91,1,threadRoot.id);
  thread.receive(threadReply,0,false);thread.tick(1000,[](const Event&,bool){assert(false);return true;});
  thread.receive(vote(92,1,threadReply.id),2000,false);std::vector<Id> requested;
  thread.tick(3000,[&](const Event& out,bool){assert(out.kind==Kind::Request);requested.push_back(out.ref);return true;});
  assert(requested==std::vector<Id>{threadRoot.id});
  thread.receive(vote(93,1,threadReply.id),3500,false);thread.tick(4000,[](const Event&,bool){assert(false);return true;}); // Deduplicated.
  threadRoot.kind=Kind::History;thread.receive(threadRoot,4500,false);assert(thread.find(threadRoot.id)&&thread.score(threadReply.id)==2);
  thread.tick(5000,[](const Event&,bool){assert(false);return true;});
  // If the reply is missing too, recover it, its parent, and the original root.
  auto chain=client(32);auto chainRoot=post(80,1),chainParent=post(81,1,chainRoot.id),chainReply=post(82,1,chainParent.id);
  chain.receive(vote(83,1,chainReply.id),0,false);requested.clear();
  auto collect=[&](const Event& out,bool){assert(out.kind==Kind::Request);requested.push_back(out.ref);return true;};
  chain.tick(1000,collect);assert(requested.back()==chainReply.id);
  chainReply.kind=Kind::History;chain.receive(chainReply,2000,false);chain.tick(3000,collect);assert(requested.back()==chainParent.id);
  chainParent.kind=Kind::History;chain.receive(chainParent,4000,false);chain.tick(5000,collect);assert(requested.back()==chainRoot.id);
  chainRoot.kind=Kind::History;chain.receive(chainRoot,6000,false);chain.tick(7000,[](const Event&,bool){assert(false);return true;});
  assert(requested.size()==3&&chain.messages.size()==3&&chain.score(chainReply.id)==1);
  // Removing the only upvote before recovery must not start parent backfill.
  auto withdrawn=client(33);withdrawn.receive(vote(84,1,chainReply.id),0,false);withdrawn.receive(vote(84,2,chainReply.id,false),100,false);
  withdrawn.receive(chainReply,200,false);withdrawn.tick(1000,[](const Event&,bool){assert(false);return true;});
  auto cap=client(40);auto parent=post(41,1);cap.receive(parent,0,false);auto child=post(42,1,parent.id);cap.receive(child,0,false);cap.receive(vote(43,1,child.id),0,false);
  for(uint16_t n=2;n<=201;++n)cap.receive(post(41,n),n,false);
  assert(cap.messages.size()<=200);assert(!cap.find(parent.id)&&!cap.find(child.id));assert(!cap.score(child.id));cap.receive(child,1000,false);assert(!cap.find(child.id));
  auto ranked=client(50);ranked.receive(post(51,1),0,false);ranked.receive(vote(52,1,id(51,1)),0,false);for(uint16_t n=2;n<=205;++n)ranked.receive(post(51,n),n,false);assert(ranked.messages.size()==200&&ranked.find(id(51,1)));assert(!ranked.find(id(51,2)));
  auto snapshot=client(60);snapshot.submit(post(1,1),0);snapshot.receive(vote(61,1,snapshot.messages[0].event.id),0,false);Memory storage;assert(writeSnapshot(storage,snapshot));Core restored;assert(readSnapshot(storage,restored,500));assert(restored.messages.size()==1&&restored.pending.size()==1&&restored.score(snapshot.messages[0].event.id)==1);storage.bytes.pop_back();storage.at=0;Core corrupt;assert(!readSnapshot(storage,corrupt,0));
  std::array<uint8_t,12> nonce{};for(size_t i=0;i<nonce.size();++i)nonce[i]=i;auto plain=encode(post(0x12345678,9));auto encrypted=seal(plain,nonce);Bytes opened;assert(open(encrypted,opened)&&opened==plain);
  for(size_t i=0;i<encrypted.size();++i){auto tampered=encrypted;tampered[i]^=1;assert(!open(tampered,opened)&&opened.empty());}
  auto parts=fragment(id(2,1),encrypted);Assembler assembler;Bytes assembled;bool relayed=false;for(auto it=parts.rbegin();it!=parts.rend();++it)assembler.accept(*it,0,true,assembled,relayed);assert(assembled==encrypted&&relayed);
  auto huge=post(1,1);huge.room=std::string(16,'r');huge.nick=std::string(16,'n');huge.text="";for(int i=0;i<160;++i)huge.text+="🙂";auto hugeParts=fragment(id(3,1),seal(encode(huge),nonce));assert(hugeParts.size()==7);
  for(const auto& p:hugeParts){auto mc=meshcorePacket(p),mt=meshtasticPacket(p,0x12345678,0x23456789,"LongFast");assert(mc.size()<=186&&mt.size()<=255);Received received;assert(parseMeshcore(mc,received)&&received.payload==p&&!received.relayed);auto echo=mc;echo[1]=1;echo.insert(echo.begin()+2,0x55);assert(parseMeshcore(echo,received)&&received.relayed&&received.payload==p);assert(parseMeshtastic(mt,"LongFast",received)&&received.payload==p&&!received.relayed);echo=mt;echo[12]-=1;assert(parseMeshtastic(echo,"LongFast",received)&&received.relayed);assert(!parseMeshtastic(mt,"Other",received));}
  // Meshtastic uses low 3 bits for remaining hops and high 3 for initial hops.
  for(uint8_t hops=0;hops<=7;++hops){
    auto packet=meshtasticPacket(parts[0],0x12345678,0x23456789,"LongFast",hops);
    assert((packet[12]&7)==hops&&(packet[12]>>5)==hops&&(packet[12]&0x18)==0);
    Received received;assert(parseMeshtastic(packet,"LongFast",received)&&!received.relayed);
    for(uint8_t remaining=0;remaining<hops;++remaining){packet[12]=(hops<<5)|remaining;assert(parseMeshtastic(packet,"LongFast",received)&&received.relayed&&received.payload==parts[0]);}
    if(hops<7){packet[12]=(hops<<5)|(hops+1);assert(!parseMeshtastic(packet,"LongFast",received));}
  }
  assert(meshtasticPacket(parts[0],1,1,"LongFast",8).empty());
  assert(meshtasticPacket(parts[0],0,1,"LongFast").empty());assert(meshtasticPacket(parts[0],1,0,"LongFast").empty());
  auto mc=meshcorePacket(parts[0]),mt=meshtasticPacket(parts[0],0x12345678,0x23456789,"LongFast");
  std::cout<<"{\"plain\":\""<<hexBytes(plain)<<"\",\"encrypted\":\""<<hexBytes(encrypted)<<"\",\"fragment\":\""<<hexBytes(parts[0])<<"\",\"meshcore\":\""<<hexBytes(mc)<<"\",\"meshtastic\":\""<<hexBytes(mt)<<"\"}\n";
}
