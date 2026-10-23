import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import vm from 'node:vm';
class Element {
  constructor(){this.children=[];this.value='';this.text='';this.hidden=false;this.disabled=false;this.dataset={};this.style={};this.scrollHeight=40;}
  set textContent(value){this.text=value;this.children=[];}
  get textContent(){return this.text+this.children.map(e=>e.textContent).join('');}
  append(...items){this.children.push(...items);}
  replaceChildren(...items){this.children=items;this.text='';}
  setAttribute(name,value){this[name]=value;}
  focus(){} scrollIntoView(){} showModal(){this.open=true;} close(){this.open=false;}
}
const tick=()=>new Promise(resolve=>setImmediate(resolve));
test('post appears before RPC resolves; errors retain text with retry; acknowledgment replaces retry',async()=>{
  const elements=new Map(),get=id=>{if(!elements.has(id))elements.set(id,new Element());return elements.get(id);};
  let feed=[],revision=1,resolvePost,rejectPost,poll,offline=false,waitState,waitMessages,churn=false;const viewed=[];let visibilityChanged;
  class Radio {
    constructor(){this.mode='wifi';this.devices=[];this.link={};this.generation=1;}
    async request(req){
      if(req.op==='viewed'){viewed.push(req);return {ok:true};}
      if(req.op==='state'&&waitState)await waitState;
      if(req.op==='messages'&&waitMessages)await waitMessages;
      if(req.op==='state'&&offline)throw new Error('Connection lost');
      if(req.op==='state')return {viewedSupported:true,node:'test',ready:true,storageReady:true,identityReady:true,revision,count:feed.length,settings:{configured:true,nickname:'Tester'},board:'Heltec V4.3',wifiSupported:true,presets:[]};
      if(req.op==='messages'){if(churn)revision++;return {revision,messages:feed.slice(req.offset,req.offset+8)};}
      if(req.op==='resend'){feed.find(m=>m.id===req.target).status='sending';revision++;return {ok:true};}
      if(req.op==='post')return new Promise((resolve,reject)=>{resolvePost=resolve;rejectPost=reject;});
    }
  }
  const source=(await readFile(new URL('../web/app.js',import.meta.url),'utf8')).replace("import {RadioConnection} from './transport.js';",'const RadioConnection=Radio;');
  const context={Radio,TextEncoder,console,document:{documentElement:new Element(),getElementById:get,createElement:()=>new Element(),querySelector:get,querySelectorAll:()=>[],addEventListener(name,callback){if(name==='visibilitychange')visibilityChanged=callback;}},localStorage:{getItem:()=>null,setItem(){},removeItem(){}},matchMedia:()=>({matches:false,addEventListener(){}}),setInterval(callback){poll=callback;}};
  await vm.runInNewContext('(async()=>{'+source+'})()',context);
  assert.deepEqual(JSON.parse(JSON.stringify(viewed)),[{op:'viewed',room:'general',revision:1}]);
  assert.equal(get('wifi-settings').hidden,false,'V4 exposes Wi-Fi configuration');
  assert.equal(get('radio-connection').dataset.connected,'true');
  assert.equal(get('radio-connection')['aria-label'],'Radio connected via Wi-Fi');
  assert.equal(get('message').placeholder,'Write something interesting...');
  assert.equal(get('message').style.height,'40px');
  get('message').scrollHeight=94;get('message').value='Long text';get('message').oninput();
  assert.equal(get('message').style.height,'94px');
  get('message').scrollHeight=40;get('message').value='';get('message').oninput();
  assert.equal(get('message').style.height,'40px');
  get('message').value='Hello radio';get('message').oninput();get('compose').onsubmit({preventDefault(){}});
  assert.match(get('feed').textContent,/Hello radio/);assert.equal(get('notice').textContent,'Sending message…');assert.equal(get('send').disabled,true);
  rejectPost(new Error('Connection timed out'));await tick();
  assert.match(get('feed').textContent,/Hello radio/);assert.match(get('feed').textContent,/↻/);assert.match(get('notice').textContent,/timed out/);
  const walk=e=>[e,...e.children.flatMap(walk)];const retry=walk(get('feed')).find(e=>e.className==='delivery');
  retry.onclick();await tick();assert.equal(get('notice').textContent,'Resending message…');
  feed=[{id:'abc',parent:'',room:'general',text:'Hello radio',nickname:'Tester',mine:true,votes:0,order:1,status:'sending'}];revision++;
  resolvePost({ok:true,messageId:'abc'});await tick();
  assert.equal((get('feed').textContent.match(/Hello radio/g)||[]).length,1);
  assert.equal(get('notice').textContent,'');
  feed[0].acknowledged=true;feed[0].status='rebroadcast heard';revision++;
  poll();await tick();assert.match(get('feed').textContent,/✓/);assert.doesNotMatch(get('feed').textContent,/↻/);
  assert.equal(get('notice').textContent,'');
  // A checkmark on an earlier post must not complete the latest post's notice.
  get('message').value='Second message';get('message').oninput();get('compose').onsubmit({preventDefault(){}});
  feed.push({id:'def',parent:'',room:'general',text:'Second message',mine:true,votes:0,order:2,status:'sending'});revision++;
  resolvePost({ok:true,messageId:'def'});await tick();assert.equal(get('notice').textContent,'');
  feed[1].status='no rebroadcast heard';revision++;poll();await tick();
  assert.equal(get('notice').textContent,'');
  const resend=walk(get('feed')).find(e=>e.className==='delivery'&&e.textContent==='↻');
  resend.onclick();await tick();assert.equal(get('notice').textContent,'');
  feed[1].acknowledged=true;feed[1].status='rebroadcast heard';revision++;poll();await tick();
  assert.equal(get('notice').textContent,'');
  // Later polling must not replace an unrelated error with an old acknowledgment.
  get('message').value='Third message';get('message').oninput();get('compose').onsubmit({preventDefault(){}});
  rejectPost(new Error('Radio queue full'));await tick();
  feed[1].acknowledged=true;revision++;poll();await tick();
  assert.equal(get('notice').textContent,'Radio queue full');
  get('settings-tab').onclick();const before=viewed.length;
  feed.push({id:'incoming',parent:'',room:'general',text:'Unread',mine:false,votes:0,order:3});revision++;
  poll();await tick();assert.equal(viewed.length,before,'Settings polling leaves messages unread');
  get('chat-tab').onclick();await tick();assert.equal(get('notice').textContent,'','returning to chat clears errors');assert.equal(viewed.at(-1).revision,revision);
  assert.equal(viewed.length,before+1,'opening chat marks the loaded room viewed');
  context.document.hidden=true;revision++;poll();await tick();assert.equal(viewed.length,before+1,'hidden client leaves messages unread');
  context.document.hidden=false;visibilityChanged();await tick();assert.equal(viewed.at(-1).revision,revision);
  feed.push({id:'other',parent:'',room:'camp',text:'Other room',mine:false,votes:0,order:4});revision++;poll();await tick();
  assert.equal(viewed.at(-1).room,'general','polling does not mark other rooms viewed');
  get('rooms').children.find(e=>e.textContent.includes('# camp')).onclick();await tick();
  assert.equal(viewed.at(-1).room,'camp','opening another room marks that room viewed');
  // An orphan moves under its recovered parent and master post on the next refresh.
  get('rooms').children.find(e=>e.textContent.includes('# general')).onclick();
  feed.push({id:'reply',parent:'parent',room:'general',text:'Maybe not',mine:false,votes:0,order:5});revision++;
  poll();await tick();assert.ok(get('feed').children.some(e=>e.textContent.includes('Maybe not')));
  feed.push({id:'parent',parent:'master',room:'general',text:'Maybe',mine:false,votes:0,order:6});revision++;
  poll();await tick();
  feed.push({id:'master',parent:'',room:'general',text:'I think this will be a hit',mine:false,votes:0,order:7});revision++;
  poll();await tick();
  const masterRow=get('feed').children.find(e=>e.textContent.includes('I think this will be a hit'));
  const parentRow=masterRow.children.find(e=>e.className==='children').children[0];
  assert.match(parentRow.textContent,/Maybe/);
  assert.match(parentRow.children.find(e=>e.className==='children').children[0].textContent,/Maybe not/);
  assert.equal(get('feed').children.filter(e=>e.textContent.includes('Maybe not')).length,1);
  const replyButton=walk(masterRow).find(e=>e.textContent==='↳ Reply'&&typeof e.onclick==='function');replyButton.onclick();
  assert.equal(get('message').placeholder,'Post a reply');
  assert.equal(get('message')['aria-label'],'Post a reply');
  get('cancel-reply').onclick();assert.equal(get('message').placeholder,'Write something interesting...');
  get('notice').textContent='Previous error';offline=true;poll();await tick();
  assert.equal(get('radio-connection').dataset.connected,'false');
  assert.equal(get('radio-connection')['aria-label'],'Radio disconnected');
  offline=false;let finishState,finishMessages;
  waitState=new Promise(resolve=>finishState=resolve);waitMessages=new Promise(resolve=>finishMessages=resolve);revision++;
  poll();await tick();assert.equal(get('download-status').textContent,'Downloading messages…','show progress while waiting for state');
  finishState();await tick();assert.equal(get('download-status').textContent,'Downloading messages…','keep progress while message pages arrive');
  get('chat-tab').onclick();assert.equal(get('download-status').textContent,'Downloading messages…','tab navigation does not hide an active download');
  finishMessages();await tick();waitState=null;waitMessages=null;
  assert.equal(get('download-status').textContent,'','clear progress after messages load');
  assert.equal(get('radio-connection').dataset.connected,'true');
  assert.equal(get('notice').textContent,'','reconnecting clears stale notices');
  // Incoming traffic must not starve downloads when revisions change between pages.
  const viewedBeforeChurn=viewed.length;
  feed.push({id:'during-download',parent:'',room:'general',text:'Arrived during download',mine:false,votes:0,order:20});revision++;churn=true;
  poll();await tick();
  assert.match(get('feed').textContent,/Arrived during download/,'valid pages appear despite revision changes');
  assert.equal(viewed.length,viewedBeforeChurn,'partial history is not marked viewed');
  assert.equal(get('download-status').textContent,'Downloading messages…');
  churn=false;poll();await tick();
  assert.equal(get('download-status').textContent,'','a stable retry completes the download');
  assert.equal(viewed.at(-1).revision,revision);
  // A reboot can reuse a revision: reconnect must fetch history again anyway.
  offline=true;poll();await tick();
  feed[0]={...feed[0],text:'History after radio restart'};
  offline=false;poll();await tick();
  assert.match(get('feed').textContent,/History after radio restart/);
  // Navigating while a slow action is pending must not restore a completion banner.
  get('message').value='Pending reply';get('message').oninput();get('compose').onsubmit({preventDefault(){}});
  assert.equal(get('notice').textContent,'Sending message…');
  get('rooms').children.find(e=>e.textContent.includes('# camp')).onclick();
  assert.equal(get('notice').textContent,'','room switch clears progress');
  resolvePost({ok:true,messageId:'late'});await tick();
  assert.equal(get('notice').textContent,'','late success does not add a completion banner');


});
