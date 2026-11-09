"""Prepare the site and matching recovery sources for the attested CI artifact."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import tarfile
import zipfile

root = Path(__file__).resolve().parents[1]
site = root / "build/site"
public = root / "build/public"
public.mkdir(parents=True, exist_ok=True)
manifest_path = site / "build-info.json"
manifest = json.loads(manifest_path.read_text())
manifest.update(channel="attested development", sourceRevision=os.environ["GITHUB_SHA"],
                workflowRun=f'https://github.com/{os.environ["GITHUB_REPOSITORY"]}/actions/runs/{os.environ["GITHUB_RUN_ID"]}')
manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
(site / ".nojekyll").touch()
landing = site / "index.html"
landing.write_text(landing.read_text().replace("<footer>",
    '<p><a href="firmware/ear-trainer-recovery-sources.zip">Matching recovery sources and images</a> · '
    f'<a href="{manifest["workflowRun"]}">Attested build log</a></p><footer>'))
recovery = site / "firmware/ear-trainer-recovery-sources.zip"
with zipfile.ZipFile(recovery, "w", zipfile.ZIP_DEFLATED) as archive:
    for name in ("fm1_recover.py", "fm1_install.py", "fm1pkg_make.py"):
        archive.write(root / "melodee/tools" / name, f"tools/{name}")
    for path in (root / "melodee/assets/recovery").iterdir():
        if path.is_file(): archive.write(path, f"assets/recovery/{path.name}")
    archive.write(root / "melodee/LICENSE", "LICENSE")
    archive.write(root / "melodee/LICENSES/Apache-2.0.txt", "LICENSES/Apache-2.0.txt")
    archive.write(root / "melodee/docs/BOOT-SAFETY.md", "BOOT-SAFETY.md")
    for name in ("melodee.bin", "melodee.fwsc", "melodee-res.bin", "loader/ota.bin"):
        archive.write(root / "melodee/build" / name, f"build/{name}")
    archive.writestr("README.txt", "Development build, not hardware-qualified. Back up your own device first.\n"
                      "See BOOT-SAFETY.md and tools/fm1_recover.py --help. Python source needs PyUSB/libusb for USB access.\n"
                      "Offline validation: python3 tools/fm1_recover.py build/melodee.fwsc --check\n"
                      "Recovery source is included; no device-specific backup is supplied.\n")
for name in ("ear-trainer-dev.fwsc", "ear-trainer-dev-res.bin", recovery.name):
    shutil.copyfile(site / "firmware" / name, public / name)
with tarfile.open(public / "ear-trainer-site.tar.gz", "w:gz") as archive:
    archive.add(site, arcname=".")
checksums = "".join(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n"
                    for p in sorted(public.iterdir()) if p.is_file() and p.name != "SHA256SUMS")
(public / "SHA256SUMS").write_text(checksums)
