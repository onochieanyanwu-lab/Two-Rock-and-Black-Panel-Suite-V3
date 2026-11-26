from svglib import *

def stack_panel(d, x0, y0, cfg):
    d.panel(x0, y0, 470, cfg.get("stack_h", 450), cfg.get("stack_title", "② TMB tone stack (exactly the topology the DSP solves)"))
    IN = (x0 + 40, y0 + 85); P = (x0 + 150, y0 + 85); Q = (x0 + 150, y0 + 205)
    d.text(x0 + 14, y0 + 48, cfg.get("stack_from", "from V1A plate"), 11, bold=True)
    d.line([(x0 + 14, IN[1]), (IN[0], IN[1]), (IN[0], Q[1])]); d.dot(IN[0], IN[1])
    d.Ch(IN[0], IN[1], "C1", cfg['C1'], 70); d.line([(IN[0] + 70, IN[1]), P]); d.dot(*P)
    d.Rv(P[0], P[1], "", "", 120)
    d.text(P[0] + 14, P[1] + 28, "TREBLE", 11, bold=True); d.text(P[0] + 14, P[1] + 41, "250k", 11)
    wy = P[1] + 60; d.line([(P[0] + 22, wy), (P[0] + 8, wy)]); d.add('<polygon points="%g,%g %g,%g %g,%g" fill="#222"/>' % (P[0] + 8, wy, P[0] + 16, wy - 4, P[0] + 16, wy + 4))
    d.line([(P[0] + 22, wy), (P[0] + 120, wy)]); d.text(P[0] + 124, wy + 4, "OUT → volume", 11, bold=True)
    d.dot(IN[0], Q[1]); d.Rh(IN[0], Q[1], "R4 (slope)", cfg['R4'], 110); d.dot(*Q)
    N4 = (Q[0], Q[1] + 44); N5 = (Q[0], N4[1] + 48)
    d.Cv(Q[0], Q[1], "C2", cfg['C2'], 44)
    d.Rv(N4[0], N4[1], "", "", 48); d.text(N4[0] + 14, N4[1] + 22, "BASS 250k", 11, bold=True); d.text(N4[0] + 14, N4[1] + 35, "(rheostat)", 10)
    d.dot(*N5)
    d.Rv(N5[0], N5[1], "", "", 48); d.text(N5[0] + 14, N5[1] + 22, "MID " + cfg['R3'], 11, bold=True); d.text(N5[0] + 14, N5[1] + 35, "(rheostat)", 10)
    d.gnd(N5[0], N5[1] + 48)
    d.line([(Q[0], Q[1]), (Q[0] + 150, Q[1])]); d.Cv(Q[0] + 150, Q[1], "C3", cfg['C3'], 92)
    d.line([(Q[0] + 150, Q[1] + 92), (N5[0], N5[1])])
    d.text(x0 + 14, y0 + 395, "Insertion loss ≈ -17 dB at the mids with all knobs at 10; deeper scoop as MID is lowered.", 10.5, italic=True)
    d.text(x0 + 14, y0 + 428, cfg['stack_note'], 10.5, col="#6a8a00", bold=True)

