# Two-note Ear Trainer prototype

Current development 0.0.8 highlights and validation: [Keyboard highlights](KEYBOARD-HIGHLIGHTS.md).
Sound: [Original preset with polyphony](ORIGINAL-PRESET-POLYPHONY.md).
Playback: [Sustained playback](SUSTAINED-PLAYBACK.md).
Knob controls: [Sequence controls](SEQUENCE-CONTROLS.md).
Scoring history: [Adaptive scoring](ADAPTIVE-SCORING.md).
Build measurements below describe the earlier prototype.

Updated playback and re-entry behavior: see [regression fix and validation](REENTRY-PLAYBACK-FIX.md).
The build sizes/hashes below describe the initial prototype; the current local
installer's `build-info.json` identifies the rebuilt artifacts.

2026-11-08. Built on Melodee source revision
`165d177a425944f3d30b0b669255a6ed0050c3a5`, using the existing FM6 engine and
factory **TINE EP**. No replacement synthesis engine or preset conversion.

## Play

With the transport stopped, hold **HOME** and tap **PLAY** to enter.
The game plays C4 and then a hidden note above it, drawn from C-major intervals
(2, 4, 5, 7, 9 or 12 semitones). The display shows `C4` and `?`.
After listening, play just the second note on the built-in keyboard or MIDI.
The built-in layout is fixed F3–G5 in this mode; octave buttons do not shift it.
Pitch matching is exact, including octave. Responses sound through TINE EP.

- **PLAY**: replay/retry the same challenge.
- **REC**: generate the next challenge.
- **HOME**: exit; generated/response notes are released.

Keys held during playback must be released before a fresh press can answer.
Generated notes never enter answer collection. Wrong results fill the screen red
and lock further musical note-ons until retry/next. Correct results fill it green
for one second and then automatically start the next challenge. Note-offs remain
active in both states. Attempts count submitted answers. No scoring/replay limits yet.

Entering loads TINE EP into track 1 in the current in-memory project, stops
transport/recording and mutes other tracks. Selection, mute, solo and record-arm
flags are restored on exit; track 1's sound remains TINE EP (the ordinary sound
undo created by engine loading is retained). The mode blocks save/preset/transport
controls while open. Avoid saving over a project whose original sound you want
to preserve. No persistent game statistics or format changes.

## Implementation and execution contexts

- `firmware/src/ear_trainer.c`: fixed game state, interval generation, scheduling,
  raw keyboard/MIDI answer handling. Included by `seq.c` in the existing audio
  context. One bounded stage transition per CTL block; no wall-clock waits.
- `firmware/src/ear_ui.c`: main-loop screen and controls, using existing engine
  loading and graphics. Commands are published with IRQs masked; rendering reads
  only a small snapshot under the same protocol. Redraws occur on state changes.
- Minimal hooks in `seq.c`, `midi_control.c`, `ui.c`, `ui_input.c`, `ui_draw.c`.
  The game implementation is host compilable in the existing firmware harness;
  platform-independent module extraction is future refactoring, not claimed here.
- Real MIDI is tracked separately by port/channel/pitch, including holds before
  entering the game. Duplicate note-ons do not add owners; velocity-zero note-offs,
  USB/BLE loss, panic controls and queue-overflow cleanup are handled. Sustain,
  bend and other MIDI controls are ignored inside the game.
- Integer game logic only; existing FM6/voice/mixer DSP unchanged. No new audio
  allocations, storage, logs, waits or worker synchronization. The MIDI ownership
  bitset takes 1024 bytes in the pool rather than the tight main data region.
- The sound path remains Melodee's audio ISR and existing dual-core scheduler.
  Nominal FS 44100, CTL 32, HALF_FRAMES 128. Device audio.c's hardware period is
  defined by 544 crystal ticks/frame; preserve its counters and cadence rather
  than interpreting desktop render speed as device performance.

## Reproduce from PowerShell

```powershell
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-Ear-Trainer/tools/build-melodee.sh target
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-Ear-Trainer/tools/build-melodee.sh host
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-Ear-Trainer/tools/build-melodee.sh game-test
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-Ear-Trainer/tools/build-melodee.sh ui-test
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-Ear-Trainer/tools/build-melodee.sh input-test
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-Ear-Trainer/tools/build-melodee.sh web
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-Ear-Trainer/tools/build-melodee.sh web-test
```

Wrapper points at existing `/root/.jieli/toolchain` and
`../FM1-X0X-mod/.toolchains/AC79_SDK`. Environment overrides are supported.
Ubuntu WSL1; JieLi clang 4.0.1, r3; upstream flags/checks retained. Shell files
were normalized to LF only. Installed missing `python3-fonttools` (4.61.1) through
Ubuntu apt; requirements remain upstream Pillow/fonttools. Emscripten uses the
local Node ELF-loader launcher on WSL1. No global Windows PATH or Git trust changed.

## Verification

**MEASURED-LOCAL, host/build only:**

