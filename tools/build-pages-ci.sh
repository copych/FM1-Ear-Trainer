#!/usr/bin/env bash
# Runs inside the required upstream attested workflow; never flashes hardware.
set -euo pipefail
cd "$(dirname "$0")/.."
sudo apt-get update -qq
sudo apt-get install -y build-essential libharfbuzz-dev libfribidi-dev libraqm-dev
python3 -m pip install Pillow==12.1.1 fonttools==4.61.1
test "$(git -C "$AC79_SDK" rev-parse HEAD)" = d179b4484759423312073f5fbb232501aa491047
git clone --depth 1 --branch 3.1.69 https://github.com/emscripten-core/emsdk.git build/emsdk
build/emsdk/emsdk install 3.1.69
build/emsdk/emsdk activate 3.1.69
source build/emsdk/emsdk_env.sh
sh tools/build-melodee.sh target
sh tools/build-melodee.sh web
sh tools/build-melodee.sh site
python3 tools/package-pages.py
