import {HttpTransport,BleTransport} from './transport.js';
const $=id=>document.getElementById(id),utf8=s=>new TextEncoder().encode(s).length;
const systemTheme=matchMedia('(prefers-color-scheme: dark)');
function updateTheme(){
  const dark=(document.documentElement.dataset.theme|| (systemTheme.matches?'dark':'light'))==='dark';
  $('appearance').value=document.documentElement.dataset.theme||'system';
  document.querySelector('meta[name="theme-color"]').content=dark?'#151515':'#fafafa';
}
$('appearance').onchange=()=>{
  const theme=$('appearance').value;
  if(theme==='system')delete document.documentElement.dataset.theme;
  else document.documentElement.dataset.theme=theme;
  try{localStorage.setItem('meshit.theme',theme);}catch{}
  updateTheme();
};
systemTheme.addEventListener('change',updateTheme);
updateTheme();
let transport=new HttpTransport(globalThis.Capacitor?.isNativePlatform()?'http://meshit.local':''),state=null,messages=[],room=localStorage.getItem('meshit.room')||'general',sort='new',reply='',view='chat',loadedRevision=0,busy=false,polling=false;
function saved(key,fallback){try{return JSON.parse(localStorage.getItem(key))??fallback;}catch{return fallback;}}
let draftRooms=saved('meshit.rooms',[]).filter(s=>typeof s==='string').slice(0,20);
const notify=(text='')=>{$('notice').textContent=text;};
function element(tag,cls,text){const e=document.createElement(tag);if(cls)e.className=cls;if(text!==undefined)e.textContent=text;return e;}
function show(which){view=which;$('chat-view').hidden=which!=='chat';$('settings-view').hidden=which!=='settings';$('chat-tab').setAttribute('aria-current',which==='chat'?'page':'false');$('settings-tab').setAttribute('aria-current',which==='settings'?'page':'false');}
function chooseRoom(name){localStorage.setItem(`meshit.draft.${room}`,JSON.stringify({text:$('message').value,parent:reply}));room=name;localStorage.setItem('meshit.room',room);reply='';$('message').value='';try{const d=JSON.parse(localStorage.getItem(`meshit.draft.${room}`)||'{}');$('message').value=d.text||'';reply=d.parent||'';}catch{}render();count();}
function regions(prefix,network){const select=$(prefix+'region');select.replaceChildren();for(const region of new Set(state.presets.filter(p=>p.network===network).map(p=>p.region))){const o=element('option',null,region==='US'?'US / Canada':region.replace('_',' '));o.value=region;select.append(o);}presets(prefix,network);}
function presets(prefix,network){const select=$(prefix+'preset');select.replaceChildren();for(const p of state.presets.filter(p=>p.network===network&&p.region===$(prefix+'region').value)){const o=element('option',null,p.name);o.value=p.id;select.append(o);}details(prefix);}
function details(prefix){const p=state.presets.find(p=>p.id===$(prefix+'preset').value);if(!p)return;$(prefix==='setup-'?'setup-detail':'preset-detail').textContent=`${p.frequency} MHz · BW ${p.bandwidth} kHz · SF${p.sf} · CR 4/${p.cr}`;if(!prefix){$('channel-label').hidden=p.network!=='meshtastic';$('channel').value=p.channel;}}
function settingsUI(){
  const s=state.settings;$('nickname').value=s.nickname||'';$('ssid').value=s.ssid||'';$('wifi-settings').hidden=state.board!=='Heltec V3';$('radio-info').textContent=`${state.board} · ${state.ready?'radio ready':s.configured?'radio unavailable':'setup needed'}`;
  const network=s.network||'meshtastic';document.querySelector(`[name=network][value=${network}]`).checked=true;regions('',network);
  const p=state.presets.find(p=>p.id===s.preset);if(p){$('region').value=p.region;presets('',network);$('preset').value=p.id;details('');}$('channel').value=s.channel||p?.channel||'LongFast';
}
function render(){
  const rooms=new Set([...messages.map(m=>m.room),...draftRooms]);if(!rooms.size)rooms.add('general');if(!rooms.has(room)){room=[...rooms][0];reply='';}
  $('rooms').replaceChildren();for(const name of [...rooms].sort()){const b=element('button','room'+(name===room?' selected':''));b.append(element('span',null,'# '+name),element('span',null,String(messages.filter(m=>m.room===name).length)));b.onclick=()=>chooseRoom(name);$('rooms').append(b);}
  $('room-title').textContent='# '+room;$('feed').replaceChildren();
  const list=messages.filter(m=>m.room===room),byId=new Map(list.map(m=>[m.id,m]));
  const sorted=items=>items.sort((a,b)=>sort==='top'?(b.votes-a.votes||b.order-a.order):b.order-a.order);
  function post(m,depth=0){
    const row=element('article','post'),vote=element('div','vote'),up=element('button',null,'↑');up.setAttribute('aria-label',m.voted?'Remove upvote':'Upvote');up.setAttribute('aria-pressed',String(m.voted));up.onclick=()=>act(async()=>{await transport.request({op:'vote',target:m.id,up:!m.voted});loadedRevision=0;await refresh();});vote.append(up,element('span',null,String(m.votes)));
    const content=element('div'),meta=element('div','meta');meta.append(element('strong',null,m.nickname||'Anonymous'));if(m.mine)meta.append(element('span',null,'you'));if(m.acknowledged){const ack=element('span','ack','✓');ack.title='Your message was heard rebroadcast by another radio';ack.setAttribute('aria-label',ack.title);meta.append(ack);}
    const actions=element('div','post-actions'),button=element('button',null,'↳ Reply');button.onclick=()=>{reply=m.id;count();$('message').focus();$('compose').scrollIntoView({behavior:'smooth',block:'center'});};actions.append(button);if(m.mine&&!m.acknowledged)actions.append(element('span',null,m.status));
    content.append(meta,element('p','body',m.text),actions);
    const children=sorted(list.filter(child=>child.parent===m.id));if(children.length&&depth<32){const box=element('div','children');for(const child of children)box.append(post(child,depth+1));content.append(box);}row.append(vote,content);return row;
  }
  const roots=sorted(list.filter(m=>!m.parent||!byId.has(m.parent)));for(const m of roots)$('feed').append(post(m));
  if(!roots.length){const empty=element('div','empty');empty.append(element('h2',null,'No messages'));$('feed').append(empty);}
  count();
}
function count(){const n=Array.from($('message').value).length;$('char-count').textContent=`${n} / 160`;$('send').disabled=busy||n>160||!$('message').value.trim()||!state?.ready;
  const parent=messages.find(m=>m.id===reply);if(reply&&!parent)reply='';$('reply-context').hidden=!reply;$('cancel-reply').hidden=!reply;$('compose-label').textContent=reply?'Reply':'Message';$('reply-context').textContent=parent?`${parent.nickname||'Anonymous'}: ${parent.text.slice(0,70)}`:'';$('send').textContent=reply?'Reply':'Post';}