def amp_diagram(cfg, path):
    d = Dwg(1500, 1010)
    d.text(24, 30, cfg['title'], 22, bold=True); d.text(24, 50, cfg['subtitle'], 12.5, col="#444")
    y0 = 70
    # ---- A: input stage
    d.panel(20, y0, 470, 450, "① Input stage V1A")
    if cfg.get('bright_in'):
        d.text(34, y0 + 44, "BRIGHT switch: HF shelf before V1A (+6 dB > 1.8 kHz)", 10.5, italic=True)
    stage(d, 330, y0 + 110, "V1A", "12AX7", cfg['B1'], "100k", "1.5k", "25µF", "INPUT", "to stack", "1M", "68k", "100nF")
    d.text(34, y0 + 420, "Guitar source ≈30k; 68k stopper; Miller C ≈100pF rolls off the top.", 10.5, italic=True)
    stack_panel(d, 510, y0, cfg)
    # ---- C: volume + V1B
    d.panel(990, y0, 490, 450, cfg['volpanel'])
    d.text(1002, y0 + 100, "from stack", 11, bold=True)
    d.line([(1010, y0 + 110), (1010, y0 + 150)]); d.Rv(1010, y0 + 150, "", "", 120)
    d.text(1024, y0 + 250, "VOLUME", 11, bold=True); d.text(1024, y0 + 263, "1M audio", 10.5)
    d.gnd(1010, y0 + 270)
    if cfg.get('bright_cap'):
        d.line([(1010, y0 + 150), (1040, y0 + 150)]); d.Cv(1040, y0 + 150, "", "", 50)
        d.text(1050, y0 + 170, "Cb 120pF", 10.5, bold=True); d.text(1050, y0 + 183, "(BRIGHT sw)", 10.5)
        d.line([(1040, y0 + 200), (1040, y0 + 210)])
    wy = y0 + 210; d.line([(1010 + 22, wy), (1010 + 8, wy)]); d.add('<polygon points="%g,%g %g,%g %g,%g" fill="#222"/>' % (1018, wy, 1026, wy - 4, 1026, wy + 4))
    d.line([(1032, wy), (1052, wy)])
    d.dot(1040, wy) if cfg.get('bright_cap') else None
    # stage V1B: gy = y+86 must equal wy -> y = wy-86
    stage(d, 1300, wy - 86, "V1B", "12AX7", cfg['B2'], "100k", "1.5k", "25µF", "", "to PI", "1M", "10k", "22nF")
    d.line([(1052, wy), (1300 - 36 - 152, wy)])
    # ---- D: reverb / tremolo
    yd = y0 + 470
    d.panel(20, yd, 470, 450, "③ Reverb + tremolo (block level)")
    bx = 60; w = 390
    d.box(bx, yd + 40, w, 44, "V1B plate (signal)", "")
    d.box(bx, yd + 120, w, 50, "Reverb driver 12AT7 → transformer", "drives spring-tank input coil")
    d.box(bx, yd + 206, w, 50, "Spring tank (2 springs, ~2.4 s)", "dispersive delay ≈ 33 ms and 38 ms")
    d.box(bx, yd + 292, w, 50, "Recovery 12AX7 → REVERB pot", "wet signal mixed back in")
    d.box(bx, yd + 372, w, 44, cfg['trem'], cfg['trem_sub'], fill="#f6efe2")
    for yy in (84, 170, 256, 342):
        d.line([(bx + w / 2, yd + yy), (bx + w / 2, yd + yy + 36)]); d.add('<polygon points="%g,%g %g,%g %g,%g" fill="#222"/>' % (bx + w / 2 - 5, yd + yy + 30, bx + w / 2 + 5, yd + yy + 30, bx + w / 2, yd + yy + 38))
    # ---- E: power amp
    d.panel(510, yd, 970, 450, cfg['powerpanel'])
    px = 600; py = yd + 170
    d.rail(px + 80, py - 70, "B+ 285V")
    d.line([(px + 20, py - 70), (px + 140, py - 70)])
    for i, xx in enumerate((px + 20, px + 140)):
        d.line([(xx, py - 70), (xx, py - 50)]); d.Rv(xx, py - 50, "Ra" if i == 0 else "Rb", "82k", 50)
        t = d.triode(xx, py + 52, "PI-A" if i == 0 else "PI-B", "12AX7")
        d.line([(xx, py), (xx, py + 16)])
        d.line([(xx, t['cath'][1]), (xx, py + 112)])
    d.line([(px + 20, py + 112), (px + 140, py + 112)]); d.dot(px + 80, py + 112)
    d.Rv(px + 80, py + 112, "Rtail", "≈ 56k", 40); d.gnd(px + 80, py + 152)
    d.line([(px - 50, py + 52), (px - 16, py + 52)]); d.text(px - 54, py + 56, "in", 11, "end", bold=True)
    d.line([(px + 104, py + 52), (px + 110, py + 52)]); d.text(px + 100, py - 80, "", 10)
    d.text(px + 150, py + 88, "← NFB enters here", 10.5, col="#a33", bold=True)
    d.dot(px + 20, py + 6); d.text(px + 24, py + 20, "to V3", 10.5, bold=True)
    d.dot(px + 140, py + 6); d.text(px + 144, py + 20, "to V4", 10.5, bold=True)
    tx = (px + 300, px + 450); pins = []
    for i, xx in enumerate(tx):
        p = d.pentode(xx, py + 52, "V%d ×2" % (3 + i), cfg['tube_short']); pins.append(p)
        gx, gy = p['g1']
        d.Ch(gx - 66, gy, "0.1µF", "", 42); d.line([(gx - 24, gy), (gx, gy)]); d.dot(gx - 24, gy)
        d.Rv(gx - 24, gy, "", "", 36); d.text(gx - 20, gy + 26, "220k", 10.5); d.text(gx - 20, gy + 54, "-%dV bias" % cfg['bias'], 10.5, bold=True)
        d.line([(gx - 24, gy + 36), (gx - 24, gy + 44)])
        d.gnd(p['cath'][0], p['cath'][1])
    ox = px + 640
    d.ot(ox, py - 10, 150)
    d.line([pins[0]['plate'], (pins[0]['plate'][0], py - 78), (ox, py - 78), (ox, py - 10)])
    d.line([pins[1]['plate'], (pins[1]['plate'][0], py - 58), (ox - 70, py - 58), (ox - 70, py + 176), (ox, py + 176), (ox, py + 140)])
    d.line([(ox, py + 65), (ox - 36, py + 65)]); d.rail(ox - 36, py + 65, "B+ (CT)")
    d.text(ox - 6, py - 88, "OT  Zpp " + cfg['zpp'], 11, bold=True)
    d.line([(ox + 34, py - 10), (ox + 96, py - 10), (ox + 96, py + 14)]); d.add('<rect x="%g" y="%g" width="40" height="64" rx="4" fill="#fff" stroke="#222" stroke-width="1.8"/>' % (ox + 76, py + 14))
    d.text(ox + 96, py + 52, "SPKR", 11, "middle", bold=True); d.text(ox + 96, py + 66, cfg['spk'], 10.5, "middle")
    d.line([(ox + 96, py + 78), (ox + 96, py + 140), (ox + 34, py + 140)]); d.gnd(ox + 96, py + 140)
    d.dot(ox + 96, py - 10); d.line([(ox + 96, py - 10), (ox + 96, py - 40), (ox + 130, py - 40)])
    d.Rh(ox + 130, py - 40, "820Ω", "", 50); d.text(ox + 60, py - 118, cfg['nfb_note'], 10.5, bold=True, col="#a33")
    d.text(ox + 60, py - 104, "(820Ω back to PI-B grid)", 10.5, col="#a33")
    d.text(530, yd + 410, cfg['power_note'], 10.5, italic=True); d.text(530, yd + 428, 'Output transformer centre-tap B+: ' + cfg['bplus'], 10.5, italic=True)
    d.save(path)

