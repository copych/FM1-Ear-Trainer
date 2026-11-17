# Metronome and rhythm games

The player controls are in the root README and `web/guide/index.html#metronome`.
This is a development patch, not a hardware-qualified release.

## Shared pitch/rhythm circle background — 2026-11-17

Pitch correct/wrong screens now use the same soft-edged circle geometry,
colors and 400 ms shrinking/fading curve as the metronome. The former full-screen
green/red fill is replaced by the theme background. Result text and note-card
highlights remain after the circle fades. Main snapshots state as before and
tracks the result's serial/attempt so an ordinary redraw does not restart the
animation. Only animation frame changes cause additional redraws; static held
results return to the existing idle draw guard. No new pitch cue or partial-answer
feedback is added.

The bounded row rasterizer and integer square root move unchanged into
`training_feedback_ui.c`, included before both screens, and remain in the
main-only size-optimization list. Text blends over the circle and restores the
renderer mode on exit. No audio, scoring, state-transition, input, persistence
or wire-format code changes. Baseline: 4f36a47; `game-test` passed before editing.

| Compliance category | Status / evidence |
| --- | --- |
| Target / build / ABI | PASS: existing classic FM-1, Clang pi32v2 4.0.1 r3, pinned SDK d179b4484759423312073f5fbb232501aa491047 and build flags retained. `target` passes package/image/manifest/SRAM/stack checks. App 576,764 B; package 668,948 B. Physical bootloader identity UNKNOWN. |
| Audio hot path / numerics | PASS for source scope: no audio path changes; circle/envelope uses bounded integer main-loop math. Shared helper remains outside ISR/render calls. Pitch host PCM remains 22.88 s, peak 19,309, RMS 5,582.0, matching the pre-change baseline. |
| Execution ownership / synchronization | PASS: existing IRQ-masked snapshot and main-owned UI state retained; no locks, workers, queues or waits added. |
| Memory / storage / I/O | PASS for build/bounds: data+BSS 97,592/98,304 B; pool 137,460/357,888 B; arena 74,720 B (minimum 49,152); ISR stack audit 2,984 B. Shared strip drawing, no new framebuffer, storage or peripheral work. Physical drawing latency UNVERIFIED. |
| Dependencies / includes / licenses | PASS: no external dependency; shared in-repo helper has original Kerem Kilic/SPDX attribution. README screenshot regenerated from the real host framebuffer. |
| Compatibility / package / recovery | PASS for source/build scope: unchanged settings/version/metric IDs, loader/layout and package contracts; existing recovery checks retained. Physical installation/rollback UNVERIFIED. |
| Host functional validation | PASS: native `game-test` checks correct/wrong pulses, 200 ms shrink, 401 ms clear, unchanged corner background, renderer-mode restoration and no static-result redraw. Full pitch behavior/audio assertions, `metro-test`, `training-test`, `web`, `web-test`, page controls, `site` and served-site checks pass. Browser assertions now check circle colors/clearance, locked note-card glyphs and ratings through animation, and final-answer-only feedback for four hidden notes. Correct/fading/wrong/cleared framebuffers visually reviewed at native resolution. |
| Hardware real-time validation | UNVERIFIED: no physical redraw/soak or recovery run. Check both result colors/decay, held answers, all palettes, rapid answers/replay, keys and audio during animation under USB/TRS load; measure ISR margin and main-loop/LCD latency, then run a sustained soak. Nominal audio remains 44.1 kHz / CTL 32 / 128-frame I2S halves (~2.90 ms). |

Local package SHA-256: `3b2139fd5afd94d7ccf5e3b1685ddc64a14a73470599f5bcf258ad2accbb462e`.
No separate sanitizer or hardware performance measurement is claimed.

## Plain-metronome sound presets — 2026-11-17

In plain Metronome, K3 now selects Click (the existing sound), 808, CR78 or FM.
K4 is hidden/unused rather than changing game settings. The three kits use the
existing DRUM synth voices: OH on the meter's strong/group accents, CH on other
pulses, including subdivided grids. A new hat replaces/chokes the previous hat.
Rhythm game retains its current click and K3 rule/K4 bar-cycle controls. Player
tap audio remains the separate lower click. Factory setups preserve the sound;
user slots save it.

