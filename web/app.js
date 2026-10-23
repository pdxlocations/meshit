import {RadioConnection} from './transport.js';
const $=id=>document.getElementById(id),utf8=s=>new TextEncoder().encode(s).length;
try{const theme=localStorage.getItem('meshit.theme');if(theme==='dark'||theme==='light')document.documentElement.dataset.theme=theme;}catch{}
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
const native=!!globalThis.Capacitor?.isNativePlatform();
let rememberedMode='wifi';try{rememberedMode=localStorage.getItem('meshit.connection')==='ble'?'ble':'wifi';}catch{}
const transport=new RadioConnection({mode:rememberedMode,wifiBase:native?'http://192.168.4.1':''});
let state=null,messages=[],room=localStorage.getItem('meshit.room')||'general',sort='top',reply='',view='chat',loadedRevision=0,busy=false,polling=false;
function saved(key,fallback){try{return JSON.parse(localStorage.getItem(key))??fallback;}catch{return fallback;}}
let draftRooms=saved('meshit.rooms',[]).filter(s=>typeof s==='string').slice(0,20);
const outgoing=new Map();
let deliveryNotice=null,radioConnected=false;
const notify=(text='')=>{deliveryNotice=null;$('notice').textContent=text;};
function watchDelivery(id){
  notify();deliveryNotice={id,node:state?.node,revision:loadedRevision};updateDeliveryNotice();
}
function updateDeliveryNotice(){
  if(!deliveryNotice)return;
  if(deliveryNotice.node!==state?.node){notify();return;}
  const m=messages.find(item=>item.id===deliveryNotice.id);if(!m)return;
  if(m.acknowledged)notify();
  else if(m.status!=='sending'&&loadedRevision&&loadedRevision!==deliveryNotice.revision)notify();
}
function element(tag,cls,text){const e=document.createElement(tag);if(cls)e.className=cls;if(text!==undefined)e.textContent=text;return e;}
let viewedKey='';
function markViewed(){
  if(polling||view!=='chat'||document.hidden||!state?.ready||!state.viewedSupported||!transport.link||$('onboarding').open)return;
  if(!state.viewedIdsSupported&&(!loadedRevision||loadedRevision!==state.revision))return;
  const shown=messages.filter(m=>m.room===room).map(m=>m.id),epoch=transport.generation,viewedRoom=room;
  const key=`${epoch}:${state.node}:${state.viewedIdsSupported?shown.join(','):loadedRevision}:${room}`;
  if(viewedKey===key)return;
  viewedKey=key;
  (async()=>{
    if(!state.viewedIdsSupported)return transport.request({op:'viewed',room:viewedRoom,revision:loadedRevision});
    for(let offset=0;offset<shown.length;offset+=32){
      if(epoch!==transport.generation||view!=='chat'||document.hidden||room!==viewedRoom)return {ok:false};
      const result=await transport.request({op:'viewed',room:viewedRoom,messages:shown.slice(offset,offset+32)});if(!result.ok)return result;
    }
    return {ok:true};
  })().then(result=>{if(!result.ok&&viewedKey===key)viewedKey='';}).catch(()=>{if(viewedKey===key)viewedKey='';});
}

