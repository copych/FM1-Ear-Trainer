// Browser input regression without audio/devices: execute the actual page script.
const fs=require('node:fs');
const vm=require('node:vm');
const assert=require('node:assert/strict');
class Element {
  constructor(){this.handlers={};this.attributes={};this.classes=new Set();this.style={setProperty(){}};this.textContent='';
    this.classList={toggle:(name,on)=>on?this.classes.add(name):this.classes.delete(name)};}
  addEventListener(name,callback){(this.handlers[name]??=[]).push(callback);}
  setAttribute(name,value){this.attributes[name]=value;}
  setPointerCapture(){}
  insertBefore(){}
  closest(){return null;}
  getContext(){return {createImageData:()=>({data:new Uint8ClampedArray(240*240*4)}),putImageData(){}};}
  fire(name,values={}){for(const callback of this.handlers[name]??[]) callback({preventDefault(){},target:this,...values});}
  click(){this.fire('click');}
}
const ids=new Map(),events=new Element(),messages=[];
const timers=new Map();let nextTimer=0;
const flushTimers=()=>{for(const [id,callback] of timers){timers.delete(id);callback();}};
const context=vm.createContext({console,messages,navigator:{},
  setTimeout:callback=>{timers.set(++nextTimer,callback);return nextTimer;},clearTimeout:id=>timers.delete(id),
  document:{getElementById:id=>{if(!ids.has(id))ids.set(id,new Element());return ids.get(id);},createElement:()=>new Element()},
  addEventListener:(name,callback)=>events.addEventListener(name,callback)});
const html=fs.readFileSync('melodee/web/emu/index.html','utf8');
const script=html.match(/<script>([\s\S]*?)<\/script>/)[1];
vm.runInContext(script,context);
vm.runInContext('node={port:{postMessage:message=>messages.push(message)}}',context);
const panel=name=>vm.runInContext(`buttons[BTN[${JSON.stringify(name)}]]`,context);
const mask=()=>messages.filter(message=>message.type==='buttons').at(-1)?.mask??0;
const home=1<<8,play=1<<12;
const release=pointerId=>ids.get('device').fire('pointerup',{pointerId});
ids.get('holdhome').fire('click');assert.equal(mask(),home);
assert.equal(ids.get('holdhome').attributes['aria-pressed'],'true');
assert(panel('HOME').classes.has('latched'));
panel('PLAY').fire('pointerdown',{pointerId:1});assert.equal(mask(),home|play);
release(1);assert.equal(mask(),home,'modifier survives release until the worklet can observe the queued combo');
flushTimers();assert.equal(mask(),0);assert.equal(ids.get('holdhome').textContent,'Hold HOME');
// Shift+click a panel modifier, then a second independent click.
panel('HOME').fire('pointerdown',{pointerId:2,shiftKey:true});release(2);assert.equal(mask(),home);
panel('PLAY').fire('pointerdown',{pointerId:3});assert.equal(mask(),home|play);release(3);flushTimers();assert.equal(mask(),0);
// Normal panel taps retain their original momentary behavior.
panel('HOME').fire('pointerdown',{pointerId:4});assert.equal(mask(),home);release(4);assert.equal(mask(),0);
// Toggle off, global reset, Escape, blur and cancellation must not leave buttons stuck.
for(const method of ['toggle','release','escape','blur','cancel']) {
  ids.get('holdhome').fire('click');assert.equal(mask(),home);
  if(method==='toggle')ids.get('holdhome').fire('click');
  if(method==='release')ids.get('releaseheld').fire('click');
  if(method==='escape')events.fire('keydown',{code:'Escape'});
  if(method==='blur')events.fire('blur');
  if(method==='cancel')ids.get('device').fire('pointercancel',{pointerId:9});
  assert.equal(mask(),0,method);
}
// Keyboard musical keys can also consume a held modifier, without stuck notes.
ids.get('holdhome').fire('click');events.fire('keydown',{code:'KeyZ'});
assert.equal(mask(),home);events.fire('keyup',{code:'KeyZ'});flushTimers();assert.equal(mask(),0);
assert.equal(messages.filter(message=>message.type==='keys').at(-1).mask,0);
// A delayed release from a previous combo cannot clear a newly requested hold.
ids.get('holdhome').fire('click');panel('PLAY').fire('pointerdown',{pointerId:10});release(10);
ids.get('holdhome').fire('click');ids.get('holdhome').fire('click');flushTimers();assert.equal(mask(),home);
ids.get('releaseheld').fire('click');assert.equal(mask(),0);
console.log('Emulator controls PASS: two-click HOME+PLAY, Shift latches, ordinary taps, toggles, release, Escape, blur and pointer cancellation');

