# FM-1 Ear Trainer

**On your FM-1: stop playback, hold HOME and press PLAY to open Ear Trainer.**

Turn SELECT (or K1) to choose **Pitch training**, **Metronome**, or **Rhythm game**, then press PLAY. HOME returns to this shared training menu; HOME again leaves training. In the browser, choose **Start training** to open the same menu. All three activities are included in one firmware.

Listen to the notes, then play the hidden ones on the keyboard—in any order, separately or together. Highlighted notes are safe to play again while you work out the rest.

**[Try this branch in your browser](https://copych.github.io/feature-metronome-training-games/emu/)** · **[Install this branch on your FM-1](https://copych.github.io/feature-metronome-training-games/webapp/installer/)** · [Branch guide](https://copych.github.io/feature-metronome-training-games/webapp/manual/) · [Main version](https://copych.github.io/FM1-Ear-Trainer/)

![Listen and find the hidden note](docs/images/guess.png) ![Correct answer](docs/images/correct.png) ![Four-note exercise](docs/images/four-notes.png)

## Pitch training controls

| Control | What it does |
| --- | --- |
| HOME + PLAY | Open the shared training menu |
| K1 | C major / A minor |
| K2 | How many notes to guess out of 1–4, in one octave or across the keyboard |
| K4 | Time between notes; 0 plays them together |
| PRESETS | Choose a polyphonic FM6 or Prophet sound |
| PLAY | Hear the question again |
| REC | Skip to the next question |
| HOME | Return to the training menu; HOME again exits |

Green means correct and moves to the next question. Red means a mistake and repeats the question. RATING starts at zero and rewards first-try answers; BEST remembers your highest rating for each mode.

Settings and ratings are saved. Leave training and let saving finish before switching off. In the browser, choose **Start training**.

## Metronome and rhythm games

Choose **Metronome** or **Rhythm game** from the shared training menu. **HOME + REC** remains a direct rhythm shortcut when outside training.

One settings page offers 30–300 BPM and 22 meters/pulse grids, including odd groupings and 4/4 eighths, triplets and sixteenths. Volume stays at 100%, subdivision at 1, and the timing window at ±60 ms (capped below half the spacing between required taps at fast tempos). PRESETS offers 16 setups and four saved user slots.

| Control | What it does |
| --- | --- |
| K1 | Tempo |
| K2 | Meter/grouping, including pulse grids |
| K3 | Full bars / Odds / Evens |
| K4 | Audible/hidden bars: 1/1 or 2/2 |
| PLAY | Start/stop; start another round after results |
| REC | Tap quarter-note tempo; averages up to four intervals |
| Any keyboard key / MIDI note | Submit a game tap |
| PRESETS | Load a factory setup or a saved user slot |
| OCT+ | Save current settings into the selected user slot |
| OCT- | Restart from the first audible guide bar |
| ALGORITHM | Input timing correction −100 to +100 ms |
| HOME | Return to the training menu and save settings/presets |

**Rhythm game:** starts immediately with the audible guide bar(s), with no extra precount. Eight bars alternate audible and hidden bars (1/1 or 2/2). Tap only the missing pulses in hidden bars: **Full bars** hides all pulses; **Odds** hides 1, 3, 5…; **Evens** hides 2, 4, 6…. Other pulses stay audible. Numbering starts again each bar. Evens needs at least two pulses, so selecting it in 1/4 switches to 2/4. BPM always counts quarter notes. Metronome plays the selected grid continuously without scoring.

Results show a timing-weighted score, hits, misses, extra taps, mean absolute error, signed early/late bias and timing consistency (SD). The header shows LISTEN in audible bars and TAP SILENCE in hidden bars; pulse visuals appear only in audible bars. The browser aligns captured taps with the audio output clock automatically when supported; ALGORITHM adds a manual input correction (start at 0 ms). Use wired audio for consistent timing. See the [metronome guide](https://copych.github.io/feature-metronome-training-games/webapp/manual/#metronome) for details. Device settings save after leaving the screen; browser settings and user slots are remembered locally. Round results are session-only.

Built on [Melodee](https://github.com/keremimo/melodee), with FM6 and Prophet sounds. [GPL-3.0](LICENSE) · [Component licences](melodee/LICENSING.md) · [Development docs](docs/DEVELOPMENT.md). Independent project; not affiliated with M-VAVE.
