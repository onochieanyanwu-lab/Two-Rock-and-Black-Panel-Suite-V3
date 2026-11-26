from svglib import *
from tube_schematics import stack_panel

def vref_tag(d, x, y):
    d.line([(x, y), (x, y + 8)]); d.line([(x - 8, y + 8), (x + 8, y + 8)], 2.2); d.text(x + 12, y + 14, "Vref 4.5V", 10.5, bold=True)

def jfet_stage(d, x0, y0, title, name, Rd, Rs, inlab, outlab, bright=False):
    d.panel(x0, y0, 470, 450, title)
    cx = x0 + 280; cy = y0 + 150
    d.rail(cx + 18, y0 + 60, "+9V")
    d.Rv(cx + 18, y0 + 60, "Rd", Rd, 54)
    j = d.jfet(cx, cy, name)
    d.dot(cx + 18, cy - 36)
    d.line([(cx + 18, cy - 36), (cx + 80, cy - 36)]); d.Ch(cx + 80, cy - 36, "Cout", "100nF", 36); d.text(cx + 120, cy - 40, outlab, 11, bold=True)
    sx = cx + 18; sy = cy + 36
    d.dot(sx, sy); d.Rv(sx, sy, "Rs", Rs, 46); d.gnd(sx, sy + 46)
    d.line([(sx, sy + 8), (sx + 56, sy + 8)]); d.Cv(sx + 56, sy + 8, "Cs", "10µF", 30, pol=True); d.line([(sx + 56, sy + 38), (sx, sy + 38)]); d.dot(sx, sy + 38)
    gx, gy = j['gate']
    d.dot(gx - 40, gy); d.line([(gx, gy), (gx - 40, gy)])
    d.Rv(gx - 40, gy, "Rg", "1M", 50); d.gnd(gx - 40, gy + 50)
    d.line([(gx - 40, gy), (gx - 60, gy)]); d.Ch(gx - 100, gy, "Cin", "22nF", 40)
    d.text(gx - 104, gy - 6, inlab, 11, "end", bold=True)
    if bright:
        d.line([(gx - 100, gy), (gx - 100, gy - 50)]); d.Ch(gx - 100, gy - 50, "", "", 0) if False else None
        d.text(x0 + 14, y0 + 410, "BRIGHT: footswitch puts 1nF across the 1M input-pad (HF lift)", 10.5, italic=True)
    d.text(x0 + 14, y0 + 428, "Trim Rs (2k trimmer) so drain sits ≈ 5–6 V.  2N5457/J201/MPF102 all work.", 10.5, italic=True)

def opamp_fb(d, ox, oy, label, rows, rg, rg_note, out_label, vref_below=True):
    """flipped op-amp (+ on top). rows = list of drawing callbacks for parallel feedback branches (x_left, x_right, y).
    Left rail runs from the inverting input down through all branches then Rg + Cg to Vref."""
    o = d.opamp(ox, oy, label, flip=True)
    xl, xr = ox - 50, ox + 120
    d.line([(o['inv'][0], o['inv'][1]), (xl, o['inv'][1])])
    d.line([(o['out'][0], o['out'][1]), (xr, o['out'][1])]); d.dot(xr, o['out'][1])
    ys = [oy + 50 + 44 * i for i in range(len(rows))]
    d.line([(xl, o['inv'][1]), (xl, ys[-1])]); d.line([(xr, o['out'][1]), (xr, ys[-1])])
    for yy, fn in zip(ys, rows):
        d.line([(xl, yy), (xl + 10, yy)]); d.line([(xr - 10, yy), (xr, yy)]); d.dot(xl, yy); d.dot(xr, yy); fn(xl + 10, xr - 10, yy)
    d.line([(xl, ys[-1]), (xl, ys[-1] + 14)]); d.Rv(xl, ys[-1] + 14, "", "", 36); d.text(xl + 12, ys[-1] + 38, rg, 11, bold=True)
    d.Cv(xl, ys[-1] + 50, "", "", 30, pol=True); d.text(xl + 14, ys[-1] + 70, "4.7µF", 10.5, bold=True); vref_tag(d, xl, ys[-1] + 80)
    d.line([(xr, o['out'][1]), (xr + 40, o['out'][1])]); d.text(xr + 44, o['out'][1] + 4, out_label, 11, bold=True)
    return o

