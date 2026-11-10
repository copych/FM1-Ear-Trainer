# FM-1 Ear Trainer

An ear-training game for the M-VAVE FM-1, built on [Melodee](https://github.com/keremimo/melodee) and its existing FM6 and Prophet synthesizers. Listen, then reproduce the hidden notes using the built-in keyboard or supported MIDI input.

## Current prototype — 0.0.13

- C major and A natural minor exercises with 1–4 distinct pitches within one octave or across the full F3–G5 keyboard range.
- Choose how many ending notes to hide; reproduce them in order, releasing keys between entries. With every note hidden, the starting pitch changes each question.
- Select any of 24 factory FM6 sounds and 200 Prophet factory programs, always polyphonic; default TINE EP, with overlapping notes and adjustable onset spacing (zero plays a simultaneous chord).
- Known notes highlight on the display and keyboard. Hidden notes never light a key during playback.
- Green/red feedback, automatic next question after success and same-question replay after mistakes. Extra musical keys are blocked during playback/feedback.
- Attempt/replay scoring and adaptive weighting toward missed degrees. Scale, complexity/range, spacing and preset save automatically; scores and practice history reset on restart.
- Browser emulator and locally generated web installer.

## Controls

With transport stopped, hold **HOME** and tap **PLAY** to enter training.

| Control | Function |
| --- | --- |
| K1 | C major / A natural minor |
| K2 | Complexity: hidden notes of total notes, e.g. 1 OF 3; octave block then full-keyboard block; default 1 OF 2 OCT |
| PRESETS | FM6 sounds, then Prophet factory programs; always poly mode |
| K3 | Reserved |
| K4 | Time between note starts: 0–1000 ms, in 25 ms steps |
| PLAY | Replay / retry |
| REC | Next question |
| HOME | Exit training |

K2 steps through 1 of 1, 1 of 2, 2 of 2, 1 of 3, 2 of 3, 3 of 3, 1 of 4, 2 of 4, 3 of 4, and 4 of 4 within OCT (C4–C5 / A3–A4), then repeats the combinations in FULL (F3–G5). Changing scale, complexity or spacing starts a fresh question; turning PRESETS replays the current question with the selected sound. The built-in keys use a fixed F3–G5 layout. Matching includes octave. All notes release one second after the last onset; the original TINE EP's natural decay remains.

Preferences save to the device during a quiet answer pause (after a one-second edit debounce) or after exiting training. The emulator remembers them in browser storage.

Entering training replaces track 1's in-memory sound with the selected training preset and temporarily mutes other parts. Track 1 retains that sound on exit. Avoid saving over a project whose previous track-1 sound you need.

## Build and run

See [build instructions](docs/BUILDING.md) for toolchain/SDK setup, host tests, emulator and installer generation. Build outputs, toolchains and local backups are excluded from Git. The development installer is hosted on GitHub Pages.

Hosted development site: **[Emulator](https://copych.github.io/FM1-Ear-Trainer/emu/)** · **[Installer](https://copych.github.io/FM1-Ear-Trainer/webapp/installer/)** · [Guide](https://copych.github.io/FM1-Ear-Trainer/webapp/manual/).

GitHub Pages deploys only after the required shared workflow builds and attests the firmware/site, host/WASM checks pass, and the deployment job verifies provenance and matching resources. Releases have been tested on FM-1 hardware. See [publishing details](docs/GITHUB-PAGES.md).

For local use after building the site, run `node tools/serve-site.cjs 8010` (or `./tools/start-web.ps1` on Windows), then open `http://127.0.0.1:8010/`.

## Validation and limits

Native/WASM game tests, target builds and package/hash checks have passed. See the [latest validation/compliance report](docs/KNOB-CARDS-PROPHET.md) and [sound/polyphony report](docs/ORIGINAL-PRESET-POLYPHONY.md).

Both known flash IDs pass software checks. Physical hardware revisions, worst-case audio timing, runtime headroom and recovery are not yet qualified. Back up your unit and establish its recovery route before installing firmware.

Broader chord recognition, arbitrary keys/scales, relative-pitch matching, persistent statistics and additional settings remain future work.

## Source and licensing

The vendored `melodee/` source is based on upstream commit `165d177a425944f3d30b0b669255a6ed0050c3a5`. Existing Melodee/Felucca attribution, licenses and build integrity checks are preserved. New firmware files follow `melodee/AGENTS.md` attribution instructions.

Firmware is GPL-3.0-only; see [LICENSE](LICENSE), [upstream licensing details](melodee/LICENSING.md) and `melodee/LICENSES/` for third-party components/assets. This independent development project is not an official Melodee or M-VAVE release. Public firmware releases must follow the attested build/recovery process in `melodee/AGENTS.md`.

The `docs/` directory retains development history. Older workstation paths and experiments are historical, not current build requirements.
