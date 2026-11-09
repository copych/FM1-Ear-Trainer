# Audible sustain — development 0.0.6

Historical sound trial. Development 0.0.7 restores the user's preferred
[original TINE EP with polyphony](ORIGINAL-PRESET-POLYPHONY.md).

The previous FM6 TINE EP preset is polyphonic: its factory mono flag is zero,
and native four-note tests verified four simultaneous gated FM6 voices. However,
its native operator envelopes decay toward zero even while gated. That can make
the newest attack dominate an arpeggiated chord.

Training now loads the existing FM6 ORGAN factory preset (index 6), whose
operator sustain levels remain high, and explicitly requests V_POLY. No new
synthesizer, patch data or DSP changes. Timing, hints and answer rules remain as
in SUSTAINED-PLAYBACK.md. Training leaves ORGAN in track 1 on exit; the web guide
and landing page identify the new sound.

## Compliance gate

| Category | Status / evidence |
| --- | --- |
| Target/build/ABI | Same baseline/compiler/SDK/flags; final build evidence below. |
| Audio hot path/numerics | PASS source review: existing engine/preset and existing scheduler/voice APIs; only main-loop preset selection/poly parameter changed. No new ISR operations, DSP, allocation, doubles, logging, I/O or waits. Hardware CPU cost UNVERIFIED. |
| Ownership/synchronization | PASS source: main loads preset through existing engine_load_of, poly assignment under existing IRQ mask; audio execution model unchanged. Hardware latency UNVERIFIED. |
| Memory/storage/I/O | No new runtime buffers or persistent formats/writes; static target budget below. Runtime peaks UNVERIFIED. |
| Dependencies/licenses | PASS: existing factory patch and notices retained, no dependencies or assets added. |
| Compatibility/package/recovery | Existing loader/flash IDs/layout/resources preserved, final package/site checks below. Both physical revisions and recovery UNVERIFIED; no device writes. |
| Host functionality | PASS native game-test: ORGAN index and V_POLY asserted, all mode/scoring/input regressions, four simultaneous FM6 gates, cancellation/release. Spectral test now requires root and second pitch to remain audible late in the held chord. |
| Hardware real time | UNVERIFIED: no hardware timing, stack, underrun, output or soak measurements. |

Full upstream golden/UI/peripheral suites and sanitizers not rerun for this
patch. Before hardware qualification, verify the organ's held chord at K4=0 and
1000 ms, repeated pitches and cancellation; test outputs/gain and dense MIDI/UI
load on both revisions, measuring maximum service time, underruns and runtime
memory peaks. Use a verified backup/recovery route. Local development package,
not an attested public release.

Native late-chord fundamental levels: root 1725.0, second 1729.8 (early solo
root 1730.5, same spectral estimator). Captured game peak 14534/32768.
The previous browser test inferred each new note from the loudest spectral peak;
that assumption fails for steady overlapping voices. It now checks every held
fundamental in the seeded E4,C4,E4,F4 scenario, then verifies the complete ordered
MIDI answer. Rebuilt WASM test PASS, including reentry, feedback and automatic
replay. This confirms host audio overlap, not hardware timing or analog quality.

Target/build/ABI PASS: app 566572 B, package 668948 B (FM-1_900); upstream
integrity, section, stack, size and package checks pass. Static memory PASS:
.data+.bss 97336/98304 B, pool 134052/357888 B, audio arena 81088 B;
static audio ISR stack audit 2980 B, 495 linked audio-RAM functions verified.
Runtime headroom remains UNVERIFIED.

Site 0.0.6 generated with matching resources/WASM; route/script/hash and installer
selection checks PASS. No device written. Firmware SHA-256:
`5874e6094f2a4e5ab7bbce079eae721326af1fcc1e00bc52ac72f409a4e4e7c0`.
Reproduce with established WSL `tools/build-melodee.sh` modes `game-test`, `web`,
`web-test`, `target`, `site`, then Windows `node tools/check-site.cjs` against
the running local server. Reload the emulator to replace an already running
AudioWorklet with the new sound.
