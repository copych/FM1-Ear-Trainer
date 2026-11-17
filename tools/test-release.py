"""Exercise complete release validation and failure gates using synthetic recovery executables."""
from pathlib import Path
import importlib.util
import io
import json
import sys
import tarfile
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "melodee/tests"))
sys.path.insert(0, str(ROOT / "melodee/tools"))
from recovery_test import make_package
import fm1_recover as R
import fm1pkg_make as P

spec = importlib.util.spec_from_file_location("ear_release", ROOT / "tools/prepare-release.py")
A = importlib.util.module_from_spec(spec)
spec.loader.exec_module(A)
VERSION = json.loads((ROOT / "project.json").read_text())["version"]
COMMIT = "a"*40

def zipper(path, files):
    with zipfile.ZipFile(path, "w") as archive:
        for name, data in files.items():
            archive.writestr(name, data)

class ReleaseValidation(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.public = self.root / "public"; self.public.mkdir()
        self.native = self.root / "native"; self.native.mkdir()
        self.output = self.root / "release"
        fixture=self.root/"fixture.fwsc";fixture.write_bytes(make_package())
        fw = P.ufw(R.Package(fixture).flash, b"FELUCCA-LOADER-1", "FM-1_327008")
        (self.public / A.PUBLIC[0]).write_bytes(fw)
        package = R.Package(self.public / A.PUBLIC[0])
        decoded = bytearray(package.flash[R.APP_LO:]); P.sfc(decoded,0,len(decoded),0,package.key)
        app = bytes(decoded[0x120:0x140]); res = b"resource fixture"
        (self.public / A.PUBLIC[1]).write_bytes(res)
        (self.public / A.PUBLIC[2]).write_bytes(app)
        sources = {"build/melodee.bin":app,"build/melodee.fwsc":fw,"build/melodee-res.bin":res}
        for name in ("fm1_recover.py","fm1_install.py","fm1pkg_make.py"):
            sources["tools/"+name]=(ROOT / "melodee/tools" / name).read_bytes()
        for path in (ROOT / "melodee/assets/recovery").iterdir():
            if path.is_file(): sources["assets/recovery/"+path.name]=path.read_bytes()
        zipper(self.public / A.PUBLIC[3],sources)
        self.rebuild_site()
        for target in A.TARGETS:
            prefix=f"fm1-recover-{VERSION}-{target}/"
            exe="fm1-recover.exe" if target=="windows-x64" else "fm1-recover"
            info={"version":VERSION,"commit":COMMIT,"target":target,"smoke_tests_passed":True,
                  "worktree_dirty":False,"libusb":"1.0.30","executable_sha256":A.digest(b"synthetic executable"),
                  "source_sha256":{name:A.digest((ROOT / "melodee/tools" / name).read_bytes())
                      for name in ("fm1_recover.py","fm1_install.py","fm1pkg_make.py","build_recovery.py")}}
            files={exe:b"synthetic executable","BUILD_INFO.json":json.dumps(info).encode(),
                   **{name:b"fixture" for name in ("LICENSE","LICENSING.md","RECOVERY.md","LICENSES/Apache-2.0.txt",
                       "LICENSES/LGPL-2.1-libusb.txt","LICENSES/Python.txt","libusb-1.0.30.tar.bz2")}}
            files["SHA256SUMS"]="".join(f"{A.digest(data)}  {name}\n" for name,data in files.items()).encode()
            zipper(self.native / (prefix[:-1]+".zip"),{prefix+n:d for n,d in files.items()})
    def rebuild_site(self):
        files={"firmware/"+n:(self.public / n).read_bytes() for n in (A.PUBLIC[0],A.PUBLIC[1],A.PUBLIC[3])}
        files["emu/melodee.wasm"]=b"wasm fixture"
        manifest={"version":VERSION,"product":"FM-1_327008","sourceBranch":"v"+VERSION,
                  "sourceRevision":COMMIT,"sourceDirty":False,"sourceRepository":"copych/FM1-Ear-Trainer",
                  "channel":"attested development","workflowRun":"https://github.com/copych/FM1-Ear-Trainer/actions/runs/1",
                  "files":{n:A.digest(d) for n,d in files.items()}}
        files["build-info.json"]=json.dumps(manifest).encode()
        with tarfile.open(self.public / A.PUBLIC[4],"w:gz") as tar:
            for name,data in files.items():
                entry=tarfile.TarInfo("./"+name);entry.size=len(data);tar.addfile(entry,io.BytesIO(data))
        self.sums()
    def sums(self):
        (self.public / "SHA256SUMS").write_text("".join(f"{A.digest((self.public/n).read_bytes())}  {n}\n" for n in A.PUBLIC))
    def prepare(self):
        A.prepare(self.public,self.native,self.output,VERSION,COMMIT,"refs/tags/v"+VERSION)
    def test_complete(self):
        self.prepare()
        self.assertEqual(len(list(self.output.iterdir())),12)
        A.sums((self.output/"SHA256SUMS").read_text(),lambda n:(self.output/n).read_bytes(),
               [p.name for p in self.output.iterdir() if p.name!="SHA256SUMS"])
    def test_corrupt_firmware(self):
        with (self.public/A.PUBLIC[0]).open("ab") as file:file.write(b"bad")
        with self.assertRaises(AssertionError):self.prepare()
        self.assertFalse(self.output.exists())
    def test_mismatched_app_even_with_valid_sums(self):
        (self.public/A.PUBLIC[2]).write_bytes(b"different app");self.sums()
        with self.assertRaisesRegex(AssertionError,"App differs"):self.prepare()
    def test_missing_native_platform(self):
        next(self.native.glob("*windows*.zip")).unlink()
        with self.assertRaises(FileNotFoundError):self.prepare()
        self.assertFalse(self.output.exists())
    def test_corrupt_native(self):
        path=next(self.native.glob("*linux*.zip"))
        with zipfile.ZipFile(path) as archive:files={n:archive.read(n) for n in archive.namelist()}
        name=next(n for n in files if n.endswith("/fm1-recover"));files[name]=b"corruption";zipper(path,files)
        with self.assertRaisesRegex(AssertionError,"Checksum mismatch"):self.prepare()
    def test_wrong_source_commit(self):
        with self.assertRaises(AssertionError):A.prepare(self.public,self.native,self.output,VERSION,"b"*40,"refs/tags/v"+VERSION)

if __name__=="__main__":unittest.main()
