# Adaptive practice and scoring — development 0.0.3

For development 0.0.4 sequence controls, see [Sequence controls](SEQUENCE-CONTROLS.md).
Scoring is now per completed sequence; the first wrong entry ends an attempt.

Wrong answers show red for one second, then automatically replay the same
two-note question. The attempt count and target are retained. Musical keys remain
blocked through feedback/playback; held keys must be released before answering.
Correct answers show green and awarded points for one second, then generate the
next question.

The interval table now includes 11 semitones: B4, degree 7 above C4. Degrees 2–8
are available (D4, E4, F4, G4, A4, B4, C5).

Awards by successful attempt are 100/70/40/10/0 (attempts 1/2/3/4/5+).
Each manual replay subtracts 10, floored at zero; automatic wrong-answer repeats
carry no additional replay penalty. No response-time bonus. PTS is the session
total (capped at 999999), S counts consecutive first-attempt successes without
manual replay (capped at 999). A miss, assisted success or skip breaks the streak.
Skipping awards nothing. Scores and practice history survive game exit/reentry
in RAM, and reset on firmware restart/power-off. No persistent storage added.

Each target degree has a need counter 0–6. A wrong answer increments the expected
degree's counter, capped at 6; an unassisted first-attempt success decrements it.
Selection weight is 4 + 2 × need. The bounded seven-entry cumulative draw keeps
every degree eligible. One maximally weak degree among otherwise fresh degrees
has probability 16/40 = 40%; weights do not guarantee a particular next question.

Changed execution paths: audio ISR → ear_audio_tick → bounded degree selection;
keyboard/MIDI → ear_player_press → ear_answer → integer score/profile updates.
UI snapshots under existing interrupt masking. Existing FM6 TINE EP, voice
manager, renderer, sample rate and control cadence remain in use.

## Validation and compliance

Native game tests pass for automatic same-question replay, key/MIDI lockout,
held-key gating, second-attempt/manual-replay award 60, first-attempt award 100,
score boundaries, cancellation and reentry. A seeded 7000-draw test found seventh
972 times with equal weights and 2739 times with maximum seventh weight; every
other degree remained above 688 occurrences. Native feedback PNG inspected.

| Category | Status and scope |
| --- | --- |
| Target/build/ABI | Target build result and package identity recorded below; same Melodee baseline/toolchain/SDK as PROTOTYPE.md, integrity checks retained. |
| Audio hot path/numerics | PASS source: bounded seven-entry integer loops, capped counters, no new allocation, I/O, logging, waits, DSP or doubles. Target division cost and worst latency UNVERIFIED. |
| Ownership/synchronization | PASS source/host: audio owns state, UI uses existing IRQ publication/snapshots; no new worker or blocking queue. Hardware masking latency UNVERIFIED. |
| Memory/storage/I/O | Fixed state only, no serialization or storage writes; static target budget recorded below. Runtime peaks UNVERIFIED. |
| Dependencies/licenses | PASS source: no new dependency/assets; GPL and Melodee attribution retained. |
| Compatibility/package/recovery | Existing flash gates, image layout, loader and paired resources retained. No device writes. Physical old/new revisions and recovery UNVERIFIED. |
| Host functionality | PASS focused native tests above; browser/package results below. Original full UI/peripheral/golden suites and sanitizers not rerun for this patch. |
| Hardware real time | UNVERIFIED: user tested prior firmware, not this build. No CPU, underrun, latency, stack or soak measurements claimed. |

Before hardware qualification: use a verified backup/recovery route, test both
known FM-1 flash revisions, confirm wrong-repeat/held-key behavior on local and
MIDI input, score/replay/skip behavior and audible B4. Measure audio service time,
underruns and stack peaks under five-note FM6 tails, dense MIDI and UI redraws;
check outputs and power-cycle/reset behavior. Local development package only;
public release requires the upstream attested release workflow.

Replay during green success advances to a new question, preventing a solved
question from earning another award. The native regression checks that this
advances the serial and retains the score.

## Final local build evidence

- JieLi target build PASS, manifest/section/stack/package checks retained.
  App 565212 B; package 668948 B, development identity FM-1_900.
- Static RAM .data+.bss 97304/98304 B (1000 B remaining), pool
  134052/357888 B; audio arena 81728 B. Static audio ISR stack audit 2980 B.
  These are build budgets, not runtime measurements.
- 495 linked audio-RAM functions verified by upstream build audit.
- Final native game-test and WASM web-test PASS, including automatic replay,
  keyboard/MIDI feedback, two distinct audible notes and three reentry cycles.
- Site 0.0.3 regenerated with paired resources and WASM; route/script/hash and
  installer-selection checks PASS. No hardware installation performed.
- Firmware SHA-256:
  `697dcba5d79fbb020088b234a419baf916d535038094c21a1b8dd6a742206cdc`.

Reproduce with `tools/build-melodee.sh game-test`, `web`, `web-test`, `target`,
then `site` in the established WSL environment; run `node tools/check-site.cjs`
on Windows with the local site server running. Target/build/ABI and static
memory checks PASS within this scope; runtime hardware limits remain above.
