#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <RadioLib.h>
#include <Core.h>
#include <CryptoBox.h>
#include <Fragments.h>
#include <Carriers.h>
#include "Storage.h"
#include "History.h"
#include "BleLink.h"
#include "BuildAssets.h"
#include "SetupAccess.h"
#include "SetupDisplay.h"
#if defined(MESHIT_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <esp_system.h>
constexpr int RADIO_CS=8,RADIO_IRQ=14,RADIO_RESET=12,RADIO_BUSY=13;
WebServer server(80);
SetupDisplay setupDisplay;
String setupName,setupPass;
#else
#include "../firmware/nrf52/Pins.h"
constexpr int RADIO_CS=NRF_CS,RADIO_IRQ=NRF_IRQ,RADIO_RESET=NRF_RESET,RADIO_BUSY=NRF_BUSY;
#endif
using namespace meshit;
SX1262 radio=new Module(RADIO_CS,RADIO_IRQ,RADIO_RESET,RADIO_BUSY);
Core core;Assembler assembler;BleLink ble;
JsonDocument settings,presets;
uint32_t identity=0,reserved=0,counter=0,revision=1,dirtyAt=0,lastChange=0,nextTx=0,txStarted=0;
bool fsReady=false,identityReady=false,ready=false,txBusy=false,dirty=false,txStateChanged=false;
volatile bool radioFlag=false;
int radioError=0;size_t txIndex=0;std::vector<Bytes> txPackets;
uint32_t rxCount=0,txCount=0;
String line[2],output[2];size_t outputAt[2]={0,0};bool discard[2]={false,false};