`metronome_hats.c` uses private trigger snapshots, one private voice, private
808 scratch and a CTL-sized output buffer in SRAM (2,288 B total added pool).
Main prepares coefficients once before publishing metro_enabled. Audio copies
the selected OH/CH snapshot on an exact pulse and renders fixed 32-sample blocks
as needed. It touches no song patch, track voice, shared kit scratch or resource
allocator. CR78 and FM coefficient floating-point setup stays outside audio;
their existing sample loops and the 808 render path use bounded integer DSP.
SRAM snapshots have an explicit BSS readiness flag because pool memory has no
boot-time zeroing guarantee. Restart, stop and exit discard pending hat audio.

Wire records are version 3, still 12 bytes each / 60 total, in the same PER5
locations. The sound ID occupies the high nibble of byte 6; its low nibble is
still fixed subdivision 1. Explicit sound IDs are 0 Click, 1 808, 2 CR78, 3 FM.
Versions 1 and 2 migrate to Click; version 2 rejects the new nibble rather than
misinterpreting it. Browser storage writes v3 and falls back through v2/v1,
retaining old keys. No flash layout or bootloader changes.

Source baseline: 81a9b17. The first target attempt correctly failed the existing
commit-marker image limit. The existing main-only minsize list now includes
the training UI files, the metronome codec and one-time hat preparation. Sample
render loops retain their existing optimization; no package limit was relaxed.

| Compliance category | Status / evidence |
| --- | --- |
| Target / build / ABI | PASS for target compilation/checks: classic FM-1, nominal 44.1 kHz, CTL 32, I2S halves 128; existing pi32v2 Clang 4.0.1 r3 and SDK d179b4484759423312073f5fbb232501aa491047. `target` passes all size/manifest/SRAM/stack/peripheral checks. App 576,580 B fits the unchanged 577,248 B app+stub limit; package 668,948 B. Device bootloader identity UNKNOWN. |
| Audio hot path / numerics | PASS for structural/generated-code review: on demand, ≤32 samples of existing integer kit DSP, fixed-size snapshot copy through resident memcpy, gain scaling and existing soft knee/master stage. `metro_sample` disassembly calls resident cr_run/fmk_run/dr8_run_into and memcpy; the SRAM closure check passes. No new float/math helper, allocation, storage, wait or unbounded loop in audio. Coefficient setup is main-only; physical cost UNVERIFIED. |
| Execution ownership / synchronization | PASS for source/host review: main prepares immutable templates before enable; audio owns live voice, scratch/buffer and pulse events. IRQ-masked parameter/command publication retained. No shared track state, worker or queue changes; sound changes restart through the existing command path. |
| Memory / storage / I/O | PASS for build/bounds/host checks: data+BSS 97,592/98,304 B; pool 137,460/357,888 B; audio arena 74,720 B (minimum 49,152); ISR stack audit 2,984 B. Fixed private buffers; no allocation or new I/O. Existing exit-only saves remain. Physical peak resource usage UNVERIFIED. |
| Dependencies / includes / licenses | PASS: new adapter includes the existing GPL kit APIs through the current translation unit; no external dependency, sample recording or asset added. Existing kit files/credits remain untouched; adapter has Kerem Kilic/SPDX attribution. |
| Compatibility / package / recovery | PASS for host/build scope: v1/v2→v3 sound-default migration, all four sounds in current/four user records, CRC/invalid-ID checks, independent corrupt slots, reboot and interrupted saves pass. Mode and metric IDs unchanged. Package/recovery-loader checks pass; physical install/rollback UNVERIFIED. |
| Host functional validation | PASS: `metro-test`, `settings-test`, `training-test`, `web`, `web-test` and page input/control regression. Native kit waveforms differ, OH tails exceed CH, 7/8 OH/CH group mapping is checked, track patches survive, game click remains, and all timing tests pass. Actual WASM mixer peaks: Click 0.1942, 808 0.3914, CR78 0.2922, FM 0.2881; finite/distinct outputs below clipping, all sound choices restore in fresh instances and K4 cannot change a game cycle in practice. Plain sound-selector framebuffer visually reviewed. |
| Hardware real-time validation | UNVERIFIED: no physical FM-1 timing, loudness or recovery run. Measure maximum ISR cost/headroom against ~2.90 ms, particularly 808 hats and rapid retriggers at 300 BPM/4/4 sixteenths, while redrawing and handling USB/TRS input. Check onset timing, OH→CH transitions, gain/clipping, patch restoration, sound/slot persistence, exit/reboot, palette readability and long active/idle soak before hardware qualification. |

