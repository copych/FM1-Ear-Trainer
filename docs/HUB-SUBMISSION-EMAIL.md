# Submission email draft — not sent

To: zven@thefirmwarehub.com

Subject: The Firmware Hub: FM-1 Ear Trainer

Hello,

I'd like to submit FM-1 Ear Trainer, an open-source ear-training game based on
Melodee and its existing FM6 engine.

Firmware name: FM-1 Ear Trainer
Author / card name: copych
Repository: https://github.com/copych/FM1-Ear-Trainer
Identity: FM-1_327008
Licence: GPL-3.0-only, with the retained upstream component licences.
Card metadata: https://github.com/copych/FM1-Ear-Trainer/blob/main/fmhub.json
Card image: https://raw.githubusercontent.com/copych/FM1-Ear-Trainer/main/docs/card.png
Web installer: https://copych.github.io/FM1-Ear-Trainer/webapp/installer/
Installable package: https://copych.github.io/FM1-Ear-Trainer/firmware/ear-trainer-dev.fwsc
Emulator: https://copych.github.io/FM1-Ear-Trainer/emu/
Guide: https://copych.github.io/FM1-Ear-Trainer/webapp/manual/
Editor / backup: https://copych.github.io/FM1-Ear-Trainer/webapp/editor/
Build/source/checksums: https://copych.github.io/FM1-Ear-Trainer/build-info.json
Recovery sources/images: https://copych.github.io/FM1-Ear-Trainer/firmware/ear-trainer-recovery-sources.zip

The game plays major/minor exercises with 1–4 notes. Players reproduce the hidden
ending notes with adjustable assistance and onset spacing. It includes scoring,
automatic retry/next, and adaptive practice for missed degrees. The built-in
keyboard works independently; known notes highlight while hidden pitches stay dark.

The firmware/site are built and attested by zvenson/fm1-build at the reviewed v1
workflow. Host/WASM tests and package/recovery checks block deployment. The package
includes the update loader; source flash gates accept both known FM-1 flash IDs.

HARDWARE TESTS: PASSED (owner-reported, 2026-11-09).
The owner confirmed installation from official V15 and another firmware, working
training/keyboard/display/audio, USB product name FM-1_327008, and successful
switching away with the current build. The physical revision was not specified;
testing on both revisions and quantified timing/soak qualification are not claimed.

Release: https://github.com/copych/FM1-Ear-Trainer/releases/tag/v0.0.8

Thank you,
copych
