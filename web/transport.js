export const SERVICE='6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const RX='6e400002-b5a3-f393-e0a9-e50e24dcca9e',TX='6e400003-b5a3-f393-e0a9-e50e24dcca9e';
const changed=()=>Object.assign(new Error('Radio connection changed; previous request cancelled.'),{code:'CONNECTION_CHANGED'});
export class HttpTransport {
  constructor(base=''){this.base=base.replace(/\/$/,'');this.label=base||'Radio webpage';this.pending=new Set();}
  async request(data){
    const url=this.base+'/api/rpc';let result;
    if(globalThis.Capacitor?.isNativePlatform()){
      const {CapacitorHttp}=await import('@capacitor/core');
      const r=await CapacitorHttp.post({url,data,headers:{'Content-Type':'application/json'},connectTimeout:6000,readTimeout:15000});
      if(r.status!==200)throw new Error(`Radio returned ${r.status}`);result=typeof r.data==='string'?JSON.parse(r.data):r.data;
    }else{
      const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),15000);this.pending.add(controller);
      try{const r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data),signal:controller.signal});if(!r.ok)throw new Error(`Radio returned ${r.status}`);result=await r.json();}
      finally{clearTimeout(timer);this.pending.delete(controller);}
    }
    if(result.error)throw new Error(result.error);return result;
  }
  async close(){for(const controller of this.pending)controller.abort();this.pending.clear();}
}
export class BleTransport {
  constructor({client=null,bluetooth=null,scanMs=8000}={}){
    this.buffer='';this.waiting=new Map();this.serial=Promise.resolve();this.label='Bluetooth';this.nextId=1;this.decoder=new TextDecoder();
    this.native=client;this.bluetooth=bluetooth;this.scanMs=scanMs;this.devices=new Map();this.generation=0;this.onDisconnect=()=>{};
  }
  async initialize(){
    if(!this.native&&globalThis.Capacitor?.isNativePlatform())this.native=(await import('@capacitor-community/bluetooth-le')).BleClient;
    if(this.native)await this.native.initialize({androidNeverForLocation:true});
  }
  async findRadios(){
    await this.initialize();this.devices.clear();
    if(this.native){
      try{
        await this.native.requestLEScan({services:[SERVICE]},result=>{
          const device=result.device,name=result.localName||device?.name;if(!device||!/^mes(?:h)?it-/i.test(name||''))return;
          this.devices.set(device.deviceId,{id:device.deviceId,name:name.replace(/^meshit-/i,'meshit-'),device});
        });
        await new Promise(resolve=>{this.finishScan=resolve;this.scanTimer=setTimeout(resolve,this.scanMs);});
      }finally{clearTimeout(this.scanTimer);this.finishScan=null;await this.native.stopLEScan();}
    }else{
      const bluetooth=this.bluetooth||globalThis.navigator?.bluetooth;
      if(!bluetooth)throw new Error('Use the mobile companion or a browser with Bluetooth support.');
      const device=await bluetooth.requestDevice({filters:[{services:[SERVICE],namePrefix:'mesit-'},{services:[SERVICE],namePrefix:'meshit-'},{services:[SERVICE],namePrefix:'Meshit-'}]});
      this.devices.set(device.id,{id:device.id,name:device.name?.replace(/^meshit-/i,'meshit-')||'meshit radio',device});
    }
    return [...this.devices.values()];
  }
  onData(bytes){
    this.buffer+=this.decoder.decode(bytes,{stream:true});
    if(this.buffer.length>32768){this.fail(new Error('Radio response too large'));return;}
    let end;while((end=this.buffer.indexOf('\n'))>=0){const line=this.buffer.slice(0,end);this.buffer=this.buffer.slice(end+1);try{const response=JSON.parse(line);const waiter=this.waiting.get(response.id);if(waiter){this.waiting.delete(response.id);clearTimeout(waiter.timer);if(response.error)waiter.reject(new Error(response.error));else waiter.resolve(response);}}catch{this.fail(new Error('Invalid radio response'));}}
  }
  fail(error){++this.generation;for(const p of this.waiting.values()){clearTimeout(p.timer);p.reject(error);}this.waiting.clear();this.buffer='';this.decoder=new TextDecoder();}
  async connect(id){
    await this.initialize();const selected=this.devices.get(id);if(!selected)throw new Error('Find radios and select a meshit radio first.');
    this.device=selected.device;const generation=this.generation;
    const disconnected=()=>{if(generation!==this.generation)return;this.fail(new Error('Bluetooth disconnected'));this.onDisconnect('Bluetooth disconnected. Reconnect in Settings.');};
    try{
      if(this.native){
        await this.native.connect(id,disconnected,{timeout:30000});
        await this.native.startNotifications(id,SERVICE,TX,value=>{if(generation===this.generation)this.onData(new Uint8Array(value.buffer,value.byteOffset,value.byteLength));});
      }else{
        this.disconnectListener=disconnected;this.device.addEventListener('gattserverdisconnected',disconnected);
        const server=await this.device.gatt.connect(),service=await server.getPrimaryService(SERVICE);this.rx=await service.getCharacteristic(RX);this.tx=await service.getCharacteristic(TX);
        this.notificationListener=e=>{if(generation===this.generation)this.onData(new Uint8Array(e.target.value.buffer,e.target.value.byteOffset,e.target.value.byteLength));};
        this.tx.addEventListener('characteristicvaluechanged',this.notificationListener);await this.tx.startNotifications();
      }
      if(generation!==this.generation)throw changed();this.label=selected.name;
    }catch(error){await this.close();throw error;}
  }
  request(data){const generation=this.generation,run=()=>{if(generation!==this.generation)throw changed();return this.exchange(data);};const pending=this.serial.then(run,run);this.serial=pending.catch(()=>{});return pending;}
  async exchange(data){
    const id=this.nextId++,bytes=new TextEncoder().encode(JSON.stringify({...data,id})+'\n');
    return new Promise((resolve,reject)=>{
      const generation=this.generation,timer=setTimeout(()=>{this.waiting.delete(id);reject(new Error('Radio response timed out'));},30000);this.waiting.set(id,{resolve,reject,timer});
      (async()=>{for(let at=0;at<bytes.length;at+=20){if(generation!==this.generation)throw changed();const part=bytes.slice(at,at+20);if(this.native)await this.native.write(this.device.deviceId,SERVICE,RX,new DataView(part.buffer),{timeout:30000});else await this.rx.writeValueWithResponse(part);}})().catch(error=>{clearTimeout(timer);this.waiting.delete(id);reject(error);});
    });
  }
  async close(){
    this.fail(new Error('Connection closed'));this.finishScan?.();
    if(this.device&&this.disconnectListener)this.device.removeEventListener('gattserverdisconnected',this.disconnectListener);
    if(this.tx&&this.notificationListener)this.tx.removeEventListener('characteristicvaluechanged',this.notificationListener);
    if(this.native&&this.device)await this.native.disconnect(this.device.deviceId);else this.device?.gatt?.disconnect();
    this.device=null;this.rx=null;this.tx=null;
  }
}
// Form8-style Wi-Fi/Bluetooth selection, with a generation guard so an old
// radio's late response can never be committed after switching connections.
export class RadioConnection {
  constructor({mode='wifi',wifiBase='',http=base=>new HttpTransport(base),ble=()=>new BleTransport()}={}){
    this.mode=mode==='ble'?'ble':'wifi';this.wifiBase=wifiBase;this.http=http;this.ble=ble;
    this.link=this.mode==='wifi'?http(wifiBase):null;this.discovery=null;this.devices=[];this.generation=0;this.onDisconnect=()=>{};
  }
  async disconnect(){
    ++this.generation;const link=this.link,discovery=this.discovery;this.link=null;this.discovery=null;
    await Promise.all([link?.close(),discovery?.close()]);
  }
  async changeMode(mode){
    if(!['wifi','ble'].includes(mode))throw new Error('Choose Wi-Fi or Bluetooth.');
    await this.disconnect();this.mode=mode;this.devices=[];if(mode==='wifi')this.link=this.http(this.wifiBase);
  }
  async findRadios(){
    if(this.mode!=='ble')throw new Error('Choose Bluetooth first.');
    await this.discovery?.close();this.devices=[];this.discovery=this.ble();const generation=this.generation;
    const devices=await this.discovery.findRadios();if(generation!==this.generation)throw changed();return this.devices=devices;
  }
  async connect(id){
    if(this.mode!=='ble'||!this.devices.some(d=>d.id===id))throw new Error('Find radios and select one first.');
    const next=this.discovery||this.ble();if(next.devices instanceof Map)next.devices=new Map(this.devices.map(d=>[d.id,d]));this.discovery=null;await this.disconnect();const generation=this.generation;
    next.onDisconnect=message=>{if(this.link!==next)return;++this.generation;this.link=null;this.onDisconnect(message);};
    try{
      await next.connect(id);if(generation!==this.generation)throw changed();
      const state=await next.request({op:'state'});if(generation!==this.generation)throw changed();
      if(!state?.settings||!Array.isArray(state.presets))throw new Error('Not a compatible meshit radio. Update its firmware.');
      this.link=next;
    }catch(error){await next.close();throw error;}
  }
  async request(data){
    const generation=this.generation,link=this.link;if(!link)throw new Error('Radio disconnected. Find and connect to a radio in Settings.');
    try{const result=await link.request(data);if(generation!==this.generation||link!==this.link)throw changed();return result;}
    catch(error){if(generation!==this.generation||link!==this.link)throw changed();throw error;}
  }
}
