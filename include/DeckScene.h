#pragma once
#include "DeckChat.h"
namespace meshit {
struct DeckBox {int x=0,y=0,w=0,h=0;bool contains(int px,int py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}};
enum class DeckShape {Fill,Round,Outline,Line,Text,Poop,Chat,Gear,Dot,Up,Reply,Check,Retry,Pending};
enum class DeckFont {Small,Body,Bold};
enum class DeckTap {Chat,Settings,Rooms,NewRoom,Top,New,Post,Cancel,Select,Reply,Vote,Resend,MenuItem};
struct DeckPaint {DeckShape shape;DeckBox box;uint16_t color;std::string text;DeckFont font=DeckFont::Small;DeckBox clip;};
struct DeckHit {DeckTap action;DeckBox box;Id target=0;size_t index=0;};
struct DeckScene {int width=320,height=240,scroll=0;std::vector<DeckPaint> paint;std::vector<DeckHit> hits;};
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
 bool connected,const std::string& name,const std::string& password,const DeckMeasure& measure,int previousScroll=0,const std::string& battery="",size_t unread=0){
  DeckScene s;const auto p=deckPalette(chat.dark);const DeckBox full{0,0,320,240};
  auto draw=[&](DeckShape shape,DeckBox b,uint16_t color,const std::string& text="",DeckFont f=DeckFont::Small,DeckBox clip=DeckBox{}){s.paint.push_back({shape,b,color,text,f,clip.w?clip:full});};
  auto text=[&](int x,int y,const std::string& value,DeckFont f=DeckFont::Small,uint16_t color=0){draw(DeckShape::Text,{x,y,0,0},color?color:p.ink,value,f);};
  auto hit=[&](DeckTap a,DeckBox b,Id id=0,size_t index=0){s.hits.push_back({a,b,id,index});};
  draw(DeckShape::Fill,full,p.paper);draw(DeckShape::Fill,{0,0,320,29},p.header);
  draw(DeckShape::Poop,{7,5,20,20},p.ink);hit(DeckTap::Chat,{0,0,205,30});text(33,19,"meshit",DeckFont::Bold);if(!battery.empty())text(165,18,battery,DeckFont::Small,p.muted);
  draw(DeckShape::Dot,{215,11,8,8},connected?0x2d2b:p.muted,connected?"on":"off");
  const bool settings=chat.page==DeckChat::Settings||chat.page==DeckChat::Presets||chat.page==DeckChat::Nickname||chat.page==DeckChat::Credentials;
  draw(DeckShape::Round,{237,2,34,25},settings?p.header:p.selected);draw(DeckShape::Chat,{245,7,17,15},p.ink);hit(DeckTap::Chat,{233,0,42,30});
  draw(DeckShape::Round,{278,2,34,25},settings?p.selected:p.header);draw(DeckShape::Gear,{287,7,17,17},p.ink);hit(DeckTap::Settings,{277,0,43,30});
  if(chat.page==DeckChat::Chat){
    text(8,46,"# "+chat.room,DeckFont::Bold);hit(DeckTap::Rooms,{0,30,200,23});
    draw(DeckShape::Round,{235,33,38,18},chat.top?p.card:p.selected);text(242,46,"New");hit(DeckTap::New,{232,30,42,23});
    draw(DeckShape::Round,{276,33,36,18},chat.top?p.selected:p.card);text(284,46,"Top");hit(DeckTap::Top,{275,30,45,23});
    const auto draft=deckWrap(chat.draft,290,DeckFont::Body,measure);const int count=std::min(3,int(draft.size()));
    const int replyHeight=chat.reply?14:0,composerHeight=42+(count-1)*18+replyHeight,composerBottom=55+composerHeight;
    draw(DeckShape::Round,{7,55,306,composerHeight},p.card);draw(DeckShape::Outline,{7,55,306,composerHeight},p.line);
    if(chat.reply){const auto* parent=core.find(chat.reply);text(14,67,deckFit(parent?(parent->event.nick.empty()?"Anonymous":parent->event.nick)+": "+parent->event.text:"Reply",250,DeckFont::Small,measure),DeckFont::Small,p.muted);text(294,67,"x");hit(DeckTap::Cancel,{280,55,33,18});}
    if(chat.draft.empty())text(14,72+replyHeight,chat.reply?"Post a reply":"Write something interesting...",DeckFont::Small,p.muted);
    else for(int i=0;i<count;++i)text(14,73+replyHeight+i*18,draft[draft.size()-count+i],DeckFont::Body);
    draw(DeckShape::Line,{14,composerBottom-19,290,0},p.line);text(14,composerBottom-5,std::to_string(chat.draft.size())+" / 160",DeckFont::Small,p.muted);
    draw(DeckShape::Round,{264,composerBottom-17,41,14},chat.draft.empty()||!connected?p.selected:p.button);text(273,composerBottom-6,"Post",DeckFont::Small,chat.draft.empty()||!connected?p.muted:p.buttonInk);hit(DeckTap::Post,{255,composerBottom-22,58,22});
    const int feedTop=composerBottom+7,feedBottom=chat.error.empty()?240:220,viewport=std::max(0,feedBottom-feedTop);
    const auto rows=chat.rows(core);std::vector<int> heights;std::vector<std::vector<std::string>> bodies;int total=0,selectedTop=0,selectedHeight=0;
    for(const auto& row:rows){const auto* m=core.find(row.id);const int indent=std::min(row.depth,4u)*14;auto body=deckWrap(m->event.text,264-indent,DeckFont::Body,measure);
      if(body.size()>3){body.resize(3);body.back()=deckFit(body.back()+"...",264-indent,DeckFont::Body,measure);}
      const int height=34+int(body.size())*18;if(row.id==chat.selected){selectedTop=total;selectedHeight=height;}heights.push_back(height);bodies.push_back(std::move(body));total+=height;
    }
    s.scroll=std::max(0,std::min(previousScroll,std::max(0,total-viewport)));
    if(selectedTop<s.scroll)s.scroll=selectedTop;else if(selectedTop+selectedHeight>s.scroll+viewport)s.scroll=std::min(selectedTop,selectedTop+selectedHeight-viewport);
    int y=feedTop-s.scroll;const DeckBox clip{0,feedTop,320,viewport};
    for(size_t i=0;i<rows.size();++i){const auto& row=rows[i];const auto* m=core.find(row.id);const int h=heights[i],indent=std::min(row.depth,4u)*14,x=8+indent,bodyX=x+32;
      if(y+h>feedTop&&y<feedBottom){
        if(row.id==chat.selected)draw(DeckShape::Fill,{x,y,305-indent,h},p.selected,"",DeckFont::Small,clip);
        draw(DeckShape::Line,{x,y,305-indent,0},p.line,"",DeckFont::Small,clip);
        if(row.depth)draw(DeckShape::Line,{x,y+4,0,h-8},p.line,"",DeckFont::Small,clip);
        draw(DeckShape::Up,{x+7,y+9,12,12},p.muted,"",DeckFont::Small,clip);draw(DeckShape::Text,{x+9,y+34,0,0},p.muted,std::to_string(core.score(row.id)),DeckFont::Small,clip);
        draw(DeckShape::Text,{bodyX,y+14,0,0},p.ink,deckFit(m->event.nick.empty()?"Anonymous":m->event.nick,175,DeckFont::Small,measure),DeckFont::Small,clip);
        if(m->mine){draw(DeckShape::Text,{251,y+14,0,0},p.muted,"you",DeckFont::Small,clip);bool pending=false;for(const auto& job:core.pending)if(job.event.id==row.id&&!job.ack&&!job.history)pending=true;
          draw(m->acked?DeckShape::Check:pending?DeckShape::Pending:DeckShape::Retry,{291,y+4,14,14},p.ink,"",DeckFont::Small,clip);
          if(!m->acked&&!pending)hit(DeckTap::Resend,{280,std::max(feedTop,y),33,std::max(0,std::min(feedBottom,y+24)-std::max(feedTop,y))},row.id);
        }
        for(size_t j=0;j<bodies[i].size();++j)draw(DeckShape::Text,{bodyX,y+33+int(j)*18,0,0},p.ink,bodies[i][j],DeckFont::Body,clip);
        draw(DeckShape::Reply,{bodyX,y+h-14,11,10},p.muted,"",DeckFont::Small,clip);draw(DeckShape::Text,{bodyX+15,y+h-5,0,0},p.muted,"Reply",DeckFont::Small,clip);
        const int top=std::max(y,feedTop),bottom=std::min(y+h,feedBottom);
        hit(DeckTap::Vote,{x+1,top,28,bottom-top},row.id);hit(DeckTap::Reply,{bodyX,std::max(feedTop,y+h-23),95,std::max(0,bottom-std::max(feedTop,y+h-23))},row.id);
        hit(DeckTap::Select,{bodyX,top,235-indent,bottom-top},row.id);
      }y+=h;
    }
    if(rows.empty())text(116,feedTop+30,"No messages",DeckFont::Small,p.muted);
  }else {
    std::string title;std::vector<std::string> items;
    if(chat.page==DeckChat::Settings){title="Settings";items={"Radio connection","Radio preset","Nickname",chat.dark?"Appearance: Dark":"Appearance: Light"};}
    else if(chat.page==DeckChat::Menu){title="Message";items=chat.menu;}
    else if(chat.page==DeckChat::Rooms){title="Rooms";items=chat.rooms(core);draw(DeckShape::Round,{244,35,68,23},p.selected);text(253,51,"+ New room");hit(DeckTap::NewRoom,{238,31,82,30});}
    else if(chat.page==DeckChat::Presets){title="Radio preset";for(const auto& preset:presets)items.push_back(preset.label);}
    else if(chat.page==DeckChat::Credentials){size_t outgoing=0;for(const auto& job:core.pending)if(!job.ack&&!job.history&&(job.event.kind==Kind::Post||job.event.kind==Kind::Vote))++outgoing;title="Radio connection";items={name,"Password: "+password,"http://192.168.4.1","Bluetooth PIN: "+(password.size()>2?password.substr(2):""),"http://meshit.local","New: "+std::to_string(unread)+"  Out: "+std::to_string(outgoing)};}
    else if(chat.page==DeckChat::NewRoom||chat.page==DeckChat::Nickname){title=chat.page==DeckChat::NewRoom?"New room":"Nickname";items={chat.draft.empty()?"Type here...":chat.draft,"Enter to save"};}
    else {title="# "+chat.room;if(const auto* m=core.find(chat.selected)){items=deckWrap(m->event.text,294,DeckFont::Body,measure);}}
    text(8,51,title,DeckFont::Bold);const size_t window=chat.page==DeckChat::Presets?5:6;
    if(chat.page==DeckChat::Read)chat.cursor=std::min(chat.cursor,items.size()>window?items.size()-window:size_t(0));
    const size_t start=chat.page==DeckChat::Read?chat.cursor:chat.cursor>=window?chat.cursor-window+1:0;
    for(size_t i=start;i<items.size()&&i<start+window;++i){const int y=63+int(i-start)*28;draw(DeckShape::Round,{7,y,306,26},i==chat.cursor?p.selected:p.card);
      text(15,y+18,deckFit(items[i],290,DeckFont::Small,measure));if(chat.page!=DeckChat::Credentials&&chat.page!=DeckChat::Read)hit(DeckTap::MenuItem,{7,y,306,27},0,i);
    }
    if(chat.page==DeckChat::Presets)text(8,234,"Changing preset clears history",DeckFont::Small,p.muted);
  }
  if(!chat.error.empty()){draw(DeckShape::Fill,{0,220,320,20},p.header);text(8,234,deckFit(chat.error,304,DeckFont::Small,measure));}
  return s;
}
inline const DeckHit* deckHitAt(const DeckScene& scene,int x,int y){for(const auto& hit:scene.hits)if(hit.box.contains(x,y))return &hit;return nullptr;}
inline DeckAction activateDeckHit(const DeckHit& hit,DeckChat& chat,const Core& core,const std::vector<DeckPreset>& catalog,uint32_t identity){
  using T=DeckTap;chat.error.clear();DeckAction action;
  if((hit.action==T::Select||hit.action==T::Reply||hit.action==T::Vote||hit.action==T::Resend)&&!core.find(hit.target)){chat.error="Message no longer stored";return action;}
  switch(hit.action){
    case T::Chat:chat.page=DeckChat::Chat;break;
    case T::Settings:chat.page=DeckChat::Settings;chat.cursor=0;break;
    case T::Rooms:chat.page=DeckChat::Rooms;chat.cursor=0;break;
    case T::NewRoom:chat.page=DeckChat::NewRoom;chat.draft.clear();break;
    case T::Top:chat.top=true;break;case T::New:chat.top=false;break;
    case T::Post:if(!chat.draft.empty())action=chat.key('\r',core,catalog,identity);break;
    case T::Cancel:chat.reply=0;break;
    case T::Select:if(chat.selected==hit.target){chat.page=DeckChat::Read;chat.cursor=0;}chat.selected=hit.target;break;
    case T::Reply:chat.selected=chat.reply=hit.target;chat.page=DeckChat::Chat;break;
    case T::Vote:action.op="vote";action.target=hit.target;action.up=!core.voted(hit.target,identity);break;
    case T::Resend:action.op="resend";action.target=hit.target;break;
    case T::MenuItem:if((chat.page==DeckChat::NewRoom||chat.page==DeckChat::Nickname)&&hit.index==0)break;chat.cursor=hit.index;action=chat.key('\r',core,catalog,identity);break;
  }return action;
}
struct DeckPointerEvent {bool tap=false;int x=0,y=0,swipe=0;};
class DeckGesture {
  bool active=false;int startX=0,startY=0,lastX=0,lastY=0,travel=0;
public:
  void contact(int x,int y){if(!active){active=true;startX=x;startY=y;travel=0;}lastX=x;lastY=y;travel=std::max(travel,std::max(std::abs(x-startX),std::abs(y-startY)));}
  DeckPointerEvent release(){if(!active)return {};active=false;const int delta=lastY-startY;if(std::abs(delta)>18)return {false,lastX,lastY,delta>0?-1:1};return {travel<18,lastX,lastY,0};}
};
// GT911 native axes are portrait. Rotate with the LCD, including the Plus flip.
inline std::pair<int,int> deckTouchPoint(int rawX,int rawY,bool plus){return plus?std::make_pair(319-rawY,rawX):std::make_pair(rawY,239-rawX);}
}
