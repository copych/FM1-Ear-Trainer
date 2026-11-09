# Original preset with polyphony — development 0.0.7

Restores the user's preferred original FM6 TINE EP factory preset (index 0).
Explicit V_POLY and the sustained overlapping playback from 0.0.5 remain.
The factory sound is unchanged: its natural decay remains, and no operator
envelopes, algorithms or synthesis code were modified. K4 still sets onset
spacing; zero starts a chord. Known-note highlighting remains.

Only main-loop ear_ui_start preset selection changed in firmware. Native tests
assert preset 0/V_POLY and retain the actual four-gated-voice, release and
cancellation checks. The late audio test requires both pitch fundamentals to
remain audible, allowing the EP's natural decay instead of requiring an organ's
flat sustain. Web guide and landing page identify TINE EP again.

## Compliance

| Category | Status / evidence |
| --- | --- |
| Target/build/ABI | Same baseline/compiler/SDK/flags; final result below. |
| Audio hot path/numerics | PASS source: no ISR/DSP changes, allocation, math, I/O, logging or waits added; original factory patch reused. Hardware timing UNVERIFIED. |
| Ownership/synchronization | PASS source: same main-loop loader and IRQ-masked poly assignment, audio-owned playback unchanged. Hardware latency UNVERIFIED. |
| Memory/storage/I/O | No new runtime buffers, persistence or storage writes; static budget below. Runtime peaks UNVERIFIED. |
| Dependencies/licenses | PASS: no new dependencies/assets, existing preset and GPL notices retained. |
| Compatibility/package/recovery | Existing loader/layout/flash gates/resources preserved; final site checks below. Both physical revisions/recovery UNVERIFIED, no device writes. |
| Host functionality | Native and WASM results below; full upstream UI/golden/peripheral suites and sanitizers not rerun for this preset-only change. |
| Hardware real time | UNVERIFIED: no local timing, underrun, stack or soak measurements. |

Before hardware qualification, use verified backup/recovery, test both revisions,
check audible overlap/repeated notes and clean cancellation at K4 extremes, and
measure worst audio service time/underruns/runtime memory under dense MIDI and UI
load. Local development package, not an attested public release.

Native `game-test` PASS: all scale/count/guess combinations, scoring and input
regressions, actual four-voice overlap, zero-spacing chord and cancellation.
Late held-chord root/second spectral levels 2232.3/2588.1 (early solo root
5344.1): both persist while naturally decaying. Captured game peak 19240/32768.

Rebuilt WASM `web-test` PASS: overlapping C4/E4 audio, feedback/replay/reentry,
four-note A-minor scenario with all held fundamentals present, and ordered MIDI
answer accepted only after the final entry.

Target/build/ABI PASS: app 566572 B, package 668948 B (FM-1_900 development
identity), upstream integrity/section/stack/size/package checks pass. Static
memory PASS: .data+.bss 97336/98304 B, pool 134052/357888 B, audio arena
81088 B; static audio ISR stack audit 2980 B, 495 linked audio-RAM functions
verified. Hardware/runtime limits remain UNVERIFIED as above.

Site 0.0.7 built with matching resources/WASM; route/script/hash and installer
selection checks PASS. No device written. Firmware SHA-256:
`6c8adfd6ee6ce6a306ef5848a921391e4bb9c95ee6b02d3ff6614faf9b8093fe`.
Reproduce with established WSL `tools/build-melodee.sh` modes `game-test`, `web`,
`web-test`, `target`, `site`, then Windows `node tools/check-site.cjs` against
the running local server. Reload an already open emulator to load this build.
