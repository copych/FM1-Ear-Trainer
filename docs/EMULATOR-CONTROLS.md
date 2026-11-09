# Emulator button combinations

Use **Hold HOME**, then click another panel button (for example PLAY). HOME stays
held between clicks and releases after the second button is released. Ordinary
HOME taps on the drawn panel retain their momentary behavior, including exiting
training. Click Release HOME to cancel a pending hold.

For other combinations, **Shift+click** a panel button to latch it, then click
another button or play a musical key. A blue outline and a persistent Held status
identify latched controls. The next button/key release clears the latch. Escape,
Release held controls, pointer cancellation and window blur release all simulated
pointer/keyboard keys and buttons. Physical multi-touch holding still works.

The modifier releases 60 ms after the second control, giving the audio worklet
time to observe the queued combination. Reset/blur cancels that timer, and a new
hold cancels an older delayed release.

Changes are confined to browser input handling. They send the same buttons/keys
messages to the existing worklet and firmware; no audio engine, firmware timing,
package layout, persistence or hardware behavior changes. Helper controls stay
disabled until the worklet produces its first frame. Training shortcut buttons
clear any pending hold before issuing their game action.

Validation: `node tools/emu-controls-test.cjs` executes the real page script with
DOM/event stubs and verifies two-click HOME+PLAY masks, Shift latches, original
momentary taps, toggling off, Escape, release button, blur, pointer cancellation
and musical-key release. The check blocks Pages publishing. Existing WASM tests
remain blocking in CI. Browser interaction and deployment are checked separately.

Compliance scope: no new firmware/DSP/ISR work or target ABI/memory changes;
main browser state uses existing event ownership and input transport. No external
dependencies/assets added, original GPL/Melodee notices retained. Host input tests
PASS as above. Pages keeps attested artifacts and installer/recovery checks.
Physical FM-1 timing/revision/recovery qualification remains UNVERIFIED from the
prior firmware; this emulator-only change does not address those categories.

Actual local browser checks PASS: Hold HOME → PLAY enters training, the hold
clears automatically, an ordinary panel HOME tap exits, and Shift+click HOME →
PLAY re-enters. No mouse button needs to stay physically depressed.
