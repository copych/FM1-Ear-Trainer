# Sequence controls — development 0.0.4

Historical timing description: development 0.0.5 replaces silent gaps with
[sustained arpeggiated playback](SUSTAINED-PLAYBACK.md).

K1 selects C major (C4 reference) or A natural minor (A3 reference).
K2 sets total notes 1–4 (default 2); K3 sets hidden ending notes 1–K2
(default 1, clamped when K2 decreases). Play the hidden notes in order,
releasing keys between answers. All exercise notes are heard; only the leading
reference notes are displayed. Hidden pitches remain hidden during feedback.

When K3 equals K2, no reference pitch is displayed. The first played note is
randomized among scale degrees 2–8 and differs from the preceding question's
first note. It is not anchored to the tonic. K2=1 is a single-pitch exercise.
Reference octaves keep all notes within the built-in F3–G5 keyboard.

K4 sets the silent note-off-to-next-note-on gap, 0–1000 ms in 25 ms steps
(default 250 ms). Notes retain a 500 ms gate. At zero gap, note-off and the next
note-on occur in the same control tick. Timing is quantized to the existing
32-frame control cadence; no waiting or per-sample scheduler added.

Turning a setting cancels playback/input and generates a fresh question, resetting
attempts for that question. Settings and scores remain in RAM across training
exit/reentry. Practice weights continue to track target scale degrees, shared
between the two scales. Score rules from ADAPTIVE-SCORING.md apply per complete
sequence, with the first wrong note ending an attempt. Partial correct entries
cannot earn points; automatic repeats retain the whole question and attempt count.

Changed paths: main-loop ear_ui_input reads panel-mapped K1–K4 and atomically
publishes settings/NEXT under existing IRQ masking. Audio ISR snapshots settings
only for NEXT, generates at most four pitches, schedules them at control rate,
and evaluates at most one input note per press/release cycle. UI snapshots a
fixed four-note array. FM6 synthesis and existing update/storage layout unchanged.

## Compliance and validation

| Category | Status / evidence |
| --- | --- |
| Target/build/ABI | Final target result recorded below; same baseline, compiler/SDK and flags as PROTOTYPE.md. |
| Audio hot path/numerics | PASS source structure: bounded four-note/seven-degree loops, integer scheduling and fixed state. No new allocation, DSP, doubles, I/O, logging or waits. Actual target instruction cost UNVERIFIED. |
| Ownership/synchronization | PASS source: existing audio ownership and IRQ-masked main publication/snapshots retained; no worker changes. Hardware latency UNVERIFIED. |
| Memory/storage/I/O | Fixed arrays/settings only; no storage writes or format changes. Static budget below; runtime stack/arena peaks UNVERIFIED. |
| Dependencies/licenses | PASS: no dependencies/assets added; existing GPL notices and attribution retained. |
| Compatibility/package/recovery | Existing loader, flash gates and paired resources preserved; final packaging checked below. Physical revisions, backup/recovery UNVERIFIED; no device writes. |
| Host functionality | Focused game/WASM checks recorded below. Full upstream golden/peripheral/UI suites and sanitizers not rerun for this patch. |
| Hardware real time | UNVERIFIED: measure worst service time, underruns, stack peaks and soak on both revisions before qualification. |

Hardware checks: verify each physical K knob/direction, extremes and changes
during playback/feedback; listen to major/minor sequences and gap endpoints;
answer 1–4 hidden notes including repeated pitches using local and MIDI keys;
check held-key release gating, wrong auto-repeat, scoring and reentry. Test under
dense MIDI, UI redraws and five-voice FM6 tails with a verified recovery backup.
This remains a local development package, not an attested public release.

## Local results

- Native `game-test` PASS: prior scoring/replay/input regressions plus all 20
  scale × count × hidden-count combinations, local ordered answers, no partial
  awards, wrong second-entry MIDI lockout and same-question replay.
- Real panel-mapped K inputs PASS: minor selection, count/guess clamps and
  0/1000 ms gap limits. Four-note 1000 ms-gap playback reaches the answer state.
- Full-hidden consecutive questions change first pitch; all generated pitches
  remain in the selected scale and the built-in keyboard range.
- Native four-hidden and mixed-reference framebuffer PNGs inspected: settings,
  hidden placeholders and progress fit the 240 × 240 display.
- Rebuilt WASM `web-test` PASS: audible default C4/E4, keyboard/MIDI feedback,
  automatic replay, three reentry cycles; actual encoder API switches to four
  hidden A-minor notes at zero gap. Spectral detection found MIDI 64,60,64,65;
  entering those notes awards success only after the fourth entry.
- Target/build/ABI PASS: app 566276 B; package 668948 B, FM-1_900 development
  identity. Upstream manifest, RAM-section, stack, bounds and package gates pass.
  Static memory PASS: .data+.bss 97336/98304 B (968 B remaining), pool
  134052/357888 B, audio arena 81472 B; static audio ISR stack audit 2980 B.
  495 linked audio-RAM functions verified. Runtime peaks/timing remain UNVERIFIED.
- Site 0.0.4 regenerated with matching resources/WASM; routes, script syntax,
  served hashes and installer local-package selection PASS. No device written.
- Firmware SHA-256:
  `799fc2ffa4bf57fc333112fb7dee07d62bda392f65a3408994a0847890ab967f`.

Reproduce in the established WSL environment with `tools/build-melodee.sh`
modes `game-test`, `web`, `web-test`, `target`, `site`, then Windows
`node tools/check-site.cjs` with the local server running.