Local package SHA-256: `9cc815ba904c8cecb46abc3f819ecd092a618f792d5057a632b594784839268c`.
No separate sanitizer, physical profiling or recovery exercise is claimed.

## Exit saves, tap audio and feedback — 2026-11-17

The user identified the momentary display/key interruption as SAVING SETTINGS
in slow-motion video. Source inspection matches that report: pitch training
checkpointed dirty ratings once a minute, drew its saving screen, then ran a
flash operation that masks interrupts. `settings_poll` now defers every settings
write while either training activity is open, including quiet answer pauses
and stopped metronome screens. Preferences/ratings remain queued/dirty and
commit after the activity's audio-owned EXIT. Leave the activity and let saving
finish before powering off; a power cut before exit loses unsaved session changes.
The user's video is reported evidence, not a locally reproduced device measurement.

Guide click envelopes now last 512 samples rather than 256, with strong/regular
attack bounds 17,920 / 12,800 instead of 6,400 / 3,200 before the existing
master processing (roughly +9 / +12 dB at the attack). Every actual key/MIDI
tap during metronome playback triggers a separate 384-sample, lower-pitched
click, including free practice and extras. A guide/tap overlap is bounded by
27,136 before the master DC blocker/limiter/soft clip. Neither audio voice can
submit an answer; judgments still consume only input edges. MASTER applies
normally. No timer, allocation, storage or float operation was added to audio.

A soft-edged background disc shrinks and fades for 400 ms: green on matched
taps (neutral tap feedback in practice), red on extras/misses. It never pulses
automatically at hidden beat times. Main-loop rendering reuses the five 48-row
strips, existing text-over-image blending and RGB565 theme handling. Each disc
uses at most 240 row iterations per screen, a bounded integer sqrt per visible
row, span fills and fractional edge coverage; no per-pixel sqrt, supersampling
or new framebuffer. Animation time is quantized to 30 Hz; normal status/input
redraws can also occur. When the fade ends, static idle redraws stop again.

All 27 musical keys now have uniform idle glow in the training menu, pitch
training and metronome; existing accepted/held highlights stay bright. Global
LIGHTS controls the idle intensity, including OFF. Uniform dimming reveals no
hidden pitch. The host lighting fixture uses the real physical matrix mapping.

