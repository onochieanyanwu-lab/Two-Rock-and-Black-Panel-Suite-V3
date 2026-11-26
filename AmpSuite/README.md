# AmpSuite – Blackface Fender + Two Rock Classic Reverb Signature V2 (VST3)

Two VST3 amp plugins built from one circuit-level DSP core (`AmpCore.h`), plus circuit diagrams and DIY pedal adaptations.

## Files (flat layout – keep them all in the repo root, except the workflow)
| File | What |
|---|---|
| `AmpCore.h` | The DSP: tube stages, tone stack, phase inverter, power amp + output transformer + speaker load, supply sag, spring reverb, tremolo, cab |
| `AmpProcessor.h/.cpp`, `AmpEditor.h` | JUCE wrapper (parameters, oversampling, cab/IR, generic knob UI) |
| `CMakeLists.txt` | Builds **both** plugins (`AMP_FENDER`, `AMP_TWOROCK` defines) |
| `.github/workflows/build.yml` | Windows VST3 build via GitHub Actions (must stay at this path) |
| `diagrams/01…04*.svg` | Reference circuits + the two DIY pedal schematics (open in a browser) |
| `test/` | Numerical test harnesses used to validate the DSP (g++ only, no JUCE) |

**Build:** push everything to a private GitHub repo → Actions tab → download artifact `AmpSuite-VST3-Windows` → copy both `.vst3` folders to `C:\Program Files\Common Files\VST3`.

## What is modelled (per sample, Newton-solved against the node equations)
* 12AX7 triode stages (Koren model): grid conduction, coupling-cap/grid-leak blocking distortion, Miller capacitance, cathode RC, plate load.
* Fender TMB tone stack – coefficients derived symbolically from the exact topology drawn in the diagram.
* Long-tail-pair phase inverter (tail-current limited), push-pull 6L6GC/6V6 (beam-tetrode Koren model), fixed bias solved to the idle current.
* Output transformer (magnetising inductance + winding capacitance) driving a speaker impedance model (Re, lossy Le, cone resonance); output is cone acceleration, so the pentode "boom at resonance" and the voltage/current-drive behaviour are real.
* Global negative feedback from the speaker node (auto-calibrated to ~6 dB, 4.5 dB on Two Rock) + Presence on Two Rock.
* Supply sag (rectifier/reservoir; Two Rock has the slower tube-rectifier behaviour), spring reverb (dispersive allpass springs), optical tremolo (Fender "vibrato").
* 2x/4x oversampling, modelled cabinet or any IR you load (**Load IR…** – use a real mic'd IR for best results).

## Controls
Both: Volume, Treble, Middle, Bass, Reverb, Sag, Bright, Input Trim, Output, Cabinet (Off/Modelled/IR), Quality (2x live / 4x render).
Fender: Vib Speed, Vib Intensity, Amp (Deluxe 22 W 6V6 / Twin 85 W 6L6).  Two Rock: Master, Presence.

## Measured behaviour (test harness, 1 kHz, tone controls at noon)
* Clean-to-breakup progression on all three voicings (e.g. Twin vol 5: 0.1 % THD at -30 dBFS in → 1.8 % at -12 dBFS → 9 % at -6 dBFS); Deluxe breaks up earlier, as it should.
* Stable at 88.2 k–384 k engine rates, with max-drive bursts and feedback, no NaNs.
* Idle bias: ≈34 mA/tube Twin; spring tail ≈ 2.4 s; tremolo depth up to ≈15 dB.
* CPU ≈ 13 % of one core at 2x (96 kHz engine) on the build machine; roughly double at 4x.

## Honest limits – please read
* **I could not listen to it or A/B it against UAD.** "As good as UAD" is a target, not something I can certify. What I did verify is the physics (levels, harmonics, stability, tone-stack curves, tube bias points against datasheets). Voicing by ear is the next step.
* **Schematics are drawn from my knowledge of the standard circuits**, not from scans (this environment couldn't fetch them). The tone stack drawing is proven identical to the DSP; other component values (plate loads, cathode networks, NFB values, supply voltages) are standard-blackface values to the best of my recollection – **check against a scan of an AB763/AA270 before building hardware.**
* **Two Rock publishes no schematic.** That voicing is a reconstruction (Fender-derived preamp, Dumble-style stack values, master volume, presence, tube-rectifier sag). It is a good-faith starting point, not a clone.
* Not modelled: output-transformer core saturation, power-tube screen-resistor dynamics, microphonics, hum/noise.
* The pedals are *adaptations* (JFET + op-amp + diodes + the same passive stack), not tube-accurate emulations; reverb/tremolo are left out of the pedals.

## Tuning hooks (all in `AmpCore.h`, `AmpEngine::configure`)
`piPad` (how hard the preamp hits the power amp), `nfbReductionDb`, `sagR`, stack `R/C` values, tube constants (`kTube*`), `reverbReturn`, cab filters in `CabSim`.
If you upload schematic scans (as you did for the Quad Eight) I can replace my recalled values with the real ones and retune.

## V2 Two Rock (Classic Reverb Signature) – panel and behaviour
Controls follow the owner's manual: FET Gain + FET switch, Treble, Middle, Bass, Gain, Master, Reverb Send, Reverb Return, Contour; switches Bright, Mid, Deep, EQ1/EQ2, Gain Structure (Down/Middle/Up), Tone Bypass, Standby, plus 100 W/50 W, Sag, Input, Output, Cabinet, Quality.
Signal flow: `diagrams/02_TwoRock_CRS_Signal_Flow.svg`. The knob/switch *effects* are modelled from the manual's wording; exact filter frequencies and gains are my interpretation (see `AmpEngine::setControls`). The reverb return now follows a smooth taper (V1's squared taper was the bug).
Because the controls match the real amp, TV settings should transfer as starting points – expect to trim by ear, since cab/mic/guitar differ from the show.

### Level fix (V2.1)
Output was far too quiet (≈ -24 dBFS peak at Master 5 / Gain 5). Added loudness compensation tied to Master (80 % of the pot's loss + 4 dB), Output range now -24…+18 dB (default 0), and a soft limiter near 0 dBFS. Typical DI levels now land around -8 dBFS peak / -20 dBFS RMS at Gain 5; Gain 10 hits the limiter.

### V2.2
Blackface Fender (Twin and Deluxe) got the same loudness compensation (80% of the Volume pot loss + per-amp base lift): typical DI now ≈ -8 dBFS peak / -20 dBFS RMS at Volume 5. Two Rock unchanged.
