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
#include "Battery.h"
#include "UnreadMessages.h"
#include "RadioStats.h"
#include "RadioPower.h"
#include "DeckUi.h"
#if defined(MESHIT_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <esp_system.h>
#include <esp_mac.h>
#include "../firmware/esp32/Pins.h"
WebServer server(80);
SetupDisplay setupDisplay;
String setupName,setupPass;
bool wifiSuspended=false,boardReady=true;
#if defined(MESHIT_TDECK)
DeckUi deckUi;
#endif
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
meshit::RadioActivityClock radioActivity;
uint32_t rxCount=0,txCount=0;UnreadMessages unreadMessages;
#if defined(MESHIT_ESP32)
uint32_t lastWebActivity=0;bool webClientSeen=false;
#endif
String line[2],output[2];size_t outputAt[2]={0,0};bool discard[2]={false,false};

uint32_t random32(){
#if defined(MESHIT_ESP32)
  return esp_random();
#else
  uint32_t n=0;NRF_RNG->TASKS_START=1;for(int i=0;i<4;++i){while(!NRF_RNG->EVENTS_VALRDY){}NRF_RNG->EVENTS_VALRDY=0;n=(n<<8)|NRF_RNG->VALUE;}NRF_RNG->TASKS_STOP=1;return n;
#endif
}
bool transmitEnabled(){return settings["txEnabled"]|true;}
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
void eventJson(JsonObject o,const Message& m){const auto& e=m.event;o["id"]=hex(e.id);o["parent"]=e.ref?hex(e.ref):"";o["room"]=e.room;o["nickname"]=e.nick;o["text"]=e.text;o["order"]=m.order;o["mine"]=m.mine;o["acknowledged"]=m.acked;o["votes"]=core.score(e.id);o["voted"]=core.voted(e.id,identity);bool pending=false;for(const auto& p:core.pending)if(p.event.id==e.id&&!p.ack)pending=true;o["status"]=m.acked?"peer confirmed":pending?"sending":m.mine?"no rebroadcast heard":"received";}
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
#if defined(MESHIT_ESP32)
  if(!boardReady){radioError=RADIOLIB_ERR_CHIP_NOT_FOUND;return;}
#endif
  bool mt=network()==Network::Meshtastic;
  uint16_t preamble=mt?16:(p["sf"].as<uint8_t>()<=8?32:16);
  const auto power=selectTxPower(boardPowerProfile,p["power"]|0);
  if(!power.valid){radioError=RADIOLIB_ERR_INVALID_OUTPUT_POWER;return;}
  radioError=radio.begin(p["frequency"],p["bandwidth"],p["sf"],p["cr"],mt?0x2b:0x12,power.drive,preamble,
#if defined(MESHIT_ESP32)
    RADIO_TCXO
#else
    1.8
#endif
  );
  if(!radioError)radioError=radio.setCurrentLimit(140.0f); // SX1262 high-power PA allowance.
  if(!radioError)radioError=radio.setCRC(1);
  if(!radioError)radioError=radio.setDio2AsRfSwitch(true);
#if defined(MESHIT_HELTEC_V4_KCT8103L)
  if(!radioError)radio.setRfSwitchPins(RADIOLIB_NC,5);
#elif defined(MESHIT_HELTEC_V4)
  if(!radioError)radio.setRfSwitchPins(RADIOLIB_NC,46);
