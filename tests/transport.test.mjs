import test from 'node:test';
import assert from 'node:assert/strict';
import {BleTransport,HttpTransport} from '../web/transport.js';
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