function show(which){if(which==='chat')notify();view=which;$('chat-view').hidden=which!=='chat';$('settings-view').hidden=which!=='settings';$('chat-tab').setAttribute('aria-current',which==='chat'?'page':'false');$('settings-tab').setAttribute('aria-current',which==='settings'?'page':'false');resizeComposer();markViewed();}
function chooseRoom(name){notify();localStorage.setItem(`meshit.draft.${room}`,JSON.stringify({text:$('message').value,parent:reply}));room=name;localStorage.setItem('meshit.room',room);reply='';$('message').value='';try{const d=JSON.parse(localStorage.getItem(`meshit.draft.${room}`)||'{}');$('message').value=d.text||'';reply=d.parent||'';}catch{}render();count();}
function regions(prefix,network){const select=$(prefix+'region');select.replaceChildren();for(const region of new Set(state.presets.filter(p=>p.network===network).map(p=>p.region))){const o=element('option',null,region==='US'?'US / Canada':region.replace('_',' '));o.value=region;select.append(o);}presets(prefix,network);}
function presets(prefix,network){const select=$(prefix+'preset');select.replaceChildren();for(const p of state.presets.filter(p=>p.network===network&&p.region===$(prefix+'region').value)){const o=element('option',null,p.name);o.value=p.id;select.append(o);}details(prefix);}
function details(prefix){const p=state.presets.find(p=>p.id===$(prefix+'preset').value);if(!p)return;$(prefix==='setup-'?'setup-detail':'preset-detail').textContent=`${p.frequency} MHz · BW ${p.bandwidth} kHz · SF${p.sf} · CR 4/${p.cr}`;}
function settingsUI(){
  const s=state.settings;$('nickname').value=s.nickname||'';$('ssid').value=s.ssid||'';$('wifi-settings').hidden=!(state.wifiSupported??/^Heltec V[34]/.test(state.board));$('radio-info').textContent=`${state.board} · ${state.ready?'radio ready':s.configured?'radio unavailable':'setup needed'}`;
  const network=s.network||'meshtastic';document.querySelector(`[name=network][value=${network}]`).checked=true;regions('',network);
  const p=state.presets.find(p=>p.id===s.preset);if(p){$('region').value=p.region;presets('',network);$('preset').value=p.id;details('');}
}
function render(){
  for(const [key,m] of outgoing){
    const found=messages.find(item=>item.id===m.id||((m.failed||m.accepted)&&item.mine&&!m.before.includes(item.id)&&item.room===m.room&&item.parent===m.parent&&item.text===m.text));
    if(found){if(deliveryNotice?.id===m.id)deliveryNotice.id=found.id;outgoing.delete(key);}
  }
  updateDeliveryNotice();
  const visible=[...messages,...outgoing.values()];
  const rooms=new Set([...visible.map(m=>m.room),...draftRooms]);if(!rooms.size)rooms.add('general');if(!rooms.has(room)){room=[...rooms][0];reply='';}
  $('rooms').replaceChildren();for(const name of [...rooms].sort()){const b=element('button','room'+(name===room?' selected':''));b.append(element('span',null,'# '+name),element('span',null,String(visible.filter(m=>m.room===name).length)));b.onclick=()=>chooseRoom(name);$('rooms').append(b);}
  $('room-title').textContent='# '+room;$('feed').replaceChildren();
  const list=visible.filter(m=>m.room===room),byId=new Map(list.map(m=>[m.id,m]));
  const sorted=items=>items.sort((a,b)=>sort==='top'?(b.votes-a.votes||b.order-a.order):b.order-a.order);
  function post(m,depth=0){
    const row=element('article','post'),vote=element('div','vote'),up=element('button',null,'↑');up.setAttribute('aria-label',m.voted?'Remove upvote':'Upvote');up.setAttribute('aria-pressed',String(m.voted));up.disabled=busy||!!m.local;up.onclick=()=>act(async()=>{await transport.request({op:'vote',target:m.id,up:!m.voted});loadedRevision=0;await refresh();},m.voted?'Removing upvote…':'Sending upvote…');vote.append(up,element('span',null,String(m.votes)));
    const content=element('div'),meta=element('div','meta');meta.append(element('strong',null,m.nickname||'Anonymous'));if(m.mine)meta.append(element('span',null,'you'));if(m.acknowledged){const ack=element('span','ack','✓');ack.title='Your message was heard rebroadcast by another radio';ack.setAttribute('aria-label',ack.title);meta.append(ack);}else if(m.mine){
      if(m.status==='sending'){const pending=element('span','delivery','…');pending.title='Sending message…';pending.setAttribute('aria-label',pending.title);meta.append(pending);}
      else {const retry=element('button','delivery','↻');retry.title='No acknowledgment received. Resend message';retry.setAttribute('aria-label',retry.title);retry.disabled=busy||!state?.ready;retry.onclick=()=>act(async()=>{
        if(m.local)await sendOutgoing(m);
        else {await transport.request({op:'resend',target:m.id});watchDelivery(m.id);loadedRevision=0;await refresh();}
      },'Resending message…');meta.append(retry);}
    }
    const actions=element('div','post-actions'),button=element('button',null,'↳ Reply');button.disabled=!!m.local;button.onclick=()=>{reply=m.id;count();$('message').focus();$('compose').scrollIntoView({behavior:'smooth',block:'center'});};actions.append(button);if(m.mine&&!m.acknowledged)actions.append(element('span',null,m.status));
    content.append(meta,element('p','body',m.text),actions);
    row.append(vote,content);
    const children=sorted(list.filter(child=>child.parent===m.id));if(children.length&&depth<32){const box=element('div','children');for(const child of children)box.append(post(child,depth+1));row.append(box);}return row;
  }
  const roots=sorted(list.filter(m=>!m.parent||!byId.has(m.parent)));for(const m of roots)$('feed').append(post(m));
  if(!roots.length){const empty=element('div','empty');empty.append(element('h2',null,'No messages'));$('feed').append(empty);}
  count();markViewed();
}
function resizeComposer(){const input=$('message');input.style.height='auto';input.style.height=`${input.scrollHeight}px`;}
function count(){resizeComposer();const n=Array.from($('message').value).length;$('char-count').textContent=`${n} / 160`;$('send').disabled=busy||n>160||!$('message').value.trim()||!state?.ready||state.storageReady===false||state.identityReady===false;
  const parent=messages.find(m=>m.id===reply);if(reply&&!parent)reply='';$('reply-context').hidden=!reply;$('cancel-reply').hidden=!reply;$('message').placeholder=reply?'Post a reply':'Write something interesting...';$('message').setAttribute('aria-label',$('message').placeholder);$('reply-context').textContent=parent?`${parent.nickname||'Anonymous'}: ${parent.text.slice(0,70)}`:'';$('send').textContent=busy?'Sending…':reply?'Reply':'Post';}
