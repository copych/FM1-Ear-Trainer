# Full-range exercises and combined complexity — 0.0.9

Each exercise now uses distinct MIDI pitches from the selected scale across the
built-in F3–G5 keyboard (53–79). C major and A natural minor both use the white
keys, with different tonic/degree labels and adaptive weighting. Octave-separated
tones remain different pitches. References remain C4/A3 when any notes are shown;
all-hidden exercises choose a new starting pitch each question. Remaining pitches
may fall above or below the reference and previous notes.

K2 selects the number hidden out of the total: 1/1, 1/2, 2/2, 1/3, 2/3, 3/3,
1/4, 2/4, 3/4, 4/4. The display reads, for example, `K2 1 OF 3`. Defaults remain
1 of 2. K1 selects the scale, K3 is reserved, and K4 retains onset spacing.
Hidden ending notes are still answered in order with exact octave matching.

## Baseline and execution contexts

Source baseline: `3065da3e0ae80cf20c0c7d117faa86f78303d80f`, vendored Melodee
baseline `165d177a425944f3d30b0b669255a6ed0050c3a5`. Existing native game/audio
tests passed before changes. Same JieLi pi32v2 clang 4.0.1 r3 and matched AC79
SDK documented in BUILDING.md; existing flags, linker, loader and board support
retained. No physical device was written during this change.

Main-loop `ear_ui_input` publishes the paired settings and NEXT under the existing
IRQ mask. Audio ISR scheduling calls `ear_audio_tick`; only NEXT generates new
pitches through `ear_pick_note` / `ear_note_degree` / `ear_random`. Each selected
pitch uses at most two 27-key scans with a seven-entry scale lookup, fixed integer
arithmetic and existing per-degree weights. A 27-bit exclusion mask prevents
repeated pitches without rejection sampling. At most four notes are generated.
Retry and automatic repeats reuse the same exercise. The bounded fallback is
unreachable for valid settings: at least 15 scale pitches and at most four
exclusions leave a positive candidate total. No sample-rendering DSP changed.

Target disassembly places `ear_note_degree` in SRAM and uses native integer
division for pitch-class arithmetic. Scale-table reads retain the existing
flash access model; training blocks storage/preset actions. No allocation,
blocking, logging, floating-point math or new external calls were introduced.
The build's audio closure and software-double gates pass. Target timing is not
inferred from these structural checks.

## Compliance and validation

| Category | Status and evidence |
| --- | --- |
| Target / build / ABI | PASS: `sh tools/build-melodee.sh target`; unchanged SDK/ABI/manifest gates; image 566764 B, package 668948 B, identity FM-1_327008. |
| Audio hot path / numerics | PASS for source/target structural review: bounded integer generation only on NEXT; no new DSP/math helpers or allocation; 496 linked audio functions pass SRAM checks. Actual latency unmeasured. |
| Ownership / synchronization | PASS for source/host review: existing main/IRQ publication and audio-owned lifecycle retained; K3 is consumed without starting a new exercise. |
| Memory / storage / I/O | PASS for static bounds: no new mutable buffers or storage format; .data+.bss 97336/98304 B, pool 134052/357888 B, available audio arena 80960 B, static audio ISR stack audit 2984 B. Runtime peaks unmeasured. |
| Dependencies / includes / licenses | PASS: no dependencies, headers or assets added; upstream notices retained. |
| Compatibility / package / recovery | PASS for host/source gates: dedicated identity and loader unchanged; both flash-ID tests and offline package/chip-key/boundary/recovery-loader checks pass. New build's physical install/rollback remains UNVERIFIED. |
| Host functional validation | PASS: native game/audio, UI, MIDI/flash, WASM audio/answers, browser control gestures, Hub metadata and served site/hash checks. |
| Hardware real-time validation | UNVERIFIED: no new-build hardware testing, timing, headroom, underrun or soak measurements. Earlier 0.0.8 owner-reported tests do not qualify 0.0.9. |

Native regression covers all ten complexity pairs, forward/reverse/clamp control
gestures, both scales, deterministic generation, and 1000 four-note exercises per
scale. Every available scale pitch appears; ascending and descending transitions
occur; no exercise repeats a pitch. Wrong-answer locking, automatic replay/next,
scoring, adaptive weighting, exact ordered answers, known-key lights, four held
voice gates and cancellation continue to pass. WASM tests detect audible
fundamentals from actual PCM and submit the detected sequence through MIDI.
The real browser also shows K2 progressing from 1 OF 2 to 2 OF 2 to 1 OF 3.

Local target package SHA-256:
`4b58dc36619a38b565e06cf0ea7cf9c8c316fc5d63b1fcf3d3418cc2398061ca`.
Public distribution uses the required attested CI build rather than this local
artifact. Hardware follow-up: test both scales at range extremes, every K2 pair,
K4 zero/max, known-note lights, retries/exit during held playback, and dense MIDI;
measure maximum ISR/render time, underruns and runtime memory on each revision.