| Compliance category | Status and evidence |
| --- | --- |
| Target / build / ABI | PASS for compilation/package checks: classic FM-1, 44.1 kHz nominal, CTL 32, 128-frame I2S half; existing pi32v2 Clang 4.0.1 r3, pinned SDK d179b4484759423312073f5fbb232501aa491047 and flags retained. `target` passed manifest, SRAM closure, stack, peripheral and package checks. App 577,220 B; FM-1_327008 package 668,948 B. Device bootloader identity UNKNOWN. |
| Audio hot path / numerics | PASS for structural/generated-code audit: guide and tap are two bounded integer envelopes. Tap adds multiply/shift, counter/branch and sum; target disassembly has no new math/allocation/helper calls or per-sample division (existing meter division remains at pulse boundaries). Feedback writes are bounded stores on input/judgment. Circle rasterization is main-only. Device cycle costs UNVERIFIED. |
| Execution ownership / synchronization | PASS for source/host review: runtime/feedback audio-owned; main snapshots under IRQ mask and draws outside it. Native MIDI/keys and same-thread browser captured input retain edge ownership. No worker, queue, locks or waits added. Flash is now excluded from both open activities; audio consumes EXIT before saving can resume. |
| Memory / storage / I/O | PASS for static/build/host checks: data+BSS 97,544/98,304 B, pool 135,172/357,888 B, audio arena 77,152 B (minimum 49,152), ISR audit 2,980 B. Small envelope/event fields, no new buffers or peripherals. Host simulated >5 minute activity pauses cause zero flash erases; exit persists settings/ratings and reboot restores them. Hardware peak load/usage UNVERIFIED. |
| Dependencies / includes / licenses | PASS: no dependencies/assets added, existing GPL/SPDX and source credits retained. Uses the existing renderer, LED planes and mixer. |
| Compatibility / package / recovery | PASS for codec/build regression scope: no persistence or Web metric IDs changed; four slots, legacy records, neighbors and interrupted A/B saves retained. Save policy intentionally changes from quiet/minute checkpoints to activity exit, documented for power-off. Existing container/loader checks retained. Physical flashing and rollback UNVERIFIED. |
| Host functional validation | PASS: `metro-test`, `training-test`, `settings-test`, `ui-test`, `web`, `web-test`, page input/control checks and framebuffer review. Louder/overlapping audio bound, tap events, decay colors, text-mode restore, all grids/rules/cycles, real-matrix idle glow, OFF and pressed highlights, no mid-training flash and exit/reboot pass. WASM real-mixer guide peak 0.2490, player tap 0.1370 (including codec -6 dB scaling), no clipping; one extra tap sounds but cannot produce a hit, and no generated audio creates answers. Captured-timestamp regressions retain exact hits under output latency. `site` and container/recovery checksum checks pass. |
| Hardware real-time validation | UNVERIFIED: no local FM-1 soak, timing, output loudness or recovery measurement. Before hardware qualification measure both envelope cost and animated drawing under USB/TRS load, ISR headroom against ~2.90 ms, input/LED scan gaps, audio peaks/noise and LCD latency. Check >10 minutes of active/idle pitch/rhythm use without saving interruptions, exit saves/reboot, LIGHTS levels, all palettes, and recovery. |

Local package SHA-256: `a2849ece63bb936c8f07f5bc0b8f29b8107134033a3f57733a5c5bceea127d8c`.
No separate sanitizer or hardware performance result is claimed. Earlier silent-tap
and minute-checkpoint descriptions below are historical and superseded here.

## Immediate guide bars and tap-window correction — 2026-11-17

Current behavior supersedes the earlier count-in descriptions below. PLAY or
restart immediately schedules the first audible guide pulse: there is no
200 ms preparation gap and no separate precount bar. The complete round is
eight bars: A/H/A/H/A/H/A/H for 1/1, or A/A/H/H/A/A/H/H for 2/2. LISTEN and
the pulse display follow each audible bar; TAP SILENCE appears in hidden bars
without moving phase hints. Guide taps, including in the first bar, are extras.
Odds still means missing positions 1,3,5…; Evens means 2,4,6…, restarted each
bar. Target indices and audible clicks use the same one-based bar cycle.

The matching window stays nominally ±60 ms. Its non-overlap cap now uses the
closest pair of actual targets, rather than the spacing between every grid
pulse. At 300 BPM on 4/4 sixteenths, alternating targets permit just under
±50 ms instead of the previous ±25 ms. Consecutive hidden odd-meter bars can
put two odd targets on adjacent pulses across a bar boundary; the actual
schedule correctly preserves the narrower cap in that case. No settings
format or fixed-default values change.

Baseline 6841a5a's native suite passed. A new start-immediately assertion then
failed against that baseline (`metro.start==metro.now`); it passes after the
change. Source review found no odd/even reversal. The extra initial guide bar
is a plausible explanation for the reported Evens/120 BPM confusion, not a
claim to have reproduced the user's exact playing session. At ordinary 4/4,
120 BPM the original window was already ±60 ms.

