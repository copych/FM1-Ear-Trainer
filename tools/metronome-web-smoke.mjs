import fs from 'node:fs';
import assert from 'node:assert/strict';
const wasm=fs.readFileSync('build/emu/melodee.wasm');
const stub=new Proxy({},{get:()=>()=>0});
async function boot() {
  const {instance}=await WebAssembly.instantiate(wasm,{env:stub,wasi_snapshot_preview1:stub});
  const ex=instance.exports;ex._initialize();ex.web_boot(1023);return ex;
}
let ex=await boot();
const runTo=ms=>{while(ex.web_now()<ms)ex.web_render(128);};
const prefs=()=>Array.from(new Uint8Array(ex.memory.buffer,ex.web_metro_settings(),60));
function crc(p) {
  let c=0;for(let k=0;k<11;k++){c^=p[k];for(let b=0;b<8;b++)c=((c<<1)^((c&128)?7:0))&255;}return c;
}
function restore(p) {
  new Uint8Array(ex.memory.buffer,ex.web_metro_settings(),60).set(p);
  return ex.web_metro_settings_restore();
}
const saved=prefs();saved[2]=100;saved[3]=0;saved[5]=(1<<4)|2;saved[11]=crc(saved);
assert.equal(restore(saved),1);
assert.equal(ex.web_metro_action(99),0);assert.equal(ex.web_metro_action(1),0);
assert.equal(ex.web_metro_action(0),1);assert.equal(ex.web_ear_action(0),0);
assert.equal(restore(saved),0,'active metronome cannot import preferences');
// Evens in hidden bars 2/4/6/8; audible bars remain fully played guides.
for(let bar=0;bar<4;bar++)for(const beat of [1,3]) {
  runTo(200+(8+bar*8+beat)*600);
  ex.web_midi_in(0x90,60,100);ex.web_render(128);
  ex.web_midi_in(0x80,60,0);ex.web_render(128);
}
runTo(23000);
assert.equal(ex.web_metro_metric(0),3,'round ends in results');
assert.equal(ex.web_metro_metric(2),8);assert.equal(ex.web_metro_metric(3),8);
assert.equal(ex.web_metro_metric(4),0);assert.equal(ex.web_metro_metric(5),0);
assert(ex.web_metro_metric(1)>=90,'on-time MIDI taps earn high timing score');
assert(ex.web_metro_metric(6)<6,'WASM scheduling error under six milliseconds in fixture');
assert.equal(ex.web_metro_action(4),1);ex.web_render(128);assert.equal(ex.web_metro_active(),0);
// Save USER 1 through the real encoder/button adapter, then a fresh VM.
assert.equal(ex.web_metro_action(0),1);ex.web_render(128);
ex.web_encoder(6,16);ex.web_render(128);
ex.web_encoder(2,23);ex.web_render(128);
ex.web_buttons(1<<1);ex.web_render(128);ex.web_buttons(0);ex.web_render(128);
const custom=prefs();assert.equal(custom[14],123);
ex.web_metro_action(4);ex.web_render(128);
ex=await boot();assert.equal(restore(custom),1);assert.deepEqual(prefs(),custom);
custom[20]^=1;assert.equal(restore(custom),1);assert.equal(prefs()[14],0,'corrupt slot recovers independently');
custom[0]^=1;assert.equal(restore(custom),0,'corrupt current record is rejected');
assert.equal(ex.web_metro_action(0),1);
let peak=0;
for(let k=0;k<300;k++) {
  ex.web_render(128);
  for(const x of new Float32Array(ex.memory.buffer,ex.web_out_l(),128)) {assert(Number.isFinite(x));peak=Math.max(peak,Math.abs(x));}
}
assert(peak>.01,'WASM produces audible metronome clicks through the real mixer');
console.log('WASM metronome PASS: audible clicks, eight-bar MIDI scoring, exclusivity, user preset save, fresh-instance restore and corrupt records');
// One firmware menu launches and returns from all three activities.
ex=await boot();assert.equal(ex.web_training_open(),1);assert.equal(ex.web_training_menu(),1);
assert.equal(ex.web_ear_action(0),0);assert.equal(ex.web_metro_action(0),0);
const button=id=>{ex.web_buttons(1<<id);ex.web_render(128);ex.web_buttons(0);ex.web_render(128);};
for(let choice=0;choice<3;choice++) {
  if(choice) {ex.web_encoder(0,1);ex.web_render(128);}
  button(12);assert.equal(ex.web_training_menu(),0);assert.equal(ex.web_training_active(),1);
  assert.equal(ex.web_ear_active(),choice===0?1:0);assert.equal(ex.web_metro_active(),choice===0?0:1);
  if(choice)assert.equal(prefs()[5]>>4,choice-1);
  button(8);assert.equal(ex.web_training_menu(),1);assert.equal(ex.web_ear_active(),0);assert.equal(ex.web_metro_active(),0);
}
button(8);assert.equal(ex.web_training_menu(),0);assert.equal(ex.web_training_active(),0);
console.log('WASM training menu PASS: pitch/metronome/unified rhythm selection and HOME return/exit');
