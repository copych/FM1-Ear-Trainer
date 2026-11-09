# FM-1 Ear Trainer: foundation verification

Inspected and built 2026-11-08. No game implementation or device writes performed.

**Updated priority:** support both known FM-1 revisions and provide a minimum
poly synth. A separate [eight-voice FM app](ear-trainer/README.md) now provides
two-operator FM with Warm/Bright presets; target/host/web builds and focused tests pass. This
supersedes the proposed requirement to integrate Melodee FM6 before proceeding.
Hardware qualification remains pending, but host game development can continue.

## Decision

Select **fm1-base on its pinned felucca-bsp** for the portable game/platform boundary.
The unmodified template builds locally for the target, host and browser. This is
a platform selection, not a claim that the requested FM synthesis is already supplied.
Milestone 1 remains open pending device checks and synthesis integration.

| Candidate | Actual source and interfaces | Suitability |
| --- | --- | --- |
| fm1-base | MPL-2.0; `include/plat.h`, device adapter, script-driven host and browser implementations; template demonstrates keys, LEDs, display, MIDI and sound | Best small application foundation. Beta API. Template sound is monophonic sine/triangle, **not an FM engine**, and cannot play five-note chords. |
| felucca-bsp | MPL-2.0; chip/board drivers, loader, packaging and recovery; no synthesis or application UI | Use through fm1-base; standalone use would require recreating its application services. |
| Melodee | GPL-3.0-only application; existing FM6 and other engines, keyboard, display, MIDI, host/web infrastructure | Best alternative if retaining a complete existing FM instrument is the priority. Substantial sequencer, storage, voice scheduler and dual-core infrastructure to retain or untangle. FM6 is not a drop-in `plat.h` service. Source obtained; target build not attempted. |
| Felucca / X0X | Existing Felucca-family application infrastructure; local X0X checkout already modified for other work | Avoid disturbing the existing project. X0X engines do not provide the requested general polyphonic FM instrument. Felucca was reviewed through published documentation, not separately built. |
| Official FM1 Open | Existing local V14 source study; matched binary SDK/ABI required | Alternative for the vendor FM engine, but generic AC79 packaging inputs are not that SDK. Not selected or built in this task. |

The existing synthesis requirement must be preserved: evaluate reuse of Melodee's
FM6/core and dependencies under GPL before game implementation. Do not describe the
template oscillator as the factory FM engine, write a new synth, or silently reduce
the chord requirement. If engine adaptation proves excessive, prefer a Melodee
application fork. Neither engine integration nor five-note polyphony is verified here.

## Source lock and environment

- `ear-trainer-base`: https://github.com/charlesvestal/fm1-base
  commit `b0976f295ccaeaf77a8053a5a785a9cc21f5345f`.
- Its `bsp` submodule: https://github.com/hugelton/felucca-bsp
  commit `5b0879bfadc69a17dbebb917fb6b86d516f19c44`.
- `ear-trainer-research/melodee`: https://github.com/keremimo/melodee
  commit `165d177a425944f3d30b0b669255a6ed0050c3a5` (shallow research checkout).
- Existing `.toolchains/AC79_SDK`: tag `AC79NN_SDK_V1.2.1_2023-12-13`,
  commit `d179b4484759423312073f5fbb232501aa491047`.
- Windows host, Ubuntu **WSL1**, x86-64, kernel `4.4.0-19041-Microsoft`.
  WSL2 is proposed but not used. Prior setup notes report firmware virtualization
  disabled; no BIOS or virtualization settings changed here.
- Existing `/root/.jieli/toolchain/pi32v2/bin/clang`: version 4.0.1.
  Archive directory recorded by earlier setup: `jieli-linux-toolchains-20260730.1`.
  Previously recorded compiler archive SHA-256:
  `a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd`.
- Host GCC 15.2.0; Python 3.14.4; Pillow 12.1.1; Emscripten 3.1.69;
  Node v22.22.1. The compiler archive hash was rechecked locally and matches above.
  No compiler, SDK or system packages were installed during this task.
