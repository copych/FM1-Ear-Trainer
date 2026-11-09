# Selected instrument: Melodee FM6

This decision supersedes the custom sine/triangle and two-operator FM experiments
in `ear-trainer/`. Retain those as experiments; do not extend them for the game.

Use Melodee as the firmware foundation and its existing FM6 engine, voice manager
and factory presets. Initial source lock remains the acquired revision
`165d177a425944f3d30b0b669255a6ed0050c3a5`. No engine extraction into fm1-base is
planned: preserve the functioning instrument infrastructure and add a game mode.

## Evidence and resource policy

Source inspection, not hardware measurement:

- `firmware/src/eng_fm6.c` exposes 24 factory sounds plus an init patch,
  native DX7 voice data, and a 16-voice engine limit.
- `firmware/src/voice.c` documents FM6 at one weighted voice-budget unit,
  ordinary engines at two, and serial Prophet at three. These are the project's
  scheduling weights, not CPU utilization percentages or a RAM comparison.
- `firmware/src/engines.c` allocates working state for the engine selected by
  each part. Leaving engines idle reduces active DSP/state needs; it does not
  remove linked code, fixed track buffers or infrastructure reservations.
- `fm6_part_t` retains state for NVOICE voices even when fewer sound. A five-note
  cap alone does not shrink that allocation.

Start with one active FM6 part, five sounding notes for chords, allowance for
release tails through the existing allocator, no unison and no active effects or
other instrument parts. Use an existing electric-piano-style factory voice as
the initial audition candidate; also audition MARIMBA and CLAVINET. Sound quality
and suitability still require listening. Reject unpredictable pitch modulation,
weak fundamentals and excessive tails for the training default.

FM6 is selected for its ready presets, polyphony and favorable existing voice
weight. Do not claim it leaves a measured percentage of CPU/RAM free. Establish
the unmodified Melodee build/map, report actual state and free-memory figures,
and measure five-note render cost including tails before making that claim.
Do not reduce native voice array sizes or disable infrastructure speculatively.

## Next work

1. Build pinned Melodee unmodified with the available JieLi toolchain/SDK.
2. Exercise its existing presets and five-note chords in the host/browser.
3. Verify its old/new flash and boot/update paths in source and focused tests.
4. Add the two-note game as a mode on the existing instrument interfaces.

No Melodee build, source modification, hardware qualification or flashing is
claimed by this selection document. Preserve the project's GPL notices and
AGENTS.md instructions when modifying it.
