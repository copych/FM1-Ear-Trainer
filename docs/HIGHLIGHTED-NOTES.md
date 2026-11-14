# Highlighted notes and player documentation — 0.0.15

The README now leads with stop playback, hold HOME and press PLAY. It contains three native game screenshots, concise controls, feedback and saving instructions. Build/provenance/validation and integration information live under Development. Both published GitHub release descriptions were rewritten for players with the start combination first; their firmware assets and tags were retained unchanged.

A project documentation audit found stale hardware-testing statements in the README, landing page, installer banner, recovery README generator, release notes and historical engineering reports. Public warnings implying the game is never hardware-tested were removed. The owner performs functional device testing. Historical reports now link to HARDWARE-TESTING.md; specific quantitative measurements remain distinguished from functional testing rather than invented or silently marked measured.

During EAR_ANSWER, an exact known reference pitch or a previously matched hidden pitch returns before attempts, scoring, matched bits, adaptive weights or feedback can change. It remains audible through the existing player-voice path. The game waits for outstanding hidden pitches. Unrelated notes still fail and lock/replay the attempt; a reference cannot substitute for a target. No change to octave matching, timing, persistence or synth DSP.

The emulator page has a single start button above the device. One click enables audio, loads saved preferences and starts training on its first ready frame. The running button is disabled; HOME exits and enables it again. Basic keyboard/knob instructions sit below the device. MIDI and held-button helpers remain under a collapsed More controls and MIDI section. Page source owns this layout, eliminating the old generated header/actions injection.

## Verification and compliance

Baseline: 389c8659e23208ba2d0e073eb3f34095809911cd; game/audio regression captured before editing (build/015-baseline.log).

- Target/build/ABI PASS: existing PI32v2 toolchain and pinned AC79 SDK; app 569576 B, package 668948 B, FM-1_327008. RAM data+bss 97368/98304 B; pool 134212/357888 B; audio ISR stack audit 2984 B. Existing target manifest, SRAM closure and recovery/package checks retained.
- Audio hot path/numerics PASS, source and target checks: one additional bounded four-note integer scan at answer submission, no allocation, DSP, doubles, I/O, logging or waits. Note generation/renderers unchanged.
- Ownership/synchronization PASS structurally: audio still owns game/input state; early return only. Main UI, loading and save publication unchanged. Browser boot commands follow the worklet's first ready frame.
- Memory/storage/I/O PASS for unchanged formats and budgets: no firmware buffers, flash layout or save-schema changes. Browser uses the existing checked settings/rating records.
- Dependencies/includes/licences PASS: no new dependency, original attribution retained, native screenshots reused. Simplified page retains source/licence attribution in its controls section.
- Compatibility/package/recovery PASS for local build/validation scope: same product, loader, resources and recovery process. Existing release assets are not modified.
- Host functional PASS: native reference-only input consumes no attempt; known and already matched tones during a partial answer leave score/state unchanged; a complete keyboard voicing including a reference solves once; repeat accepted tones are harmless but cannot fill missing slots. Existing chord permutation, wrong lock/replay, sound and rating regressions pass. WASM known-reference input leaves normal feedback and ratings unchanged; actual unknown input still fails. Browser script tests cover one top button, first-click audio/load/start, HOME re-entry and retained latch/storage behavior. Native/browser page screenshots are checked separately.
- Hardware validation is performed by the project owner. This report records the agent's code/build checks; instrumented timing, headroom and exact device coverage are recorded only when supplied, per HARDWARE-TESTING.md.

Local firmware SHA-256: 4adfd49059eb174fbf83bd71c0d30faa93fa33e1e0ae4be981cfe0c9248aedba. Public packages require the unchanged shared attested workflow, blocking tests and source/hash verification.

Real browser check PASS: a single click on the top button loaded sound/preferences and entered the game; the page showed only that button above the complete device and the instructions below it. Clicking the panel HOME enabled Start training again; clicking it re-entered successfully. Screenshot: build/015-clean-emu.png. Local served-site JS, asset hashes and installer selection checks PASS.
