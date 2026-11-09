# Octave/full complexity and polyphonic presets — 0.0.10

K2 now has twenty positions: the ten hidden/total combinations (1/1 through
4/4) in OCT, followed by the same ten in FULL. OCT spans C4–C5 in C major or
A3–A4 in A natural minor, including both tonic octaves. FULL spans the built-in
F3–G5 keyboard. The default is 1 OF 2 OCT. Pitches remain distinct within each
exercise; matching is ordered and octave-specific. All-hidden first pitches
change between questions. A reference remains C4/A3 when hints are shown.

K3 browses the 24 existing factory FM6 presets, clamped at each end. The name is
visible in the footer. Every load explicitly sets `V_POLY` after the factory
defaults, including factory mono bass, lead and flute sounds. TINE EP remains
the default. The selection survives leaving/re-entering training during the
same session; it is not persisted across reboot. Selecting a sound cancels old
playback and replays the current question without a replay penalty. If already
correct, it advances instead, preventing a duplicate score award. Scale,
complexity or spacing changes still create a new question. Track 1 retains the
selected sound on exit, as documented in README.md.

## Baseline, ownership and critical path

Source baseline: `59ba17dd76099844429b0a89091a0618d39d3f96` (0.0.9).
The existing native game/audio suite passed before editing. Vendored Melodee,
JieLi pi32v2 clang 4.0.1 r3, matched AC79 SDK, build flags, boot/update loader,
board support, linker layout, audio sample/block configuration and execution
model remain unchanged; see BUILDING.md and FULL-RANGE-COMPLEXITY.md.

Main-loop `ear_ui_input` publishes paired range/count/guess settings under the
existing IRQ protocol. Audio-owned `ear_audio_tick` copies range on NEXT and
`ear_pick_note` adds a bounded range filter to each existing 27-key scan. The
smallest candidate set has eight scale pitches; at most four are excluded, so
weighted selection retains a positive total. No rejection loops, allocation,
storage work, logging, floating-point arithmetic or new render calls are added.

`ear_ui_load_preset` runs only in main, reusing `engine_load_of` and the existing
factory/undo/FM6 patch publication path. Main first publishes `ear_loading` with
IRQs masked. The audio tick releases generated/player notes and blocks new
playback while that flag is set; raw player note-on cannot answer during the
publication window. Main loads the factory sound, then publishes forced poly
mode, zero FX sends and the repeat/next command before clearing the flag.
Existing FM6 patch generations and panic requests retire old voices. PREPARE
still waits for the FM6 engine/fade before the exercise's initial 200 ms delay.
No factory/undo copying moves into the audio callback. MIDI held-key ownership
and release gating remain active during the change.

## Validation and compliance

| Category | Status and evidence |
| --- | --- |
| Target / build / ABI | PASS: target build with original manifest, section, software-double, size and stack checks. No ABI or toolchain change. |
| Audio hot path / numerics | PASS for structural review: range filtering is bounded integer work on NEXT, loading guard is bounded, synthesis unchanged. Existing target RAM closure gates pass; actual latency unmeasured. |
| Ownership / synchronization | PASS for source/host scope: existing IRQ publication retained, main-only preset loading, audio/input guard, poly/send state published before restart, held-key gating preserved. |
| Memory / storage / I/O | PASS for static target bounds: two small range/preset fields and loading flag, no new dynamic buffers, storage formats or peripherals. Target budgets recorded below; runtime peaks unmeasured. |
| Dependencies / includes / licenses | PASS: existing factory presets and loading APIs only; no dependency, header, asset or licence changes; notices retained. |
| Compatibility / package / recovery | PASS for source/host gates: product FM-1_327008 and loader unchanged, MIDI and both flash-ID checks pass. Offline package/recovery checks required before publication. Physical new-build install/rollback UNVERIFIED. |
| Host functional validation | PASS: game suite covers both ranges/scales, all twenty K2 positions forward/back/clamped, all 24 K3 presets and clamps, four simultaneous voice gates after mono-preset overrides, same-question replay, loading-window input lock, no duplicate award, selection on re-entry and cancellation. Existing scoring, lights, MIDI and UI suites pass. WASM/site checks recorded separately with the build logs. |
| Hardware real-time validation | UNVERIFIED: no new-build device, timing/headroom, underrun, analog output or soak measurements. Earlier owner reports do not qualify this revision. |

Generator tests run 1000 four-note exercises for each scale/range, verifying
complete eligible-note coverage, bounds, distinct pitches, both directions and
deterministic replay. Native screen captures verify the three-row footer with
range/complexity, preset name, scale and spacing. WASM tests select FULL via K2,
detect four audible fundamentals, change K3 to normally-mono FM BASS, then submit
the same ordered answer. No physical device was written by this work.

Hardware follow-up: both scales and range boundaries, all K2 settings, all presets
with four overlapping notes, rapid K3 turns during playback/answers, simultaneous
key holds/MIDI input, replay/exit and re-entry; measure ISR/render maximum time,
underruns, runtime memory and a representative soak on each hardware revision.
Public distribution continues through the required attested shared CI workflow.

Local target result: app 567148 B, package 668948 B; .data+.bss 97352/98304 B,
pool 134052/357888 B, available audio arena 80928 B. Static audio ISR stack
audit 2984 B. All target gates, native/UI/MIDI/flash suites, WASM audio/ordered
answers with K3 preset change, metadata, emulator controls, served site/hash and
offline package/recovery checks pass. These establish functional/static evidence,
not hardware timing. Local package SHA-256:
`c0624300571a254afecbe1a5d2b24caa826945076a70225e13b4e3e7b0faccef`.
