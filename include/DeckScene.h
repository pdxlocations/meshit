#pragma once
#include "DeckChat.h"
#include "DeckCalibration.h"
#include "RadioStats.h"
#include "BatteryGauge.h"
#include <cmath>
namespace meshit {
struct DeckBox {int x=0,y=0,w=0,h=0;bool contains(int px,int py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}};
enum class DeckShape {Fill,Round,Outline,Line,Text,Poop,Chat,Gear,Dot,Up,Reply,Check,Retry,Pending,Radio,Down,Battery,Score};
enum class DeckFont {Small,Body,Bold};
enum class DeckTap {Chat,Settings,Rooms,NewRoom,Top,New,Post,Cancel,Select,Reply,Vote,Resend,MenuItem,Focus,Save,Back,Connection,CancelEditor,Downvote};
struct DeckPaint {DeckShape shape;DeckBox box;uint16_t color;std::string text;DeckFont font=DeckFont::Small;DeckBox clip;};
struct DeckHit {DeckTap action;DeckBox box;Id target=0;size_t index=0;};
struct DeckScene {int width=320,height=240,scroll=0,maxScroll=0;DeckBox viewport;std::vector<DeckPaint> paint;std::vector<DeckHit> hits;};
struct DeckPalette {uint16_t paper,card,ink,muted,line,header,selected,button,buttonInk;};
inline DeckPalette deckPalette(bool dark){return dark?DeckPalette{0x1082,0x2104,0xef7d,0xad55,0x39e7,0x18c3,0x2945,0xdedb,0x10a2}:DeckPalette{0xf7be,0xffff,0x2104,0x6b6d,0xdefb,0xef7d,0xe73c,0x3186,0xffff};}
using DeckMeasure=std::function<int(const std::string&,DeckFont)>;
inline std::string deckFit(std::string text,int width,DeckFont font,const DeckMeasure& measure){
  if(measure(text,font)<=width)return text;
  while(!text.empty()&&measure(text+"...",font)>width)text.pop_back();return text+"...";
}
inline std::vector<std::string> deckWrap(const std::string& text,int width,DeckFont font,const DeckMeasure& measure){
  std::vector<std::string> lines;std::string rest=text;
  while(!rest.empty()){size_t n=0;while(n<rest.size()&&rest[n]!='\n'&&(n==0||measure(rest.substr(0,n+1),font)<=width))++n;
    if(n<rest.size()&&rest[n]!='\n'){const auto space=rest.rfind(' ',n);if(space!=std::string::npos&&space)n=space;}
    lines.push_back(rest.substr(0,n));rest.erase(0,n);if(!rest.empty()&&rest.front()=='\n')rest.erase(0,1);else while(!rest.empty()&&rest.front()==' ')rest.erase(0,1);
  }if(lines.empty())lines.push_back("");return lines;
}
// A display-independent scene also supplies the exact touch targets used to draw it.
inline DeckScene makeDeckScene(DeckChat& chat,const Core& core,const std::vector<DeckPreset>& presets,
 bool connected,const std::string& name,const std::string& password,const DeckMeasure& measure,int previousScroll=0,uint16_t battery=0,size_t unread=0,bool followSelection=true,const std::function<size_t(const std::string&)>& roomUnread={},uint32_t identity=0,RadioActivity activity=RadioActivity::Standby){
  DeckScene s;const auto p=deckPalette(chat.dark);const DeckBox full{0,0,320,240};
  auto draw=[&](DeckShape shape,DeckBox b,uint16_t color,const std::string& text="",DeckFont f=DeckFont::Small,DeckBox clip=DeckBox{}){s.paint.push_back({shape,b,color,text,f,clip.w?clip:full});};
  auto text=[&](int x,int y,const std::string& value,DeckFont f=DeckFont::Small,uint16_t color=0){draw(DeckShape::Text,{x,y,0,0},color?color:p.ink,value,f);};
  auto hit=[&](DeckTap a,DeckBox b,Id id=0,size_t index=0){s.hits.push_back({a,b,id,index});};
  draw(DeckShape::Fill,full,p.paper);draw(DeckShape::Fill,{0,0,320,29},p.header);
  if(chat.page==DeckChat::Calibrate){
    text(72,72,"Touch calibration",DeckFont::Bold);
    text(60,94,chat.cursor<3?"Tap the crosshair center":"Tap center to verify and save");
    text(92,158,"Step "+std::to_string(chat.cursor+1)+" / 4");
    text(72,180,"Enter / Esc: cancel");
    if(!chat.error.empty())text(25,202,deckFit(chat.error,270,DeckFont::Small,measure));
    const auto point=DeckCalibration::target(chat.cursor);
    draw(DeckShape::Line,{point.first-10,point.second,20,0},p.ink);
    draw(DeckShape::Line,{point.first,point.second-10,0,20},p.ink);
    draw(DeckShape::Outline,{point.first-4,point.second-4,9,9},p.ink);
    return s;
  }
  draw(DeckShape::Poop,{7,5,20,20},p.ink);hit(DeckTap::Chat,{0,0,94,30});text(96,18,"New "+std::to_string(unread),DeckFont::Small,unread?p.ink:p.muted);hit(DeckTap::Rooms,{94,0,80,30});hit(DeckTap::Chat,{174,0,35,30});text(33,19,"meshit",DeckFont::Bold);draw(DeckShape::Battery,{177,8,29,14},plausibleBatteryVoltage(battery)?(batteryBars(battery)?p.ink:0xf800):p.muted,std::to_string(plausibleBatteryVoltage(battery)?int(batteryBars(battery)):-1));
  draw(DeckShape::Radio,{212,5,21,20},connected?(activity==RadioActivity::Standby?0x260b:p.ink):p.muted,connected?radioActivityName(activity):"disconnected");hit(DeckTap::Connection,{209,0,24,30});
  const bool settings=chat.page==DeckChat::Settings||chat.page==DeckChat::Presets||chat.page==DeckChat::Nickname||chat.page==DeckChat::Credentials||chat.page==DeckChat::Wifi||chat.page==DeckChat::Ssid||chat.page==DeckChat::WifiPassword||chat.page==DeckChat::Appearance||chat.page==DeckChat::Network||chat.page==DeckChat::Regions;
  draw(DeckShape::Round,{237,2,34,25},settings?p.header:p.selected);draw(DeckShape::Chat,{245,7,17,15},p.ink);hit(DeckTap::Chat,{233,0,42,30});
  draw(DeckShape::Round,{278,2,34,25},settings?p.selected:p.header);draw(DeckShape::Gear,{287,7,17,17},p.ink);hit(DeckTap::Settings,{277,0,43,30});
  if(chat.page==DeckChat::Chat){
    text(8,46,deckFit("# "+chat.room,220,DeckFont::Bold,measure),DeckFont::Bold);hit(DeckTap::Rooms,{0,30,230,23});
    draw(DeckShape::Round,{231,32,43,21},chat.top?p.card:p.selected);text(236,47,"New");hit(DeckTap::New,{230,30,45,25});
    draw(DeckShape::Round,{277,32,36,21},chat.top?p.selected:p.card);text(280,47,"Top");hit(DeckTap::Top,{275,30,45,25});
    const auto draft=deckWrap(chat.draft,290,DeckFont::Body,measure);const int count=std::min(3,int(draft.size()));
    const int replyHeight=chat.reply?34:0,composerHeight=42+(count-1)*18+replyHeight,composerBottom=55+composerHeight;
    draw(DeckShape::Round,{7,55,306,composerHeight},p.card);draw(DeckShape::Outline,{7,55,306,composerHeight},chat.focused?p.muted:p.line);hit(DeckTap::Focus,{8,56,304,composerHeight-23});
    if(chat.reply){const auto* parent=core.find(chat.reply);text(14,76,deckFit(parent?(parent->event.nick.empty()?"Anonymous":parent->event.nick)+": "+parent->event.text:"Reply",178,DeckFont::Small,measure),DeckFont::Small,p.muted);draw(DeckShape::Round,{199,57,112,30},p.selected);text(201,76,"Cancel reply");hit(DeckTap::Cancel,{197,55,116,34});}
    if(chat.draft.empty())text(14,72+replyHeight,chat.reply?"Post a reply":"Write something interesting...",DeckFont::Small,p.muted);
    else for(int i=0;i<count;++i)text(14,73+replyHeight+i*18,draft[draft.size()-count+i],DeckFont::Body);
    if(chat.focused){const std::string last=chat.draft.empty()?"":draft.back();const int caret=14+measure(last,DeckFont::Body);draw(DeckShape::Line,{std::min(caret,303),60+replyHeight+(count-1)*18,0,14},p.ink);}
    draw(DeckShape::Line,{14,composerBottom-19,290,0},p.line);text(14,composerBottom-5,std::to_string(chat.draft.size())+" / 160",DeckFont::Small,p.muted);
    draw(DeckShape::Round,{264,composerBottom-17,41,14},chat.draft.empty()||!connected||!chat.transmitEnabled?p.selected:p.button);text(266,composerBottom-6,"Post",DeckFont::Small,chat.draft.empty()||!connected||!chat.transmitEnabled?p.muted:p.buttonInk);hit(DeckTap::Post,{255,composerBottom-22,58,22});
    const int feedTop=composerBottom+7,feedBottom=chat.error.empty()?240:220,viewport=std::max(0,feedBottom-feedTop);
    const auto rows=chat.rows(core);std::vector<int> heights;std::vector<std::vector<std::string>> bodies;int total=0,selectedTop=0,selectedHeight=0;
    for(const auto& row:rows){const auto* m=core.find(row.id);const int indent=std::min(row.depth,4u)*14;auto body=deckWrap(m->event.text,252-indent,DeckFont::Body,measure);
      // Reserve a separate footer so Reply does not cover the last line of message text.
      const int height=std::max(56,38+int(body.size())*(chat.largeText?18:16));if(row.id==chat.selected){selectedTop=total;selectedHeight=height;}heights.push_back(height);bodies.push_back(std::move(body));total+=height;
    }
    s.maxScroll=std::max(0,total-viewport);s.viewport={0,feedTop,320,viewport};
    s.scroll=std::max(0,std::min(previousScroll,s.maxScroll));
    if(followSelection){if(selectedTop<s.scroll)s.scroll=selectedTop;else if(selectedTop+selectedHeight>s.scroll+viewport)s.scroll=std::min(selectedTop,selectedTop+selectedHeight-viewport);}
    int y=feedTop-s.scroll;const DeckBox clip{0,feedTop,320,viewport};
    for(size_t i=0;i<rows.size();++i){const auto& row=rows[i];const auto* m=core.find(row.id);const int h=heights[i],indent=std::min(row.depth,4u)*14,x=8+indent,bodyX=x+44;
      if(y+h>feedTop&&y<feedBottom){
        if(row.id==chat.selected)draw(DeckShape::Fill,{x,y,305-indent,h},p.selected,"",DeckFont::Small,clip);
        draw(DeckShape::Line,{x,y,305-indent,0},p.line,"",DeckFont::Small,clip);
        if(row.depth)draw(DeckShape::Line,{x,y+4,0,h-8},p.line,"",DeckFont::Small,clip);
        const uint16_t voteColor=identity&&core.voted(row.id,identity)?p.button:p.muted;
        draw(DeckShape::Up,{x+16,y+7,12,12},voteColor,"",DeckFont::Small,clip);const auto score=std::to_string(core.score(row.id));draw(DeckShape::Score,{x+2,y+22,41,16},p.muted,score,DeckFont::Small,clip);
        draw(DeckShape::Text,{bodyX,y+14,0,0},p.ink,deckFit(m->event.nick.empty()?"Anonymous":m->event.nick,175,DeckFont::Small,measure),DeckFont::Small,clip);
        if(m->mine){draw(DeckShape::Text,{251,y+14,0,0},p.muted,"you",DeckFont::Small,clip);bool pending=false;for(const auto& job:core.pending)if(job.event.id==row.id&&!job.ack&&!job.history)pending=true;
          draw(m->acked?DeckShape::Check:pending?DeckShape::Pending:DeckShape::Retry,{291,y+4,14,14},p.ink,"",DeckFont::Small,clip);
          if(DeckChat::canResend(core,row.id))hit(DeckTap::Resend,{280,std::max(feedTop,y),33,std::max(0,std::min(feedBottom,y+24)-std::max(feedTop,y))},row.id);
        }
        for(size_t j=0;j<bodies[i].size();++j)draw(DeckShape::Text,{bodyX,y+33+int(j)*(chat.largeText?18:16),0,0},p.ink,bodies[i][j],DeckFont::Body,clip);
        draw(DeckShape::Reply,{bodyX,y+h-14,11,10},p.muted,"",DeckFont::Small,clip);draw(DeckShape::Text,{bodyX+15,y+h-5,0,0},p.muted,"Reply",DeckFont::Small,clip);
        const int top=std::max(y,feedTop),bottom=std::min(y+h,feedBottom);
        const int split=std::max(top,std::min(bottom,y+29));
        hit(DeckTap::Vote,{x+1,top,40,split-top},row.id);hit(DeckTap::Downvote,{x+1,split,40,bottom-split},row.id);
        draw(DeckShape::Down,{x+16,y+40,12,12},core.downvoted(row.id,identity)?p.button:p.muted,"",DeckFont::Small,clip);
        const int replyTop=std::max(feedTop,y+h-23);if(replyTop<bottom)hit(DeckTap::Reply,{bodyX,replyTop,95,bottom-replyTop},row.id);
        // The whole visible row is tappable, including the gutter of indented replies.
        hit(DeckTap::Select,{0,top,320,bottom-top},row.id);
      }y+=h;
    }
    if(rows.empty())text(116,feedTop+30,"No messages",DeckFont::Small,p.muted);
  }else {
    std::string title;std::vector<std::string> items;
    if(chat.page==DeckChat::Settings){title="Settings";items=chat.settingsItems();}
    else if(chat.page==DeckChat::Appearance){title="Appearance";items={"System","Light","Dark"};items[chat.appearance]+=" (selected)";}
    else if(chat.page==DeckChat::Network){title="Network";items={"Meshtastic","MeshCore"};}
    else if(chat.page==DeckChat::Regions){title="Region";items=chat.regions(presets);}
    else if(chat.page==DeckChat::Navigation){title="Navigation";items=chat.navigationItems;}
    else if(chat.page==DeckChat::Menu){title="Message";items=chat.menuItems(core,identity);}
    else if(chat.page==DeckChat::Rooms){title="Rooms";items=chat.rooms(core);draw(DeckShape::Round,{225,33,88,26},p.selected);text(232,51,"+ Room");hit(DeckTap::NewRoom,{220,31,100,30});}
    else if(chat.page==DeckChat::Presets){title="Radio preset";for(const auto& preset:chat.filtered(presets))items.push_back(preset.label);}
    else if(chat.page==DeckChat::Credentials){const size_t outgoing=outgoingCount(core);title="Radio connection";items={name,"Password: "+password,"http://192.168.4.1","Bluetooth PIN: "+(password.size()>2?password.substr(2):""),"http://meshit.local","New: "+std::to_string(unread)+"  Out: "+std::to_string(outgoing)};}
    else if(chat.page==DeckChat::Wifi){title="Local Wi-Fi";items={"Network: "+chat.wifiSsid,"Password: "+(chat.wifiPassword.empty()?std::string("keep saved"):std::string(chat.wifiPassword.size(),'*')),"Save settings"};}
    else if(chat.isEditor()){title=chat.page==DeckChat::NewRoom?"New room":chat.page==DeckChat::Nickname?"Nickname":chat.page==DeckChat::Ssid?"Network name":"Wi-Fi password";}

    else {title="# "+chat.room;if(const auto* m=core.find(chat.selected)){items=deckWrap(m->event.text,294,DeckFont::Body,measure);}}
    text(10,50,"Back",DeckFont::Small);hit(DeckTap::Back,{0,30,60,30});text(65,51,deckFit(title,chat.page==DeckChat::Rooms?150:243,DeckFont::Bold,measure),DeckFont::Bold);
    if(chat.isEditor()){
      draw(DeckShape::Round,{8,68,304,56},p.card);draw(DeckShape::Outline,{8,68,304,56},chat.focused?p.muted:p.line);
      std::string value=chat.page==DeckChat::WifiPassword?std::string(chat.draft.size(),'*'):chat.draft;
      auto wrapped=deckWrap(value,286,DeckFont::Body,measure);const size_t first=wrapped.size()>2?wrapped.size()-2:0;
      for(size_t i=first;i<wrapped.size();++i)text(15,88+int(i-first)*18,wrapped[i],DeckFont::Body);
      if(value.empty())text(15,89,"Type here...",DeckFont::Small,p.muted);
      if(chat.focused)draw(DeckShape::Line,{15+measure(wrapped.back(),DeckFont::Body),75+int(wrapped.size()-first-1)*18,0,14},p.ink);
      hit(DeckTap::Focus,{8,68,304,56});
      draw(DeckShape::Round,{8,137,148,40},p.selected);text(62,162,"Cancel");hit(DeckTap::CancelEditor,{8,137,148,40});
      draw(DeckShape::Round,{164,137,148,40},p.button);text(222,162,"Save",DeckFont::Small,p.buttonInk);hit(DeckTap::Save,{164,137,148,40});
    }else{
      if(chat.hasBackRow())items.insert(items.begin(),"Back");
      const int bottom=chat.page==DeckChat::Read?205:chat.page==DeckChat::Presets?200:240;
      s.viewport={0,63,320,bottom-63};s.maxScroll=std::max(0,int(items.size())*28-s.viewport.h);
      s.scroll=std::max(0,std::min(previousScroll,s.maxScroll));
      if(followSelection){const int selected=int(chat.cursor)*28;if(selected<s.scroll)s.scroll=selected;else if(selected+28>s.scroll+s.viewport.h)s.scroll=selected+28-s.viewport.h;s.scroll=std::min(s.scroll,s.maxScroll);}
      if(chat.page==DeckChat::Read){s.scroll=std::max(0,std::min(previousScroll,s.maxScroll));}
      for(size_t i=0;i<items.size();++i){const int y=63+int(i)*28-s.scroll;if(y+26<=63||y>=bottom)continue;
        draw(DeckShape::Round,{7,y,306,26},i==chat.cursor?p.selected:p.card,"",DeckFont::Small,s.viewport);
        draw(DeckShape::Text,{15,y+18,0,0},p.ink,deckFit(items[i],chat.page==DeckChat::Rooms?192:290,DeckFont::Small,measure),DeckFont::Small,s.viewport);
        if(chat.page==DeckChat::Rooms&&i>0){const size_t fresh=roomUnread?roomUnread(items[i]):0;const std::string count=std::to_string(roomMessageCount(core,items[i]))+(fresh?" / "+std::to_string(fresh)+" new":"");draw(DeckShape::Text,{310-measure(count,DeckFont::Small),y+18,0,0},fresh?p.ink:p.muted,count,DeckFont::Small,s.viewport);}
        if(chat.page!=DeckChat::Credentials&&chat.page!=DeckChat::Read)hit(DeckTap::MenuItem,{7,std::max(63,y),306,std::min(bottom,y+27)-std::max(63,y)},0,i);
      }
      if(chat.page==DeckChat::Read&&chat.selected){
        text(7,226,"Back");hit(DeckTap::Back,{0,207,64,33});
        text(70,226,"Reply");hit(DeckTap::Reply,{64,207,64,33},chat.selected);
        text(134,226,"Up");hit(DeckTap::Vote,{128,207,64,33},chat.selected);
        text(198,226,"Down");hit(DeckTap::Downvote,{192,207,64,33},chat.selected);
        const auto* m=core.find(chat.selected);if(m&&DeckChat::canResend(core,chat.selected)){text(260,226,"Resend");hit(DeckTap::Resend,{256,207,64,33},chat.selected);}
      }
    }
    if(chat.page==DeckChat::Presets){const auto list=chat.filtered(presets);if(chat.cursor>0&&chat.cursor<=list.size())text(8,215,deckFit(list[chat.cursor-1].detail,304,DeckFont::Small,measure),DeckFont::Small,p.muted);text(8,234,"Changing preset clears history",DeckFont::Small,p.muted);}
  }
  if(!chat.error.empty()){draw(DeckShape::Fill,{0,220,320,20},p.header);text(8,234,deckFit(chat.error,304,DeckFont::Small,measure));}
  return s;
}
inline const DeckHit* deckHitAt(const DeckScene& scene,int x,int y){for(const auto& hit:scene.hits)if(hit.action!=DeckTap::Focus&&hit.action!=DeckTap::Select&&hit.box.contains(x,y))return &hit;for(const auto& hit:scene.hits)if(hit.box.contains(x,y))return &hit;return nullptr;}
inline DeckAction activateDeckHit(const DeckHit& hit,DeckChat& chat,const Core& core,const std::vector<DeckPreset>& catalog,uint32_t identity){
  using T=DeckTap;chat.error.clear();DeckAction action;
  if((hit.action==T::Select||hit.action==T::Reply||hit.action==T::Vote||hit.action==T::Downvote||hit.action==T::Resend)&&!core.find(hit.target)){chat.error="Message no longer stored";return action;}
  if(chat.isEditor()&&(hit.action==T::Chat||hit.action==T::Settings||hit.action==T::Rooms||hit.action==T::Connection||hit.action==T::NewRoom))chat.leaveEditor();
  if(!chat.transmitEnabled&&(hit.action==T::Vote||hit.action==T::Downvote||hit.action==T::Resend)){chat.error="Transmit is disabled";return action;}
  switch(hit.action){
    case T::Chat:chat.page=DeckChat::Chat;break;
    case T::Settings:chat.page=DeckChat::Settings;chat.cursor=0;break;
    case T::Rooms:chat.page=DeckChat::Rooms;chat.cursor=0;break;
    case T::NewRoom:chat.edit(DeckChat::NewRoom,"",DeckChat::Rooms);break;
    case T::Top:chat.top=true;break;case T::New:chat.top=false;break;
    case T::Post:if(!chat.draft.empty())action=chat.key('\r',core,catalog,identity);break;
    case T::Cancel:chat.cancelReply();break;
    case T::Select:chat.selected=hit.target;chat.page=DeckChat::Menu;chat.cursor=0;break;
    case T::Reply:chat.selected=chat.reply=hit.target;chat.page=DeckChat::Chat;break;
    case T::Vote:action.op="vote";action.target=hit.target;action.up=!core.voted(hit.target,identity);break;
    case T::Downvote:action.op="vote";action.target=hit.target;action.up=false;action.down=!core.downvoted(hit.target,identity);break;
    case T::Resend:if(!DeckChat::canResend(core,hit.target)){chat.error="Cannot resend this message";break;}action.op="resend";action.target=hit.target;break;
    case T::Focus:chat.focused=true;break;
    case T::Save:action=chat.key('\r',core,catalog,identity);break;
    case T::CancelEditor:if(chat.isEditor())chat.back();break;
    case T::Back:chat.back();break;
    case T::Connection:chat.page=DeckChat::Credentials;chat.cursor=0;break;
    case T::MenuItem:if((chat.page==DeckChat::NewRoom||chat.page==DeckChat::Nickname)&&hit.index==0)break;chat.cursor=hit.index;action=chat.key('\r',core,catalog,identity);break;
  }return action;
}
class DeckTapLatch {
  DeckHit target{};bool valid=false;DeckChat::Page page=DeckChat::Chat;std::string room;
public:
  void begin(const DeckScene& scene,const DeckChat& chat,int x,int y){page=chat.page;room=chat.room;const auto* hit=deckHitAt(scene,x,y);valid=hit;if(hit)target=*hit;}
  const DeckHit* finish(const DeckChat& chat,bool tap){const bool accept=valid&&tap&&chat.page==page&&chat.room==room;valid=false;return accept?&target:nullptr;}
};
struct DeckPointerEvent {bool tap=false;int x=0,y=0,swipe=0,dragY=0;bool released=false,pressed=false;int originY=0,swipeX=0;};
inline int deckTouchScrollDelta(const DeckChat& chat,const DeckScene& scene,const DeckPointerEvent& event){
  // Touch coordinates already follow the display after calibration.
  if(chat.page==DeckChat::Chat)return event.originY>=30?event.dragY:0;
  return scene.viewport.contains(event.x,event.originY)?event.dragY:0;
}
class DeckGesture {
  bool active=false,dragged=false;int axis=0;int startX=0,startY=0,lastX=0,lastY=0,travel=0;
public:
  DeckPointerEvent contact(int x,int y){const bool fresh=!active;if(fresh){active=true;dragged=false;axis=0;startX=x;startY=y;travel=0;}const int previousY=lastY;lastX=x;lastY=y;travel=std::max(travel,std::max(std::abs(x-startX),std::abs(y-startY)));
    int delta=0;const bool alreadyDragged=dragged;
    if(!axis&&travel>=18){const int dx=std::abs(x-startX),dy=std::abs(y-startY);if(dy>=dx+6)axis=2;else if(dx>=dy+6)axis=1;if(axis)dragged=true;}
    if(axis==2)delta=alreadyDragged?y-previousY:y-startY;
    return {false,x,y,0,delta,false,fresh,startY};
  }
  DeckPointerEvent cancel(){if(!active)return {};active=false;axis=0;dragged=false;DeckPointerEvent event;event.released=true;return event;}
  DeckPointerEvent release(){if(!active)return {};active=false;const int delta=lastY-startY;return {!dragged&&travel<18,startX,startY,std::abs(delta)>18?(delta>0?-1:1):0,0,true,false,startY,axis==1&&std::abs(lastX-startX)>=60&&std::abs(lastX-startX)>2*std::abs(lastY-startY)?(lastX>startX?1:-1):0};}
};
// Scroll content with the finger; preserve pixel distance instead of advancing selection.
class DeckScroll {
  float offset=0,velocity=0;uint32_t last=0,sample=0;bool dragging=false;
public:
  bool followSelection=true;
  int position()const{return int(std::round(offset));}
  void stop(uint32_t now){velocity=0;dragging=false;last=sample=now;}
  void reset(uint32_t now){offset=0;stop(now);followSelection=true;}
  void drag(int dy,uint32_t now){const uint32_t dt=std::max(8u,std::min(80u,uint32_t(now-sample)));offset-=dy;velocity=std::max(-2.5f,std::min(2.5f,-float(dy)/dt));sample=last=now;dragging=true;followSelection=false;}
  void release(uint32_t now){dragging=false;last=now;if(uint32_t(now-sample)>100)velocity=0;}
  void move(int pixels,uint32_t now){offset+=pixels;stop(now);followSelection=false;}
  void select(uint32_t now){stop(now);followSelection=true;}
  int tick(uint32_t now,int maximum){if(!dragging){const auto dt=std::min(50u,uint32_t(now-last));offset+=velocity*dt;velocity*=std::pow(0.90f,float(dt)/16);if(std::abs(velocity)<0.02f)velocity=0;}last=now;
    if(offset<0){offset=0;velocity=0;}if(offset>maximum){offset=maximum;velocity=0;}return position();}
  void setPosition(int value){offset=value;}
};
// GT911 native axes are portrait. Rotate with the LCD, including the Plus flip.
inline std::pair<int,int> deckTouchPoint(int rawX,int rawY,bool plus){return plus?std::make_pair(319-rawY,rawX):std::make_pair(rawY,239-rawX);}
}
