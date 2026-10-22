import test from 'node:test';
import assert from 'node:assert/strict';
import {BleTransport,HttpTransport,RadioConnection,SERVICE} from '../web/transport.js';
import {spawn} from 'node:child_process';
import {createInterface} from 'node:readline';

test('BLE preserves UTF-8 across notification boundaries and serializes requests',async()=>{
  const link=new BleTransport(),decoder=new TextDecoder();let buffer='',writes=0;
  link.rx={async writeValueWithResponse(part){assert.ok(part.length<=20);writes++;buffer+=decoder.decode(part,{stream:true});if(buffer.endsWith('\n')){const request=JSON.parse(buffer);buffer='';const response=new TextEncoder().encode(JSON.stringify({id:request.id,text:'Hello 🙂 café',operation:request.op})+'\n');for(const byte of response)link.onData(Uint8Array.of(byte));}}};
  const [one,two]=await Promise.all([link.request({op:'post',text:'🙂'.repeat(160)}),link.request({op:'state'})]);
  assert.equal(one.text,'Hello 🙂 café');assert.equal(one.operation,'post');assert.equal(two.operation,'state');assert.ok(writes>30);assert.equal(link.waiting.size,0);
});
test('BLE errors and disconnects reject outstanding operations',async()=>{
  const link=new BleTransport();link.rx={async writeValueWithResponse(bytes){const request=JSON.parse(new TextDecoder().decode(bytes));link.onData(new TextEncoder().encode(JSON.stringify({id:request.id,error:'queue full'})+'\n'));}};
  await assert.rejects(link.request({op:'x'}),/queue full/);
  link.rx={async writeValueWithResponse(){link.fail(new Error('Disconnected'));}};await assert.rejects(link.request({op:'x'}),/Disconnected/);
});
test('HTTP forwards structured commands and surfaces firmware errors',async()=>{
  const previous=globalThis.fetch;globalThis.fetch=async(url,options)=>{assert.equal(url,'http://meshit.local/api/rpc');assert.equal(JSON.parse(options.body).op,'state');return {ok:true,json:async()=>({error:'radio offline'})};};
  try{await assert.rejects(new HttpTransport('http://meshit.local/').request({op:'state'}),/radio offline/);}finally{globalThis.fetch=previous;}
});

test('BLE discovery filters meshit radios and uses explicit selected-device connections',async()=>{
  const calls=[];
  const client={
    async initialize(){calls.push('initialize');},
    async requestLEScan(options,found){assert.deepEqual(options.services,[SERVICE]);found({device:{deviceId:'other',name:'Form8-test'}});found({device:{deviceId:'one',name:'meshit-one'}});found({device:{deviceId:'two',name:'Meshit-two'}});found({device:{deviceId:'three',name:'mesit-00af'}});},
    async stopLEScan(){calls.push('stop scan');},
    async connect(id,disconnected){calls.push('connect '+id);this.disconnected=disconnected;},
    async startNotifications(id,service,tx,callback){calls.push('notify '+id);this.notification=callback;},
    async disconnect(id){calls.push('disconnect '+id);}
  };
  const link=new BleTransport({client,scanMs:0});
  const devices=await link.findRadios();assert.deepEqual(devices.map(d=>d.id),['one','two','three']);
  await link.connect('two');assert.equal(link.label,'meshit-two');assert.ok(calls.includes('connect two'));
  let disconnected='';link.onDisconnect=message=>disconnected=message;client.disconnected();assert.match(disconnected,/disconnected/);
  await link.close();assert.ok(calls.includes('disconnect two'));
});
test('closing BLE cancels queued requests instead of sending to the next radio',async()=>{
  const link=new BleTransport();let wire='';link.rx={async writeValueWithResponse(bytes){wire+=new TextDecoder().decode(bytes);}};
  const first=link.request({op:'state'}),second=link.request({op:'post',text:'do not send'});
  const failures=Promise.all([assert.rejects(first,/closed/),assert.rejects(second,/changed/)]);
  await new Promise(resolve=>setTimeout(resolve,0));await link.close();await failures;assert.equal(JSON.parse(wire).op,'state');
});
test('Wi-Fi/Bluetooth switches reject a late response from the previous radio',async()=>{
  let finish,closed=false;
  const old={request:()=>new Promise(resolve=>finish=resolve),async close(){closed=true;}};
  const connection=new RadioConnection({http:()=>old});
  const request=connection.request({op:'state'}),failure=assert.rejects(request,error=>error.code==='CONNECTION_CHANGED');
  await connection.changeMode('ble');finish({board:'old'});await failure;assert.equal(closed,true);
  await assert.rejects(connection.request({op:'post'}),/disconnected/);
});
test('Form8-style find, connect, disconnect and reconnect use a verified radio handshake',async()=>{
  const created=[];
  const ble=()=>{const link={devices:new Map(),async findRadios(){return [{id:'radio',name:'meshit-radio'}];},async connect(id){assert.equal(id,'radio');},async request(data){assert.equal(data.op,'state');return {settings:{},presets:[],board:'Heltec V3'};},async close(){this.closed=true;}};created.push(link);return link;};
  const connection=new RadioConnection({mode:'ble',ble});
  await connection.findRadios();await connection.connect('radio');assert.equal((await connection.request({op:'state'})).board,'Heltec V3');
  await connection.disconnect();assert.equal(created[0].closed,true);
  await connection.connect('radio');assert.equal(created.length,2);
  let disconnected='';connection.onDisconnect=message=>disconnected=message;connection.link.onDisconnect('lost');assert.equal(disconnected,'lost');assert.equal(connection.link,null);
});
test('failed Bluetooth handshake closes the candidate and keeps sending disabled',async()=>{
  let closed=false;const candidate={async findRadios(){return [{id:'one'}];},async connect(){},async request(){return {wrong:'protocol'};},async close(){closed=true;}};
  const connection=new RadioConnection({mode:'ble',ble:()=>candidate});await connection.findRadios();
  await assert.rejects(connection.connect('one'),/compatible meshit/);assert.equal(closed,true);assert.equal(connection.link,null);
});

test('browser chooser accepts MAC-based mesit names and legacy radios',async()=>{
  const link=new BleTransport({bluetooth:{async requestDevice({filters}){
    assert.deepEqual(filters.map(f=>f.namePrefix),['mesit-','meshit-','Meshit-']);
    assert.ok(filters.every(f=>f.services.includes(SERVICE)));
    return {id:'mac-radio',name:'mesit-00af'};
  }}});
  const devices=await link.findRadios();assert.equal(devices[0].name,'mesit-00af');
});
