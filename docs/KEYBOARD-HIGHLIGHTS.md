# Reference keyboard lights — development 0.0.8

Keyboard keys light as the known exercise notes enter, stay lit while those notes
are held and clear at note-off/cancellation. Hidden ending notes never drive the
keyboard lights. Fully hidden exercises keep the entire keyboard dark during
playback. Display-label highlighting and original polyphonic TINE EP remain.

Main-loop ear_ui_key_lights snapshots the generated sounding mask/known prefix
under the existing IRQ mask, using at most four iterations and the fixed F3–G5
key layout. ui_leds overrides normal track/layout/input LEDs during training so
the synth's hidden generated voices cannot leak answers through existing LEDs.
The same output drives hardware LEDs and emulator web_led_keys/worklet frames;
no browser-only pitch inference or input injection is used.

## Compliance and validation

| Category | Status / evidence |
| --- | --- |
| Target/build/ABI | Same baseline/compiler/SDK/flags; final result below. |
| Audio hot path/numerics | PASS source: no audio ISR, scheduler or DSP changes; main-loop fixed four-slot integer mask only. No new allocations, doubles, library math, storage, logging or waits. Hardware CPU cost UNVERIFIED. |
| Ownership/synchronization | PASS source/host: read-only main-loop snapshot under existing IRQ masking; audio ownership unchanged. Actual masking latency UNVERIFIED. |
| Memory/storage/I/O | Fixed scalar snapshot, existing LED planes only; no persistence/format/write changes. Static target budget below, runtime peaks UNVERIFIED. |
| Dependencies/licenses | PASS: no dependency/assets; existing GPL/Melodee/Felucca notices retained. |
| Compatibility/package/recovery | Existing key matrix, panel mapping, loader, flash gates and resources retained; final package/site checks below. Physical revisions/recovery UNVERIFIED, no device writes. |
| Host functionality | Native game-test PASS: C4-only mask on first entry, C4/E4/G4 on known prefix, hidden B4 remains dark, clearing after release, fully hidden mask zero; prior mode/scoring/audio/polyphony/input tests pass. WASM result below. |
| Hardware real time | UNVERIFIED: no physical LED, timing, underrun, stack or soak measurements. |

Full upstream UI/golden/peripheral suites and sanitizers not rerun. Before hardware
qualification, confirm actual key-to-LED mapping on both revisions at K4=0/1000,
partial/all-hidden exercises, repeated pitches, settings changes/replay/exit and
held player keys. Measure worst audio service/masking time, underruns and memory
peaks with dense MIDI/UI and four-note exercises. Use verified backup/recovery.
Local development package only, not an attested public release.

Rebuilt WASM web-test PASS: actual web_led_keys output equals C4's key bit during
the known note and after the hidden second enters, then zero after release.
All-hidden four-note playback returns zero both during the first note and the
sustained chord. Existing audio, MIDI answer, feedback, replay and reentry
checks also pass. These test the simulator's actual firmware LED output; manual
browser interaction/physical LEDs were not tested.

Target/build/ABI PASS: app 566640 B, package 668948 B (FM-1_900 development
identity); upstream integrity, section, stack, size and package checks pass.
Static memory PASS: .data+.bss 97336/98304 B, pool 134052/357888 B, audio
arena 81088 B; static audio ISR stack audit 2980 B, 495 linked audio-RAM
functions verified. Hardware/runtime measurements remain UNVERIFIED.

Site 0.0.8 generated with matching resources/WASM; routes, script syntax, hashes
and installer selection checks PASS. No device written. Firmware SHA-256:
`a0d786e6a80b4d4075e99ca564dcbe313bf335d4c846ffdfe469ed5139dc888f`.
Reproduce with established WSL `tools/build-melodee.sh` modes `game-test`, `web`,
`web-test`, `target`, `site`, then Windows `node tools/check-site.cjs` against
the local server. Reload an open emulator to load the new LED behavior.
