"""Validate attested Ear Trainer bytes and native recovery archives before publication.

SPDX-License-Identifier: GPL-3.0-only
Copyright (C) 2026 Kerem Kilic (Ellic Studio)
"""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tarfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "melodee/tools"))
from fm1_recover import APP_LO, Package, VERSION
from fm1pkg_make import sfc

TARGETS = ("windows-x64", "linux-x64", "macos-arm64", "macos-x64")
PUBLIC = ("ear-trainer-dev.fwsc", "ear-trainer-dev-res.bin", "ear-trainer-app.bin",
          "ear-trainer-recovery-sources.zip", "ear-trainer-site.tar.gz")

def digest(data):
    return hashlib.sha256(data).hexdigest()

def sums(text, read, names):
    declared = {}
    for line in text.splitlines():
        sha, name = line.split("  ", 1)
        path = PurePosixPath(name)
        assert not path.is_absolute() and ".." not in path.parts and "\\" not in name, "Unsafe checksum path"
        assert name not in declared and digest(read(name)) == sha, f"Checksum mismatch: {name}"
        declared[name] = sha
    assert set(declared) == set(names), "Missing or unexpected checksum entries"
    return declared

def prepare(public, native, output, version, commit, ref):
    project = json.loads((ROOT / "project.json").read_text())
    assert version == project["version"] == VERSION and ref == f"refs/tags/v{version}"
    assert not output.exists(), "Use a fresh release output directory"
    sums((public / "SHA256SUMS").read_text(), lambda n: (public / n).read_bytes(), PUBLIC)
    package = Package(public / "ear-trainer-dev.fwsc")
    assert package.product == project["product"], "Wrong firmware product"
    app = (public / "ear-trainer-app.bin").read_bytes()
    decoded = bytearray(package.flash[APP_LO:])
    sfc(decoded, 0, len(decoded), 0, package.key)
    assert decoded[0x120:0x120+len(app)] == app, "App differs from firmware package"
    with zipfile.ZipFile(public / "ear-trainer-recovery-sources.zip") as recovery:
        for member, source in (("build/melodee.bin", "ear-trainer-app.bin"),
                               ("build/melodee.fwsc", "ear-trainer-dev.fwsc"),
                               ("build/melodee-res.bin", "ear-trainer-dev-res.bin")):
            assert recovery.read(member) == (public / source).read_bytes(), member
        for name in ("fm1_recover.py", "fm1_install.py", "fm1pkg_make.py"):
            assert recovery.read("tools/"+name) == (ROOT / "melodee/tools" / name).read_bytes()
        for path in (ROOT / "melodee/assets/recovery").iterdir():
            if path.is_file():
                assert recovery.read("assets/recovery/"+path.name) == path.read_bytes()
    with tarfile.open(public / "ear-trainer-site.tar.gz") as site:
        def read(name):
            member = site.getmember("./"+name)
            assert member.isfile(), "Site entry must be a regular file"
            return site.extractfile(member).read()
        manifest = json.loads(read("build-info.json"))
        assert manifest["sourceRevision"] == commit and manifest["sourceDirty"] is False
        assert manifest["sourceBranch"] == f"v{version}" and manifest["version"] == version
        assert manifest["sourceRepository"] == os.environ.get("GITHUB_REPOSITORY", "copych/FM1-Ear-Trainer")
        assert manifest["product"] == project["product"] and manifest["channel"] == "attested development"
        for name, sha in manifest["files"].items():
            assert digest(read(name)) == sha, f"Site mismatch: {name}"
        for name in ("ear-trainer-dev.fwsc", "ear-trainer-dev-res.bin", "ear-trainer-recovery-sources.zip"):
            assert read("firmware/"+name) == (public / name).read_bytes()
    recovery_info = {}
    for target in TARGETS:
        name = f"fm1-recover-{version}-{target}.zip"
        prefix = name[:-4]+"/"
        with zipfile.ZipFile(native / name) as archive:
            assert len(archive.namelist()) == len(set(archive.namelist())), "Duplicate archive paths"
            files = [p for p in archive.namelist() if not p.endswith("/")]
            assert all(p.startswith(prefix) for p in files), "Unexpected archive prefix"
            sums(archive.read(prefix+"SHA256SUMS").decode(), lambda n: archive.read(prefix+n),
                 [p[len(prefix):] for p in files if p != prefix+"SHA256SUMS"])
            info = json.loads(archive.read(prefix+"BUILD_INFO.json"))
            assert info["version"] == version and info["commit"] == commit and info["target"] == target
            assert info["smoke_tests_passed"] is True and info["worktree_dirty"] is False
            required = {"LICENSE", "LICENSING.md", "RECOVERY.md", "LICENSES/Apache-2.0.txt",
                        "LICENSES/LGPL-2.1-libusb.txt", "LICENSES/Python.txt", f"libusb-{info['libusb']}.tar.bz2"}
            assert required <= {p[len(prefix):] for p in files}, "Incomplete recovery licences/sources"
            assert set(info["source_sha256"]) == {"fm1_recover.py", "fm1_install.py", "fm1pkg_make.py", "build_recovery.py"}
            exe = "fm1-recover.exe" if target == "windows-x64" else "fm1-recover"
            assert digest(archive.read(prefix+exe)) == info["executable_sha256"]
            for source, sha in info["source_sha256"].items():
                assert digest((ROOT / "melodee/tools" / source).read_bytes()) == sha
            recovery_info[name] = info
    # All validation above completes before creating publishable output.
    output.mkdir(parents=True)
    renamed = {"ear-trainer-dev.fwsc": f"ear-trainer-{version}.fwsc",
               "ear-trainer-dev-res.bin": f"ear-trainer-{version}-res.bin",
               "ear-trainer-app.bin": f"ear-trainer-{version}-app.bin",
               "ear-trainer-recovery-sources.zip": f"ear-trainer-{version}-recovery-sources.zip",
               "ear-trainer-site.tar.gz": f"ear-trainer-{version}-site.tar.gz"}
    for source, name in renamed.items():
        shutil.copyfile(public / source, output / name)
    for name in recovery_info:
        shutil.copyfile(native / name, output / name)
    with zipfile.ZipFile(output / f"ear-trainer-{version}-licenses.zip", "w", zipfile.ZIP_DEFLATED) as licenses:
        for path in (ROOT / "LICENSE", ROOT / "melodee/LICENSING.md", *(ROOT / "melodee/LICENSES").glob("*.txt")):
            licenses.write(path, path.name if path.parent.name != "LICENSES" else "LICENSES/"+path.name)
    info = {"name": project["name"], "version": version, "product": project["product"],
            "commit": commit, "sourceRef": ref, "sourceRepository": manifest["sourceRepository"],
            "workflowRun": manifest["workflowRun"], "signerWorkflow": "zvenson/fm1-build/.github/workflows/build.yml@v1",
            "buildCommand": "sh tools/build-melodee.sh target", "hardwareTested": False,
            "recoveryBuilds": recovery_info}
    (output / "BUILD_INFO.json").write_text(json.dumps(info, indent=2)+"\n")
    (output / "SHA256SUMS").write_text("".join(f"{digest(p.read_bytes())}  {p.name}\n"
                                              for p in sorted(output.iterdir()) if p.is_file()))
    print(f"Validated release {version}: {commit}; four native recovery tools; product {project['product']}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("public", type=Path)
    parser.add_argument("native", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    prepare(args.public, args.native, args.output, VERSION, commit, os.environ["GITHUB_REF"])
