# Build FM-1 Ear Trainer

The application source is vendored in `melodee/`, based on Melodee commit
`165d177a425944f3d30b0b669255a6ed0050c3a5`. Upstream's build integrity and
license checks remain enabled. See `melodee/BUILDING.md` for platform details.

## Dependencies

Use Linux x86-64, or Ubuntu under WSL on Windows. Install a native C compiler,
Python 3 with Pillow/fonttools and Raqm font support, Node.js and Emscripten.
The locally verified environment used JieLi pi32v2 clang 4.0.1 r3, Python
3.14.4, Pillow 12.1.1, fonttools 4.61.1 and Emscripten 3.1.69.

Obtain the JieLi compiler using `melodee/tools/get_toolchain.sh` and the AC79
SDK using the instructions in `melodee/BUILDING.md`. The SDK packaging inputs
are checked by the upstream build; don't bypass these checks. The locally
verified SDK tag is `AC79NN_SDK_V1.2.1_2023-12-13`, commit
`d179b4484759423312073f5fbb232501aa491047`.

Set paths explicitly; SDK and compiler are not included in this repository:

```sh
export JIELI_TOOLCHAIN="$HOME/.jieli/toolchain"
export AC79_SDK="$HOME/fw-AC79_AIoT_SDK"
sh tools/build-melodee.sh target
sh tools/build-melodee.sh game-test
sh tools/build-melodee.sh web
sh tools/build-melodee.sh web-test
sh tools/build-melodee.sh site
```

On Windows, start the local server with `./tools/start-web.ps1`. On Linux,
run `node tools/serve-site.cjs 8010`. Open `http://127.0.0.1:8010/`.
Run `node tools/check-site.cjs` to verify served package/resource/WASM hashes.
The `ui-test`, `input-test` and `host` wrapper modes provide additional checks.

The wrapper retains a local-workspace SDK fallback for the original developer's
checkout; other users should set `AC79_SDK` as above. `tools/node-wsl.sh` handles
the locally encountered Node/WSL1 loader issue; ordinary Linux Node is used when
available. Build outputs appear under `melodee/build/` and `build/site/`.

## Development packages and hardware

Project builds use `project.json`: version 0.0.9 and dedicated `FM-1_327008`
identity. Earlier prototypes used shared `FM-1_900`. These development builds
are not official Melodee releases or hardware-qualified builds. Back up the
actual unit and confirm its matching recovery procedure before installing.
Both known flash IDs pass source/host checks; physical revision compatibility,
worst-case CPU/stack headroom and recovery remain unverified.

Publishing source does not publish a firmware release. Firmware releases must
follow `melodee/AGENTS.md` and its upstream attested build/recovery workflow.
The upstream workflows remain under `melodee/.github/` as reference and do not
run as root GitHub Actions workflows for this repository.
