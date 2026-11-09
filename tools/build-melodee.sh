#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
export AC79_SDK="${AC79_SDK:-$ROOT/../FM1-X0X-mod/.toolchains/AC79_SDK}"
if ! node --version >/dev/null 2>&1; then
    export EM_NODE_JS="$ROOT/tools/node-wsl.sh"
fi
cd "$ROOT/melodee"
case "${1:-target}" in
  target) exec sh build.sh --project ../project.json ;;
  host) mkdir -p build/ear-baseline
        cc -O2 -ffp-contract=off -w -Ibuild/gen -Ifirmware/src tests/hostsim.c -lm -o build/ear-baseline/hostsim
        CHORD=5 VOICES=1 SENDS=0,0,0 exec build/ear-baseline/hostsim 12 0 0 build/ear-baseline/tine-ep.wav ;;
  ui-test) mkdir -p build/ear-baseline
           cc -O1 -ffp-contract=off -w -Ibuild/gen -Ifirmware/src tests/ui_test.c -lm -o build/ear-baseline/ui_test
           exec build/ear-baseline/ui_test ;;
  game-test) mkdir -p build/ear-baseline
             cc -O1 -ffp-contract=off -w -Ibuild/gen -Ifirmware/src tests/ear_trainer_test.c -lm -o build/ear-baseline/ear_test
             build/ear-baseline/ear_test
             exec python3 ../tools/ear-artifacts.py ;;
  input-test) mkdir -p build/ear-baseline
              cc -O1 -ffp-contract=off -w -Ibuild/gen -Ifirmware/src tests/midi_control_test.c -lm -o build/ear-baseline/midi_control_test
              build/ear-baseline/midi_control_test
              cc -O1 -ffp-contract=off -w -Ibuild/gen -Ifirmware/src tests/flash_id_test.c -lm -o build/ear-baseline/flash_id_test
              exec build/ear-baseline/flash_id_test ;;
  web) exec sh web/emu/build.sh ;;
  settings-test) mkdir -p build/ear-baseline
                 for suite in ear_persistence_test settings_test persistence_test settings_origin_test; do
                   cc -O1 -ffp-contract=off -w -Ibuild/gen -Ifirmware/src "tests/$suite.c" -lm -o "build/ear-baseline/$suite"
                   "build/ear-baseline/$suite"
                 done ;;
  web-test) exec sh ../tools/node-wsl.sh ../tools/ear-web-smoke.mjs ;;
  site) exec python3 ../tools/prepare-web.py ;;
  *) echo 'usage: build-melodee.sh [target|host|ui-test|game-test|input-test|settings-test|web|web-test]' >&2; exit 2 ;;
esac