async function act(action,label='Working…'){if(busy)return;busy=true;count();connectionControls();notify(label);try{await action();if($('notice').textContent===label)notify();}catch(e){downloadStatus(false);notify(e.message);}finally{busy=false;render();connectionControls();}}
async function refresh(){
  if(polling)return;polling=true;const epoch=transport.generation;if(!radioConnected)downloadStatus(true);try{
    const next=await transport.request({op:'state'});if(epoch!==transport.generation)return;const switched=!!(next.node&&state?.node&&next.node!==state.node),first=!state||switched;if(switched){outgoing.clear();messages=[];loadedRevision=0;reply='';}state=next;setConnected(true);
    if(next.revision!==loadedRevision||next.count!==messages.length){
      downloadStatus(true);loadedRevision=0;const result=[];let stable=true;
      for(let offset=0;offset<next.count;offset+=8){
        const p=await transport.request({op:'messages',offset});if(epoch!==transport.generation)return;
        if(p.revision!==next.revision)stable=false;
        if(!Array.isArray(p.messages))throw new Error('Invalid message response');
        result.push(...p.messages);
        // Keep useful pages even while incoming radio traffic changes the revision.
        // Only a complete, consistent pass may replace history or mark it viewed.
        const merged=new Map(messages.map(m=>[m.id,m]));for(const m of p.messages)merged.set(m.id,m);
        messages=[...merged.values()];render();
      }
      const unique=[...new Map(result.map(m=>[m.id,m])).values()];
      if(stable&&unique.length===next.count){messages=unique;loadedRevision=next.revision;draftRooms=draftRooms.filter(r=>!messages.some(m=>m.room===r));localStorage.setItem('meshit.rooms',JSON.stringify(draftRooms));localStorage.setItem('meshit.cache',JSON.stringify({state:next,messages:messages.slice(-200)}));render();downloadStatus(false);}
      else loadedRevision=0;
    }else downloadStatus(false);
    connectionStatus(`Connected via ${transport.mode==='wifi'?'Wi-Fi':'Bluetooth'}${transport.link?.label?' · '+transport.link.label:''}`);
    $('connection').textContent=`${next.settings.network||'Choose a network'} · ${next.ready?'connected':'radio not ready'}${next.demo?' · SIMULATOR':''}`;
    if(first)settingsUI();if(!next.settings.configured&&!$('onboarding').open&&view==='chat')$('onboarding').showModal();
    markViewed();
    if(!next.storageReady||!next.identityReady)notify('Radio storage is unavailable. Sending is disabled until it is repaired.');
  }catch(e){if(epoch!==transport.generation||e.code==='CONNECTION_CHANGED')return;setConnected(false);connectionStatus(e.message);if(state)state.ready=false;$('connection').textContent='Radio disconnected · showing saved messages';if(state){state.ready=false;count();}else{show('settings');notify('Connect to a meshit radio to begin. '+e.message);}}finally{polling=false;markViewed();}
}
$('chat-tab').onclick=()=>show('chat');$('settings-tab').onclick=()=>{if(state)settingsUI();show('settings');};
$('message').oninput=()=>{count();localStorage.setItem(`meshit.draft.${room}`,JSON.stringify({text:$('message').value,parent:reply}));};
$('cancel-reply').onclick=()=>{reply='';count();};
async function sendOutgoing(m){
  m.status='sending';render();
  try{
    // A lost response may still have stored the post. Reconcile before retrying.
    if(m.failed){loadedRevision=0;await refresh();const found=messages.find(item=>item.mine&&!m.before.includes(item.id)&&item.room===m.room&&item.parent===m.parent&&item.text===m.text);if(found){outgoing.delete(m.key);watchDelivery(found.id);render();return;}}
    const result=await transport.request({op:'post',room:m.room,text:m.text,parent:m.parent});
    m.accepted=true;if(result.messageId){m.id=result.messageId;m.local=false;}watchDelivery(m.id);
    loadedRevision=0;await refresh();
    if(!result.messageId){const found=messages.find(item=>item.mine&&!m.before.includes(item.id)&&item.room===m.room&&item.parent===m.parent&&item.text===m.text);if(found){outgoing.delete(m.key);watchDelivery(found.id);}}
  }catch(e){m.status='not confirmed';m.failed=true;throw e;}finally{render();}
}
$('compose').onsubmit=e=>{e.preventDefault();if(busy||$('send').disabled)return;act(async()=>{
  const text=$('message').value;if(Array.from(text).length>160)throw new Error('Use up to 160 characters');
  const key='local-'+Date.now()+'-'+Math.random().toString(36).slice(2),m={key,id:key,local:true,room,text,parent:reply,nickname:state.settings.nickname||'',mine:true,acknowledged:false,votes:0,voted:false,status:'sending',order:Math.max(0,...messages.map(item=>item.order))+1,before:messages.map(item=>item.id)};
  outgoing.set(key,m);$('message').value='';reply='';localStorage.removeItem(`meshit.draft.${room}`);render();await sendOutgoing(m);
},'Sending message…');};
$('sort-new').onclick=()=>{sort='new';$('sort-new').setAttribute('aria-pressed','true');$('sort-top').setAttribute('aria-pressed','false');render();};$('sort-top').onclick=()=>{sort='top';$('sort-new').setAttribute('aria-pressed','false');$('sort-top').setAttribute('aria-pressed','true');render();};
$('new-room').onclick=()=>{$('room-name').value='';$('room-dialog').showModal();};$('room-cancel').onclick=()=>$('room-dialog').close();
$('room-form').onsubmit=e=>{e.preventDefault();const name=$('room-name').value.trim().toLowerCase();if(!name||utf8(name)>16||/[\x00-\x1f]/.test(name))return notify('Use a room name of up to 16 UTF-8 bytes');if(draftRooms.length>=20)return notify('Post in an existing empty room first');draftRooms.push(name);localStorage.setItem('meshit.rooms',JSON.stringify(draftRooms));$('room-dialog').close();chooseRoom(name);};
for(const input of document.querySelectorAll('[name=network]'))input.onchange=()=>regions('',input.value);
$('region').onchange=()=>presets('',document.querySelector('[name=network]:checked').value);$('preset').onchange=()=>details('');
for(const input of document.querySelectorAll('[name=setup-network]'))input.onchange=()=>{$('setup-options').hidden=false;regions('setup-',input.value);};
$('setup-region').onchange=()=>presets('setup-',document.querySelector('[name=setup-network]:checked').value);$('setup-preset').onchange=()=>details('setup-');
$('setup-form').onsubmit=e=>{e.preventDefault();act(async()=>{const network=document.querySelector('[name=setup-network]:checked')?.value;if(!network||!$('setup-preset').value)throw new Error('Choose a network and preset');await transport.request({op:'settings',network,preset:$('setup-preset').value});$('onboarding').close();loadedRevision=0;await refresh();settingsUI();show('chat');},'Saving setup…');};
$('setup-connect').onclick=()=>{$('onboarding').close();show('settings');};
$('settings-form').onsubmit=e=>{e.preventDefault();act(async()=>{if(utf8($('nickname').value)>16)throw new Error('Nickname must fit 16 UTF-8 bytes');const data={op:'settings',network:document.querySelector('[name=network]:checked').value,preset:$('preset').value,nickname:$('nickname').value};const wifiChanged=$('ssid').value!==(state.settings.ssid||'')||$('password').value.length;if(wifiChanged){data.ssid=$('ssid').value;data.password=$('password').value;}await transport.request(data);loadedRevision=0;await refresh();notify(wifiChanged?'Restart the radio to join Wi-Fi, then open meshit.local.':'');$('password').value='';},'Saving settings…');};
function setConnected(connected){
  if(connected&&!radioConnected)notify();
  radioConnected=connected;if(!connected){loadedRevision=0;viewedKey='';downloadStatus(false);}
  const indicator=$('radio-connection'),label=connected?`Radio connected via ${transport.mode==='wifi'?'Wi-Fi':'Bluetooth'}`:'Radio disconnected';
  indicator.dataset.connected=String(connected);indicator.title=label;indicator.setAttribute('aria-label',label);
}
function downloadStatus(active){$('download-status').textContent=active?'Downloading messages…':'';}
function connectionStatus(message){$('link-status').textContent=message;connectionControls();}
function connectionControls(){
  const bluetooth=transport.mode==='ble';$('transport').value=transport.mode;
  $('ble-controls').hidden=!bluetooth;$('wifi-help').hidden=bluetooth;
  $('wifi-help').textContent=native?'Join mesit-… in your device’s Wi-Fi settings. The app connects at 192.168.4.1.':'Join the radio’s Wi-Fi network and open http://192.168.4.1, or use meshit.local on its LAN.';
  $('devices').disabled=busy||!transport.devices.length;
  $('scan').disabled=busy;$('connect').disabled=busy||!transport.devices.length;
  $('disconnect').disabled=busy||!transport.link;$('transport').disabled=busy;
  for(const control of document.querySelectorAll('#settings-form input,#settings-form select,#settings-form button'))control.disabled=busy||!state||!transport.link;
}
function changingRadio(){
  setConnected(false);
  state=null;outgoing.clear();messages=[];loadedRevision=0;reply='';render();
  $('radio-info').textContent='';
}
transport.onDisconnect=message=>{setConnected(false);if(state)state.ready=false;count();connectionStatus(message);};
$('transport').onchange=()=>{const mode=$('transport').value;act(async()=>{
  changingRadio();await transport.changeMode(mode);
  try{localStorage.setItem('meshit.connection',mode);}catch{}
  $('devices').replaceChildren();connectionStatus(mode==='wifi'?'Join the radio’s Wi-Fi network.':'Find your radio, select it, then connect.');
  if(mode==='wifi')await refresh();
});};
$('scan').onclick=()=>act(async()=>{
  $('devices').replaceChildren();connectionStatus('Finding meshit radios…');
  const devices=await transport.findRadios();$('devices').replaceChildren();
  for(const device of devices){const option=element('option',null,device.name||'meshit radio');option.value=device.id;$('devices').append(option);}
  connectionStatus(devices.length?'Select a radio and tap Connect.':'No meshit radios found. Check Bluetooth and radio power.');
});
$('connect').onclick=()=>act(async()=>{
  changingRadio();downloadStatus(true);connectionStatus('Connecting… ESP32 may request its 6-digit pairing PIN.');
  await transport.connect($('devices').value);await refresh();
});
$('disconnect').onclick=()=>act(async()=>{
  await transport.disconnect();setConnected(false);if(state)state.ready=false;count();
  connectionStatus('Disconnected. ESP32 Wi-Fi returns; messages remain on the radio.');
});
connectionControls();
const cache=saved('meshit.cache',null);if(cache?.state&&Array.isArray(cache.messages)){state=cache.state;state.ready=false;messages=cache.messages.slice(-200);}
show('chat');chooseRoom(room);await refresh();if(state)settingsUI();setInterval(()=>{if(!busy)refresh();},3000);
globalThis.addEventListener?.('resize',resizeComposer);
document.addEventListener('visibilitychange',()=>{if(!document.hidden&&!busy)refresh();});