uint32_t random32(){
#if defined(MESHIT_ESP32)
  return esp_random();
#else
  uint32_t n=0;NRF_RNG->TASKS_START=1;for(int i=0;i<4;++i){while(!NRF_RNG->EVENTS_VALRDY){}NRF_RNG->EVENTS_VALRDY=0;n=(n<<8)|NRF_RNG->VALUE;}NRF_RNG->TASKS_STOP=1;return n;
#endif
}
void changed(){++revision;dirty=true;if(!dirtyAt)dirtyAt=millis();lastChange=millis();}
bool saveIdentity(){JsonDocument d;d["sender"]=identity;d["reserved"]=reserved;String s;serializeJson(d,s);return writeFile("/identity.json",s);}
Id allocateId(){
  if(!identityReady)return 0;
  if(counter>=65535){identity=random32();if(!identity)identity=1;counter=reserved=0;}
  if(counter>=reserved){reserved=std::min(uint32_t(65535),counter+64);if(!saveIdentity()){identityReady=false;return 0;}}
  return (Id(identity)<<16)|++counter;
}
Network network(){return String(settings["network"]|"")=="meshcore"?Network::MeshCore:Network::Meshtastic;}
JsonObjectConst preset(){for(JsonObjectConst p:presets.as<JsonArrayConst>())if(p["id"]==settings["preset"])return p;return {};}
void eventJson(JsonObject o,const Message& m){const auto& e=m.event;o["id"]=hex(e.id);o["parent"]=e.ref?hex(e.ref):"";o["room"]=e.room;o["nickname"]=e.nick;o["text"]=e.text;o["order"]=m.order;o["mine"]=m.mine;o["acknowledged"]=m.acked;o["votes"]=core.score(e.id);o["voted"]=core.voted(e.id,identity);bool pending=false;for(const auto& p:core.pending)if(p.event.id==e.id&&!p.ack)pending=true;o["status"]=m.acked?"rebroadcast heard":pending?"sending":m.mine?"no rebroadcast heard":"received";}
void persist(){
  if(fsReady&&saveHistory(core)){dirty=false;dirtyAt=0;}
}
void restore(){
  if(!fsReady)return;
  if(!loadHistory(core,millis())){core=Core();core.allocate=allocateId;core.jitter=random32;}
}
void radioIRQ(){radioFlag=true;}
void startRx(){radioFlag=false;radio.setPacketReceivedAction(radioIRQ);radio.startReceive();}
void initRadio(){
  ready=false;txBusy=false;txPackets.clear();radioFlag=false;radio.standby();auto p=preset();
  if(p.isNull()||!settings["configured"].as<bool>())return;
  bool mt=network()==Network::Meshtastic;
  uint16_t preamble=mt?16:(p["sf"].as<uint8_t>()<=8?32:16);
  radioError=radio.begin(p["frequency"],p["bandwidth"],p["sf"],p["cr"],mt?0x2b:0x12,p["power"],preamble,1.8);
  if(!radioError)radioError=radio.setCRC(1);
  if(!radioError)radioError=radio.setDio2AsRfSwitch(true);
  if(!radioError){ready=true;startRx();}
}
Bytes wrap(const Bytes& fragment){
  auto p=preset();return network()==Network::Meshtastic?meshtasticPacket(fragment,identity,random32()|1,String(settings["channel"]|p["channel"].as<const char*>()).c_str()):meshcorePacket(fragment);
}
bool beginPacket(){
  if(txIndex>=txPackets.size())return false;
  if(radio.scanChannel()!=RADIOLIB_CHANNEL_FREE){startRx();return false;}
  radio.standby();radioFlag=false;radio.setPacketSentAction(radioIRQ);
  radioError=radio.startTransmit(txPackets[txIndex].data(),txPackets[txIndex].size());
  if(radioError){startRx();return false;}txBusy=true;txStarted=millis();return true;
}
bool sendEvent(const Event& e,bool){
  if(!ready||txBusy||!txPackets.empty()||int32_t(millis()-nextTx)<0)return false;
  std::array<uint8_t,12> nonce{};for(size_t i=0;i<12;i+=4){uint32_t n=random32();for(size_t j=0;j<4;++j)nonce[i+j]=n>>(8*j);}
  Id transfer=(Id(random32())<<16)|(random32()&65535);auto pieces=fragment(transfer,seal(encode(e),nonce));
  for(const auto& piece:pieces){auto packet=wrap(piece);if(packet.empty()){txPackets.clear();return false;}txPackets.push_back(std::move(packet));}
  txIndex=0;if(txPackets.empty())return false;
  if(!beginPacket()){txPackets.clear();return false;}txStateChanged=true;return true;
}
void pollRadio(){
  if(!ready)return;
  if(txBusy){
    if(!radioFlag&&uint32_t(millis()-txStarted)<12000)return;
    bool success=radioFlag;radio.finishTransmit();txBusy=false;
    uint32_t air=radio.getTimeOnAir(txPackets[txIndex].size())/1000+1;uint32_t duty=preset()["duty"]|100;
    nextTx=millis()+std::max(uint32_t(250),air*(100-duty)/duty)+random32()%400;
    if(success){++txCount;++txIndex;}else txIndex=txPackets.size();
    if(txIndex>=txPackets.size())txPackets.clear();startRx();
  }
  if(radioFlag){
    radioFlag=false;size_t len=radio.getPacketLength();Bytes raw(std::min(size_t(255),len));int err=radio.readData(raw.data(),raw.size());startRx();
    if(!err&&len<=255){Received rx;bool ok=network()==Network::Meshtastic?parseMeshtastic(raw,String(settings["channel"]|preset()["channel"].as<const char*>()).c_str(),rx):parseMeshcore(raw,rx);
      Bytes encrypted,plain;bool relayed=false;Event e;
      if(ok&&assembler.accept(rx.payload,millis(),rx.relayed,encrypted,relayed)&&open(encrypted,plain)&&decode(plain,e)){++rxCount;core.receive(e,millis(),relayed);changed();}
    }
  }
  if(!txBusy&&!txPackets.empty()&&int32_t(millis()-nextTx)>=0)beginPacket();
}
String dispatch(const String& input){
  JsonDocument req,res;if(input.length()>2048||deserializeJson(req,input)){res["error"]="Invalid request";}
  else {
    res["id"]=req["id"];String op=req["op"]|"";
    if(op=="state"){
      res["version"]=FIRMWARE_VERSION;
      res["revision"]=revision;res["count"]=core.messages.size();res["settings"]=settings;res["settings"].remove("password");res["settings"].remove("setupPassword");res["settings"]["wifiConfigured"]=String(settings["ssid"]|"").length()>0;
      res["presets"]=presets;res["ready"]=ready;res["radioError"]=radioError;res["storageReady"]=fsReady;res["identityReady"]=identityReady;res["rx"]=rxCount;res["tx"]=txCount;
      res["board"]=
#if defined(MESHIT_ESP32)
      "Heltec V3";
#else
      "RAK4631";
#endif
    }else if(op=="messages"){
      res["revision"]=revision;auto a=res["messages"].to<JsonArray>();size_t offset=req["offset"]|0;for(size_t i=offset;i<core.messages.size()&&i<offset+8;++i)eventJson(a.add<JsonObject>(),core.messages[i]);
    }else if(op=="post"||op=="vote"){
      if(!ready)res["error"]="Choose a network and preset; radio must be ready";
      else {Event e;e.kind=op=="post"?Kind::Post:Kind::Vote;e.ref=fromHex(req[op=="post"?"parent":"target"]|"");
        e.room=req["room"].as<std::string>();e.nick=std::string(settings["nickname"]|"");e.text=req["text"].as<std::string>();e.up=req["up"]|true;
        if(op=="vote"&&!core.find(e.ref))res["error"]="Message is no longer stored";
        else if(!core.submit(e,millis()))res["error"]="Cannot queue: invalid text, displaced thread, full queue, or storage unavailable";
        else {res["ok"]=true;changed();persist();}
      }
    }else if(op=="settings"){
      String selected=req["preset"]|settings["preset"]|"",net=req["network"]|settings["network"]|"";JsonObjectConst p;
      for(JsonObjectConst v:presets.as<JsonArrayConst>())if(v["id"]==selected&&v["network"]==net)p=v;
      std::string nick=req["nickname"]|settings["nickname"]|"",channel=req["channel"]|p["channel"]|"LongFast";
      if(p.isNull()||!textValid(nick,MAX_NICK,MAX_NICK,true)||!textValid(channel,32,32)||channel.find('\n')!=std::string::npos)res["error"]="Choose a matching network preset; nickname must fit 16 UTF-8 bytes";
      else if(txBusy||!txPackets.empty()||!core.pending.empty())res["error"]="Wait for queued transmissions before changing settings";
      else {
        JsonDocument next;next.set(settings);next["network"]=net;next["preset"]=selected;next["nickname"]=nick;next["channel"]=channel;next["configured"]=true;
        if(req["ssid"].is<const char*>())next["ssid"]=req["ssid"];
        if(req["password"].is<const char*>()&&String(req["password"]).length())next["password"]=req["password"];
        String s;serializeJson(next,s);
        if(!fsReady||!writeFile("/settings.json",s))res["error"]="Could not save settings";
        else {bool changedNetwork=net!=String(settings["network"]|"")||selected!=String(settings["preset"]|"");settings.set(next);if(changedNetwork){core=Core();core.allocate=allocateId;core.jitter=random32;assembler=Assembler();}initRadio();changed();persist();res["ok"]=true;res["reconnectRequired"]=req["ssid"].is<const char*>();}
      }
    }else res["error"]="Unknown operation";
  }
  String result;serializeJson(res,result);return result+"\n";
}
void pollLinks(){
  static uint32_t lastNotify=0;bool connected=ble.connected();static bool previous=false;
  if(connected!=previous){line[1]="";output[1]="";outputAt[1]=0;discard[1]=false;previous=connected;}
  for(size_t link=0;link<2;++link){
    if(outputAt[link]<output[link].length()){
      if(link==1&&uint32_t(millis()-lastNotify)<15)continue;
      size_t n=std::min(size_t(link?20:128),output[link].length()-outputAt[link]);String part=output[link].substring(outputAt[link],outputAt[link]+n);
      bool sent=link?ble.send(part):Serial.availableForWrite()>=int(n)&&Serial.write((const uint8_t*)part.c_str(),n)==n;
      if(sent)outputAt[link]+=n;if(link)lastNotify=millis();continue;
    }
    output[link]="";outputAt[link]=0;
    for(size_t i=0;i<128;++i){char c;if(link){if(!ble.read(c))break;}else{if(!Serial.available())break;c=Serial.read();}
      if(c=='\r')continue;if(c=='\n'){if(!discard[link]&&line[link].length())output[link]=dispatch(line[link]);line[link]="";discard[link]=false;if(output[link].length())break;}
      else if(line[link].length()<2048&&!discard[link])line[link]+=c;else{discard[link]=true;line[link]="";}
    }
  }
}
#if defined(MESHIT_ESP32)
void setupWifi(){
  WiFi.setHostname("meshit");WiFi.mode(WIFI_AP_STA);
  const bool displayReady=setupDisplay.begin();
  const String saved=settings["setupPassword"]|"";
  setupPass=meshit::setupPassword(displayReady,saved.c_str(),random32()).c_str();
  if(setupPass!=saved){settings["setupPassword"]=setupPass;String s;serializeJson(settings,s);writeFile("/settings.json",s);}
  setupName="Meshit-"+String(identity&65535,HEX);
  const bool apStarted=WiFi.softAP(setupName.c_str(),setupPass.c_str());
  if(apStarted)setupDisplay.showAccessPoint(setupName,setupPass);
  else setupDisplay.showNetwork(false,"");
  Serial.println("Setup Wi-Fi: "+setupName+" password: "+setupPass);
  String ssid=settings["ssid"]|"";if(ssid.length())WiFi.begin(ssid.c_str(),settings["password"]|"");
  MDNS.begin("meshit");MDNS.addService("http","tcp",80);
  server.on("/api/rpc",HTTP_POST,[]{
    if(server.hasHeader("Origin")){String origin=server.header("Origin"),host=server.hostHeader();if(origin!="http://"+host){server.send(403,"application/json","{\"error\":\"Origin rejected\"}");return;}}
    String result=dispatch(server.arg("plain"));server.send(200,"application/json",result);
  });
  const char* headers[]={"Origin"};server.collectHeaders(headers,1);
  server.onNotFound([]{String uri=server.uri();if(uri=="/")uri="/index.html";for(const auto& asset:WEB_ASSETS)if(uri==asset.path){server.sendHeader("Content-Encoding","gzip");server.sendHeader("Cache-Control","no-cache");server.sendHeader("X-Content-Type-Options","nosniff");server.sendHeader("Content-Security-Policy","default-src 'self'; connect-src 'self'; style-src 'self'; script-src 'self'; frame-ancestors 'none'; base-uri 'none'");server.send_P(200,asset.mime,(const char*)asset.data,asset.size);return;}server.send(404,"text/plain","Not found");});server.begin();
}
#endif
void setup(){
  Serial.begin(115200);fsReady=storageBegin();deserializeJson(presets,PRESETS);
  if(fsReady)deserializeJson(settings,recoverFile("/settings.json"));
  JsonDocument id;bool have=fsReady&&!deserializeJson(id,recoverFile("/identity.json"));identity=have?id["sender"].as<uint32_t>():random32();if(!identity)identity=1;
  counter=reserved=have?id["reserved"].as<uint32_t>():0;identityReady=fsReady&&counter<=65535&&saveIdentity();core.allocate=allocateId;core.jitter=random32;restore();
#if defined(MESHIT_ESP32)
  SPI.begin(9,11,10);setupWifi();
#else
  pinMode(37,OUTPUT);digitalWrite(37,HIGH);delay(10);SPI.begin();
#endif
  ble.begin("Meshit-"+String(identity&65535,HEX));initRadio();
}
void loop(){
#if defined(MESHIT_ESP32)
  server.handleClient();
  static uint32_t connectedAt=0;
  if(WiFi.status()==WL_CONNECTED){if(!connectedAt)connectedAt=millis();if(uint32_t(millis()-connectedAt)>60000)WiFi.softAPdisconnect(true);}else connectedAt=0;
  static uint32_t lastDisplay=0;
  if(uint32_t(millis()-lastDisplay)>=1000){
    lastDisplay=millis();
    if(WiFi.getMode()&WIFI_AP)setupDisplay.showAccessPoint(setupName,setupPass);
    else setupDisplay.showNetwork(WiFi.status()==WL_CONNECTED,WiFi.localIP().toString());
  }
#endif
  pollLinks();pollRadio();size_t before=core.pending.size();
  if(ready&&identityReady)core.tick(millis(),sendEvent);
  if(core.pending.size()!=before||txStateChanged)changed();
  if(txStateChanged){txStateChanged=false;persist();}
  if(dirty&&uint32_t(millis()-lastChange)>5000)persist();delay(1);
}