| Compliance category | Status / scope / evidence |
| --- | --- |
| Target / build / ABI | PASS for build: classic FM-1, nominal 44.1 kHz / CTL 32 / I2S half 128; unchanged Clang pi32v2 4.0.1 r3 and pinned SDK d179b4484759423312073f5fbb232501aa491047. `target` passed manifest, RAM/stack, peripheral-access and packaging checks; app 577,240 B, package 668,948 B. Physical bootloader identity UNKNOWN. |
| Audio hot path / numerics | PASS for structural/generated-code audit: restart scans ≤128 candidate pulses and ≤64 targets; extra spacing calculation is bounded integer subtraction/shift/min in that existing control-block restart. `metro_sample` and its `metro_hidden_bar`/`metro_target` callees use integer operations and hardware division/bit operations, all linked in SRAM; no allocation, I/O, wait or floating/library helper added. Per-pulse phase selection adds a bounded helper call. Hardware cost unmeasured. |
| Execution ownership / synchronization | PASS for source review: existing audio-owned clock/commands/targets and IRQ-masked main-loop settings publication retained. Browser captured event timestamps/grace and input ownership unchanged. Result wait stops scheduling new pulses at the eight-bar end. |
| Memory / storage / I/O | PASS for static/build/host checks: data+BSS 97,560/98,304 B; pool 135,172/357,888 B; audio arena 77,440 B (minimum 49,152); ISR stack audit 2,984 B. No new buffers, storage writes or allocation. Deferred settings flash and existing 120-slot target pool unchanged. Physical peak usage UNVERIFIED. |
| Dependencies / includes / licenses | PASS: no new dependencies, includes, assets or platform changes; GPL/SPDX and existing attribution retained. |
| Compatibility / package / recovery | PASS for host/build checks: v1/v2 settings codec unchanged; settings/slots, CRC, reboot, deferred flash and interrupted A/B regression pass. State ID 1 now means audible guide bars rather than a separate precount; result ID 3 and 9-field metrics remain compatible. Container/loader checks retained. Physical installation/rollback UNVERIFIED; no flashing performed. |
| Host functional validation | PASS: `metro-test`, `training-test`, `settings-test`, `web`, `web-test`, page control/input-clock regressions, `site` and served-site checks. All 22 grids × 3 rules × 2 cycles validate independent target positions and caps, with wrap/offset/input cases retained. Worklet/WASM: Evens/120 BPM accepts ±55 ms (8/8), rejects ±65 ms and opposite odd pulses (0/8); nonzero start, odd 7/8 at 137 BPM, fast 7/16/sixteenths, audio delays and short keys pass. Guide/hidden framebuffers were rendered and visually reviewed. |
| Hardware real-time validation | UNVERIFIED: no physical FM-1 measurements or recovery exercise. Run the retained stress plan below: ISR latency/headroom against the ~2.90 ms half-buffer period, USB/TRS input/output latency, underruns, peak stack, rapid settings, speaker/headphone gain, deferred saves, cold boots, rollback and sustained soak. Host/WASM results do not establish device real-time safety. |

No separate sanitizer or hardware performance run is claimed. Local firmware
SHA-256: `8fdee697648abcc649f2a88af77636b6447cc0ca77391212b19d2ded2db0591b`.
Only upstream-attested CI packages may be copied to public hosting.

## Unified rhythm game — previous revision (2026-11-16)

The shared menu now contains Pitch training, Metronome and Rhythm game.
There is one settings page: K1 tempo, K2 meter/pulse grid, K3 Full bars /
Odds / Evens, K4 audible/hidden bars 1/1 or 2/2. Click volume is fixed at
100%, subdivision at 1, and nominal matching tolerance at ±60 ms, capped
below half a pulse at fast tempos. Meter IDs 0–18 are unchanged; IDs 19–21
add 4/4 eighths, triplets and sixteenths with quarter-note accents. These
grids are represented directly by pulse count/denominator (8/8, 12/12,
16/16) and retain four-quarter-note bar duration.

One count-in bar precedes eight scored bars. The cycle selects alternating
guide/hidden blocks of one or two bars. Full bars hides every pulse in the
hidden bars; Odds hides 1,3,5…; Evens hides 2,4,6…. Other pulses stay audible.
Numbering restarts each bar. Evens in 1/4 normalizes to 2/4 to avoid a round
with no targets. Presets preserve the current activity. Maximum targets are
64 (four hidden bars × sixteen pulses), within the existing 120-slot pool.

