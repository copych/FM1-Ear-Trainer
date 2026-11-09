const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const root = path.resolve(__dirname, '../build/site');
const port = Number(process.argv[2] || 8010);
const mount = process.argv[3] || '/';
if (!/^\/(?:[A-Za-z0-9_-]+\/)*$/.test(mount)) throw new Error('Invalid mount path');
const mime = {'.html':'text/html; charset=utf-8','.js':'text/javascript; charset=utf-8','.wasm':'application/wasm','.json':'application/json','.png':'image/png','.svg':'image/svg+xml','.ttf':'font/ttf','.md':'text/plain; charset=utf-8'};
http.createServer((req,res) => {
  if (!['GET','HEAD'].includes(req.method)) { res.writeHead(405).end(); return; }
  let pathname;
  try { pathname = decodeURIComponent(new URL(req.url,'http://localhost').pathname); }
  catch { res.writeHead(400).end(); return; }
  if (!pathname.startsWith(mount)) { res.writeHead(404).end(); return; }
  pathname = '/' + pathname.slice(mount.length);
  const file = path.resolve(root, '.' + (pathname.endsWith('/') ? pathname+'index.html' : pathname));
  if (!file.startsWith(root + path.sep)) { res.writeHead(403).end(); return; }
  fs.stat(file,(err,stat) => {
    if (err || !stat.isFile()) { res.writeHead(404).end(); return; }
    res.writeHead(200, {'Content-Type':mime[path.extname(file)] || 'application/octet-stream', 'Cache-Control':'no-store', 'Content-Length':stat.size});
    if (req.method==='HEAD') res.end(); else fs.createReadStream(file).pipe(res);
  });
}).listen(port,'127.0.0.1',()=>console.log(`Ear Trainer: http://127.0.0.1:${port}/`));
