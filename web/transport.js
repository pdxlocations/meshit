export const SERVICE='6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const RX='6e400002-b5a3-f393-e0a9-e50e24dcca9e',TX='6e400003-b5a3-f393-e0a9-e50e24dcca9e';
export class HttpTransport {
  constructor(base=''){this.base=base.replace(/\/$/,'');this.label=base||'Local radio';}
  async request(data){
    const url=this.base+'/api/rpc';let result;
    if(globalThis.Capacitor?.isNativePlatform()){
      const {CapacitorHttp}=await import('@capacitor/core');
      const r=await CapacitorHttp.post({url,data,headers:{'Content-Type':'application/json'},connectTimeout:10000,readTimeout:20000});
      if(r.status!==200)throw new Error(`Radio returned ${r.status}`);result=typeof r.data==='string'?JSON.parse(r.data):r.data;
    }else{const r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data),signal:AbortSignal.timeout(20000)});if(!r.ok)throw new Error(`Radio returned ${r.status}`);result=await r.json();}
    if(result.error)throw new Error(result.error);return result;
  }
  async close(){}
}
export class BleTransport {
  constructor(){this.buffer='';this.waiting=new Map();this.serial=Promise.resolve();this.label='Bluetooth';this.nextId=1;this.decoder=new TextDecoder();}
  onData(bytes){
    this.buffer+=this.decoder.decode(bytes,{stream:true});
    if(this.buffer.length>32768){this.fail(new Error('Radio response too large'));return;}
    let end;while((end=this.buffer.indexOf('\n'))>=0){const line=this.buffer.slice(0,end);this.buffer=this.buffer.slice(end+1);try{const response=JSON.parse(line);const waiter=this.waiting.get(response.id);if(waiter){this.waiting.delete(response.id);clearTimeout(waiter.timer);if(response.error)waiter.reject(new Error(response.error));else waiter.resolve(response);}}catch{this.fail(new Error('Invalid radio response'));}}
  }
  fail(error){for(const p of this.waiting.values()){clearTimeout(p.timer);p.reject(error);}this.waiting.clear();this.buffer='';}
  async connect(){
    if(globalThis.Capacitor?.isNativePlatform()){
      const {BleClient}=await import('@capacitor-community/bluetooth-le');this.native=BleClient;await BleClient.initialize({androidNeverForLocation:true});
      this.device=await BleClient.requestDevice({services:[SERVICE]});await BleClient.connect(this.device.deviceId,()=>this.fail(new Error('Bluetooth disconnected')));
      await BleClient.startNotifications(this.device.deviceId,SERVICE,TX,value=>this.onData(new Uint8Array(value.buffer,value.byteOffset,value.byteLength)));
    }else{
      if(!navigator.bluetooth)throw new Error('Use the mobile companion or a browser with Bluetooth support');
      this.device=await navigator.bluetooth.requestDevice({filters:[{services:[SERVICE]}]});this.device.addEventListener('gattserverdisconnected',()=>this.fail(new Error('Bluetooth disconnected')));
      const server=await this.device.gatt.connect(),service=await server.getPrimaryService(SERVICE);this.rx=await service.getCharacteristic(RX);this.tx=await service.getCharacteristic(TX);
      this.tx.addEventListener('characteristicvaluechanged',e=>this.onData(new Uint8Array(e.target.value.buffer,e.target.value.byteOffset,e.target.value.byteLength)));await this.tx.startNotifications();
    }this.label=this.device.name||'Bluetooth radio';
  }
  request(data){const run=()=>this.exchange(data);const pending=this.serial.then(run,run);this.serial=pending.catch(()=>{});return pending;}
  async exchange(data){
    const id=this.nextId++,bytes=new TextEncoder().encode(JSON.stringify({...data,id})+'\n');
    return new Promise((resolve,reject)=>{
      const timer=setTimeout(()=>{this.waiting.delete(id);reject(new Error('Radio response timed out'));},30000);this.waiting.set(id,{resolve,reject,timer});
      (async()=>{for(let at=0;at<bytes.length;at+=20){const part=bytes.slice(at,at+20);if(this.native)await this.native.write(this.device.deviceId,SERVICE,RX,new DataView(part.buffer));else await this.rx.writeValueWithResponse(part);}})().catch(error=>{clearTimeout(timer);this.waiting.delete(id);reject(error);});
    });
  }
  async close(){this.fail(new Error('Connection closed'));if(this.native&&this.device)await this.native.disconnect(this.device.deviceId);else this.device?.gatt?.disconnect();}
}
