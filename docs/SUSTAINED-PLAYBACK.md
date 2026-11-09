# Sustained playback — development 0.0.5

Development 0.0.6 replaces the decaying EP with [FM6 ORGAN](POLYPHONIC-ORGAN.md)
so sustained voices remain clearly audible. Timing and hints below still apply.

K4 now specifies onset-to-onset time, 0–1000 ms in 25 ms steps. Every exercise
note remains gated as subsequent notes enter. All notes release together one
second after the last onset, followed by the existing 100 ms answer-settle stage.
K4=0 launches all notes in one control tick, as a simultaneous chord. A repeated
pitch retriggers its existing tone rather than accumulating duplicate held voices.
The FM6 TINE EP envelope is unchanged, so its natural decay remains audible.

Known note labels turn bright green and gain a solid underline while their notes
are sustained. Hidden pitches remain '?' without pitch disclosure. Answer entry
remains ordered note-by-note; this change affects exercise playback only.

Audio ISR ear_audio_tick schedules starts at the existing 32-frame control rate.
ear_generated maintains a four-bit ownership mask, calling existing track/voice
note-on/off APIs. Cancelling, replaying, advancing, settings changes and exit
release every owned pitch before modifying the exercise. Zero spacing uses one
bounded four-note loop. UI snapshots the mask under existing IRQ masking.

## Compliance gate

| Category | Status / evidence |
| --- | --- |
| Target/build/ABI | Final build and package evidence below; same Melodee revision/compiler/SDK/flags as PROTOTYPE.md. |
| Audio hot path/numerics | PASS structural source review: bounded four-slot loops, integer timers, existing FM6 voice API; no new allocation, DSP, doubles, I/O, logging, waits or library math. Hardware instruction cost UNVERIFIED. |
| Ownership/synchronization | PASS source/host: audio owns note mask and state, main snapshots under existing IRQ mask; no worker/queue changes. Hardware latency UNVERIFIED. |
| Memory/storage/I/O | Fixed mask/state only, no persistent data or writes. Static target result below; runtime stack/arena peaks UNVERIFIED. |
| Dependencies/licenses | PASS source: no dependencies/assets added; GPL and Melodee attribution retained. |
| Compatibility/package/recovery | Existing flash gates, loader, layout and paired resources retained. Final site/package checks below. Physical revisions and recovery UNVERIFIED; no device writes. |
| Host functionality | PASS native game-test: earlier 20 mode combinations, scoring/replay and input tests; new 1/2/3/4-note accumulation, four actual FM6 gates held concurrently, timed release, zero-spacing chord and cancellation with every gate released. Native known-note highlight PNG inspected. |
| Hardware real time | UNVERIFIED: no hardware latency, underrun, stack or soak measurements. |

Captured native game audio peak 19240/32768, no clipping in this scenario.
Full upstream golden/UI/peripheral suites and sanitizers were not rerun for this
patch. Before hardware qualification, test both revisions and real knob timing,
listen for correct sustained note arrivals/repeated-pitch retrigger and clean
cancellation, check highlights hide target pitches, then measure maximum audio
service time, underruns, stack/arena peaks under four-note exercises plus tails,
dense MIDI and UI redraws. Use a verified backup and recovery procedure.
Local development package only; public release still needs upstream attestation.

Rebuilt WASM web-test PASS: default audio C4/E4 detected, keyboard/MIDI feedback,
automatic repeat and three reentry cycles; real knobs selected A minor with
four sustained notes at 500 ms onset spacing. Audio detection found MIDI
64,60,64,65 and ordered MIDI answers earned success only after the final entry.

Target/build/ABI PASS: app 566536 B, package 668948 B, development identity
FM-1_900; upstream integrity, RAM placement, stack, size and package checks pass.
Static memory PASS: .data+.bss 97336/98304 B (968 B remaining), pool
134052/357888 B, audio arena 81088 B; static audio ISR stack audit 2980 B.
495 linked audio-RAM functions verified. These do not measure hardware headroom.

Site 0.0.5 regenerated with matching resources and WASM. Route/script/hash and
installer selection checks PASS. Firmware SHA-256:
`3f64c264b661709ad6783a6f3a519e43adde99f3063efe005dd51183002f98b0`.
No hardware installation performed.

Reproduce using the established WSL `tools/build-melodee.sh` modes `game-test`,
`web`, `web-test`, `target`, `site`; run Windows `node tools/check-site.cjs`
against the local server. Build baseline and toolchain are documented in
PROTOTYPE.md and the project build procedure.
