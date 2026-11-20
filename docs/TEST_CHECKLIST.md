# Manual test checklist (Ableton Live)

Do these in order in a new Live set. Tick each box. Note anything odd next to it.

Setup: `scripts/build.sh`, then in Live: Settings → Plug-Ins → Rescan. Create a MIDI track and drop **Turntable Sampler** (VST3 or AU) on it. Set the Live tempo to 120 and press Live's play.

- [ ] **Load sample:** drag a drum loop from Finder onto the platter. The label shows its name, the waveform appears, and SAMPLE BPM shows a sensible tempo (fix it with /2 or x2 if needed).
- [ ] **Play:** click inside the plugin window, press **Space**. The record spins up and plays in time with Live's metronome.
- [ ] **Scratch:** hold **S** and move the mouse in circles over the platter (or drag on it). The sound follows your hand forwards and backwards with no clicks. Let go: it ramps back to normal speed.
- [ ] **Tape stop:** hold **G**. It slows down and stops over about 1 second. Release: it spins back up. Turn the TAPE STOP knob to 3 s and try again.
- [ ] **Repeater 1/4:** hold **E** while it plays. It stutters one beat at a time, in time with Live. Release: it carries on exactly in time.
- [ ] **Change host BPM mid-playback:** while it plays, change Live's tempo from 120 to 140. Speed and pitch rise smoothly, with no jump or click. Press **K** (Keep pitch) and change the tempo again: the speed changes but the pitch stays.
- [ ] **Loop and pads:** press **L** (a 1-bar loop starts), then **Shift+1** to store it on pad 1. Press **L** to turn the loop off, let it play on, then press **1**: it jumps back into the stored loop.
- [ ] **Filter:** drag in the FILTER pad. The sound gets darker to the left and more resonant to the top. Press **F** to bypass.
- [ ] **Reload the Ableton set:** save the set, close it, reopen it. The sample, sample BPM, loop, pad 1 and every knob are as you left them.
- [ ] **Hotkey overlay:** press **?**. The list appears; Esc closes it.
- [ ] **No dropouts:** in Live set the audio buffer to 128 samples and repeat scratch, tape stop and repeater. No crackles.
