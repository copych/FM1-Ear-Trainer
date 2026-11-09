# Reusing the ear trainer in another firmware

Current status: separated files, but not a drop-in portable library. This fork
deliberately made a playable game on the existing Melodee engine first. File
separation is not yet an independent platform interface.

## Current integration points

- `ear_trainer.c`: challenge generation, lifecycle, ordered matching, scoring,
  adaptive weights, keyboard/MIDI collection and playback scheduling. Direct
  dependencies include `FS`, `trk[0]`, `trk_note_on/off`, engine/fade state,
  FM6/Prophet sound IDs and Melodee's MIDI-source conventions. Included by seq.c;
  it is not independently compiled against an API header.
- `ear_ui.c`: panel mapping, IRQ publication, track/preset loading and temporary
  part muting, theme/font/canvas rendering and settings-save requests. It reads
  the core's static state directly. Included by ui.c.
- seq.c / midi_control.c: sample/block tick, built-in keyboard routing, raw MIDI
  ownership, disconnect handling and separation of exercise notes from answers.
- ui_input.c / ui_draw.c: entering/leaving the modal game, input consumption,
  rendering and reference-only keyboard lights.
- settings_persist.c / project.c: validated preference record and quiet flash
  commit window in the host's existing A/B store.
- web/emu: host-compiled game plus browser preference transport. This exercises
  application logic, not the hardware instruction set or timing.

## Practical portability boundary

A close Melodee fork can reuse these integration points if its APIs and storage
contracts match. Another FM-1 firmware needs deliberate adaptation even if it
shares Felucca ancestry. A compatible chip alone does not establish compatible
voice allocation, interrupt ownership, display or flash layout.

The next architectural step is to extract three parts:

1. **Portable game core:** fixed-size challenge/settings/state types, seeded
   generation, matching, score and hints. Expose init/start/action/note/tick and
   a read-only view snapshot; keep synth, panel, graphics and flash out.
2. **Sound/input/timing adapter:** owned exercise voices and cancellation,
   player/exercise source distinction, note-on/off, engine-ready notification,
   sample or monotonic scheduling, held-key/disconnect ownership and the host's
   safe main/audio publication protocol. At least four simultaneous voices are
   needed for the current maximum exercise.
3. **UI/storage adapters:** four encoder roles, page entry/exit and lights,
   view rendering, preset names/handles and a checked preference/statistics
   record saved through the target firmware's own safe flash mechanism.

The current engine/preset mapping helpers are small, but do not by themselves
complete this extraction. Host-independent tests should compile the extracted
core without Melodee sources, followed by adapter/target tests. This refactor is
not part of the 0.0.12 visual/preset update.

Each port must retain fixed bounds, deterministic scheduling, release/cancel
behavior and source separation; keep storage/graphics/allocation out of audio;
and independently verify target memory, CPU timing, keyboard/display, polyphony,
package/update/recovery and both physical revisions. Reuse the new host's flash
allocation and loader rather than copying Melodee's map. Preserve applicable
GPL-3.0-only and third-party notices and review the destination's licence/SDK
terms before combining sources. No other firmware port is currently qualified.