def volume_opamp(d, x0, y0, title, cfg):
    d.panel(x0, y0, 480, 450, title)
    d.text(x0 + 14, y0 + 48, "from stack", 11, bold=True)
    d.line([(x0 + 40, y0 + 56), (x0 + 40, y0 + 100)]); d.Rv(x0 + 40, y0 + 100, "", "", 130)
    wy = y0 + 165
    d.text(x0 + 52, y0 + 205, "VOLUME", 11, bold=True); d.text(x0 + 52, y0 + 218, "1M audio", 10.5); d.gnd(x0 + 40, y0 + 230)
    d.line([(x0 + 62, wy), (x0 + 48, wy)]); d.add('<polygon points="%g,%g %g,%g %g,%g" fill="#222"/>' % (x0 + 48, wy, x0 + 56, wy - 4, x0 + 56, wy + 4))
    if cfg.get('fender_bright'):
        d.line([(x0 + 40, y0 + 100), (x0 + 96, y0 + 100)]); d.Cv(x0 + 96, y0 + 100, "", "", 65); d.text(x0 + 110, y0 + 126, "Cb 120pF", 10.5, bold=True); d.text(x0 + 110, y0 + 139, "(BRIGHT sw)", 10.5)
        d.dot(x0 + 96, wy)
    d.line([(x0 + 62, wy), (x0 + 100, wy)]); d.Ch(x0 + 100, wy, "Cin", "22nF", 40)
    nonY = wy; ox = x0 + 270; oy = nonY + 14
    d.line([(x0 + 140, wy), (x0 + 200, wy)]); d.dot(x0 + 180, wy); d.Rv(x0 + 180, wy, "", "", 40); d.text(x0 + 192, wy + 26, "1M", 10.5, bold=True); vref_tag(d, x0 + 180, wy + 40)
    d.line([(x0 + 200, wy), (x0 + 200, wy), (ox, wy)])
    def rf(xa, xb, yy): d.Rh(xa + 30, yy, "Rf", "100k", 70); d.line([(xa, yy), (xa + 30, yy)]); d.line([(xa + 100, yy), (xb, yy)])
    opamp_fb(d, ox, oy, "U1A", [rf], "Rg " + cfg['U1_Rg'], "", "to U1B")
    d.text(x0 + 14, y0 + 410, "Gain = 1 + Rf/Rg ≈ %s. Replaces V1B; makes up the ≈ -17 dB stack loss." % cfg['U1_gain'], 10.5, italic=True)
    d.text(x0 + 14, y0 + 428, "TL072 / TL082 / NE5532, single +9V supply, Vref = ½ supply.", 10.5, italic=True)

def clip_panel(d, x0, y0, title, cfg):
    d.panel(x0, y0, 470, 420, title)
    d.text(x0 + 14, y0 + 48, "from U1A", 11, bold=True)
    yy = y0 + 90
    d.line([(x0 + 14, yy), (x0 + 30, yy)]); d.Ch(x0 + 30, yy, "Cin", "22nF", 40); d.line([(x0 + 70, yy), (x0 + 130, yy)])
    d.dot(x0 + 100, yy); d.Rv(x0 + 100, yy, "", "", 40); d.text(x0 + 112, yy + 26, "1M", 10.5, bold=True); vref_tag(d, x0 + 100, yy + 40)
    ox = x0 + 210; oy = yy + 14
    def drive(xa, xb, y_): d.line([(xa, y_), (xa + 14, y_)]); d.Rh(xa + 14, y_, "DRIVE 100k", "+1k stopper", 90); d.line([(xa + 104, y_), (xb, y_)])
    def d1(xa, xb, y_): d.line([(xa, y_), (xa + 30, y_)]); d.diode_h(xa + 30, y_, "D1", 44); d.line([(xa + 74, y_), (xb, y_)])
    def d23(xa, xb, y_): d.line([(xa, y_), (xa + 4, y_)]); d.diode_h(xa + 4, y_, "D2", 44, rev=True); d.diode_h(xa + 48, y_, "D3", 44, rev=True); d.line([(xa + 92, y_), (xb, y_)])
    d.line([(x0 + 130, yy), (ox, yy)])
    opamp_fb(d, ox, oy, "U1B", [drive, d1, d23], "Rg 4.7k", "", "→ MASTER")
    if cfg.get('presence'):
        d.text(x0 + 14, y0 + 346, "PRESENCE: 50k pot + 2.2nF also wired from Rg's top to Vref (parallel with Rg):", 10.5, bold=True, col="#a33")
        d.text(x0 + 14, y0 + 360, "turning the pot toward 0 Ω lifts gain above ≈1.5 kHz.", 10.5, col="#a33")
    d.text(x0 + 14, y0 + 378, cfg['clip_note1'], 10.5, italic=True); d.text(x0 + 14, y0 + 392, cfg['clip_note2'], 10.5, italic=True)
    d.text(x0 + 14, y0 + 406, "Diodes across the feedback resistor soft-clip the stage like a pushed power section.", 10.5, italic=True)

