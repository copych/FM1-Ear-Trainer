import fs from 'node:fs';
import assert from 'node:assert/strict';
const stub = new Proxy({}, { get: () => () => 0 });
const { instance } = await WebAssembly.instantiate(fs.readFileSync('build/emu/melodee.wasm'), {
  env: stub, wasi_snapshot_preview1: stub,
});
const ex = instance.exports;
ex._initialize(); ex.web_boot(1023);
const Action={START:0,RETRY:1,NEXT:2,EXIT:3};
let capture=null;
function run(ms) {
  let peak = 0;
  for (let n=0; n<Math.ceil(ms*44.1); n+=128) {
    ex.web_render(128);
    const pcm = new Float32Array(ex.memory.buffer, ex.web_out_l(), 128);
    for (const v of pcm) {
      assert(Number.isFinite(v)); peak=Math.max(peak, Math.abs(v));
      if(capture) capture.push(v);
    }
  }
  return peak;
}
function screen() { return Buffer.from(new Uint8Array(ex.memory.buffer, ex.web_fb(), 240*240*2)); }
run(200);
assert.equal(ex.web_ear_action(Action.RETRY),0);
assert.equal(ex.web_ear_action(Action.NEXT),0);
assert.equal(ex.web_ear_active(),0);
assert.equal(ex.web_ear_action(Action.START),1);
capture=[];
const firstPeak=run(300);
assert.equal(ex.web_led_keys()>>>0,1<<7,'known C4 lights its keyboard key');
const secondPeak=run(300);
assert.equal(ex.web_led_keys()>>>0,1<<7,'hidden second pitch must not light a key');
const peak=Math.max(firstPeak,secondPeak,run(1200));
assert.equal(ex.web_led_keys()>>>0,0,'reference key light clears after release');
const exercise=capture;capture=null;
assert(peak>0.01, 'FM6 exercise must produce real audio in WASM');
const heard=[];
for (const [name,from,to] of [['first',.25,.4],['second',.5,.7]]) {
  const x=exercise.slice(Math.floor(from*44100),Math.floor(to*44100));
  const rms=Math.sqrt(x.reduce((s,v)=>s+v*v,0)/x.length);
  assert(rms>.001,`${name} note must be audible`);
  let best={note:0,level:0};
  for(const note of Array.from({length:27},(_,k)=>53+k).filter(n=>[0,2,4,5,7,9,11].includes(n%12)&&(name==='first'||n!==60))) {
    const w=2*Math.PI*440*2**((note-69)/12)/44100;
    let re=0,im=0;
    for(let i=0;i<x.length;i++){re+=x[i]*Math.cos(w*i);im+=x[i]*Math.sin(w*i);}
    const level=Math.hypot(re,im)/x.length;
    if(level>best.level)best={note,level};
  }
  heard.push(best.note);console.log(`${name} note: MIDI ${best.note}, RMS ${rms.toFixed(4)}`);
}
assert.equal(heard[0],60);assert(heard[1]!==60,'second note must have a different pitch');
const waiting=screen();
ex.web_keys(1<<7); run(40); // C4 cannot repeat as the target
const wrong=screen();
assert(!waiting.equals(wrong), 'wrong answer must update feedback');
function bg() { const s=screen();return (s[0]<<8)|s[1]; }
assert.equal(bg(),0x9105,'wrong feedback fills the background red');
ex.web_keys(0); run(20);
for(let note=53;note<=79;note++) {
  ex.web_keys(1<<(note-53));run(5);ex.web_keys(0);run(5);
  assert(screen().equals(wrong),'wrong attempt must remain locked while playing the scale');
}
ex.web_midi_in(0x90,72,100);run(50);ex.web_midi_in(0x80,72,0);run(20);
assert(screen().equals(wrong),'MIDI cannot change a locked failure');
run(2900);assert.notEqual(bg(),0x9105,'wrong answer automatically replays the same question');
ex.web_keys(1<<(heard[1]-53));run(40);
assert.equal(bg(),0x1346,'correct feedback fills the background green');
run(1100);assert.notEqual(bg(),0x1346,'success automatically starts the next exercise');
ex.web_keys(0);run(1800);
ex.web_ear_action(Action.NEXT);run(1800);
ex.web_ear_action(Action.EXIT);run(1500);
assert(!screen().equals(waiting), 'HOME must return to the instrument');
for(let cycle=0;cycle<3;cycle++) {
  assert.equal(ex.web_ear_active(),0);
  assert.equal(ex.web_ear_action(Action.RETRY),0);
  assert.equal(ex.web_ear_action(Action.NEXT),0);
  assert.equal(ex.web_ear_action(Action.START),1);run(1800);
  assert.equal(ex.web_ear_active(),1);
  assert.equal(ex.web_ear_action(Action.START),1);run(1800);
  assert.equal(ex.web_ear_active(),1);
  ex.web_ear_action(Action.EXIT);run(100);
}
console.log(`WASM game passed: two distinct audible notes; keyboard feedback, retry/next, three exit/re-entry/restart cycles`);
// Real panel encoder IDs: K1=2, K2=3, K3=4, K4=5.
ex.web_ear_action(Action.START);run(20);
ex.web_encoder(2,1);ex.web_encoder(3,18);ex.web_encoder(5,10);
capture=[];run(300);assert.equal(ex.web_led_keys()>>>0,0,'all-hidden first note stays dark');
run(1600);assert.equal(ex.web_led_keys()>>>0,0,'all-hidden sustained chord stays dark');
run(1100);const sequence=capture;capture=null;
// Detect each new fundamental against the decaying tones already held.
// The expected sequence is checked through the real ordered-answer path below.
function levelAt(note,from,to) {
  const samples=sequence.slice(Math.floor(from*44100),Math.floor(to*44100));
  const w=2*Math.PI*440*2**((note-69)/12)/44100;let re=0,im=0;
  for(let i=0;i<samples.length;i++){re+=samples[i]*Math.cos(w*i);im+=samples[i]*Math.sin(w*i);}
  return Math.hypot(re,im)/samples.length;
}
const pitches=[];
for(let k=0;k<4;k++) {
  let best={note:0,gain:-Infinity};
  for(let note=53;note<=79;note++) {
    if(![9,11,0,2,4,5,7].includes(note%12)||pitches.includes(note))continue;
    const gain=levelAt(note,.25+.5*k,.4+.5*k)-(k?levelAt(note,.05+.5*k,.18+.5*k):0);
    if(gain>best.gain)best={note,gain};
  }
  assert(best.gain>.003,'each distinct note must add an audible fundamental');
  pitches.push(best.note);
}
console.log('Detected hidden pitches:',pitches);
for(const note of pitches)assert(levelAt(note,2.1,2.3)>.003,'all four pitches must sustain');
// PRESETS selects factory FM BASS (normally mono), then replays this question in poly.
ex.web_encoder(6,2);const bassPeak=run(3000);
assert(bassPeak>.01,'selected preset must produce audio');
assert.equal(ex.web_led_keys()>>>0,0,'changing preset must not reveal hidden notes');
for(let k=0;k<4;k++) {
  ex.web_midi_in(0x90,pitches[k],100);run(20);
  assert.equal(bg()===0x1346,k===3,'award only after all four ordered notes');
  ex.web_midi_in(0x80,pitches[k],0);run(20);
}
ex.web_ear_action(Action.EXIT);run(100);
console.log(`WASM knobs passed: A minor, four sustained hidden notes, 500ms onset spacing, MIDI sequence ${pitches.join(',')}`);

