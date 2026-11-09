# Four-zone game screen and Prophet presets — 0.0.12

The top 48 rows contain four equal zones corresponding left-to-right to K1–K4:
scale, hidden/total plus range, reserved K3, onset milliseconds. Keeping K3
reserved preserves the established mapping until its role is chosen. The
middle contains larger reference/hidden-note cards, a clear LISTEN/YOUR TURN/
RESULT line, attempt/answer progress, and labelled SCORE and STREAK. The bottom
48 rows show engine, POLY and patch name. Rendering retains the existing five
48-row canvas tiles; no framebuffer or new graphical asset is added.

Known cards highlight during playback and accepted answer positions turn green.
The failed position has a red card; its pitch stays hidden. Only success reveals
the completed sequence. Existing reference-only keyboard lights, red/green
feedback, one-error lockout, auto-repeat/advance, scoring and timing remain.
SCORE is the session total; STREAK counts consecutive first-attempt successes
without replay. The score award is written explicitly in the correct message.
The requested top knob strip supersedes the host instrument's normal status
header for this dedicated screen; game phase is central and saving still uses
the header. Both font faces have compact knob values when their widths require
them, and free patch names are fit/ellipsised without crossing screen bounds.

PRESETS browses stable IDs 0–23 (existing FM6) then 24–223 (the existing 200
Prophet factory programs, mapped to Prophet engine presets 1–200). INIT is not
included in either factory bank. Both use the existing preset loader in main,
then explicitly force poly mode and zero effect sends before playback resumes.
PREPARE waits for the selected engine and its fade, not specifically FM6.
No synth is imported and no DSP/voice-allocation implementation changes.

The 12-byte saved preference record, 604-byte PER5 envelope, flash map and loader
remain unchanged. FM6 records still use version 1 with their original IDs. A
Prophet selection uses version 2 with the extended sound ID; the new decoder
accepts both and checks each version's bounds and CRC. Older firmware will use
its default game preferences if given a version-2 record; unrelated settings
are preserved. Browser storage uses the same record and existing key.

## Validation and compliance

Baseline `f8b06b08982e390faabcb962b4fed2268d098b76` (0.0.11), original Melodee
baseline `165d177a425944f3d30b0b669255a6ed0050c3a5`. Native game/audio baseline
PASS before editing. Same JieLi pi32v2 clang 4.0.1 r3, matched AC79 SDK, flags,
board, bootloader, sample/block configuration and linker contracts as BUILDING.md.
No hardware writes were performed. Portability limits are documented in PORTING.md.

| Category | Status and evidence |
| --- | --- |
| Target / build / ABI | PASS: final target integrity checks and budgets below. No ABI/SDK/clock/layout change. |
| Audio hot path / numerics | PASS for structural review: selected-engine readiness check is bounded integer work; preset loading/naming/drawing stay in main. Existing Prophet/FM6 renderers untouched. Actual Prophet headroom UNVERIFIED. |
| Ownership / synchronization | PASS for source/host scope: existing loading/save guards and IRQ publication retained, old voices cancel, selected engine/fade settles before first onset; generated/player sources remain separate. |
| Memory / storage / I/O | PASS for bounds/schema review: same small settings/state, canvas and A/B allocation. Versioned preset IDs/CRC accepted; no new buffers, storage area or peripheral. Runtime peaks unmeasured. |
| Dependencies / includes / licenses | PASS: reuse already-built Prophet engine/factory resources and existing graphics. No new dependency or asset, notices retained. |
| Compatibility / package / recovery | PASS for host/source scope: old FM6 preferences and new Prophet selections round-trip, invalid/truncated records rejected, existing interrupted A/B tests pass. Product/loader unchanged. New-build device install/rollback UNVERIFIED. |
| Host functional validation | PASS: native game tests exercise all 224 presets with four gates and engine transitions, same-question replay, cancellation, controls, lights and scoring; all sound IDs round-trip through settings. Both fonts/extreme values captured. WASM/UI/package checks accompany final build. |
| Hardware real-time validation | UNVERIFIED: no new-build analog, both-revision, render/ISR timing, underrun, headroom or soak measurements. Host tests do not qualify four-voice Prophet timing. |

Hardware follow-up: both font layouts, four-card/full-length names and score
extremes; switching both ways across the FM6/Prophet boundary during playback
and answer entry, held keyboard/MIDI input, all four sustained notes, save/reboot
and cancellation. Measure worst render/ISR time, underruns and runtime memory
with representative Prophet factory programs on both revisions. Public updates
continue through the mandatory attested shared workflow with blocking tests.

Final local target: app 568756 B, package 668948 B; .data+.bss 97336/98304 B,
pool 134052/357888 B, available audio arena 80896 B. Static audio ISR stack
audit 2984 B and all 496 linked audio functions pass SRAM closure checks.
Native game/audio, both-font screen captures, all four persistence suites, UI,
MIDI/flash, WASM audio and cross-engine/persisted Prophet selection, metadata
and offline package/recovery checks PASS. Firmware SHA-256 (local build):
`b279b691754390482411a00e1d66760252a0f5e7ddf1c561bdc7ec3156133851`.
The Hub card is refreshed from the native answer-screen capture; no external
graphical assets were added. A compile-time bank-size assertion requires an
explicit settings migration before future factory-bank changes can shift IDs.
Real-browser check also passed: existing A-minor/2-of-2/275 ms/BELL preferences
restore under the new layout, then PRESETS crosses to the first Prophet factory
program and the footer shows PROPHET, POLY and the correct program name.