def out_panel(d, x0, y0, title, cfg):
    d.panel(x0, y0, 470, 420, title)
    d.text(x0 + 14, y0 + 48, "from drive stage", 11, bold=True)
    yy = y0 + 100
    d.line([(x0 + 14, yy), (x0 + 24, yy)]); d.Ch(x0 + 24, yy, "C", "100nF", 36); d.line([(x0 + 60, yy), (x0 + 84, yy)])
    d.dot(x0 + 84, yy); d.line([(x0 + 84, yy), (x0 + 84, yy)]); d.Rv(x0 + 84, yy, "", "", 110); d.text(x0 + 76, yy + 52, "MASTER", 11, "end", bold=True); d.text(x0 + 76, yy + 65, "10k log", 10.5, "end"); d.gnd(x0 + 84, yy + 110)
    wy = yy + 55; d.line([(x0 + 106, wy), (x0 + 92, wy)]); d.add('<polygon points="%g,%g %g,%g %g,%g" fill="#222"/>' % (x0 + 92, wy, x0 + 100, wy - 4, x0 + 100, wy + 4))
    d.line([(x0 + 106, wy), (x0 + 130, wy)])
    d.text(x0 + 130, wy - 30, "CAB-SIM (switch bypasses)", 10.5, bold=True)
    d.Rh(x0 + 130, wy, "R1", "2.2k", 44); d.dot(x0 + 174, wy); d.Cv(x0 + 174, wy, "C1", "15nF", 34); d.gnd(x0 + 174, wy + 34)
    d.line([(x0 + 174, wy), (x0 + 200, wy)]); d.Rh(x0 + 200, wy, "R2", "2.2k", 44); d.dot(x0 + 244, wy); d.Cv(x0 + 244, wy, "C2", "15nF", 34); d.gnd(x0 + 244, wy + 34)
    o = d.opamp(x0 + 300, wy + 14, "U2A", flip=True)
    d.line([(x0 + 244, wy), (x0 + 262, wy), (x0 + 262, o['non'][1]), (o['non'][0], o['non'][1])])
    d.line([(o['out'][0], o['out'][1]), (o['out'][0] + 22, o['out'][1])]); d.dot(o['out'][0] + 22, o['out'][1])
    d.line([(o['out'][0] + 22, o['out'][1]), (o['out'][0] + 22, o['inv'][1] + 34), (o['inv'][0] - 20, o['inv'][1] + 34), (o['inv'][0] - 20, o['inv'][1]), (o['inv'][0], o['inv'][1])])
    d.line([(o['out'][0] + 22, o['out'][1]), (o['out'][0] + 40, o['out'][1])]); d.text(o['out'][0] + 28, o['out'][1] - 18, "100µF+1k", 10.5, bold=True); d.text(o['out'][0] + 28, o['out'][1] + 16, "→ OUT", 11, bold=True)
    d.text(x0 + 300, wy + 90, "U2A: unity-gain buffer", 10.5)
    d.text(x0 + 14, y0 + 372, "CAB-SIM ≈ 2-pole low-pass (-3 dB ≈ 3 kHz): use it going straight to a mixer/interface,", 10.5, italic=True)
    d.text(x0 + 14, y0 + 388, "bypass it when feeding a real power amp and speaker cabinet.", 10.5, italic=True)