// Switch across engines: Prophet must finish fading/loading and play a new exercise.
ex.web_ear_action(Action.START);run(20);
ex.web_encoder(6,22);run(1000);
assert(run(1800)>.01,'Prophet must play after the engine switch, not wait for FM6');
assert.equal(ex.web_led_keys()>>>0,0,'all-hidden Prophet pitches stay dark');
ex.web_ear_action(Action.EXIT);run(100);
// Persist the checked record across a fresh emulator/WASM instance.
const saved=Array.from(new Uint8Array(ex.memory.buffer,ex.web_ear_settings(),12));
assert.equal(saved[3],2,'Prophet preferences use the checked extended sound namespace');
assert.equal(saved[8],24,'PRESETS selected the first Prophet factory program');
const fresh=(await WebAssembly.instantiate(fs.readFileSync('build/emu/melodee.wasm'),{
  env:stub,wasi_snapshot_preview1:stub,
})).instance.exports;
fresh._initialize();fresh.web_boot(1023);
new Uint8Array(fresh.memory.buffer,fresh.web_ear_settings(),12).set(saved);
assert.equal(fresh.web_ear_settings_restore(),1);
assert.deepEqual(Array.from(new Uint8Array(fresh.memory.buffer,fresh.web_ear_settings(),12)),saved);
const damaged=[...saved];damaged[4]^=1;
new Uint8Array(fresh.memory.buffer,fresh.web_ear_settings(),12).set(damaged);
assert.equal(fresh.web_ear_settings_restore(),0,'damaged settings rejected without changing runtime');
assert.deepEqual(Array.from(new Uint8Array(fresh.memory.buffer,fresh.web_ear_settings(),12)),saved);
fresh.web_ear_action(Action.START);
assert.equal(fresh.web_ear_settings_restore(),0,'cannot restore settings during an active question');
console.log('WASM settings PASS: fresh-instance restore, exact preferences, corrupt-record rejection and active-game guard');
