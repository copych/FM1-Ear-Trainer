# Re-entry and two-note playback regression

2026-11-08. Responds to the reported web-emulator re-entry and single-note symptoms.

## Findings and changes

The original web Retry button sent a normal PLAY gesture even when the game was
closed. It could start Melodee's sequencer, after which the stopped-transport game
entry guard rejected Start. Next and Exit also leaked ordinary panel actions
outside training. Replace this with explicit simulator game commands; reject
inactive Retry/Next/Exit, expose active state to the page, and disable those
buttons outside the game. Start is idempotent: active Start replays rather than
injecting a HOME exit gesture. Entry still refuses a deliberately running music
transport; the page explains that it must be stopped.

The previous host capture contained both C4 and C5, so no universal missing-note
root cause is claimed from that recording. The old test only asserted nonzero
audio overall and could not detect a missing first note. New tests analyze both
note windows separately in native and WASM output and require distinct pitches.

Playback now waits for the existing engine-switch fade to finish, adds a 200 ms
lead-in, plays note 1 for 500 ms, pauses 250 ms, plays note 2 for 500 ms, then
settles 100 ms before accepting input. This wait is state-driven across audio
blocks, never a busy wait. The screen shows GET READY, NOTE 1 OF 2 and NOTE 2 OF 2
without exposing the hidden target's pitch. FM6 and its preset remain unchanged.

## Validation and compliance

- Native integration regression: game controls, answers, ownership, cancellation,
  and three exit/re-entry/restart cycles passed. Original UI regressions passed.
- Native PCM checks: first window C4 (MIDI 60), RMS 6856.6; second window C5
  (MIDI 72), RMS 6329.5, on signed 16-bit output. No note-window silence accepted.
- WASM regression: first C4 RMS 0.1047; second C5 RMS 0.0967. Both distinct pitch
  checks, wrong-answer feedback, retry/next, three re-entry/restart cycles and
  inactive-command refusal passed.
- Final browser UI check passed: Retry/Next/Exit are disabled before entry and
  after exit; Start becomes Restart; two manual exit/re-entry cycles returned
  to the game. Screenshot: `build/emulator-fixed.png`.
- Target/build/ABI PASS: app 564420 bytes; data+bss 97272/98304 and pool
  134052/357888 unchanged from the prior prototype. Audio working arena 82208
  bytes. Existing compiler, SRAM-call graph (495 linked audio functions) and
  stack/memory checks pass. Package identity remains FM-1_900; the local site's
  build-info.json records the rebuilt hash.
  Package SHA-256: `a7d85c2c1bf547eee29aef1e9a16ff4d472acdfef9ae81c47d9c709d16e0eaf2`.
- Hot path/numerics: PASS structural scope; bounded integer stage logic, existing
  voice calls, no new allocation/I/O/math/blocking work. PREPARE polls readiness
  once per block rather than spinning. UI and simulator actions stay outside DSP.
- Ownership/synchronization: PASS source/host scope; existing IRQ publication and
  single-threaded AudioWorklet dispatch preserved. No new core synchronization.
- Memory/storage/I/O and dependencies/licenses: fixed state, no new dependency,
  storage-format, flash-layout, synth or preset change; GPL notices retained.
  Static target limits checked; runtime memory peaks remain unmeasured.
- Compatibility/package/recovery: updater and flash support unchanged; local
  site is regenerated with the paired rebuilt package/resources/WASM. No flashing.
- Hardware real-time validation: UNVERIFIED, including physical old/new revisions,
  exact deadlines, CPU/stack headroom, I/O contention and recovery.

Full engine golden/reference, sanitizer and hardware suites were not repeated;
FM6 DSP is unchanged. The native/WASM regressions exercise the changed lifecycle,
input gate and audible playback rather than relying on state changes alone.