FENDER = dict(title="Blackface Fender (AB763 / AA270 family) — circuit referenced by the DSP model",
              subtitle="Signal path redrawn from the standard blackface vibrato-channel topology; component values are the ones used in AmpCore.h (verify against a scan before building).",
              B1="B+ ≈ 285V", B2="B+ ≈ 270V", C1="250pF", C2="0.1µF", C3="0.047µF", R3="10k (Twin) / 6.8k (Deluxe)", R4="100k",
              stack_note="Treble 250k, Bass 250k audio taper, Mid per amp model.",
              volpanel="③ Volume (+ bright cap) and V1B recovery/gain stage", bright_cap=True,
              trem="Optical tremolo (\"vibrato\")", trem_sub="lamp + LDR modulates level, 1.5–10.5 Hz",
              powerpanel="④ Phase inverter, push-pull output stage, output transformer, NFB",
              tube="6L6GC (Twin) / 6V6 (Deluxe)", tube_short="6L6GC/6V6", bias=46, bplus="440 V (Twin) / 420 V (Deluxe)", zpp="2k (Twin) / 8k", spk="4 Ω / 8 Ω",
              nfb_note="NFB ≈ 6 dB (820Ω into PI)", power_note="Twin: 4×6L6GC (two per side), SS rectifier, 85 W.  Deluxe: 2×6V6, 22 W.  Bias is fixed (negative supply), screens ≈ B+.")
TWOROCK = dict(title="Two Rock Classic Reverb Signature 100 — RECONSTRUCTED topology used by the DSP model",
              subtitle="Two Rock has not published a schematic. This is a Fender-derived reconstruction with a Dumble-style stack, master volume, presence and tube-rectifier supply.",
              B1="B+ ≈ 300V", B2="B+ ≈ 285V", C1="470pF", C2="47nF", C3="22nF", R3="25k", R4="47k",
              stack_note="Reconstruction values — edit in AmpCore.h to taste.",
              volpanel="③ Volume and V1B gain stage", bright_cap=False, bright_in=True,
              trem="MASTER volume (before phase inverter)", trem_sub="10k audio pot; sets power-stage drive independent of preamp gain",
              powerpanel="④ Phase inverter, 4×6L6GC output stage, output transformer, NFB + PRESENCE",
              tube="6L6GC", tube_short="6L6GC", bias=49, bplus="470 V (GZ34 rectifier)", zpp="2k", spk="4 Ω",
              nfb_note="NFB ≈ 4.5 dB + PRESENCE", power_note="4×6L6GC (two per side), tube rectifier = more sag, fixed bias. No tremolo on this voicing.", presence=True)

if __name__ == "__main__":
    amp_diagram(FENDER, "01_Fender_Blackface_Circuit.svg")
    amp_diagram(TWOROCK, "02_TwoRock_Classic_Reverb_Circuit.svg")
