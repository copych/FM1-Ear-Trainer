const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const base = process.argv[2] || 'http://127.0.0.1:8010/';
const root = path.resolve(__dirname, '../build/site');
(async()=>{
  const routes = ['', 'emu/', 'webapp/installer/', 'webapp/installer/?demo', 'webapp/editor/', 'webapp/manual/',
    'emu/worklet.js', 'emu/melodee.wasm', 'firmware/ear-trainer-dev.fwsc', 'firmware/ear-trainer-dev-res.bin',
    'firmware/LICENSES/', 'build-info.json'];
  for (const route of routes) {
    const r=await fetch(new URL(route,base)); assert.equal(r.status,200,route);
    const bytes=Buffer.from(await r.arrayBuffer()); assert(bytes.length,route);
    if(route.endsWith('.wasm')) assert.match(r.headers.get('content-type'),/application\/wasm/);
  }
  for(const page of ['emu/index.html','webapp/installer/index.html']) {
    const html=fs.readFileSync(path.join(root,page),'utf8');
    for (const [,src] of html.matchAll(/<script\b[^>]*>([\s\S]*?)<\/script>/g)) new vm.Script(src,{filename:page});
    assert(!html.includes('/*META*/'),page);
  }
  const manifest=JSON.parse(fs.readFileSync(path.join(root,'build-info.json')));
  for (const [name,expected] of Object.entries(manifest.files)) {
    const bytes=Buffer.from(await (await fetch(new URL(name,base))).arrayBuffer());
    assert.equal(crypto.createHash('sha256').update(bytes).digest('hex'),expected,name);
  }
  const html=fs.readFileSync(path.join(root,'webapp/installer/index.html'),'utf8');
  const meta=JSON.parse(html.match(/^const meta = (\{.*\});$/m)[1]);
  assert.equal(meta.product,manifest.product);
  assert.equal(meta.sha256.dual,manifest.files['firmware/ear-trainer-dev.fwsc']);
  assert.equal(new URL(meta.pkg.dual,new URL('webapp/installer/',base)).pathname,'/firmware/ear-trainer-dev.fwsc');
  assert(!/melodee-\d+\.\d+\.\d+\.fwsc/.test(html));
  console.log('Site checks passed: pages/assets, JS syntax, served firmware/resource/WASM hashes, installer selection');
})().catch(e=>{console.error(e);process.exitCode=1;});