// The real page stores/reloads the preference record; denied storage stays usable.
const browserStorage=new Map();
context.localStorage={getItem:key=>browserStorage.get(key)??null,setItem:(key,value)=>browserStorage.set(key,value)};
vm.runInContext('storeEarSettings(Array.from({length:12},(_,k)=>k))',context);
assert.equal(vm.runInContext('JSON.stringify(readEarSettings())',context),JSON.stringify(Array.from({length:12},(_,k)=>k)));
const reloaded=vm.runInNewContext(script+'\nJSON.stringify(readEarSettings())',{
  console,navigator:{},document:context.document,localStorage:context.localStorage,
  addEventListener(){},setTimeout(){},clearTimeout(){},
});
assert.equal(reloaded,JSON.stringify(Array.from({length:12},(_,k)=>k)));
browserStorage.set('fm1-ear-trainer.settings.v1','broken JSON');
assert.equal(vm.runInContext('readEarSettings()',context),null);
browserStorage.set('fm1-ear-trainer.settings.v1','[1,2]');
assert.equal(vm.runInContext('readEarSettings()',context),null);
context.localStorage={getItem(){throw Error('denied');},setItem(){throw Error('denied');}};
assert.equal(vm.runInContext('readEarSettings()',context),null);
vm.runInContext('storeEarSettings([])',context);
  context.localStorage={getItem:key=>browserStorage.get(key)??null,setItem:(key,value)=>browserStorage.set(key,value)};
  vm.runInContext('storeEarRatings(Array.from({length:168},(_,k)=>k))',context);
  assert.equal(vm.runInContext('JSON.stringify(readEarRatings())',context),JSON.stringify(Array.from({length:168},(_,k)=>k)));
  browserStorage.set('fm1-ear-trainer.ratings.v1','[1,2]');
  assert.equal(vm.runInContext('readEarRatings()',context),null);
console.log('Browser preferences PASS: real page save/reload, invalid JSON/length and unavailable storage');

// Event times map to the speaker clock, not to the later message delivery.
context.performance={now:()=>1100};
vm.runInContext('node.context={currentTime:10,baseLatency:.01,outputLatency:.1,getOutputTimestamp:()=>({contextTime:9.8,performanceTime:1000})}',context);
assert.equal(vm.runInContext('audioClock(1000).audioTime',context),9.8);
events.fire('keydown',{code:'KeyZ',timeStamp:1000});
assert.equal(messages.filter(m=>m.type==='keys').at(-1).audioTime,9.8);
assert(messages.some(m=>m.type==='audio-clock'));
events.fire('keyup',{code:'KeyZ',timeStamp:1000});
vm.runInContext('delete node.context.getOutputTimestamp',context);
assert(Math.abs(vm.runInContext('audioClock(1000).audioTime',context)-9.79)<1e-9);
console.log('Browser input clock PASS: captured key time, speaker timestamp and latency fallback');
// Turning a knob leaves focus on its slider. Note letters must still reach
// the game; arrows remain owned by the knob's own key handler.
events.fire('keydown',{code:'KeyZ',timeStamp:1000,target:{closest:selector=>selector.includes('[role=slider]')}});
assert.equal(messages.filter(m=>m.type==='keys').at(-1).mask,1,'note input works after selecting an activity or turning a knob');
events.fire('keyup',{code:'KeyZ',timeStamp:1000,target:{closest:()=>true}});
assert.equal(messages.filter(m=>m.type==='keys').at(-1).mask,0,'key release follows the press across focus changes');
events.fire('keydown',{code:'KeyZ',timeStamp:1000,target:{closest:()=>true}});
assert.equal(messages.filter(m=>m.type==='keys').at(-1).mask,0,'typing in a real form control is not a game tap');

async function testGameStart() {
  const beforeDevice=html.slice(html.indexOf('<main>'),html.indexOf('<div class="device"'));
  assert.equal((beforeDevice.match(/<button\b/g)||[]).length,1,'one shared Ear Trainer entry');
  assert(beforeDevice.includes('Start training'));
  context.fetch=async()=>({arrayBuffer:async()=>new ArrayBuffer(8)});
  context.AudioContext=class {constructor(){this.audioWorklet={addModule:async()=>{}};this.destination={};}async resume(){}};
  context.AudioWorkletNode=class {constructor(){this.port={postMessage:m=>messages.push(m)};}connect(){}};
  vm.runInContext('node=null',context);messages.length=0;
  await ids.get('power').handlers.click[0]({});
  assert.equal(messages.at(-1).type,'load','one click initializes audio and loads firmware');
  vm.runInContext('node.port.onmessage({data:{type:"frame",earActive:false,keys:0,buttons:0}})',context);
  assert.equal(messages.at(-1).type,'training','first ready frame opens the training menu');
  assert.equal(ids.get('power').disabled,true);
  vm.runInContext('node.port.onmessage({data:{type:"frame",trainingActive:true,trainingMenu:true,earActive:false,keys:0,buttons:0}})',context);
  assert.equal(ids.get('power').textContent,'Choose an activity');
  vm.runInContext('node.port.onmessage({data:{type:"frame",trainingActive:true,earActive:true,keys:0,buttons:0}})',context);
  assert.equal(ids.get('power').textContent,'Training running');
  vm.runInContext('node.port.onmessage({data:{type:"frame",earActive:false,keys:0,buttons:0}})',context);
  assert.equal(ids.get('power').disabled,false,'HOME exit permits re-entry');
  await ids.get('power').handlers.click[0]({});
  assert.equal(messages.at(-1).type,'training');
  vm.runInContext('node.port.onmessage({data:{type:"frame",metroActive:true,earActive:false,keys:0,buttons:0}})',context);
  assert.equal(ids.get('power').disabled,true);
  vm.runInContext('node.port.onmessage({data:{type:"frame",metroActive:true,metroStats:[3,0,0,16,16,8,0,0,0],keys:0,buttons:0}})',context);
  assert.equal(ids.get('power').textContent,'Round finished');
  assert.match(ids.get('status').textContent,/Timing unavailable: no matched taps/);
  vm.runInContext('node.port.onmessage({data:{type:"frame",metroActive:false,earActive:false,keys:0,buttons:0}})',context);
  assert.equal(ids.get('power').disabled,false);
  console.log('Activity start PASS: one shared entry, audio boot, training menu state, mutual exclusion and re-entry after HOME');
}
testGameStart().catch(error=>{console.error(error);process.exitCode=1;});