The metronome's 12-byte wire record is version 2, with unchanged PER5 object
size and five record locations. Version 1 records remain readable: tempo
and input correction survive; old keep-tempo becomes Full bars and old
missing-beats becomes Evens; silent counts above one become 2/2. Old 1/4
missing-beats becomes Full bars. Straight 4/4 subdivisions map to the new
grids; other old rhythm/volume/tolerance controls adopt the fixed defaults.
Browser storage writes a v2 key and reads the v1 key as a fallback, retaining
the old browser record. CRC-invalid/unsupported records are still rejected.

Changed execution paths remain main-loop UI/codec and audio-owned bounded
restart/target/sample/input logic. IRQ-masked settings publication, deferred
flash writes and captured browser tap times retain their existing ownership.
No dependencies, flash layout, clocks, bootloader or pitch-training behavior
are changed. Source baseline: 8dfba79 on feature-metronome-training-games.

Current revision compliance:

| Category | Status and evidence |
| --- | --- |
| Target / build / ABI | PASS for compilation/package checks: classic FM-1, nominal 44.1 kHz, 32-sample control blocks, 128-sample I2S halves; matched SDK d179b4484759423312073f5fbb232501aa491047 and existing pi32v2 Clang 4.0.1 r3 build/flags. `target` passed all manifest, RAM closure, register and package checks. Bootloader identity on a physical unit remains UNKNOWN. |
| Audio hot path / numerics | PASS for source/generated-code review: integer-only bounded restart (≤144 candidate pulses), tap search (≤64 actual targets), one expiration per sample, constant-work click. `metro_sample` disassembly uses hardware integer division at pulse boundaries and calls only the bounded RAM-resident `metro_target`; no allocation, I/O, waits or new floating/library helpers. Flash meter table access retains the existing scheme. Cycle division replaces the old silence-pattern division; device costs remain unmeasured. |
| Execution ownership / synchronization | PASS for structure: existing main/ISR split, IRQ-masked setting publication, audio-owned runtime/commands and browser captured timestamps retained; no worker, queue, blocking or ownership changes. |
| Memory / storage / I/O | PASS for static/build/host checks: data+BSS 97,560/98,304 B; pool 135,172/357,888 B; audio arena 77,536 B (minimum 49,152); ISR stack audit 2,984 B. Existing 120-target pool retained, settings record/object size unchanged, flash remains deferred until activity exit. Hardware peak usage and flash latency UNVERIFIED. |
| Dependencies / includes / licenses | PASS: no new dependencies/assets or execution model changes; original GPL/SPDX and Kerem Kilic notices retained. |
| Compatibility / package / recovery | PASS for host/build validation: v1→v2 codec migration, four slots, CRC/bounds, neighboring records, reboot and interrupted A/B writes pass; fixed volume/subdivision/window values serialized. FM-1_327008 package built at 668,948 B; existing resource/loader/manifest checks retained. Physical installation and rollback UNVERIFIED; no flashing performed. |
| Host functional validation | PASS: `metro-test`, `training-test`, `settings-test`, `game-test`, `input-test`, `ui-test`, `web`, `web-test`, `node tools/emu-controls-test.cjs`, `site` and served-site checks. Native tests cover all 22 grids × 3 rules × 2 cycles, independent click schedules in 7/8, wraps, errors, duplicates, ownership, one-page controls and migration. Worklet/WASM tests score 16/16 Full, 8/8 Odds, 8/8 Evens 2/2 and 64/64 sixteenths with delayed speaker clocks, and reject 90 ms-late taps. One-page framebuffer visually reviewed at 240×240 (enlarged for inspection). |
| Hardware real-time validation | UNVERIFIED: no physical FM-1 connected/tested. Before hardware qualification measure ISR deadline/headroom (<2.90 ms half-buffer period), clock jitter, audio/input latency, underruns and stack peaks across all grids/tempo extremes, USB/TRS MIDI and rapid UI activity; test speaker/headphone gain, entry/exit and deferred saves, cold boots and recovery, then run a sustained soak. No hardware timing or audible-output claim follows from these host checks. |

The previously recorded hardware/recovery tests remain skipped. No separate
performance benchmark or sanitizer run is claimed for this revision.

## Earlier implementation and investigation (historical)

The following sections describe the original separate games and two-page UI.
The current controls and migration above supersede those descriptions.

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
