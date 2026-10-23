import test from 'node:test';
import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {createInterface} from 'node:readline';
const delay=ms=>new Promise(resolve=>setTimeout(resolve,ms));
test('companion RPC setup, rooms, posts, replies, votes and implicit checks use the shared core',async t=>{
  const sim=spawn('./output/simulator',[],{stdio:['pipe','pipe','inherit']}),waiting=[];
  t.after(()=>sim.kill());createInterface({input:sim.stdout}).on('line',line=>waiting.shift()?.(JSON.parse(line)));
  const rpc=data=>new Promise(resolve=>{waiting.push(resolve);sim.stdin.write(JSON.stringify(data)+'\n');});
  const initial=await rpc({op:'state'});assert.equal(initial.ready,false);assert.deepEqual(initial.settings,{txEnabled:true});assert.equal(initial.demo,true);assert.ok(initial.presets.some(p=>p.name==='MeshOregon'&&p.frequency===918.5));
  assert.ok((await rpc({op:'post',room:'general',text:'before setup'})).error);
  assert.equal((await rpc({op:'settings',network:'meshtastic',preset:'mt-us-meshoregon',nickname:'Test',channel:'ignored-legacy-name'})).ok,true);
  assert.equal((await rpc({op:'state'})).settings.channel,undefined);
  const posted=await rpc({op:'post',room:'campfire',text:'hello mesh',parent:''});assert.equal(posted.ok,true);assert.ok(posted.messageId);
  let page=await rpc({op:'messages',offset:0});const parent=page.messages[0];assert.equal(parent.id,posted.messageId);assert.ok((await rpc({op:'resend',target:parent.id})).error);assert.equal(parent.nickname,'Test');assert.equal(parent.room,'campfire');assert.equal(parent.acknowledged,false);
  assert.equal((await rpc({op:'post',room:'campfire',text:'a reply',parent:parent.id})).ok,true);
  page=await rpc({op:'messages',offset:0});const reply=page.messages[1];assert.equal(reply.parent,parent.id);
  assert.equal((await rpc({op:'vote',target:reply.id,up:true})).ok,true);page=await rpc({op:'messages',offset:0});assert.equal(page.messages[1].votes,1);
  assert.equal((await rpc({op:'vote',target:reply.id,up:false})).ok,true);page=await rpc({op:'messages',offset:0});assert.equal(page.messages[1].votes,0);
  assert.ok((await rpc({op:'post',room:'campfire',text:'x'.repeat(161)})).error);
  await delay(800);await rpc({op:'state'});await delay(1200);page=await rpc({op:'messages',offset:0});assert.equal(page.messages[0].acknowledged,true);
});

test('transmit toggle keeps reads available and blocks posts, votes and resends',async t=>{
  const sim=spawn('./output/simulator',[],{stdio:['pipe','pipe','inherit']}),waiting=[];
  t.after(()=>sim.kill());createInterface({input:sim.stdout}).on('line',line=>waiting.shift()?.(JSON.parse(line)));
  const rpc=data=>new Promise(resolve=>{waiting.push(resolve);sim.stdin.write(JSON.stringify(data)+'\n');});
  assert.equal((await rpc({op:'settings',network:'meshtastic',preset:'mt-us-longfast'})).ok,true);
  const post=await rpc({op:'post',room:'general',text:'Before disabling'});assert.equal(post.ok,true);
  assert.equal((await rpc({op:'txEnabled',enabled:false})).ok,true);
  const state=await rpc({op:'state'});assert.equal(state.ready,true);assert.equal(state.settings.txEnabled,false);
  for(const request of [{op:'post',room:'general',text:'Blocked'},{op:'vote',target:post.messageId,up:true},{op:'resend',target:post.messageId}])assert.match((await rpc(request)).error,/Transmit is disabled/);
  assert.equal((await rpc({op:'messages'})).messages.length,1);
  assert.ok((await rpc({op:'txEnabled',enabled:'false'})).error);assert.equal((await rpc({op:'state'})).settings.txEnabled,false);
  assert.equal((await rpc({op:'txEnabled',enabled:true})).ok,true);assert.equal((await rpc({op:'post',room:'general',text:'Enabled again'})).ok,true);
});