def power_panel(d, x0, y0, w, cfg):
    d.panel(x0, y0, w, 420, "Power + bias (shared by all stages)")
    x = x0 + 24; y = y0 + 80
    d.text(x, y - 18, "9V DC (centre-negative)", 11, bold=True)
    d.line([(x, y), (x + 30, y)])
    d.add('<polygon points="%g,%g %g,%g %g,%g" fill="#222"/>' % (x + 30, y - 8, x + 30, y + 8, x + 46, y)); d.line([(x + 48, y - 8), (x + 48, y + 8)], 2.2)
    d.text(x + 26, y + 24, "1N5817", 10.5, bold=True)
    d.line([(x + 48, y), (x + 100, y)]); d.dot(x + 100, y); d.Cv(x + 100, y, "", "", 40, pol=True); d.text(x + 112, y + 26, "100µF", 10.5, bold=True); d.gnd(x + 100, y + 40)
    d.line([(x + 100, y), (x + 170, y)]); d.dot(x + 170, y); d.Cv(x + 170, y, "", "", 40); d.text(x + 182, y + 26, "100nF", 10.5, bold=True); d.gnd(x + 170, y + 40)
    d.line([(x + 170, y), (x + 230, y)]); d.rail(x + 230, y, "+9V"); d.text(x + 246, y + 4, "→ all stages", 10.5)
    vx = x + 60; vy = y0 + 170
    d.rail(vx, vy, "+9V"); d.Rv(vx, vy, "", "", 36); d.text(vx + 12, vy + 24, "100k", 10.5, bold=True); d.dot(vx, vy + 36)
    d.Rv(vx, vy + 36, "", "", 36); d.text(vx + 12, vy + 60, "100k", 10.5, bold=True); d.gnd(vx, vy + 72)
    d.line([(vx, vy + 36), (vx + 60, vy + 36)]); d.Cv(vx + 60, vy + 36, "", "", 36, pol=True); d.text(vx + 74, vy + 60, "10µF", 10.5, bold=True); d.gnd(vx + 60, vy + 72)
    d.line([(vx + 60, vy + 36), (vx + 130, vy + 36)]); d.text(vx + 136, vy + 40, "Vref 4.5V → bias nodes", 11, bold=True)
    import textwrap
    for i, ln in enumerate(textwrap.wrap(cfg['power_note'], 70)): d.text(x0 + 20, y0 + 372 + 16 * i, ln, 10.5, italic=True)

def pedal(cfg, path):
    d = Dwg(1500, 1050)
    d.text(24, 30, cfg['title'], 22, bold=True); d.text(24, 50, cfg['subtitle'], 12.5, col="#444")
    # signal flow strip
    flow = cfg['flow']; x = 24; y = 62
    for i, f in enumerate(flow):
        w = 150 if len(f) < 18 else 190
        d.box(x, y, w, 34, f, "", fill="#eaf1e6"); 
        if i < len(flow) - 1: d.line([(x + w, y + 17), (x + w + 18, y + 17)]); d.add('<polygon points="%g,%g %g,%g %g,%g" fill="#222"/>' % (x + w + 12, y + 12, x + w + 12, y + 22, x + w + 18, y + 17))
        x += w + 18
    y0 = 115
    jfet_stage(d, 20, y0, cfg['p1'], "Q1", "10k", "1.5k", "INPUT", "to stack", bright=cfg.get('bright', False))
    stack_panel(d, 510, y0 - 120 + 120, cfg['stack']) if False else None
    cs = dict(cfg['stack']); cs.update(stack_h=450, stack_title='② Passive TMB stack (same network as the amp)', stack_from='from Q1 drain'); stack_panel(d, 510, y0, cs)
    d.rect(510, y0 + 450, 470, 0)
    volume_opamp(d, 1000, y0, cfg['p3'], cfg)
    clip_panel(d, 20, y0 + 470, cfg['p4'], cfg)
    out_panel(d, 510, y0 + 470, cfg['p5'], cfg)
    power_panel(d, 1000, y0 + 470, 480, cfg)
    d.text(24, 1034, cfg['footer'], 11, italic=True)
    d.save(path)

