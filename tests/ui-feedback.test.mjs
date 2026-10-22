import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import vm from 'node:vm';
class Element {
  constructor(){this.children=[];this.value='';this.text='';this.hidden=false;this.disabled=false;this.dataset={};}
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
  let feed=[],revision=1,resolvePost,rejectPost;
  class Radio {
    constructor(){this.mode='wifi';this.devices=[];this.link={};this.generation=1;}
    async request(req){
      if(req.op==='state')return {node:'test',ready:true,storageReady:true,identityReady:true,revision,count:feed.length,settings:{configured:true,nickname:'Tester'},presets:[]};
      if(req.op==='messages')return {revision,messages:feed};
      if(req.op==='post')return new Promise((resolve,reject)=>{resolvePost=resolve;rejectPost=reject;});
    }
  }
  const source=(await readFile(new URL('../web/app.js',import.meta.url),'utf8')).replace("import {RadioConnection} from './transport.js';",'const RadioConnection=Radio;');
  const context={Radio,TextEncoder,console,document:{documentElement:new Element(),getElementById:get,createElement:()=>new Element(),querySelector:get,querySelectorAll:()=>[],addEventListener(){}},localStorage:{getItem:()=>null,setItem(){},removeItem(){}},matchMedia:()=>({matches:false,addEventListener(){}}),setInterval(){}};
  await vm.runInNewContext('(async()=>{'+source+'})()',context);
  get('message').value='Hello radio';get('message').oninput();get('compose').onsubmit({preventDefault(){}});
  assert.match(get('feed').textContent,/Hello radio/);assert.equal(get('notice').textContent,'Sending message…');assert.equal(get('send').disabled,true);
  rejectPost(new Error('Connection timed out'));await tick();
  assert.match(get('feed').textContent,/Hello radio/);assert.match(get('feed').textContent,/↻/);assert.match(get('notice').textContent,/timed out/);
  const walk=e=>[e,...e.children.flatMap(walk)];const retry=walk(get('feed')).find(e=>e.className==='delivery');
  retry.onclick();await tick();assert.equal(get('notice').textContent,'Resending message…');
  feed=[{id:'abc',parent:'',room:'general',text:'Hello radio',nickname:'Tester',mine:true,votes:0,order:1,status:'sending'}];revision++;
  resolvePost({ok:true,messageId:'abc'});await tick();
  assert.equal((get('feed').textContent.match(/Hello radio/g)||[]).length,1);
  feed[0].acknowledged=true;feed[0].status='rebroadcast heard';
  get('sort-new').onclick();assert.match(get('feed').textContent,/✓/);assert.doesNotMatch(get('feed').textContent,/↻/);
});
