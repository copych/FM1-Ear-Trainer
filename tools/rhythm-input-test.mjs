// Exercise the actual AudioWorklet + WASM with a delayed speaker clock.
import fs from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const wasm=fs.readFileSync('build/emu/melodee.wasm');
const worklet=fs.readFileSync('web/emu/worklet.js','utf8');
function crc(p) {let c=0;for(let k=0;k<11;k++){c^=p[k];for(let b=0;b<8;b++)c=((c<<1)^((c&128)?7:0))&255;}return c;}
async function round({input='keys',delay=.12,half=false,quick=false,error=0,mode=1}={}) {
  let Processor;
  const origin=5;
  const scope=vm.createContext({WebAssembly,console,Float32Array,Uint8Array,
    currentTime:origin,currentFrame:origin*44100,sampleRate:44100,
    AudioWorkletProcessor:class {constructor(){this.port={postMessage(){}};}},
    registerProcessor(name,type){Processor=type;}});
  vm.runInContext(worklet,scope);
  const p=new Processor();await p.onMessage({type:'load',wasm,master:1023});
  const ex=p.ex,prefs=new Uint8Array(ex.memory.buffer,ex.web_metro_settings(),60);
  prefs[5]=mode<<4;prefs[11]=crc(prefs);assert.equal(ex.web_metro_settings_restore(),1);
  await p.onMessage({type:'audio-clock',outputDelay:delay});
  await p.onMessage({type:'metro',action:0});
  const events=[];
  const targets=[];
  if(mode===1)for(let bar=0;bar<4;bar++)for(let beat=0;beat<4;beat++)targets.push(.2+(8+bar*8+beat)*.5);
  else for(let bar=0;bar<8;bar++)for(const beat of [1,3])targets.push(.2+(4+bar*4+beat)*.5);
  for(const [index,target] of targets.entries())if(!half || !(index&1)) {
    const heard=target+error;
    const down=input==='midi'?{type:'midi',data:[0x90,60,100]}:{type:'keys',mask:1};
    const up=input==='midi'?{type:'midi',data:[0x80,60,0]}:{type:'keys',mask:0};
    events.push({at:heard+delay,message:{...down,audioTime:origin+heard}});
    events.push({at:heard+delay+(quick?0:.04),message:{...up,audioTime:origin+heard+.04}});
  }
  let next=0;
  const output=[[new Float32Array(128),new Float32Array(128)]];
  for(let frames=0;frames<44100*21;frames+=128) {
    scope.currentTime=origin+frames/44100;scope.currentFrame=origin*44100+frames;
    while(next<events.length && events[next].at<=frames/44100)await p.onMessage(events[next++].message);
    p.process([],output);
  }
  const hits=ex.web_metro_metric(2),misses=ex.web_metro_metric(4),extra=ex.web_metro_metric(5);
  console.log(`Mode ${mode}, speaker delay ${delay*1000} ms, ${input}, ${half?'eight':'sixteen'} taps${quick?', short press':''}, error ${error*1000} ms: ${hits} hits, ${misses} misses, ${extra} extras`);
  const expected=Math.abs(error)<=.06?(half?8:16):0;
  assert.equal(hits,expected);assert.equal(misses,16-expected);assert.equal(extra,expected?0:events.length/2);
  if(!error)assert(ex.web_metro_metric(6)<=1,'captured event timestamps retain millisecond precision');
}
await round({half:true});
await round();
await round({input:'midi',delay:.2});
await round({delay:.35,quick:true});
await round({mode:2,delay:.2});
await round({delay:.12,error:.09});
console.log('AudioWorklet rhythm input PASS: speaker delay, captured times, keyboard/MIDI and short presses');