- Target build flags from `tools/build.py`: `-Os -ffunction-sections -fno-builtin
  -Wall -Wno-unused-function`, CPU `r3`, `-mfprev1 -ffp-contract=off`.
  Keep upstream build/link flags and checks; no tuning changes made.

## Reproduce

From PowerShell in this workspace:

```powershell
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-X0X-mod/tools/build-ear-baseline.sh target
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-X0X-mod/tools/build-ear-baseline.sh host
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-X0X-mod/tools/build-ear-baseline.sh web
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-X0X-mod/tools/build-ear-baseline.sh tests
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-X0X-mod/tools/build-ear-baseline.sh versions
```

The wrapper sets `JIELI_TOOLCHAIN` and `AC79_SDK`, then invokes upstream
`python3 tools/build.py template`, `--host`, `--web`, or `sh tests/run_tests.sh`.
Host mode also executes `template/tests/basic.txt`.
It never calls an installer or publishes anything.

Fresh source acquisition:

```powershell
git -c core.autocrlf=false clone --recursive https://github.com/charlesvestal/fm1-base.git ear-trainer-base
git -C ear-trainer-base checkout b0976f295ccaeaf77a8053a5a785a9cc21f5345f
git -C ear-trainer-base submodule update --init --recursive
```

Existing Windows checkout shell files were converted CRLF to LF so Linux can run
them; no application C, build algorithms or configuration changed. Use LF on fresh
checkouts too. WSL1 could not directly execute `/usr/bin/node` (Exec format error).
The wrapper reuses `tools/node-wsl.sh` and `tools/wsl-bin/node`, which invoke Node
through `/lib64/ld-linux-x86-64.so.2`; this fixed the browser build.
The loader also makes Node report the loader as `process.execPath`. Local preload
`tools/ear-node-wsl.cjs` redirects child Node launches to the Node wrapper, without
changing upstream firmware or tests. This is scoped to the build wrapper environment.

## Local evidence

**MEASURED-LOCAL (build/host only):**

- Unmodified target template build passed. Package
  `ear-trainer-base/template/build/template.fwsc`: 614938 bytes,
  identity `FM-1_1100`, SHA-256
  `13b732d56a2ad39b1b92bd42e16fd7731cc768206a51072df955877634484cea`.
- App 225728 bytes; loader image 11782 bytes. RAM code 9164/32768 bytes;
  data+bss 76420/98304; pool 93960/344064. These are static budgets,
  not stack peaks or measured audio headroom.
- Upstream checks passed: no soft-double helpers, no rejected interleaved wide
  multiply/load, image/RAM bounds, BSP-only register access. Default template
  includes Bluetooth MIDI and USB audio; excludes CPU2 and USB host.
- Host basic scenario passed key note-on/off, octave and encoder controls,
  save/reboot, incoming MIDI, routing, velocity/pressure and generated audio
  peak (-26.3 dBFS against -30 minimum). Simulated 13186 ms.
  Host CPU performance does not predict device timing.
- Host `key.png` visually inspected: C4 displayed, matching key highlighted.
- Browser build passed: `template/build/emu/app.wasm`, 50655 bytes.
  Browser interaction/audio not yet manually tested. fm1-reader voice dependency
  absent; build provides plain text screen reading instead.
- Host compilation reports two upstream signedness warnings in `lib/sense.c`.
- The full upstream `tests/run_tests.sh` completed: all tests passed except
  `emu_resample`, which failed when child Node launched the ELF loader as Node.
  After the local launcher fix, the focused `resample-test` wrapper command passed
  at 44100, 48000, 96000 and 22050 Hz: C4 measured 261.63 Hz, with timing and
  sample-step assertions passing. The entire suite was not repeated after this
  environment-only fix. Per-test logs are in `template/build/tests/`; the initial
  `emu_resample.log` retains the original failure. These are simulated browser
  tests, not a manual browser session or device audio measurements.

