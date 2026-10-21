#pragma once
#include <Arduino.h>
#include <functional>
#if defined(MESHIT_ESP32)
#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
class BleLink {
  struct Chunk {uint8_t n;char data[20];};
  QueueHandle_t queue=nullptr;NimBLECharacteristic* tx=nullptr;
  class Writes:public NimBLECharacteristicCallbacks {
    BleLink& link;
  public:
    explicit Writes(BleLink& b):link(b){}
    void onWrite(NimBLECharacteristic* c)override{auto s=c->getValue();for(size_t i=0;i<s.size();i+=20){Chunk part{};part.n=std::min(size_t(20),s.size()-i);memcpy(part.data,s.data()+i,part.n);if(xQueueSend(link.queue,&part,0)!=pdTRUE)link.overflow=true;}}
  } writes{*this};
  volatile bool overflow=false;
public:
  void begin(const String& name){queue=xQueueCreate(80,sizeof(Chunk));if(!queue)return;NimBLEDevice::init(name.c_str());auto server=NimBLEDevice::createServer();auto service=server->createService("6e400001-b5a3-f393-e0a9-e50e24dcca9e");auto rx=service->createCharacteristic("6e400002-b5a3-f393-e0a9-e50e24dcca9e",NIMBLE_PROPERTY::WRITE);tx=service->createCharacteristic("6e400003-b5a3-f393-e0a9-e50e24dcca9e",NIMBLE_PROPERTY::NOTIFY);rx->setCallbacks(&writes);service->start();auto advertising=NimBLEDevice::getAdvertising();advertising->addServiceUUID(service->getUUID());advertising->setScanResponse(true);advertising->start();}
  bool read(char& c){if(overflow){xQueueReset(queue);overflow=false;c='\n';return true;}static Chunk part{};static size_t i=0;if(i>=part.n){if(!queue||xQueueReceive(queue,&part,0)!=pdTRUE)return false;i=0;}c=part.data[i++];return true;}
  bool connected(){return tx&&NimBLEDevice::getServer()->getConnectedCount()>0;}
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
