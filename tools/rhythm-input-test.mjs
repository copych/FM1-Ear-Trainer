// Exercise the actual AudioWorklet + WASM with a delayed speaker clock.
import fs from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const wasm=fs.readFileSync('build/emu/melodee.wasm');
const worklet=fs.readFileSync('web/emu/worklet.js','utf8');
function crc(p) {let c=0;for(let k=0;k<11;k++){c^=p[k];for(let b=0;b<8;b++)c=((c<<1)^((c&128)?7:0))&255;}return c;}
async function round({input='keys',delay=.12,half=false,quick=false,error=0,game=0,cycle=0,meter=0,bpm=120,warmup=0}={}) {
  let Processor;
  const origin=5;
  const scope=vm.createContext({WebAssembly,console,Float32Array,Uint8Array,
    currentTime:origin,currentFrame:origin*44100,sampleRate:44100,
    AudioWorkletProcessor:class {constructor(){this.port={postMessage(){}};}},
    registerProcessor(name,type){Processor=type;}});
  vm.runInContext(worklet,scope);
  const p=new Processor();await p.onMessage({type:'load',wasm,master:1023});
  const ex=p.ex,prefs=new Uint8Array(ex.memory.buffer,ex.web_metro_settings(),60);
  prefs[2]=bpm&255;prefs[3]=bpm>>8;prefs[4]=meter;prefs[5]=(1<<4)|game;prefs[9]=cycle;prefs[11]=crc(prefs);assert.equal(ex.web_metro_settings_restore(),1);
  await p.onMessage({type:'audio-clock',outputDelay:delay});
  const output=[[new Float32Array(128),new Float32Array(128)]];
  for(let block=0;block<warmup;block++) {
    scope.currentTime=origin+block*128/44100;scope.currentFrame=origin*44100+block*128;p.process([],output);
  }
  const start=warmup*128/44100;
  await p.onMessage({type:'metro',action:0});
  const events=[];
  const targets=[];
  const [beats,den]=({0:[4,4],8:[7,8],17:[7,16],20:[12,12],21:[16,16]})[meter];
  const step=240/bpm/den,bars=cycle?2:1;
  for(let bar=0;bar<8;bar++)if(bar%(bars*2)>=bars)
    for(let beat=0;beat<beats;beat++)if(game===0 || (game===1?!(beat&1):!!(beat&1)))targets.push(start+(bar*beats+beat)*step);
  for(const [index,target] of targets.entries())if(!half || !(index&1)) {
    const heard=target+error;
    const down=input==='midi'?{type:'midi',data:[0x90,60,100]}:{type:'keys',mask:1};
    const up=input==='midi'?{type:'midi',data:[0x80,60,0]}:{type:'keys',mask:0};
    events.push({at:heard+delay,message:{...down,audioTime:origin+heard}});
    events.push({at:heard+delay+(quick?0:.04),message:{...up,audioTime:origin+heard+.04}});
  }
  let next=0;
  for(let frames=warmup*128;frames<44100*(start+8*beats*step+delay+1);frames+=128) {
    scope.currentTime=origin+frames/44100;scope.currentFrame=origin*44100+frames;
    while(next<events.length && events[next].at<=frames/44100)await p.onMessage(events[next++].message);
    p.process([],output);
  }
  const hits=ex.web_metro_metric(2),misses=ex.web_metro_metric(4),extra=ex.web_metro_metric(5);
  console.log(`Game ${game}, ${bpm} BPM, cycle ${bars}/${bars}, grid ${beats}/${den}, speaker delay ${delay*1000} ms, ${input}${quick?', short press':''}, error ${error*1000} ms: ${hits} hits, ${misses} misses, ${extra} extras`);
  const window=Math.min(.06,...targets.slice(1).map((at,i)=>(at-targets[i])/2));
  const expected=Math.abs(error)<window?(half?Math.ceil(targets.length/2):targets.length):0;
  assert.equal(hits,expected);assert.equal(misses,targets.length-expected);assert.equal(extra,expected?0:events.length/2);
  if(!error)assert(ex.web_metro_metric(6)<=1,'captured event timestamps retain millisecond precision');
}
await round({half:true});
await round();
await round({input:'midi',delay:.2});
await round({delay:.35,quick:true});
await round({game:1,delay:.2});
await round({game:2,cycle:1,delay:.2});
// Reported setup: Evens at 120 BPM. No extra bar, ±55 ms accepted,
// ±65 ms rejected; tapping the audible odd pulses never earns a hit.
await round({game:2,error:-.055});
await round({game:2,error:.055});
await round({game:2,error:-.065});
await round({game:2,error:.065});
await round({game:2,error:-.5});
await round({game:2,cycle:1,warmup:137});
// Odd meter boundaries restart pulse numbering; consecutive hidden odd
// bars can have adjacent odd targets. Fractional tempi must not drift.
for(const game of [1,2])for(const cycle of [0,1])await round({game,cycle,meter:8,bpm:137});
await round({game:2,meter:21,bpm:300,error:.04});
await round({game:1,meter:17,bpm:300,error:.04});
await round({game:1,cycle:1,meter:17,bpm:300,error:.02});
await round({meter:21,delay:.2,quick:true});
await round({delay:.12,error:.09});
console.log('AudioWorklet rhythm input PASS: speaker delay, captured times, keyboard/MIDI and short presses');
