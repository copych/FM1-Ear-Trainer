# FM-1 Ear Trainer 0.0.16

Pitch training, a metronome and rhythm games now share one firmware and training menu.

- Metronome: tap tempo, 22 meter/pulse grids, odd groupings and four user presets.
- Rhythm games: Full bars, Odds or Evens, with 1/1 or 2/2 audible/hidden bars; immediate audible guide bars without an extra precount.
- Timing results: hits, misses, extras, score, mean error, early/late bias and consistency. Browser inputs use captured timestamps aligned to output audio.
- Metronome sounds: current click, 808, CR78 and FM hats; OH on strong/group accents, CH on other pulses. Separate audible player taps.
- Shared shrinking/fading green/red feedback circles for pitch and rhythm training.
- Dim musical keys during training, following LIGHTS. Settings/progress save on activity exit, avoiding periodic SAVING SETTINGS interruptions.

Open training with HOME + PLAY while stopped. Choose an activity with SELECT or K1, then PLAY. Plain Metronome uses K1 tempo, K2 meter and K3 sound. Rhythm game uses K3 game rule and K4 bar cycle. HOME returns to the training menu.

Leave the activity and wait for saving to finish before powering off. Changes since the last activity exit are lost if power is cut. Older preferences migrate automatically; existing sound setups default to the current click.

Firmware is built and source-attested by the required `zvenson/fm1-build` workflow, with blocking host/web tests. Assets include the matching application/resource images, recovery sources, native recovery tools for Windows x64, Linux x64, macOS ARM64 and macOS x64, licences, provenance and SHA-256 checksums.

Host/build validation is complete; physical FM-1 real-time, loudness, soak and recovery qualification remain unverified. See the source compliance report in `docs/METRONOME.md`. Back up device content before installation.
