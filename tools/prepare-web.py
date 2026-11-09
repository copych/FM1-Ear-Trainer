"""Assemble a local Ear Trainer site using Melodee's checked installer builder."""
from pathlib import Path
import hashlib
import json
import shutil
import sys

ROOT = Path(__file__).resolve().parents[1]
FW = ROOT / "melodee"
OUT = ROOT / "build/site"
SITE_VERSION = "0.0.8"
sys.path.insert(0, str(FW / "web"))
import make_site

for src in (FW / "build/melodee.fwsc", FW / "build/melodee-res.bin", FW / "build/emu/melodee.wasm"):
    if not src.is_file():
        raise SystemExit(f"Build target and emulator first; missing {src}")

# Retain upstream package/bootloader validation, resource pairing, licenses,
# backup editor and updater. This writes local files only; never publishes.
make_site.main(FW / "build/melodee.fwsc", SITE_VERSION, OUT, manual=ROOT / "web/guide")
inst = OUT / "webapp/installer/index.html"
page = inst.read_text(encoding="utf-8")
page = page.replace(f"melodee-{SITE_VERSION}.fwsc", "ear-trainer-dev.fwsc")
page = page.replace("Install Melodee", "Install FM-1 Ear Trainer")
page = page.replace("Melodee {v}", "Ear Trainer {v}")
page = page.replace("Installing Melodee", "Installing Ear Trainer")
page = page.replace(" · Melodee", " · Ear Trainer")
page = page.replace("Stable version", "Development build")
page = page.replace("Latest ·", "Local ·")
page = page.replace("Synths, sequencing and effects for the FM-1, installed over USB from this page.",
                    "Two-note ear training with Melodee FM6 and TINE EP, installed over USB from this page.")
page = page.replace('<a class="brand" href="../editor/">Melodee</a>',
                    '<a class="brand" href="../../">Ear Trainer</a>')
banner = '''<aside style="padding:14px 20px;background:#242126;color:#fff;text-align:center;font:15px/1.5 system-ui">
<strong>FM-1 Ear Trainer · development prototype</strong><br>
This installs the locally built Ear Trainer fork, based on Melodee FM6.
Hardware timing and both physical revisions still await validation.
<a style="color:#ffb58a" href="../../emu/">Try the emulator</a> ·
<a style="color:#ffb58a" href="../../">Home</a></aside>'''
page = page.replace("<body>", "<body>" + banner, 1)
inst.write_text(page, encoding="utf-8")
for suffix in (".fwsc", "-res.bin"):
    (OUT / "firmware" / (f"melodee-{SITE_VERSION}" + suffix)).replace(OUT / "firmware" / ("ear-trainer-dev" + suffix))
for name in ("release.json", "releases.json"):
    p = OUT / "firmware" / name
    p.write_text(p.read_text().replace(f"melodee-{SITE_VERSION}.fwsc", "ear-trainer-dev.fwsc"))

emu = OUT / "emu"
emu.mkdir(parents=True, exist_ok=True)
for name in ("index.html", "melodee.wasm", "worklet.js"):
    shutil.copyfile(FW / "build/emu" / name, emu / name)
page = (emu / "index.html").read_text(encoding="utf-8")
page = page.replace("<title>Melodee in the browser</title>", "<title>FM-1 Ear Trainer · Emulator</title>")
page = page.replace("<h1>Melodee", "<h1>FM-1 Ear Trainer")
page = page.replace("<main>", '''<main><nav><a href="../">Home</a> · <a href="../webapp/installer/">Install on FM-1</a> · <a href="../webapp/manual/">Guide</a></nav>
<p>Switch on, then choose <strong>Start training</strong>. Listen and reproduce the hidden ending notes in order. K1: major/minor; K2: 1–4 notes; K3: hidden notes; K4: time between note starts (0 = chord). Release keys between answers.</p>
<p><button id="ear-start" disabled>Start training</button>
<button id="ear-retry" disabled>Replay / Retry</button>
<button id="ear-next" disabled>Next question</button>
<button id="ear-exit" disabled>Exit training</button></p>''', 1)
# The original emulator still supplies the actual device/audio/MIDI implementation.
page = page.replace('if (m.fb) draw(m.fb);', '''if (m.fb) {
        draw(m.fb);
      }
      if (m.type === "ear-result" && !m.accepted) $("status").textContent = "Stop playback before starting training.";
      if (m.type === "frame") {
        $("ear-start").disabled = false;
        $("ear-start").textContent = m.earActive ? "Restart training" : "Start training";
        for (const id of ["ear-retry","ear-next","ear-exit"]) $(id).disabled = !m.earActive;
      }''')
controls = '''
const EarAction = { START:0, RETRY:1, NEXT:2, EXIT:3 };
function earAction(action) {
  if (!node) return;
  $("status").textContent = "";
  node.port.postMessage({type:"ear", action});
}
$("ear-start").onclick = () => earAction(EarAction.START);
$("ear-retry").onclick = () => earAction(EarAction.RETRY);
$("ear-next").onclick = () => earAction(EarAction.NEXT);
$("ear-exit").onclick = () => earAction(EarAction.EXIT);
'''
page = page.replace("</script>", controls + "</script>", 1)
(emu / "index.html").write_text(page, encoding="utf-8")
shutil.copyfile(ROOT / "web/index.html", OUT / "index.html")

digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
manifest = {
    "name": "FM-1 Ear Trainer", "channel": "local development",
    "version": SITE_VERSION,
    "baseRevision": "165d177a425944f3d30b0b669255a6ed0050c3a5",
    "product": make_site.product_of((FW / "build/melodee.fwsc").read_bytes()),
    "files": {name: digest(OUT / name) for name in
              ("firmware/ear-trainer-dev.fwsc", "firmware/ear-trainer-dev-res.bin", "emu/melodee.wasm")},
}
(OUT / "build-info.json").write_text(json.dumps(manifest, indent=2)+"\n")
# Verify the advertised installer hash against the actual bytes, not a hosted release.
release = json.loads((OUT / "firmware/release.json").read_text())
assert release["sha256"]["dual"] == manifest["files"]["firmware/ear-trainer-dev.fwsc"]
assert manifest["files"]["firmware/ear-trainer-dev.fwsc"] == digest(FW / "build/melodee.fwsc")
assert manifest["files"]["firmware/ear-trainer-dev-res.bin"] == digest(FW / "build/melodee-res.bin")
assert "/*META*/" not in page
print(f"Ear Trainer site: {OUT}")
print("Firmware SHA-256:", release["sha256"]["dual"])
