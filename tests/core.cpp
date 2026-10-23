#include <Core.h>
#include <CryptoBox.h>
#include <Fragments.h>
#include <Carriers.h>
#include <Snapshot.h>
#include "../include/SetupAccess.h"
#include "../include/BatteryGauge.h"
#include "../include/UnreadMessages.h"
#include "../include/DeckChat.h"
#include "../include/DeckScene.h"
#include "../include/DeckRequest.h"
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
  {
    auto c=client(1);UnreadMessages unread;
    auto a=post(2,1),b=post(2,2);b.room="camp";
    c.receive(a,0,false);c.receive(b,0,false);unread.add(a.id,c);unread.add(b.id,c);unread.add(a.id,c);
    assert(unread.count("general",c)==1&&unread.count("camp",c)==1);
    assert(unread.count()==2); // Retransmissions do not double-count.
    assert(!unread.viewed("general",1,2,c)&&unread.count()==2); // Stale snapshot.
    assert(unread.viewed("general",2,2,c)&&unread.count()==1);
    assert(unread.viewed("general",2,2,c)&&unread.count()==1);
    assert(unread.viewed("camp",2,2,c)&&unread.count()==0);
    unread.add(a.id,c);c.messages.clear();unread.prune(c);assert(unread.count()==0);
  }

  {
    auto c=client(1);UnreadMessages unread;auto a=post(2,1),b=post(2,2),other=post(2,3);other.room="camp";
    c.receive(a,0,false);c.receive(b,0,false);c.receive(other,0,false);for(auto id:{a.id,b.id,other.id})unread.add(id,c);
    unread.viewedIds("general",{a.id,other.id},c);assert(unread.count()==2&&unread.count("general",c)==1&&unread.count("camp",c)==1);
    unread.viewedIds("general",{a.id},c);assert(unread.count()==2); // Repeated views never clear later arrivals.
    DeckChat d;d.page=DeckChat::Rooms;auto measure=[](const std::string& text,DeckFont){return int(text.size())*5;};
    auto scene=makeDeckScene(d,c,{},true,"mesit-0001","12345678",measure,0,"4.1V",unread.count(),true,[&](const std::string& room){return unread.count(room,c);});
    assert(std::any_of(scene.paint.begin(),scene.paint.end(),[](const DeckPaint& p){return p.text=="New 2";}));
    assert(std::any_of(scene.paint.begin(),scene.paint.end(),[](const DeckPaint& p){return p.text=="2 / 1 new";}));
    assert(std::any_of(scene.paint.begin(),scene.paint.end(),[](const DeckPaint& p){return p.text=="1 / 1 new";}));
    const auto* badge=deckHitAt(scene,125,15);assert(badge&&badge->action==DeckTap::Rooms);
    Event outgoing;outgoing.room="general";outgoing.text="Outgoing";assert(c.submit(outgoing,0));assert(outgoingCount(c)==1);
    const auto sent=c.pending.back().event;c.tick(10000,[](const Event&,bool){return true;});assert(outgoingCount(c)==1);
    c.receive(sent,10001,true);assert(outgoingCount(c)==0); // Acknowledgment updates immediately, before pending cleanup.
    c.pending.push_back({other,0,0,false,true});assert(outgoingCount(c)==0); // Recovery traffic is not outgoing user activity.
    c.pending.clear();assert(c.submit(outgoing,10002));c.pending.back().attempts=4;c.pending.back().due=0;c.tick(20000,[](const Event&,bool){return true;});assert(outgoingCount(c)==0);
  }

  {
    auto radio=client(300);auto root=post(301,1),replyPost=post(302,1,root.id);
    radio.receive(root,0,false);radio.receive(replyPost,0,false);
    DeckChat deck;deck.sync(radio);auto rows=deck.rows(radio);assert(rows.size()==2&&rows[0].id==root.id&&rows[1].depth==1);
    const std::vector<DeckPreset> catalog={{"mt-us-longfast","meshtastic","US LongFast"},{"mc-eu-narrow","meshcore","EU narrow"}};
    auto key=[&](int k){return deck.key(k,radio,catalog,300);};
    key(DeckMenu);assert(deck.menu.front()=="Back"&&deck.cursor==0);key('\r');assert(deck.page==DeckChat::Chat&&!deck.reply);
    key(DeckMenu);key(DeckDown);key('\r');assert(deck.reply==root.id&&deck.page==DeckChat::Chat);
    for(char c:std::string("Keyboard reply"))key(c);
    auto action=key('\r');assert(action.op=="post"&&action.target==root.id&&action.text=="Keyboard reply");
    // An unsuccessful queue attempt leaves the draft and reply target intact.
    assert(deck.draft=="Keyboard reply"&&deck.reply==root.id);
    Event outgoing;outgoing.room=action.room;outgoing.ref=action.target;outgoing.text=action.text;assert(radio.submit(outgoing,100));
    deck.accepted(action,radio.pending.back().event.id);assert(deck.draft.empty()&&!deck.reply&&radio.find(deck.selected)->mine);
    key(DeckMenu);deck.cursor=2;action=key('\r');assert(action.op=="vote"&&action.up&&action.target==deck.selected);
    radio.receive(vote(300,2,action.target),200,false);key(DeckMenu);deck.cursor=2;action=key('\r');assert(!action.up);
    key(DeckMenu);deck.cursor=3;action=key('\r');assert(action.op=="resend"&&action.target==deck.selected);
    key(DeckMenu);deck.cursor=5;key('\r');for(char c:std::string("camp"))key(c);key('\r');assert(deck.room=="camp"&&!deck.reply);
    key(DeckMenu);deck.cursor=8;key('\r');key(DeckDown);key('\r');assert(deck.page==DeckChat::Regions);key('\r');assert(deck.page==DeckChat::Presets);action=key('\r');assert(action.op=="settings"&&action.preset=="mc-eu-narrow"&&action.network=="meshcore");
    deck.accepted(action);assert(deck.page==DeckChat::Chat);
    for(unsigned i=0;i<170;++i)key('x');assert(deck.draft.size()==160);key(8);assert(deck.draft.size()==159);
    deck.draft="Hi 🙂";key(8);assert(deck.draft=="Hi ");
    key(DeckMenu);deck.cursor=10;key('\r');assert(deck.page==DeckChat::Credentials);key('\r');assert(deck.page==DeckChat::Chat);
    deck.room="general";deck.sync(radio);key(DeckMenu);deck.cursor=11;key('\r');assert(deck.page==DeckChat::Read);key(DeckDown);assert(deck.cursor==1);key('\r');assert(deck.page==DeckChat::Chat);
  }
  {
    assert((deckTouchPoint(0,0,true)==std::make_pair(319,0)));
    assert((deckTouchPoint(239,319,true)==std::make_pair(0,239)));
    assert((deckTouchPoint(0,319,false)==std::make_pair(319,239)));
    DeckGesture pointer;pointer.contact(100,150);pointer.contact(102,151);auto e=pointer.release();assert(e.tap&&e.x==102);
    assert(!pointer.release().tap); // Held fingers and repeated release frames never post twice.
    pointer.contact(100,180);pointer.contact(100,110);e=pointer.release();assert(!e.tap&&e.swipe==1);
    pointer.contact(100,100);pointer.contact(150,100);pointer.contact(100,100);assert(!pointer.release().tap); // Dragging away and back is not a tap.
    pointer.contact(220,120);auto horizontal=pointer.contact(160,123);assert(horizontal.dragY==0);e=pointer.release();assert(e.swipeX==-1&&!e.tap);
    pointer.contact(60,120);pointer.contact(130,122);e=pointer.release();assert(e.swipeX==1&&!e.tap);
    pointer.contact(100,180);pointer.contact(102,140);pointer.contact(180,120);e=pointer.release();assert(!e.swipeX&&!e.tap); // Vertical drags never switch tabs.
    pointer.contact(100,120);pointer.contact(145,121);e=pointer.release();assert(!e.tap&&!e.swipeX); // Short horizontal drift cannot open Rooms.
    pointer.contact(100,120);pointer.contact(170,160);e=pointer.release();assert(!e.tap&&!e.swipeX); // Diagonal scrolling cannot navigate sideways.
    DeckChat navigation;navigation.key(DeckLeft,Core(),{},1);assert(navigation.page==DeckChat::Chat);navigation.key(DeckRight,Core(),{},1);assert(navigation.page==DeckChat::Chat);
    navigation.swipe(-1);assert(navigation.page==DeckChat::Settings);navigation.swipe(1);assert(navigation.page==DeckChat::Chat);navigation.swipe(1);assert(navigation.page==DeckChat::Rooms);navigation.swipe(-1);assert(navigation.page==DeckChat::Chat);
    pointer.contact(70,150);pointer.contact(140,150);auto canceled=pointer.cancel();assert(canceled.released&&!canceled.tap&&!canceled.swipeX);assert(!pointer.cancel().released);
    pointer.contact(100,180);auto recovered=pointer.contact(101,130);assert(recovered.dragY==-50);pointer.release(); // A missing release cannot leave the next gesture locked horizontally.
    pointer.contact(100,180);assert(pointer.contact(110,170).dragY==0);assert(pointer.contact(112,140).dragY==-40);pointer.release(); // Diagonal touch jitter waits for a clear axis.
    auto c=client(400);auto root=post(401,1),child=post(402,1,root.id);c.receive(root,0,false);c.receive(child,0,false);
    DeckChat d;d.sync(c);const std::vector<DeckPreset> catalog={{"mt-us-longfast","meshtastic","US LongFast"}};
    auto measure=[](const std::string& text,DeckFont f){return int(text.size())*(f==DeckFont::Small?5:8);};
    auto scene=makeDeckScene(d,c,catalog,true,"mesit-00af","12345678",measure);
    assert(scene.width==320&&scene.height==240);
    auto find=[&](DeckTap action,Id id=0){for(const auto& hit:scene.hits)if(hit.action==action&&(!id||hit.target==id))return hit;assert(false);return DeckHit{};};
    for(const auto& hit:scene.hits)if(hit.action==DeckTap::Select){
      const auto* bodyTap=deckHitAt(scene,hit.box.x+45,hit.box.y+3);assert(bodyTap&&bodyTap->action==DeckTap::Select&&bodyTap->target==hit.target);
      d.selected=0;activateDeckHit(*bodyTap,d,c,catalog,400);assert(d.page==DeckChat::Menu&&d.selected==hit.target&&d.cursor==0&&d.menu.front()=="Back");
      d.key('\r',c,catalog,400);assert(d.page==DeckChat::Chat);
      activateDeckHit(*bodyTap,d,c,catalog,400);assert(d.page==DeckChat::Menu); // Repeated taps also open actions, never Rooms or the full-text view.
      activateDeckHit({DeckTap::MenuItem,{},0,1},d,c,catalog,400);assert(d.page==DeckChat::Chat&&d.reply==hit.target);d.reply=0;
    }
    auto replyHit=find(DeckTap::Reply,root.id);assert(replyHit.box.y>=100&&replyHit.box.y+replyHit.box.h<=240);
    const auto* picked=deckHitAt(scene,replyHit.box.x+2,replyHit.box.y+2);assert(picked&&picked->action==DeckTap::Reply);
    activateDeckHit(*picked,d,c,catalog,400);assert(d.reply==root.id);
    scene=makeDeckScene(d,c,catalog,true,"mesit-00af","12345678",measure);
    assert(std::any_of(scene.paint.begin(),scene.paint.end(),[](const DeckPaint& op){return op.text=="Post a reply";}));
    auto cancel=find(DeckTap::Cancel);assert(cancel.box.w>=100&&cancel.box.h>=32);
    const auto* cancelEdge=deckHitAt(scene,cancel.box.x+1,cancel.box.y+cancel.box.h-1);assert(cancelEdge&&cancelEdge->action==DeckTap::Cancel);
    d.draft="Keep this text";activateDeckHit(*cancelEdge,d,c,catalog,400);assert(!d.reply&&d.draft=="Keep this text");
    d.reply=root.id;
    d.draft="Touch reply";auto action=activateDeckHit(find(DeckTap::Post),d,c,catalog,400);assert(action.op=="post"&&action.target==root.id&&action.text=="Touch reply");
    action=activateDeckHit(find(DeckTap::Vote,root.id),d,c,catalog,400);assert(action.op=="vote"&&action.up);
    activateDeckHit(find(DeckTap::Settings),d,c,catalog,400);assert(d.page==DeckChat::Settings);
    activateDeckHit({DeckTap::MenuItem,{},0,3},d,c,catalog,400);assert(d.page==DeckChat::Appearance);activateDeckHit({DeckTap::MenuItem,{},0,2},d,c,catalog,400);assert(d.dark);
    scene=makeDeckScene(d,c,catalog,true,"mesit-00af","12345678",measure);assert(scene.paint.front().color==deckPalette(true).paper);
    const std::vector<DeckPreset> regional={{"mt-us","meshtastic","US LongFast","US","915 MHz"},{"mt-eu","meshtastic","EU LongFast","EU_868","869 MHz"},{"mc-us","meshcore","US Narrow","US","915 MHz"}};
    d.page=DeckChat::Settings;d.cursor=1;d.key('\r',c,regional,400);assert(d.page==DeckChat::Network);
    d.cursor=0;d.key('\r',c,regional,400);assert(d.page==DeckChat::Regions&&d.regions(regional).size()==2);
    d.cursor=1;d.key('\r',c,regional,400);assert(d.page==DeckChat::Presets&&d.filtered(regional).size()==1);
    auto preset=d.key('\r',c,regional,400);assert(preset.network=="meshtastic"&&preset.preset=="mt-eu");
    d.back();assert(d.page==DeckChat::Regions);d.back();assert(d.page==DeckChat::Network);d.back();assert(d.page==DeckChat::Settings);
    d.page=DeckChat::Appearance;d.cursor=1;auto appearance=d.key('\r',c,regional,400);assert(!d.dark&&d.appearance==1);
    JsonDocument appearanceRequest;deckRequest(appearanceRequest,appearance);assert(appearanceRequest["op"]=="deckAppearance"&&appearanceRequest["appearance"]==1);
    d.page=DeckChat::Appearance;d.cursor=0;d.systemDark=true;d.key('\r',c,regional,400);assert(d.dark&&d.appearance==0);
    c.messages.clear();action=activateDeckHit(replyHit,d,c,catalog,400);assert(action.op.empty()&&!d.error.empty()); // Stale frames cannot target evicted posts.
  }
  {
    // Drag distance controls pixels directly, with the content following the finger.
    DeckGesture gesture;DeckScroll scroll;scroll.move(100,100);
    gesture.contact(100,190);auto move=gesture.contact(100,150);assert(move.dragY==-40&&!move.tap);
    scroll.drag(move.dragY,116);assert(scroll.position()==140&&!scroll.followSelection);
    auto release=gesture.release();assert(release.released&&!release.tap);scroll.release(116);
    assert(scroll.tick(132,1000)>140); // Momentum continues after release.
    const int before=scroll.position();scroll.stop(150);scroll.drag(30,166);assert(scroll.position()==before-30); // Finger down moves content down.
    scroll.move(-10000,200);assert(scroll.tick(200,1000)==0);
    scroll.move(10000,200);assert(scroll.tick(200,1000)==1000);
    auto c=client(450);for(int n=1;n<=12;++n)c.receive(post(451,n),n,false);
    DeckChat d;d.sync(c);const Id selected=d.selected;
    auto measure=[](const std::string& text,DeckFont f){return int(text.size())*(f==DeckFont::Small?5:8);};
    DeckScene touchScene;touchScene.viewport={0,130,320,110};DeckPointerEvent touchMove;touchMove.originY=75;touchMove.x=150;touchMove.dragY=40;
    assert(deckTouchScrollDelta(d,touchScene,touchMove)==-40); // Chat scrolling also works when a swipe begins above the short feed viewport.
    DeckScroll reverseScroll;reverseScroll.move(100,100);reverseScroll.drag(deckTouchScrollDelta(d,touchScene,touchMove),116);assert(reverseScroll.position()==140);
    touchMove.dragY=-40;reverseScroll.drag(deckTouchScrollDelta(d,touchScene,touchMove),132);assert(reverseScroll.position()==100);
    touchMove.originY=10;assert(!deckTouchScrollDelta(d,touchScene,touchMove));
    d.page=DeckChat::Settings;touchMove.originY=150;assert(deckTouchScrollDelta(d,touchScene,touchMove)==-40);d.page=DeckChat::Chat;
    auto frame=makeDeckScene(d,c,{},true,"mesit-00af","12345678",measure,120,"",0,false);
    assert(frame.scroll==120&&d.selected==selected); // Touch scrolling must not snap to the selected message.
    for(const auto& hit:frame.hits)assert(hit.box.h>=0&&hit.box.y>=0&&hit.box.y+hit.box.h<=240);
    auto tap=[&](DeckTap kind){for(const auto& hit:frame.hits)if(hit.action==kind)return activateDeckHit(hit,d,c,{},450);assert(false);return DeckAction{};};
    d.focused=false;tap(DeckTap::Focus);assert(d.focused);
    d.page=DeckChat::Read;frame=makeDeckScene(d,c,{},true,"mesit-00af","12345678",measure);
    tap(DeckTap::Back);assert(d.page==DeckChat::Chat);
    d.page=DeckChat::Read;frame=makeDeckScene(d,c,{},true,"mesit-00af","12345678",measure);tap(DeckTap::Reply);assert(d.page==DeckChat::Chat&&d.reply==selected);
    d.draft="Draft kept while editing settings";d.savedNickname="Old nickname";
    d.page=DeckChat::Settings;d.cursor=2;d.key('\r',c,{},450);assert(d.page==DeckChat::Nickname);
    d.draft="New nickname";frame=makeDeckScene(d,c,{},true,"mesit-00af","12345678",measure);
    auto action=tap(DeckTap::Save);assert(action.op=="nickname"&&action.text=="New nickname");d.accepted(action);
    assert(d.page==DeckChat::Settings&&d.draft=="Draft kept while editing settings"&&d.reply==selected);
    d.back();assert(d.page==DeckChat::Chat);d.page=DeckChat::Settings;
    d.cursor=4;d.savedSsid="Old wifi";d.key('\r',c,{},450);assert(d.page==DeckChat::Wifi);
    d.key('\r',c,{},450);assert(d.page==DeckChat::Ssid);d.draft="New wifi";d.key('\r',c,{},450);assert(d.page==DeckChat::Wifi&&d.wifiSsid=="New wifi");
    d.cursor=1;d.key('\r',c,{},450);d.draft="secret123";frame=makeDeckScene(d,c,{},true,"mesit-00af","12345678",measure);
    assert(std::none_of(frame.paint.begin(),frame.paint.end(),[](const DeckPaint& op){return op.text.find("secret123")!=std::string::npos;}));
    tap(DeckTap::Back);assert(d.page==DeckChat::Wifi&&d.wifiPassword.empty());
    d.cursor=2;action=d.key('\r',c,{},450);JsonDocument request;deckRequest(request,action);
    assert(request["op"]=="settings"&&request["ssid"]=="New wifi"&&request["password"].isNull());
    d.page=DeckChat::Chat;d.draft="A real reply";frame=makeDeckScene(d,c,{},true,"mesit-00af","12345678",measure);
    action=tap(DeckTap::Post);deckRequest(request,action);
    assert(request["parent"]==hex(selected)&&action.target==selected);
    Event e;e.room=request["room"].as<std::string>();e.text=request["text"].as<std::string>();e.ref=fromHex(request["parent"]|"");assert(c.submit(e,100));
    assert(c.find(c.pending.back().event.id)->event.ref==selected); // Existing threading behavior stays intact.
    d.edit(DeckChat::NewRoom,"unused",DeckChat::Rooms);frame=makeDeckScene(d,c,{},true,"mesit-00af","12345678",measure);
    tap(DeckTap::Back);assert(d.page==DeckChat::Rooms&&d.draft=="A real reply");
  }
  assert(!plausibleBatteryVoltage(0));assert(!plausibleBatteryVoltage(5000));
  assert(batteryBars(3400)==0&&batteryBars(3600)==1&&batteryBars(3800)==2&&batteryBars(4000)==3&&batteryBars(4200)==4);

  assert(setupPassword(false,"",0)=="12345678");
  assert(setupPassword(false,"48214821",99)=="48214821");
  assert(setupPassword(true,"12345678",99)=="12345678");
  assert(setupRadioName(0x000a)=="mesit-000a");
  assert(setupRadioName(0xabcd)=="mesit-abcd");
  assert(setupRadioName(0xffff)=="mesit-ffff");
  assert(setupPassword(true,"48214821",99)=="48214821");
  for(const auto& saved:{"","abcdefghijkl","1234123x","123412341234","１２３４１２３４"}){
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
  // A live reply immediately requests its missing parent, without an upvote.
  auto thread=client(31);auto threadRoot=post(90,1),threadReply=post(91,1,threadRoot.id);
  thread.receive(threadReply,0,false);std::vector<Id> requested;
  thread.tick(1000,[&](const Event& out,bool){assert(out.kind==Kind::Request);requested.push_back(out.ref);return true;});
  thread.receive(vote(92,1,threadReply.id),2000,false);
  thread.tick(3000,[](const Event&,bool){assert(false);return true;});
  assert(requested==std::vector<Id>{threadRoot.id});
  thread.receive(vote(93,1,threadReply.id),3500,false);thread.tick(4000,[](const Event&,bool){assert(false);return true;}); // Deduplicated.
  threadRoot.kind=Kind::History;thread.receive(threadRoot,4500,false);assert(thread.find(threadRoot.id)&&thread.score(threadReply.id)==2);
  thread.tick(5000,[](const Event&,bool){assert(false);return true;});
  // Unrelated traffic cannot restart exhausted parent-recovery retries.
  auto unavailable=client(34);auto orphan=post(94,1,id(95,1));unavailable.receive(orphan,0,false);
  int historyRequests=0;
  for(uint32_t t=1000;t<=100000;t+=1000)unavailable.tick(t,[&](const Event& out,bool){assert(out.kind==Kind::Request&&out.ref==id(95,1));++historyRequests;return true;});
  assert(historyRequests==3);unavailable.receive(post(97,1),101000,false);
  unavailable.tick(102000,[](const Event&,bool){assert(false);return true;});
  // Reproduce "Maybe not" arriving before "Maybe" and its master post.
  auto replyOnly=client(35);auto master=post(101,1),maybe=post(102,1,master.id),maybeNot=post(103,1,maybe.id);
  replyOnly.receive(maybeNot,0,false);std::vector<Id> ancestry;
  auto requestAncestor=[&](const Event& out,bool){assert(out.kind==Kind::Request);ancestry.push_back(out.ref);return true;};
  replyOnly.tick(1000,requestAncestor);assert(ancestry==std::vector<Id>{maybe.id});
  maybe.kind=Kind::History;replyOnly.receive(maybe,2000,false);replyOnly.tick(3000,requestAncestor);
  assert((ancestry==std::vector<Id>{maybe.id,master.id}));
  master.kind=Kind::History;replyOnly.receive(master,4000,false);
  replyOnly.tick(5000,[](const Event&,bool){assert(false);return true;});
  assert(replyOnly.find(maybeNot.id)->event.ref==maybe.id&&replyOnly.find(maybe.id)->event.ref==master.id);
  assert(replyOnly.messages.size()==3&&replyOnly.score(maybeNot.id)==0);
  // A complete live chain does not generate redundant history requests.
  auto complete=client(36);master.kind=Kind::Post;maybe.kind=Kind::Post;
  complete.receive(master,0,false);complete.receive(maybe,0,false);complete.receive(maybeNot,0,false);
  complete.tick(1000,[](const Event&,bool){assert(false);return true;});
  // If the reply is missing too, recover it, its parent, and the original root.
  auto chain=client(32);auto chainRoot=post(80,1),chainParent=post(81,1,chainRoot.id),chainReply=post(82,1,chainParent.id);
  chain.receive(vote(83,1,chainReply.id),0,false);requested.clear();
  auto collect=[&](const Event& out,bool){assert(out.kind==Kind::Request);requested.push_back(out.ref);return true;};
  chain.tick(1000,collect);assert(requested.back()==chainReply.id);
  chainReply.kind=Kind::History;chain.receive(chainReply,2000,false);chain.tick(3000,collect);assert(requested.back()==chainParent.id);
  chainParent.kind=Kind::History;chain.receive(chainParent,4000,false);chain.tick(5000,collect);assert(requested.back()==chainRoot.id);
  chainRoot.kind=Kind::History;chain.receive(chainRoot,6000,false);chain.tick(7000,[](const Event&,bool){assert(false);return true;});
  assert(requested.size()==3&&chain.messages.size()==3&&chain.score(chainReply.id)==1);
  // Once a reply arrives, its thread is recovered even if the upvote was withdrawn.
  auto withdrawn=client(33);withdrawn.receive(vote(84,1,chainReply.id),0,false);withdrawn.receive(vote(84,2,chainReply.id,false),100,false);
  withdrawn.receive(chainReply,200,false);withdrawn.tick(1000,[&](const Event& out,bool){assert(out.kind==Kind::Request&&out.ref==chainParent.id);return true;});
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
