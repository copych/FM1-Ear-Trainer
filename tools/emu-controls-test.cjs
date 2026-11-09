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
console.log('Browser preferences PASS: real page save/reload, invalid JSON/length and unavailable storage');