- Unmodified Melodee target baseline passed before game edits. App 559148 bytes,
  data+bss 97192/98304, pool 133028/357888, audio working arena 84192 bytes.
  Baseline package SHA-256
  `6bdd474b6ce6f5cc10107e09bc9278ef92052cafc6fd147f3cc8840d4c218c9e`.
  Original app/package copied to `baseline/`.
- Baseline and modified upstream UI regression executable: all passed.
- Modified upstream MIDI control/clock/ownership/panic integration suite: passed.
- Modified upstream flash-ID/persistence test: accepts 856014 (older Puya) and
  B36014 (newer UCUN), rejects absent/unknown/near-miss parts. This is software
  policy validation, not a hardware compatibility certificate.
- Existing host renderer, FM6 preset 0, CHORD=5, poly mode, sends disabled:
  five active voices confirmed. `tine-ep.wav` is a desktop render.
- Game integration test uses actual FM6, mixer, UI and input code: generated
  playback does not answer itself; wrong/retry/correct paths; MIDI and keyboard;
  held-input release gate; duplicate messages; port-loss cleanup; next/exit
  cancellation. FM6 part state is 7356 bytes in the host layout; target free RAM
  must be measured independently.
- Captured game PCM: 4.28 s, peak 14239/32768, RMS 5669.4; meaningful audio asserted.
  Screenshot `melodee/build/ear-baseline/ear-answer.png` visually inspected at
  240x240: first pitch, hidden target, state, attempts and controls readable.
  `game-demo.wav` captures the scripted challenge/answer scenario.
- Final target build passed: app 564228 bytes (+5080 over baseline),
  data+bss 97272/98304 (+80), pool 134052/357888 (+1024).
  Audio working arena 82336 bytes; static graph verifies 495 linked audio
  functions in SRAM and the FM6 DSP helpers. Stack audit reports audio ISR 2980
  bytes; this is static analysis, not a measured stack high-water mark.
  Main data region has 1032 bytes remaining: future additions need explicit
  budgeting. Neither these figures nor five voices on a desktop prove target
  real-time headroom.
- Final package SHA-256:
  `bbc1efa10fabe150bb0858c5a051bda53cc933b750e495d9ad9ee601834e2805`.
  `melodee/build/melodee.fwsc` retains the development identity FM-1_900;
  `melodee-res.bin` is its paired resource image. Not a published/attested release.
- Browser emulator compiled successfully: `melodee/build/emu/melodee.wasm`,
  1030319 bytes. Serve `melodee/build/emu` over localhost (for example, in WSL:
  `python3 -m http.server 8000 --directory melodee/build/emu` from project root),
  open `http://localhost:8000`, enable sound, then hold HOME and press PLAY on
  its panel. The browser simulator omits real USB/flash/peripheral behavior.
- WASM runtime smoke test passed against the built binary: FM6 audio peak 0.184,
  keyboard error feedback, replay, next and exit. This is an automated runtime
  test through the emulator exports, not manual browser interaction.

Full upstream sound golden/reference, all peripheral suites, sanitizers, manual
browser interaction and device tests have not been run for this patch. The selected
tests exercise the changed UI/input paths and reused preset; they are not substitutes
for the complete release suite. No flashing, bootloader installation or release.

## Compliance gate

| Category | Status / evidence |
| --- | --- |
| Target/build/ABI | PASS baseline and final prototype builds, same compiler/SDK/ABI and upstream integrity checks; final identity/hash/budgets above. |
| Audio hot path/numerics | PASS structural review: bounded integer game logic, direct existing voice calls; no new DSP, doubles, allocations, I/O, sleeps or blocking locks. Target audio-RAM graph checked. Actual instruction/cache cost UNVERIFIED. |
| Execution ownership/synchronization | PASS source/host scope: audio owns game and input maps, main publishes commands/snapshots under existing IRQ masking. No new core work. Hardware concurrency and maximum masking latency UNVERIFIED. |
| Memory/storage/I/O | PASS static layout checks, fixed state/bitmaps and existing storage layout. Runtime peak stack, dynamic arena use and I/O contention UNVERIFIED. |
| Dependencies/licenses | PASS: existing Melodee FM6/presets and GPL notices retained; new code follows AGENTS.md attribution. Build-only fontTools installed. No external synth dependency/assets imported. |
| Compatibility/package/recovery | PASS source/host flash gate for both known parts; existing packaging/recovery preserved. Development package retains Melodee identity and must not be presented as an official Melodee release. Device recovery/backup and both physical revisions UNVERIFIED. |
| Host functional validation | PASS focused game, preset/polyphony, original UI and MIDI/flash tests above; omissions stated explicitly. |
| Hardware real-time validation | UNVERIFIED: no local hardware cadence, CPU headroom, underruns, stack peaks, power-cycle or soak measurements. |

Before a hardware-qualified release: back up the actual unit and establish its
matching recovery route, test both flash revisions, verify speaker/headphone/MIDI
and key releases, then measure maximum audio service time and deadline misses
with five-note chords/tails plus dense MIDI and UI activity. Keep the existing
hardware counters and avoid deliberate power interruption on the user's sole unit.
