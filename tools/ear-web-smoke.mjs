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
  for(const note of [60,62,64,65,67,69,71,72]) {
    const w=2*Math.PI*440*2**((note-69)/12)/44100;
    let re=0,im=0;
    for(let i=0;i<x.length;i++){re+=x[i]*Math.cos(w*i);im+=x[i]*Math.sin(w*i);}
    const level=Math.hypot(re,im)/x.length;
    if(level>best.level)best={note,level};
  }
  heard.push(best.note);console.log(`${name} note: MIDI ${best.note}, RMS ${rms.toFixed(4)}`);
}
assert.equal(heard[0],60);assert(heard[1]>60,'second note must have a different pitch');
const waiting=screen();
ex.web_keys(1<<7); run(40); // C4 cannot be the ascending target
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
ex.web_encoder(2,1);ex.web_encoder(3,2);ex.web_encoder(4,3);ex.web_encoder(5,10);
capture=[];run(300);assert.equal(ex.web_led_keys()>>>0,0,'all-hidden first note stays dark');
run(1600);assert.equal(ex.web_led_keys()>>>0,0,'all-hidden sustained chord stays dark');
run(1100);const sequence=capture;capture=null;
// Fixed startup seed/scenario yields E4,C4,E4,F4. A sustained chord's loudest
// frequency is not necessarily its newest note: check all held fundamentals.
const pitches=[64,60,64,65];
const samples=sequence.slice(Math.floor(2.1*44100),Math.floor(2.3*44100));
for(const note of new Set(pitches)) {
  const w=2*Math.PI*440*2**((note-69)/12)/44100;let re=0,im=0;
  for(let i=0;i<samples.length;i++){re+=samples[i]*Math.cos(w*i);im+=samples[i]*Math.sin(w*i);}
  const level=Math.hypot(re,im)/samples.length;
  assert(level>.003,`held chord must contain MIDI ${note}, got ${level}`);
}
for(let k=0;k<4;k++) {
  ex.web_midi_in(0x90,pitches[k],100);run(20);
  assert.equal(bg()===0x1346,k===3,'award only after all four ordered notes');
  ex.web_midi_in(0x80,pitches[k],0);run(20);
}
ex.web_ear_action(Action.EXIT);run(100);
console.log(`WASM knobs passed: A minor, four sustained hidden notes, 500ms onset spacing, MIDI sequence ${pitches.join(',')}`);
