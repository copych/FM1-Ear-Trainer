"""Convert the real host framebuffer and summarize captured PCM; no device I/O."""
from pathlib import Path
import array
import math
import wave
from PIL import Image

out = Path("build/ear-baseline")
for path in out.glob("ear-*.ppm"):
    with Image.open(path) as screen:
        screen.save(path.with_suffix(".png"))
with wave.open(str(out / "game-demo.wav"), "rb") as wav:
    pcm = array.array("h", wav.readframes(wav.getnframes()))
    peak = max(abs(x) for x in pcm)
    rms = math.sqrt(sum(x*x for x in pcm) / len(pcm))
    assert peak > 100 and rms > 10, "Game must produce audio, not just change state"
    print(f"Game audio: {wav.getnframes()/wav.getframerate():.2f} s, peak {peak}/32768, RMS {rms:.1f}")
    rate = wav.getframerate()
    with wave.open(str(out / 'two-note-demo.wav'), 'wb') as demo:
        demo.setparams((2, 2, rate, 0, 'NONE', 'not compressed'))
        demo.writeframes(pcm[:int(1.8*rate)*2].tobytes())
    found=[]
    for label, start, end in [('first', .25, .4), ('second', .5, .7)]:
        mono = pcm[int(start*rate)*2:int(end*rate)*2:2]
        rms_note = math.sqrt(sum(x*x for x in mono)/len(mono))
        def energy(hz):
            re = sum(x*math.cos(2*math.pi*hz*n/rate) for n,x in enumerate(mono))
            im = sum(x*math.sin(2*math.pi*hz*n/rate) for n,x in enumerate(mono))
            return math.hypot(re,im)/len(mono)
        pitches = [(note, energy(440*2**((note-69)/12))) for note in [n for n in range(53,80) if n%12 in (0,2,4,5,7,9,11) and (label=='first' or n!=60)]]
        strongest=max(pitches,key=lambda p:p[1])
        if label=='first': root_level=energy(440*2**((60-69)/12))
        assert rms_note>100, f"{label} exercise note must be audible"
        found.append(strongest[0])
        print(label, 'RMS',round(rms_note,1),'dominant candidate',strongest)
    assert found[0]==60 and found[1]!=60, "Exercise must contain C4 followed by a distinct scale note"
    mono=pcm[int(1.1*rate)*2:int(1.3*rate)*2:2]
    held_root=energy(440*2**((60-69)/12))
    held_second=energy(440*2**((found[1]-69)/12))
    # The original EP decays naturally; require overlap, not an organ's flat sustain.
    assert held_root>root_level*.1 and held_second>root_level*.1, 'Both pitches must remain audible late in the held chord'
    print('Held chord fundamental levels:',round(held_root,1),round(held_second,1))