async function act(action){if(busy)return;busy=true;count();try{notify();await action();}catch(e){notify(e.message);}finally{busy=false;count();}}
async function refresh(){
  if(polling)return;polling=true;try{
    const next=await transport.request({op:'state'}),first=!state;state=next;
    if(next.revision!==loadedRevision){const result=[];let version=next.revision;for(let offset=0;offset<next.count;offset+=8){const p=await transport.request({op:'messages',offset});if(p.revision!==version){version=0;break;}result.push(...p.messages);}if(version){messages=result;loadedRevision=version;draftRooms=draftRooms.filter(r=>!messages.some(m=>m.room===r));localStorage.setItem('meshit.rooms',JSON.stringify(draftRooms));localStorage.setItem('meshit.cache',JSON.stringify({state:next,messages:messages.slice(-200)}));render();}}
    $('connection').textContent=`${next.settings.network||'Choose a network'} · ${next.ready?'connected':'radio not ready'}${next.demo?' · SIMULATOR':''}`;
    if(first)settingsUI();if(!next.settings.configured&&!$('onboarding').open&&view==='chat')$('onboarding').showModal();
    if(!next.storageReady||!next.identityReady)notify('Radio storage is unavailable. Sending is disabled until it is repaired.');
  }catch(e){$('connection').textContent='Radio disconnected · showing saved messages';if(state){state.ready=false;count();}else{show('settings');notify('Connect to a Meshit radio to begin. '+e.message);}}finally{polling=false;}
}
$('chat-tab').onclick=()=>show('chat');$('settings-tab').onclick=()=>{if(state)settingsUI();show('settings');};
$('message').oninput=()=>{count();localStorage.setItem(`meshit.draft.${room}`,JSON.stringify({text:$('message').value,parent:reply}));};
$('cancel-reply').onclick=()=>{reply='';count();};
$('compose').onsubmit=e=>{e.preventDefault();act(async()=>{if(Array.from($('message').value).length>160)throw new Error('Use up to 160 characters');await transport.request({op:'post',room,text:$('message').value,parent:reply});$('message').value='';reply='';localStorage.removeItem(`meshit.draft.${room}`);loadedRevision=0;await refresh();});};
$('sort-new').onclick=()=>{sort='new';$('sort-new').setAttribute('aria-pressed','true');$('sort-top').setAttribute('aria-pressed','false');render();};$('sort-top').onclick=()=>{sort='top';$('sort-new').setAttribute('aria-pressed','false');$('sort-top').setAttribute('aria-pressed','true');render();};
$('new-room').onclick=()=>{$('room-name').value='';$('room-dialog').showModal();};$('room-cancel').onclick=()=>$('room-dialog').close();
$('room-form').onsubmit=e=>{e.preventDefault();const name=$('room-name').value.trim().toLowerCase();if(!name||utf8(name)>16||/[\x00-\x1f]/.test(name))return notify('Use a room name of up to 16 UTF-8 bytes');if(draftRooms.length>=20)return notify('Post in an existing empty room first');draftRooms.push(name);localStorage.setItem('meshit.rooms',JSON.stringify(draftRooms));$('room-dialog').close();chooseRoom(name);};
for(const input of document.querySelectorAll('[name=network]'))input.onchange=()=>regions('',input.value);
$('region').onchange=()=>presets('',document.querySelector('[name=network]:checked').value);$('preset').onchange=()=>details('');
for(const input of document.querySelectorAll('[name=setup-network]'))input.onchange=()=>{$('setup-options').hidden=false;regions('setup-',input.value);};
$('setup-region').onchange=()=>presets('setup-',document.querySelector('[name=setup-network]:checked').value);$('setup-preset').onchange=()=>details('setup-');
$('setup-form').onsubmit=e=>{e.preventDefault();act(async()=>{const network=document.querySelector('[name=setup-network]:checked')?.value;if(!network||!$('setup-preset').value)throw new Error('Choose a network and preset');await transport.request({op:'settings',network,preset:$('setup-preset').value});$('onboarding').close();loadedRevision=0;await refresh();settingsUI();show('chat');});};
$('setup-connect').onclick=()=>{$('onboarding').close();show('settings');};
$('settings-form').onsubmit=e=>{e.preventDefault();act(async()=>{if(utf8($('nickname').value)>16)throw new Error('Nickname must fit 16 UTF-8 bytes');const data={op:'settings',network:document.querySelector('[name=network]:checked').value,preset:$('preset').value,nickname:$('nickname').value,channel:$('channel').value};const wifiChanged=$('ssid').value!==(state.settings.ssid||'')||$('password').value.length;if(wifiChanged){data.ssid=$('ssid').value;data.password=$('password').value;}await transport.request(data);loadedRevision=0;await refresh();notify(wifiChanged?'Saved. Restart the radio to join Wi-Fi, then open meshit.local.':'Settings saved.');$('password').value='';});};
$('transport').onchange=()=>{$('address-label').hidden=$('transport').value!=='wifi';};
$('connect').onclick=()=>act(async()=>{let next;const kind=$('transport').value;if(kind==='ble'){next=new BleTransport();await next.connect();}else{let base=kind==='wifi'?$('address').value.trim():globalThis.Capacitor?.isNativePlatform()?'http://meshit.local':'';if(base){const u=new URL(base);if(u.protocol!=='http:'&&u.protocol!=='https:')throw new Error('Use an HTTP radio address');base=u.origin;}next=new HttpTransport(base);}await transport.close();transport=next;state=null;loadedRevision=0;await refresh();if(state){settingsUI();show(state.settings.configured?'chat':'settings');}});
const cache=saved('meshit.cache',null);if(cache?.state&&Array.isArray(cache.messages)){state=cache.state;state.ready=false;messages=cache.messages.slice(-200);}
show('chat');chooseRoom(room);await refresh();if(state)settingsUI();setInterval(()=>{if(!busy)refresh();},3000);
