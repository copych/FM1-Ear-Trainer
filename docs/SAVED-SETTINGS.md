# PRESETS knob and persistent game preferences — 0.0.11

The printed PRESETS encoder now selects the 24 factory FM6 sounds during
training. Every sound remains forced to poly mode, with the existing safe
same-question replay and duplicate-award protection. K3 is reserved again.
The footer identifies the sound with `PRE`.

Scale, complexity (count/guess/range), onset spacing and preset now persist.
Scores, adaptive practice weights and attempts remain session-only. Hardware
preferences use the existing A/B device settings store. Browser preferences use
localStorage on the emulator's origin; clearing site data clears them, and
unavailable browser storage leaves the emulator playable without persistence.

## Save timing and ownership

Actual control changes request a settings save and restart a one-second edit
debounce. The hardware main loop commits only in a quiet answer pause: no
pending game command, loading operation, held input, player note or active voice
on any part. It also respects the existing flash-ID, transport, USB audio,
protected-template and failed-write retry gates. After exiting training, the
existing stopped/idle settings policy drains any pending save.

Main establishes the quiet window with IRQs masked and publishes `ear_saving`.
Audio-owned scheduling preserves its state without starting generated notes,
and player note-on cannot start a voice while that flag is set. Main displays
`SAVING SETTINGS`, flushes the display, runs the existing flash transaction, then
clears the flag and redraws. Flash erase/program remains the platform's deliberate
silenced operation outside musical playback. No flash work, CRC or serialization
is added to the audio ISR. Held-key/MIDI ownership still drains normally.

Preset load ownership, poly/send publication and PREPARE scheduling remain as
described in RANGES-PRESETS.md. Only encoder ownership changes from K3 to
`EN_PRESET`, respecting the calibrated panel mapping and direction.

## Storage contract and compatibility

PER5 remains exactly 604 bytes; its magic, template offset, A/B sectors, storage
CRC and update/rollback layout do not change. A 12-byte record occupies retired
DIGITAL favorites row 1 bytes 8–19, preserving its first 64 legacy favorite bits
and the adjacent MIDI-knob (20–23), click (24–27), and owner (28–31) records.
The allocation is documented in favorites.c and settings_persist.c. Builds
without the game preserve these bytes through the existing import/export path.

Wire bytes: `EAR`, version 1, scale ID (0 major / 1 minor), range ID (0 octave /
1 full), count, guess, preset index, little-endian onset milliseconds, CRC8
(polynomial 0x07, initial zero, covering bytes 0–10). No compiler enum/struct
layout is serialized. Decoder bounds: count 1–4, guess 1–count, known scale and
range IDs, preset below 24, gap 0–1000 in 25 ms increments. Missing, unknown,
truncated or CRC-invalid game data falls back to game defaults; unrelated valid
device preferences and musical data remain intact. Upgrading older settings
normalizes this reserved record once. Unchanged settings do not cause an erase.

The same codec serves the emulator. Its worklet restores a validated record
before training starts and posts changed snapshots to the page. localStorage
access stays on the page, outside the AudioWorklet. Browser and hardware stores
are independent; neither transmits preferences externally.

## Validation and compliance

Baseline source `80b8e68bf78bed329441e4877d4f51243f8501bd` (0.0.10), vendored
Melodee `165d177a425944f3d30b0b669255a6ed0050c3a5`. The native game/audio suite
passed before editing. Existing JieLi pi32v2 clang 4.0.1 r3, matched AC79 SDK,
flags, board/loader, audio sample/block configuration and linker contracts are
retained; see BUILDING.md. No physical device writes were performed.

| Category | Status and evidence |
| --- | --- |
| Target / build / ABI | PASS: target integrity gates and final artifact budgets below; no ABI, SDK or compiler change. |
| Audio hot path / numerics | PASS for structural review: only bounded save guards added; codec and flash stay in main; no new floating-point expressions, allocations, math calls or render changes. Hardware timing unmeasured. |
| Ownership / synchronization | PASS for source/host scope: calibrated PRESETS ownership, main-only settings publication, atomic quiet-window guard, blocked note starts during commit, existing MIDI release ownership. |
| Memory / storage / I/O | PASS for storage/bounds review: fixed 12-byte checked record in existing settings allocation, explicit schema, unchanged 604-byte PER5/template layout, debounced A/B writes with existing retry. Runtime headroom unmeasured. |
| Dependencies / includes / licenses | PASS: no dependencies or external assets; existing browser localStorage only; upstream notices retained. |
| Compatibility / package / recovery | PASS for host/source scope: legacy/current settings and musical-data preservation, truncated/corrupt input rejection, simulated interrupted A/B save/retry. Product/loader unchanged. Actual new-build install/rollback UNVERIFIED. |
| Host functional validation | PASS: game/audio, UI, MIDI/flash, settings codec, simulated NOR persistence/origin, browser controls/localStorage, fresh-instance WASM restore and corrupt-record rejection. Package/site checks accompany the final build. |
| Hardware real-time validation | UNVERIFIED: no device save/reboot, both-revision, timing, underrun, runtime memory or soak measurements for 0.0.11. Prior owner reports do not qualify this build. |

The persistence test round-trips every scale/range/count/guess/preset combination
through PER5; checks malformed fields, all truncated lengths and single-bit CRC
errors; simulates reboot, write interruption, A/B fallback and retry; proves
debounce and playback/command/held-key deferral; and asserts that an attempted
player note cannot start a voice in the flash erase hook. Existing settings
origin tests still verify project/preset data checksums across cleanup and
stable settings on subsequent boot. The legacy zero-favorites assertion now
excludes the reserved game-record bytes and explicitly checks game defaults.

Hardware follow-up: change every preference, wait for the quiet save indicator
to finish, power-cycle and check restoration; repeat on both supported flash
revisions. Check rapid encoder turns, held keyboard/MIDI input, USB audio,
save retries and exit/re-entry; measure worst render/ISR latency, underruns and
runtime memory through a representative soak. Do not power-cut a sole device
as a routine test; interrupted writes are covered by simulated NOR tests here.

Final local target: app 567992 B, package 668948 B; .data+.bss 97336/98304 B,
pool 134052/357888 B, available audio arena 80928 B. Static audio ISR stack
audit 2984 B; 496 linked audio functions verified in SRAM. All four settings
suites (ear_persistence_test, settings_test, persistence_test,
settings_origin_test), game/audio, UI, MIDI/flash, WASM, browser controls/storage,
metadata, package/recovery and served site/hash checks PASS. Public distribution
uses the required attested shared workflow. Local firmware SHA-256:
`859a7a0454282f8306583efa8e21bfd32a75ed85ad661c63bbef0043d6670027`.

Real browser check: changed the printed PRESETS knob to BELL, K1 to A minor,
K2 to 2 OF 2 OCT and K4 to 275 ms. A separately opened emulator page restored
all four preferences before starting training; the captured display matches.