## Hardware interfaces verified in source

**VERIFIED-SOURCE**, not device measurements:

- `plat_keys()` is a 27-key bitmask, F3 through G5; template key 7 maps to C4
  (MIDI 60). Built-in keys work independently of external MIDI in the source path.
- `plat_buttons()`, `plat_enc()`, `plat_lights()` provide panel navigation/LEDs;
  drawing uses `lib/gfx.c` and platform LCD functions. BSP display is 240x240 RGB565.
- `plat_midi_in()` yields USB-MIDI packets and port; USB, TRS and optional BLE
  are distinct from locally generated challenge events. Keep exercise playback
  internal; never feed it back through the answer collector.
- `app_render()` runs in the audio interrupt, stereo int32 24-bit samples;
  template `APP_BLOCK=256`, rate 44100 Hz (~5.805 ms/block).
- `app_frame()` handles main-loop UI at about 60 Hz; `app_input_only()` polls
  between frames. Optional `APP_TICK` is a 1 kHz timer interrupt. Use bounded
  scheduling and explicit publication between contexts; no UI/storage in audio.
- USB host exists behind `usb_host=true`, but is excluded from the baseline;
  BSP labels its port as needing hardware validation. Leave optional for milestone 1.
- Flash writes can suspend sound; save only outside challenge playback.

## Backup, recovery and device acceptance gate

No attached unit, its bootloader revision, flash ID or saved data was inspected.
Recovery is documented from pinned source, **not locally verified**.

1. Export projects, voices, samples and settings using the firmware currently
   installed. Preserve exports separately; firmware rollback does not restore them.
2. Obtain matching official stock packages before any write. The pinned BSP
   rescue accepts specific V15/V16 hashes and selects by existing flash head;
   do not assume a generic stock file fits every bootloader.
3. Inspect `bsp/tools/fm1_rescue.py`: without `--write` it loads a RAM helper,
   checks the device and reads a full flash backup. This still changes running
   device state and has not been executed. Its raw USB route is Linux/macOS;
   WSL1 USB access is not established. Arrange supported USB access first.
4. Confirm the backup exists, is 1 MiB for a supported unit, record its hash,
   and retain an additional copy. Verify identity/flash/bootloader and the
   matching package before proceeding. Preserve bootloader and device data.
5. The template manual says REC at power-on: behavior differs with m-bootloader
   versus chip UBOOT. Identify the actual boot chain first; do not use another
   firmware's button sequence or install a new bootloader as part of setup.
6. After backup and a reviewable recovery route, test the baseline on hardware:
   display/encoders/buttons, all 27 keys and release behavior, octave controls,
   speaker/headphones at safe volume, tuning, MIDI input and interruption/panic.
   Then verify engine polyphony and audio cadence under UI/MIDI load.

## Next functional target (not implemented)

Once hardware and reused synthesis are verified, add portable fixed-size challenge,
evaluation and playback modules. Generate two in-range diatonic notes, play them
in order, display the first pitch and `?` for the second, accept only the player's
target note after playback, then show correct/incorrect with Retry and Next.
Ignore held keys crossing playback completion until released. Cancel outstanding
note-offs safely on retry/next. Test scripted correct/wrong/retry/next, key release,
MIDI velocity-zero note-off and cancellation before adding longer melodies/chords.

## Compliance scope

No firmware patch was made. Target/build and host basic function: PASS for the
baseline above. Numerics: upstream linked-helper checks PASS; full hot-path timing
audit UNVERIFIED. Execution ownership: source paths identified, hardware concurrency
UNVERIFIED. Memory: static bounds PASS; runtime stack/storage timing UNVERIFIED.
Dependencies/licenses: source headers inspected; no engine/assets imported.
Package generation PASS; backup/rollback compatibility and all hardware real-time
categories UNVERIFIED. Resolve through the device acceptance gate above.
