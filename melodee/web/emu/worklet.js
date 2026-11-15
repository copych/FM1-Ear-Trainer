// SPDX-License-Identifier: GPL-3.0-only
// Melodee in an AudioWorklet: melodee.wasm is the firmware's UI and sound (web/emu/melodee_web.c). Each
// render quantum asks it for 128 frames, which runs the device's clock forward; about 30 times a second
// the screen and the LEDs go to the page.

class Melodee extends AudioWorkletProcessor {
  constructor() {
    super();
    this.ex = null;
    this.audioOrigin = null;
    this.port.onmessage = (e) => this.onMessage(e.data);
  }

  async onMessage(m) {
    if (m.type === "load") {
      const stub = new Proxy({}, { get: () => () => 0 });   // (WASI: nothing is printed or opened)
      const { instance } = await WebAssembly.instantiate(m.wasm, { env: stub, wasi_snapshot_preview1: stub });
      const ex = instance.exports;
      ex._initialize();
      ex.web_boot(m.master);
      if (Array.isArray(m.earSettings) && m.earSettings.length === 12 &&
          m.earSettings.every(v => Number.isInteger(v) && v >= 0 && v <= 255)) {
        new Uint8Array(ex.memory.buffer, ex.web_ear_settings(), 12).set(m.earSettings);
        ex.web_ear_settings_restore();
      }
      this.earPrefs = "";
      if(Array.isArray(m.earRatings) && m.earRatings.length===168 &&
         m.earRatings.every(v=>Number.isInteger(v) && v>=0 && v<=255)) {
        new Uint8Array(ex.memory.buffer,ex.web_ear_ratings(),168).set(m.earRatings);
        ex.web_ear_ratings_restore();
      }
      this.earScores="";
      if(Array.isArray(m.metroSettings) && m.metroSettings.length===60 &&
          m.metroSettings.every(v=>Number.isInteger(v) && v>=0 && v<=255)) {
        new Uint8Array(ex.memory.buffer,ex.web_metro_settings(),60).set(m.metroSettings);
        ex.web_metro_settings_restore();
      }
      this.metroPrefs="";
      this.mem = ex.memory;
      this.sentFb = -1;
      this.last = 0;
      this.ex = ex;
      return;
    }
    const ex = this.ex;
    if (!ex) return;
    if (m.type === "audio-clock") {
      if(Number.isFinite(m.outputDelay) && m.outputDelay>=0 && m.outputDelay<=2)
        ex.web_metro_latency(Math.ceil((m.outputDelay+.25)*44100));
    }
    else if (m.type === "keys") {
      const at=this.inputSample(m.audioTime);
      if(at===null)ex.web_keys(m.mask >>> 0);else ex.web_keys_at(m.mask >>> 0,at);
    }
    else if (m.type === "training") this.port.postMessage({type:"training-result",accepted:!!ex.web_training_open()});
    else if (m.type === "ear") this.port.postMessage({type:"ear-result", accepted:!!ex.web_ear_action(m.action)});
    else if (m.type === "metro") this.port.postMessage({type:"metro-result", accepted:!!ex.web_metro_action(m.action)});
    else if (m.type === "buttons") ex.web_buttons(m.mask >>> 0);
    else if (m.type === "encoder") ex.web_encoder(m.id, m.n | 0);
    else if (m.type === "master") ex.web_master(m.value | 0);
    else if (m.type === "midi") {
      const at=this.inputSample(m.audioTime);
      if(at===null)ex.web_midi_in(m.data[0],m.data[1]||0,m.data[2]||0);
      else ex.web_midi_at(m.data[0],m.data[1]||0,m.data[2]||0,at);
    }
  }

  inputSample(time) {
    if(this.audioOrigin===null || !Number.isFinite(time) || time>currentTime+.02 || time<currentTime-2)return null;
    return Math.max(0,Math.round((time-this.audioOrigin)*44100))>>>0;
  }

  process(inputs, outputs) {
    const ex = this.ex;
    if (!ex) return true;
    const out = outputs[0], n = out[0].length;
    if(this.audioOrigin===null)this.audioOrigin=currentTime;
    try {
      ex.web_render(n);
    } catch (err) {
      this.ex = null;
      this.port.postMessage({ type: "crash", message: String(err && err.message || err) });
      return true;
    }
    out[0].set(new Float32Array(this.mem.buffer, ex.web_out_l(), n));
    if (out[1]) out[1].set(new Float32Array(this.mem.buffer, ex.web_out_r(), n));
    if (currentTime - this.last >= 1 / 30) {
      this.last = currentTime;
      const msg = { type: "frame", ms: ex.web_now(), keys: ex.web_led_keys() >>> 0, buttons: ex.web_led_buttons() >>> 0, earActive:!!ex.web_ear_active() };
      msg.metroActive=!!ex.web_metro_active();
      msg.trainingActive=!!ex.web_training_active();msg.trainingMenu=!!ex.web_training_menu();
      if(msg.metroActive)msg.metroStats=Array.from({length:9},(_,k)=>ex.web_metro_metric(k));
      const metroPrefs=Array.from(new Uint8Array(this.mem.buffer,ex.web_metro_settings(),60));
      const metroKey=metroPrefs.join(",");
      if(metroKey!==this.metroPrefs) {this.metroPrefs=metroKey;msg.metroSettings=metroPrefs;}
      const prefs = Array.from(new Uint8Array(this.mem.buffer, ex.web_ear_settings(), 12));
      const key = prefs.join(",");
      if (key !== this.earPrefs) { this.earPrefs = key; msg.earSettings = prefs; }
      const ratings=Array.from(new Uint8Array(this.mem.buffer,ex.web_ear_ratings(),168));
      const ratingKey=ratings.join(",");
      if(ratingKey!==this.earScores) {this.earScores=ratingKey;msg.earRatings=ratings;}
      const gen = ex.web_fb_gen(), transfer = [];
      if (gen !== this.sentFb) {
        this.sentFb = gen;
        msg.fb = new Uint16Array(this.mem.buffer, ex.web_fb(), 240 * 240).slice();
        transfer.push(msg.fb.buffer);
      }
      this.port.postMessage(msg, transfer);
    }
    return true;
  }
}

registerProcessor("melodee", Melodee);
