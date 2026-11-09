# Hardware feedback follow-up

Historical 0.0.2 report. Wrong-answer replay and scoring are superseded by
[Adaptive scoring](ADAPTIVE-SCORING.md) in development build 0.0.3.

The user tested the preceding prototype on an FM-1 and requested stronger
correct/wrong indication, automatic next after success, and no scale-searching
after a wrong answer. This report does not treat that play test as quantified
hardware timing or old/new-revision qualification.

## Behavior

- Correct: full green background, white CORRECT!, and one second of feedback.
  The existing audio scheduler then generates and plays the next exercise.
- Wrong: full red background, white WRONG and PLAY TO RETRY. No automatic next.
  Musical note-ons cannot audition more pitches or resubmit an answer. PLAY /
  Replay retries the same exercise; REC / Next selects another one.
- Note-offs, held-note ownership and disconnect cleanup remain active. A key held
  through success cannot answer the new exercise until released and pressed again.
- The first submitted response is still audible. Subsequent notes during feedback
  are blocked, including after success, until a new answer phase is armed.

## Change and validation scope

Only ear_trainer.c and ear_ui.c change firmware behavior. The audio ISR reuses
the existing `left` frame countdown and `armed` gate; no buffers, allocations,
storage, DSP, synthesis/presets or synchronization mechanisms are added. Rendering
colors live in main-loop stack variables; persistent state remains in the shared
game state. Correct/next is a bounded state transition; it does not sleep or wait.

Native integration tests check red/green framebuffer values, all 27 local keys,
a MIDI pitch sweep after failure, unchanged voice allocation age (no new voices),
manual retry, the one-second success hold, automatic next, held-key gating,
note releases and repeated game entry/exit. Rendered red/green screens were
visually inspected at 240x240. The existing separate-note audio checks are retained.
WASM tests exercise feedback colors, locked key/MIDI sweeps and automatic next.

## Compliance gate

| Category | Status and evidence |
| --- | --- |
| Target/build/ABI | PASS: app 564524 bytes; data+bss 97272/98304 and pool 134052/357888 unchanged. Audio working arena 82176 bytes; 495 linked audio functions verified in SRAM. Existing ABI, stack and integrity checks pass. |
| Audio hot path/numerics | PASS structural scope: integer countdown and state checks, no new float/DSP work, allocation, I/O, logging, waits or blocking. |
| Ownership/synchronization | PASS source/host scope: audio owns result/timer/arming; UI snapshot still uses existing IRQ masking. No worker changes. |
| Memory/storage/I/O | Fixed state and storage layout unchanged; countdown reuses existing field. Static build checks retained; runtime peaks and I/O contention UNVERIFIED. |
| Dependencies/licenses | PASS: no dependency/asset imports; existing GPL notices and FM6 preset retained. |
| Compatibility/package/recovery | Existing flash/update/recovery paths unchanged; regenerated local installer pairs firmware, resources and WASM. Hardware qualification/recovery of this revision UNVERIFIED. |
| Host functional validation | PASS: focused native and WASM regressions above, including framebuffer colors and blocked voice allocation; full engine golden, sanitizer and unrelated peripheral suites not repeated because engine/platform code is unchanged. |
| Hardware real-time validation | UNVERIFIED for this revision: no measured deadlines, underruns, CPU/stack peaks, soak or validation on both physical revisions. |

No firmware was flashed by the agent. The updated package is available as local
development build 0.0.2 through the installer. Page/script, package/resource/WASM
hash and installer-selection checks pass. This revision needs hardware retesting.
