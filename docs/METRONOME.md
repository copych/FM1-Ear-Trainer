# Metronome and rhythm games

The player controls are in the root README and `web/guide/index.html#metronome`.
This is a development patch, not a hardware-qualified release.

## Browser tap-timing correction

A second reproduced defect ignored note keys after SELECT or another knob
held focus. Note-letter input now works with slider focus; the knob retains
its arrow keys and real select/text fields remain protected. The page input
regression failed before this change and passes after it.

The original browser path graded keys against the firmware render clock rather
than the sound reaching the player. The new actual-AudioWorklet/WASM regression
reproduced the reported result exactly: with 120 ms simulated output delay,
eight on-time computer-key taps produced 0 hits, 16 misses and 8 extras. The
corrected path produces 8 hits, 8 misses and 0 extras; a full sixteen-tap round
produces 16/16. Both keep-tempo and missing-beat games are exercised. A separate
case still rejects genuinely 90 ms-late taps outside the 60 ms matching window.

The page captures pointer/keyboard/MIDI event timestamps and maps them using
the standard [Web Audio output timestamp](https://webaudio.github.io/web-audio-api/#dom-audiocontext-getoutputtimestamp).
When unavailable, it estimates timing from base/output latency. Render-to-output
clock differences are used only as a conservative message-delivery budget,
not as a calibrated physical latency measurement. The worklet maps the audible
AudioContext timeline to the first actual firmware output frame; delayed delivery
does not become player timing error. A bounded output-delay + 250 ms grace
(at most two seconds) postpones miss/result finalization without widening the
matching window. This runtime field is zero on hardware and is not serialized.

Timestamped browser key edges submit immediately on the AudioWorklet thread,
so down/up messages between render quanta are neither lost nor counted twice.
MIDI reuses the same source/channel/pitch ownership collector with an explicit
input time. Native keyboard/MIDI wrappers retain the sample-clock path. Missed
targets have a separate state so an expired target cannot later count as a hit
and a miss together. With no matched taps, browser results report unavailable
timing instead of misleading 0 ms precision; the header says Round finished.

Regression: `tools/rhythm-input-test.mjs` executes the real worklet source and
WASM using simulated 120/200/350 ms speaker delays, local keys, MIDI, short
presses, both games and out-of-window errors. `tools/emu-controls-test.cjs`
checks event-to-output clock mapping, fallback and zero-hit presentation.
Both are included in the existing blocking browser test path. This is software
validation, not a physical measurement of the user's browser or audio device.

Follow-up checks PASS: target/ABI and package/recovery checksum build gates;
integer-only generated metronome IR; bounded audio ownership and timestamp
paths; unchanged persistence schema/dependencies/licenses; native `metro-test`,
`input-test`, `game-test`; page controls; complete `web-test`; local site hashes.
General native UI/settings suites were not repeated for this browser-input fix.
Physical hardware deadlines, stacks/latency and install/recovery remain
UNVERIFIED under the existing device test plan.

The actual open in-app browser was reloaded and exercised through real keyboard
input after using SELECT. A rapid-key stress round registered 16/16 beats,
zero misses and 130 deliberate extra taps (182 key presses across count-in,
guide bars, silent bars and results); the low stress-test score correctly
penalizes those extras. Proof: `build/rhythm-input-browser-proof.jpg`.

## Shared Ear Trainer menu

HOME + PLAY now opens one training chooser: Pitch training, Metronome,
Keep tempo and Missing beats. SELECT or K1 moves the selection; PLAY or OCT+
starts it. HOME returns from an activity to this chooser; HOME again exits.
The browser has one Start training button that opens the same chooser.
HOME + REC remains a direct rhythm shortcut outside a shared training session.

`training_ui.c` owns navigation in main. Its one IRQ-published visibility byte
keeps local/MIDI input silent while still observing held-key ownership; MIDI
transport cannot start the underlying sequencer while the chooser is open.
Returning to the chooser reuses each activity's existing exit, track restoration
and save path. Starting another activity waits until audio consumes EXIT, so
rapid PLAY cannot replace teardown or capture temporary track mutes as the
next activity's restoration state. No DSP, storage schema, dependency or
peripheral ownership changes were introduced by this navigation follow-up.

Follow-up validation: `training-test`, `game-test`, `metro-test`, `ui-test`,
page input regression, target build/recovery check, browser build and tests,
and local site checks. The native fixture exercises all four activities,
rapid restart before teardown, silent local/MIDI input, menu bounds, HOME
return/exit and the direct shortcut. The WASM fixture selects all four using
the real encoder/button adapters. The 240×240 chooser framebuffer was visually
inspected (`melodee/build/ear-baseline/training-menu.png`). Unchanged settings
and broad MIDI suites were not repeated for this navigation-only follow-up;
their earlier results below remain scoped to the original metronome patch.
Hardware timing and physical install/recovery remain UNVERIFIED, with the
same device test plan below.

## Source and execution record

Base revision: `704f365`, vendored Melodee baseline
`165d177a425944f3d30b0b669255a6ed0050c3a5`. No submodules are registered.
Target: existing FM-1 `FM-1_327008` application; connected board revision and
installed bootloader are UNKNOWN. No device was flashed.

Build uses Ubuntu WSL, JieLi clang 4.0.1 r3, matched AC79 SDK revision
`d179b4484759423312073f5fbb232501aa491047`, and upstream build checks.
Host C compiler: GCC 15.2.0. Flags retain `-mcpu=r3 -mfprev1
-ffp-contract=off -Werror=double-promotion`, upstream audio optimization and
main-loop size passes. No clock, loader, flash-layout or peripheral changes.

Nominal audio rate is 44,100 frames/s, CTL is 32 frames, and the I2S half-buffer
is 128 frames (about 2.9 ms). The DAC timer source comments describe
44,117.6 Hz; absolute wall-clock tempo and round-trip latency require measurement.
The existing audio ISR owns events, keyboard/MIDI dispatch and rendering.

Changed path: `events_block -> metro_commands / keyboard_block / ear_midi ->
metro_tap`, and `mix_block -> master_samples -> metro_sample -> master_out`.
Main owns drawing, encoders, preference codecs, and saves. Complete control
updates and snapshots use the existing IRQ-mask protocol. There is one command
mailbox; the latest command wins if several arrive before an audio block.
No new worker, task, timer, interrupt, dependency or engine is introduced.

Click scheduling carries the rational interval remainder, avoiding accumulated
integer-BPM drift. Only bounded integer operations run in the new audio path.
No floating point, allocation, storage, logging, waits, locks, or math-library
calls were added there. Scheduling divisions occur on restart/onsets; sample
rendering uses additions, comparisons and a decaying square click. The final
DC blocker, limiter and master attenuation apply to the click. A fixed pool
array holds at most 120 expected taps. Restart and each input scan are bounded
by that capacity. Generated clicks never enter the player input collector.

One count-in bar precedes eight scored bars. Keep-tempo targets are the silent
bars of a one-guide/N-silent cycle; missing-beat targets are alternate meter
beats (every scored beat in 1/4). Games suppress visual beat/bar cues and use
meter beats, independently of practice rhythm/subdivision. Each tap matches at
most one target, inside a window capped below half a beat. Omissions and extra
taps count against the result. Timing credit decreases linearly with absolute
error; mean error, signed bias and population SD use integer milliseconds.
SD multiplies before dividing to avoid inflating jitter through rounded means.
Positive manual latency correction subtracts input delay and also extends the
judgment deadline, so corrected late arrivals are not prematurely missed.

## Persistence and compatibility

PER5 remains 604 bytes; EAR settings and rating formats are unchanged. Five
individually checked 12-byte metronome records use explicit IDs, version 1,
little-endian BPM and CRC8. Current settings occupy retired row 14 bytes 4–15;
USER 1–4 occupy retired rows 4/13 bytes 8–31. Neighboring live settings,
favorites and the first 64 legacy SAMPLE/SLICE star bits are preserved.
A static assertion prevents enabling SLICE with these storage assignments.
Missing/invalid current records restore defaults; invalid user slots become
empty independently. Browser storage uses a separate 60-byte preference key.

Flash writes are deferred for the entire metronome screen, including stopped
practice and results. HOME hands exit to audio, then existing A/B settings
storage commits on an idle poll. No flash operation interrupts a timed round.
Round results are session-only. The metronome temporarily mutes tracks and
restores their mute/selection/record state without loading a new sound.

## Validation and compliance

Run from the workspace with `sh tools/build-melodee.sh MODE` (the established
WSL wrapper on Windows). CI now blocks on the metronome suites as well.

| Category | Status and evidence |
| --- | --- |
| Target / build / ABI | PASS: development target builds with SDK, package, SRAM, stack, register-ownership and audio-residency checks retained. Artifact hashes are recorded in the generated site's `build-info.json`. |
| Audio hot path / numerics | PASS for source/generated-code inspection: new path is bounded integer code; fractional clock/wrap, matching windows, score arithmetic and SD regressions pass. Hardware execution cost remains unmeasured. |
| Execution ownership / synchronization | PASS for source and host behavior: audio owns runtime/taps; IRQ-protected publication/snapshots, silent input interception, mutually exclusive activities and bounded queues/scans. Existing CPU1 contract retained. |
| Memory / storage / I/O | PASS for static bounds, target size/stack gates, CRC/neighbor tests, deferred writes and interrupted A/B recovery. No new peripheral ownership; runtime stack/headroom remains unmeasured. |
| Dependencies / includes / licenses | PASS: no new dependency or asset, GPL notices and required Melodee attribution retained; no SDK/installer integrity checks removed. |
| Compatibility / package / recovery | PASS for software codecs, legacy/default handling, neighboring data, A/B recovery and local package checks. Physical install/rollback is UNVERIFIED. Release attestation/publication was not performed. |
| Host functional validation | PASS: `metro-test`, `settings-test`, `game-test`, `ui-test`, `input-test`, `web`, `web-test`, and page input regression. Native fixtures cover every meter/pattern/subdivision, fractional timing/wrap, tap averaging, perfect/early/late/omitted/duplicate taps, input ownership, latency corrections, user slots and corruption. WASM exercises real mixer clicks and an eight-bar MIDI round, scores, entry/exit, slot saving and fresh-instance restore. Framebuffer count-in/options/results were rendered and visually inspected at 240×240. `site` and `node tools/check-site.cjs http://127.0.0.1:8012/` passed routes, JS syntax, served firmware/resource/WASM hashes and installer selection. |
| Hardware real-time validation | UNVERIFIED: no connected hardware measurements, audio capture, cadence/headroom/underrun/soak or physical input/output latency results. |

Host screen fixtures are in `melodee/build/ear-baseline/metro-*.ppm`.
The final target check reported 97,576/98,304 bytes data/BSS,
135,172/357,888 bytes pool, a 77,408-byte available audio arena (49,152 required),
and a 2,984-byte static audio-ISR stack bound. The local `.fwsc` is 668,948 bytes,
SHA-256 `d9db084d63abc331cb873197b7ea1e1a1dec2239d5696a3a0fc870559b2ae44e`.
`fm1_recover.py --check` verified package checksums, chip key, application
boundary and recovery-loader checksum. Free SRAM is split
across banks and is not one contiguous heap.

Before hardware qualification, back up the actual unit and confirm matching
recovery artifacts. Measure click intervals at 30/137/300 BPM in simple and
7/16 meters, long-run drift, local-key/USB-MIDI latency and correction, worst
ISR duration and underruns with USB/UI contention, peak stacks, speaker and
headphone gain/pops, repeated entry/exit and save/reboot. Exercise an hour-long
practice soak and repeated game rounds. Physical flashing and rollback require
the appropriate device authorization; no such action was taken for this patch.