FENDER_PEDAL = dict(
    title="PEDAL ADAPTATION 1 — Blackface Preamp / Drive (9 V, JFET + op-amp)",
    subtitle="The tube circuit re-expressed at pedal voltages: JFET stands in for V1A, the stack is the SAME passive network, an op-amp replaces V1B, diodes replace power-tube saturation.",
    flow=["Input", "Q1 JFET (V1A)", "Passive TMB stack", "VOLUME + bright", "U1A gain (V1B)", "U1B drive + diodes", "MASTER", "Cab-sim / out"],
    p1="① V1A → JFET common-source stage", p3="③ Volume (+bright) and U1A recovery gain", p4="④ Power-tube saturation → op-amp + diodes", p5="⑤ Master, cab-sim, output buffer",
    stack=dict(C1="250pF", C2="0.1µF", C3="0.047µF", R3="10k", R4="100k", stack_note="Same values as the amp. Use 250k pots (B for treble, A for bass), 10k linear for mid."),
    fender_bright=True, U1_Rg="4.7k", U1_gain="22 (27 dB)", bright=False,
    clip_note1="Gain = 1 + DRIVE/4.7k  (0 → 22× with the pot fully up; keep a 1k stopper in series so it never reaches 0 Ω).",
    clip_note2="Output level ≈ ±0.6 V on one polarity and ±1.2 V on the other = the asymmetric 'push-pull gets lopsided' character.",
    power_note="Build tip: 9 V gives ≈ ±3 V of headroom. For cleaner Twin-style headroom run the op-amps from 18 V (TL072 rated) and double the Vref divider.",
    footer="Reverb and tremolo are deliberately left out of the pedal (spring tank + 12AT7 driver are bulky); use a dedicated reverb/tremolo pedal after it. Circuit values are a starting point: socket the JFET and trim by ear.")
TWOROCK_PEDAL = dict(
    title="PEDAL ADAPTATION 2 — Two Rock-style Preamp / Drive with Master + Presence (9 V)",
    subtitle="Fender-derived chain with the reconstructed Two Rock stack, a brighter input, MASTER after the drive stage and a PRESENCE control standing in for the NFB loop.",
    flow=["Input + BRIGHT", "Q1 JFET (V1A)", "Passive stack", "VOLUME", "U1A gain (V1B)", "U1B drive + diodes", "MASTER", "Cab-sim / out"],
    p1="① V1A → JFET stage (+ input bright)", p3="③ Volume and U1A gain", p4="④ Drive stage (diodes) + PRESENCE", p5="⑤ Master, cab-sim, output buffer",
    stack=dict(C1="470pF", C2="47nF", C3="22nF", R3="25k", R4="47k", stack_note="Reconstruction values. 250k treble/bass, 25k mid (linear)."),
    fender_bright=False, U1_Rg="4.7k", U1_gain="22 (27 dB)", bright=True,
    clip_note1="Gain = 1 + DRIVE/4.7k. PRESENCE branch (50k pot + 2.2nF) is wired in parallel with Rg to Vref.",
    clip_note2="Diodes: use 1N4148 / 1N914; swap for BAT41 / germanium for a softer knee.",
    power_note="Tube-rectifier 'sag' feel: add a 470 Ω series resistor + 470 µF on the +9 V feed to U1B only; the sag now follows how hard you play.",
    footer="This is the same circuit family as the Blackface pedal with different stack values and the extra PRESENCE/MASTER controls; both share the same PCB layout idea.", presence=True)

if __name__ == "__main__":
    pedal(FENDER_PEDAL, "03_Fender_Blackface_Pedal_DIY.svg")
    pedal(TWOROCK_PEDAL, "04_TwoRock_Pedal_DIY.svg")
