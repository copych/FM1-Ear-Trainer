"""Validate the project's Hub card and its build/USB identity manifest."""
from pathlib import Path
import json
import re
import sys
import tempfile
from PIL import Image

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "melodee/tools"))
import build

project = json.loads((root / "project.json").read_text())
assert build.project_identity(root / "project.json") == ("FM-1_327008", "v0.0.8")
with tempfile.TemporaryDirectory() as temp:
    path = Path(temp) / "bad.json"
    for product in ("FM-1_12", "FM-1_123456789", "OTHER_327008", 'FM-1_327008"'):
        path.write_text(json.dumps({**project, "product": product}))
        try:
            build.project_identity(path)
        except ValueError:
            pass
        else:
            raise AssertionError(f"Invalid identity accepted: {product}")
card = json.loads((root / "fmhub.json").read_text())
assert 1 <= len(card["what"]) <= 300 and not re.search(r"<|https?://", card["what"])
assert len(card.get("author", "")) <= 60
assert card["kinds"] and set(card["kinds"]) <= {"synth", "groove", "drums", "chords", "tracker", "other"}
assert re.fullmatch(r"#[0-9A-Fa-f]{6}", card["color"])
for key in ("play", "editor", "cheat"):
    assert card[key].startswith("https://copych.github.io/FM1-Ear-Trainer/")
image_path = root / card["image"]
assert image_path.stat().st_size <= 300 * 1024
with Image.open(image_path) as image:
    assert image.width == image.height and 240 <= image.width <= 1024
print("Hub metadata PASS: bounded card text, links, image and validated dedicated build identity")
sys.path.insert(0, str(root / "melodee/web"))
import make_site
package = root / "melodee/build/melodee.fwsc"
if package.exists():
    assert make_site.check_package(package, project["product"])[1] == project["product"]
    for expected in (None, "FM-1_327009"):
        try:
            make_site.check_package(package, expected)
        except SystemExit:
            pass
        else:
            raise AssertionError("Project package accepted without matching explicit identity")
    with tempfile.TemporaryDirectory() as temp:
        fake = Path(temp) / "no-loader.fwsc"
        fake.write_bytes(package.read_bytes().replace(b"FELUCCA-LOADER-1", b"NO-VALID-LOADER!"))
        try:
            make_site.check_package(fake, project["product"])
        except SystemExit:
            pass
        else:
            raise AssertionError("Package without update loader accepted")
    print("Installer gate PASS: exact project identity required and update-loader marker retained")