#endif
  if(!radioError){ready=true;startRx();}
}
Bytes wrap(const Bytes& fragment,bool directReceipt=false){
  auto p=preset();return network()==Network::Meshtastic?meshtasticPacket(fragment,identity,random32()|1,String(p["channel"]|"LongFast").c_str(),directReceipt?0:7):meshcorePacket(fragment);
}
bool beginPacket(){
  if(!transmitEnabled()||txIndex>=txPackets.size())return false;
  if(radio.scanChannel()!=RADIOLIB_CHANNEL_FREE){startRx();return false;}
  radio.standby();radioFlag=false;radio.setPacketSentAction(radioIRQ);
  radioError=radio.startTransmit(txPackets[txIndex].data(),txPackets[txIndex].size());
  if(radioError){startRx();return false;}txBusy=true;txStarted=millis();return true;
}
bool sendEvent(const Event& e,bool){
  if(!transmitEnabled()||!ready||txBusy||!txPackets.empty()||int32_t(millis()-nextTx)<0)return false;
  std::array<uint8_t,12> nonce{};for(size_t i=0;i<12;i+=4){uint32_t n=random32();for(size_t j=0;j<4;++j)nonce[i+j]=n>>(8*j);}
  Id transfer=(Id(random32())<<16)|(random32()&65535);auto pieces=fragment(transfer,seal(encode(e),nonce));
  for(const auto& piece:pieces){auto packet=wrap(piece,e.kind==Kind::Ack);if(packet.empty()){txPackets.clear();return false;}txPackets.push_back(std::move(packet));}
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
    if(!err&&len<=255){radioActivity.received(millis());Received rx;bool ok=network()==Network::Meshtastic?parseMeshtastic(raw,String(preset()["channel"]|"LongFast").c_str(),rx):parseMeshcore(raw,rx);
      Bytes encrypted,plain;bool relayed=false;Event e;
      if(ok&&assembler.accept(rx.payload,millis(),rx.relayed,encrypted,relayed)&&open(encrypted,plain)&&decode(plain,e)){++rxCount;const bool unseen=!core.find(e.id);core.receive(e,millis(),relayed);if(unseen&&e.kind==Kind::Post&&core.find(e.id)&&!core.find(e.id)->mine)unreadMessages.add(e.id,core);changed();}
    }
  }
  if(!txBusy&&!txPackets.empty()&&int32_t(millis()-nextTx)>=0)beginPacket();
}
String dispatch(const String& input){
  JsonDocument req,res;if(input.length()>2048||deserializeJson(req,input)){res["error"]="Invalid request";}
  else {
    res["id"]=req["id"];String op=req["op"]|"";
    if(op=="state"){
      unreadMessages.prune(core);res["viewedSupported"]=true;res["viewedIdsSupported"]=true;res["unread"]=unreadMessages.count();res["outgoing"]=meshit::outgoingCount(core);res["version"]=FIRMWARE_VERSION;res["node"]=String(identity,HEX);
      res["revision"]=revision;res["count"]=core.messages.size();res["settings"]=settings;res["settings"].remove("password");res["settings"].remove("setupPassword");res["settings"].remove("channel");res["settings"]["txEnabled"]=transmitEnabled();res["settings"]["wifiConfigured"]=String(settings["ssid"]|"").length()>0;
      res["presets"]=presets;res["maxTxPowerDbm"]=maxTxPower(boardPowerProfile);
      if(ready){const auto power=selectTxPower(boardPowerProfile,preset()["power"]|0);res["txPowerDbm"]=power.output;res["radioDriveDbm"]=power.drive;}
      res["ready"]=ready;res["radioError"]=radioError;res["storageReady"]=fsReady;res["identityReady"]=identityReady;res["rx"]=rxCount;res["tx"]=txCount;res["radioState"]=radioActivityName(radioActivity.state(ready,txBusy,millis()));
      res["wifiSupported"]=
#if defined(MESHIT_ESP32)
      true;
#else
      false;
#endif
      res["board"]=
#if defined(MESHIT_ESP32)
      MESHIT_BOARD_NAME;
#else
      NRF_BOARD_NAME;
#endif
    }else if(op=="viewed"){
      if(req["messages"].is<JsonArrayConst>()){std::vector<Id> viewed;for(JsonVariantConst id:req["messages"].as<JsonArrayConst>())if(viewed.size()<32)viewed.push_back(fromHex(id|""));unreadMessages.viewedIds(req["room"]|"",viewed,core);res["ok"]=true;}
      else res["ok"]=unreadMessages.viewed(req["room"]|"",req["revision"]|0u,revision,core);
    }else if(op=="messages"){
      res["revision"]=revision;auto a=res["messages"].to<JsonArray>();size_t offset=req["offset"]|0;for(size_t i=offset;i<core.messages.size()&&i<offset+8;++i)eventJson(a.add<JsonObject>(),core.messages[i]);
    }else if(op=="txEnabled"){
      if(!req["enabled"].is<bool>())res["error"]="Transmit enabled must be true or false";
      else {JsonDocument next;next.set(settings);next["txEnabled"]=req["enabled"].as<bool>();String serialized;serializeJson(next,serialized);
        if(!fsReady||!writeFile("/settings.json",serialized))res["error"]="Could not save transmit setting";
        else {settings.set(next);if(!transmitEnabled()&&ready){radio.standby();txBusy=false;txPackets.clear();startRx();}changed();res["ok"]=true;}
      }
    }else if(op=="resend"){
      if(!transmitEnabled()){res["error"]="Transmit is disabled";}
      else if(!ready||!core.resend(fromHex(req["target"]|""),millis()))res["error"]="Cannot resend: message unavailable, acknowledged, already queued, or radio not ready";
      else {res["ok"]=true;res["messageId"]=hex(core.pending.back().event.id);changed();persist();}
    }else if(op=="post"||op=="vote"){
      if(!transmitEnabled())res["error"]="Transmit is disabled";
      else if(!ready)res["error"]="Choose a network and preset; radio must be ready";
      else {Event e;e.kind=op=="post"?Kind::Post:Kind::Vote;e.ref=fromHex(req[op=="post"?"parent":"target"]|"");
        e.room=req["room"].as<std::string>();e.nick=std::string(settings["nickname"]|"");e.text=req["text"].as<std::string>();e.up=req["up"]|true;
        if(op=="vote"&&!core.find(e.ref))res["error"]="Message is no longer stored";
        else if(!core.submit(e,millis()))res["error"]="Cannot queue: invalid text, displaced thread, full queue, or storage unavailable";
        else {res["ok"]=true;res["messageId"]=hex(core.pending.back().event.id);changed();persist();}
      }
    }else if(op=="deckAppearance"){
      const int appearance=req["appearance"]|-1;
      if(appearance<0||appearance>2)res["error"]="Choose System, Light or Dark";
      else {JsonDocument next;next.set(settings);next["deckAppearance"]=appearance;String s;serializeJson(next,s);
        if(!fsReady||!writeFile("/settings.json",s))res["error"]="Could not save appearance";
        else {settings.set(next);res["ok"]=true;}
      }
    }else if(op=="settings"){
      String selected=req["preset"]|settings["preset"]|"",net=req["network"]|settings["network"]|"";JsonObjectConst p;
      for(JsonObjectConst v:presets.as<JsonArrayConst>())if(v["id"]==selected&&v["network"]==net)p=v;
      std::string nick=req["nickname"]|settings["nickname"]|"";
      if(p.isNull()||!textValid(nick,MAX_NICK,MAX_NICK,true))res["error"]="Choose a matching network preset; nickname must fit 16 UTF-8 bytes";
      else if(txBusy||!txPackets.empty()||!core.pending.empty())res["error"]="Wait for queued transmissions before changing settings";
      else {
        JsonDocument next;next.set(settings);next["network"]=net;next["preset"]=selected;next["nickname"]=nick;next.remove("channel");next["configured"]=true;
        if(req["ssid"].is<const char*>())next["ssid"]=req["ssid"];
        if(req["password"].is<const char*>()&&String(req["password"]).length())next["password"]=req["password"];
        String s;serializeJson(next,s);
        if(!fsReady||!writeFile("/settings.json",s))res["error"]="Could not save settings";
        else {bool changedNetwork=net!=String(settings["network"]|"")||selected!=String(settings["preset"]|"");settings.set(next);if(changedNetwork){unreadMessages=UnreadMessages();core=Core();core.allocate=allocateId;core.jitter=random32;assembler=Assembler();}initRadio();changed();persist();res["ok"]=true;res["reconnectRequired"]=req["ssid"].is<const char*>();}
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
  uint8_t mac[6]={};esp_efuse_mac_get_default(mac);
  const uint16_t macSuffix=(uint16_t(mac[4])<<8)|mac[5];
  const bool displayReady=setupDisplay.begin();
  const String saved=settings["setupPassword"]|"";
  setupPass=meshit::setupPassword(displayReady,saved.c_str(),macSuffix).c_str();
  if(setupPass!=saved){settings["setupPassword"]=setupPass;String s;serializeJson(settings,s);writeFile("/settings.json",s);}
  setupName=meshit::setupRadioName(macSuffix).c_str();
  const bool apStarted=WiFi.softAP(setupName.c_str(),setupPass.c_str());
  if(apStarted)setupDisplay.showAccessPoint(setupName,setupPass);
  else setupDisplay.showNetwork(false,"");
  Serial.println("Setup Wi-Fi: "+setupName+" password: "+setupPass);
  String ssid=settings["ssid"]|"";if(ssid.length())WiFi.begin(ssid.c_str(),settings["password"]|"");
  MDNS.begin("meshit");MDNS.addService("http","tcp",80);
  server.on("/api/rpc",HTTP_POST,[]{
    if(server.hasHeader("Origin")){String origin=server.header("Origin"),host=server.hostHeader();if(origin!="http://"+host){server.send(403,"application/json","{\"error\":\"Origin rejected\"}");return;}}
    lastWebActivity=millis();webClientSeen=true;String result=dispatch(server.arg("plain"));server.send(200,"application/json",result);
  });
  const char* headers[]={"Origin"};server.collectHeaders(headers,1);
  server.onNotFound([]{String uri=server.uri();if(uri=="/")uri="/index.html";for(const auto& asset:WEB_ASSETS)if(uri==asset.path){server.sendHeader("Content-Encoding","gzip");server.sendHeader("Cache-Control","no-cache");server.sendHeader("X-Content-Type-Options","nosniff");server.sendHeader("Content-Security-Policy","default-src 'self'; connect-src 'self'; style-src 'self'; script-src 'self'; frame-ancestors 'none'; base-uri 'none'");server.send_P(200,asset.mime,(const char*)asset.data,asset.size);return;}server.send(404,"text/plain","Not found");});server.begin();
}
#endif
void setup(){
  Serial.begin(115200);fsReady=storageBegin();deserializeJson(presets,PRESETS);
  if(fsReady)deserializeJson(settings,recoverFile("/settings.json"));
  settings.remove("channel");
  JsonDocument id;bool have=fsReady&&!deserializeJson(id,recoverFile("/identity.json"));identity=have?id["sender"].as<uint32_t>():random32();if(!identity)identity=1;
  counter=reserved=have?id["reserved"].as<uint32_t>():0;identityReady=fsReady&&counter<=65535&&saveIdentity();core.allocate=allocateId;core.jitter=random32;restore();
#if defined(MESHIT_ESP32)
#if defined(MESHIT_HELTEC_V4)
  pinMode(7,OUTPUT);digitalWrite(7,HIGH);pinMode(2,OUTPUT);digitalWrite(2,HIGH);
#if defined(MESHIT_HELTEC_V4_KCT8103L)
  pinMode(5,OUTPUT);digitalWrite(5,LOW);
#else
  pinMode(46,OUTPUT);digitalWrite(46,LOW);
#endif
  delay(10);
#endif
#if defined(MESHIT_TDECK)
  boardReady=beginDeckHardware();
#endif
  SPI.begin(RADIO_SCK,RADIO_MISO,RADIO_MOSI);setupWifi();
#else
  pinMode(MESHIT_BOARD_POWER,OUTPUT);digitalWrite(MESHIT_BOARD_POWER,HIGH);delay(10);SPI.begin();
#endif
#if defined(MESHIT_ESP32)
  ble.begin(setupName,setupPass.substring(2).toInt());
#else
  ble.begin(meshit::setupRadioName(uint16_t(NRF_FICR->DEVICEADDR[0])).c_str());
#endif
  initRadio();
#if defined(MESHIT_TDECK)
  deckUi.begin(settings["configured"]|false,settings["deckDark"]|false,settings["deckAppearance"]|-1);
  // E-paper busy waits release SPI; keep receiving radio packets during refresh.
  deckPanel().serviceWhileBusy([](const void*){pollRadio();delay(1);});
#endif
}
void loop(){
#if defined(MESHIT_ESP32)
  ble.setAccessPointOccupied(!wifiSuspended&&WiFi.softAPgetStationNum()>0);
  ble.maintain();
  const bool bluetooth=ble.connected();
  static uint32_t connectedAt=0;
  if(bluetooth!=wifiSuspended){
    wifiSuspended=bluetooth;connectedAt=0;
    if(bluetooth){server.stop();MDNS.end();WiFi.softAPdisconnect(true);WiFi.disconnect();WiFi.mode(WIFI_OFF);}
    else{
      WiFi.mode(WIFI_AP_STA);WiFi.softAP(setupName.c_str(),setupPass.c_str());
      const String ssid=settings["ssid"]|"";if(ssid.length())WiFi.begin(ssid.c_str(),settings["password"]|"");
      MDNS.begin("meshit");MDNS.addService("http","tcp",80);server.begin();
    }
  }
  if(!wifiSuspended){
    server.handleClient();
    if(WiFi.status()==WL_CONNECTED){if(!connectedAt)connectedAt=millis();if(uint32_t(millis()-connectedAt)>60000)WiFi.softAPdisconnect(true);}else connectedAt=0;
  }
  updateBattery(millis());
  static uint32_t lastDisplay=0;
  if(uint32_t(millis()-lastDisplay)>=100){
    lastDisplay=millis();
    if(ble.pairing())setupDisplay.showBluetooth(setupName,setupPass.substring(2));
#if !defined(MESHIT_TDECK)
    else if(bluetooth||(!wifiSuspended&&webClientSeen&&uint32_t(millis()-lastWebActivity)<15000)){
      const size_t outgoing=meshit::outgoingCount(core);
      unreadMessages.prune(core);
      setupDisplay.showStatus(bluetooth,batteryReading().millivolts,outgoing,unreadMessages.count(),ready,preset()["frequency"]|0.0f);
    }
    else if(WiFi.getMode()&WIFI_AP)setupDisplay.showAccessPoint(setupName,setupPass);
    else setupDisplay.showNetwork(WiFi.status()==WL_CONNECTED,WiFi.localIP().toString());
#endif
  }
#endif
  pollLinks();pollRadio();size_t before=core.pending.size();
  if(ready&&identityReady&&transmitEnabled())core.tick(millis(),sendEvent);
  if(core.pending.size()!=before||txStateChanged)changed();
  if(txStateChanged){txStateChanged=false;persist();}
  if(dirty&&uint32_t(millis()-lastChange)>5000)persist();
#if defined(MESHIT_TDECK)
  if(!ble.pairing())deckUi.tick(core,settings,presets,identity,revision,unreadMessages,ready,ble.connected(),batteryReading().millivolts,setupName,setupPass,dispatch,radioActivity.state(ready,txBusy,millis()));
#endif
  delay(1);
}
