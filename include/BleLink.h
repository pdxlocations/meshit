#pragma once
#include <Arduino.h>
#include <functional>
#if defined(MESHIT_ESP32)
#include <NimBLEDevice.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
class BleLink {
  struct Chunk {uint8_t n;char data[20];};
  QueueHandle_t queue=nullptr;NimBLECharacteristic* tx=nullptr;
  class Writes:public NimBLECharacteristicCallbacks {
    BleLink& link;
  public:
    explicit Writes(BleLink& b):link(b){}
    void onWrite(NimBLECharacteristic* c)override{link.lastActivity=millis();if(!link.authorized)return;auto s=c->getValue();for(size_t i=0;i<s.size();i+=20){Chunk part{};part.n=std::min(size_t(20),s.size()-i);memcpy(part.data,s.data()+i,part.n);if(xQueueSend(link.queue,&part,0)!=pdTRUE)link.overflow=true;}}
  } writes{*this};
  volatile bool overflow=false;
  std::atomic<bool> linked{false},authorized{false},accessPointOccupied{false};
  std::atomic<uint32_t> generation{0},lastActivity{0};
  uint32_t readGeneration=0;Chunk part{};size_t partAt=0;
  class Connections:public NimBLEServerCallbacks {
    BleLink& link;
  public:
    explicit Connections(BleLink& b):link(b){}
    void onConnect(NimBLEServer* server,ble_gap_conn_desc* peer)override{
      if(link.accessPointOccupied){server->disconnect(peer->conn_handle);return;}
      xQueueReset(link.queue);link.overflow=false;link.authorized=false;link.linked=true;link.lastActivity=millis();++link.generation;
      server->updateConnParams(peer->conn_handle,12,24,0,200);NimBLEDevice::startSecurity(peer->conn_handle);
    }
    void onDisconnect(NimBLEServer*)override{link.linked=false;link.authorized=false;++link.generation;}
    void onAuthenticationComplete(ble_gap_conn_desc* peer)override{
      if(!link.accessPointOccupied&&peer->sec_state.encrypted&&peer->sec_state.authenticated){link.authorized=true;link.lastActivity=millis();}
      else NimBLEDevice::getServer()->disconnect(peer->conn_handle);
    }
  } connections{*this};
public:
  void begin(const String& name,uint32_t pin=0){
    queue=xQueueCreate(80,sizeof(Chunk));if(!queue)return;
    NimBLEDevice::init(name.c_str());NimBLEDevice::setSecurityAuth(true,true,true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);NimBLEDevice::setSecurityPasskey(pin);
    auto server=NimBLEDevice::createServer();server->setCallbacks(&connections,false);server->advertiseOnDisconnect(false);
    auto service=server->createService("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
    auto rx=service->createCharacteristic("6e400002-b5a3-f393-e0a9-e50e24dcca9e",NIMBLE_PROPERTY::WRITE|NIMBLE_PROPERTY::WRITE_ENC|NIMBLE_PROPERTY::WRITE_AUTHEN);
    tx=service->createCharacteristic("6e400003-b5a3-f393-e0a9-e50e24dcca9e",NIMBLE_PROPERTY::NOTIFY);
    rx->setCallbacks(&writes);service->start();auto advertising=NimBLEDevice::getAdvertising();advertising->addServiceUUID(service->getUUID());advertising->setScanResponse(true);advertising->start();
  }
  void setAccessPointOccupied(bool occupied){accessPointOccupied=occupied;}
  void maintain(){
    if(!queue)return;
    auto server=NimBLEDevice::getServer();auto advertising=NimBLEDevice::getAdvertising();
    if(accessPointOccupied){
      if(advertising->isAdvertising())advertising->stop();
      for(auto peer:server->getPeerDevices())server->disconnect(peer);
    }else if(!linked){if(!advertising->isAdvertising())advertising->start();}
    else{
      const uint32_t activity=lastActivity.load(),now=millis();
      if(overflow||uint32_t(now-activity)>(authorized?30000u:60000u))for(auto peer:server->getPeerDevices())server->disconnect(peer);
    }
  }
  bool read(char& c){
    const uint32_t current=generation.load();if(current!=readGeneration){readGeneration=current;part={};partAt=0;}
    if(!connected())return false;
    if(overflow){xQueueReset(queue);overflow=false;c='\n';return true;}
    if(partAt>=part.n){if(!queue||xQueueReceive(queue,&part,0)!=pdTRUE)return false;partAt=0;}c=part.data[partAt++];return true;
  }
  bool connected(){return tx&&linked&&authorized&&!accessPointOccupied;}
  bool send(const String& s){if(!connected())return false;auto peers=NimBLEDevice::getServer()->getPeerDevices();if(peers.empty())return false;auto buffer=ble_hs_mbuf_from_flat(s.c_str(),s.length());return buffer&&ble_gattc_notify_custom(peers.front(),tx->getHandle(),buffer)==0;}
};
#else
#include <bluefruit.h>
class BleLink {
  BLEUart uart;
public:
  void begin(const String& name){Bluefruit.autoConnLed(false);Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);Bluefruit.begin();Bluefruit.setName(name.c_str());uart.begin();Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);Bluefruit.Advertising.addService(uart);Bluefruit.ScanResponse.addName();Bluefruit.Advertising.restartOnDisconnect(true);Bluefruit.Advertising.setInterval(160,244);Bluefruit.Advertising.start(0);}
  bool read(char& c){if(!uart.available())return false;c=uart.read();return true;}
  bool connected(){return Bluefruit.connected();}
  bool send(const String& s){return connected()&&uart.write((const uint8_t*)s.c_str(),s.length())==s.length();}
};
#endif
