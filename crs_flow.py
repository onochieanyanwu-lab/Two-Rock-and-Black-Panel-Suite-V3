from svglib import *
d = Dwg(1500, 760)
d.text(24, 32, "Two Rock Classic Reverb Signature 100 — V2 signal flow modelled by the plugin", 22, bold=True)
d.text(24, 54, "Built from the owner's manual's control descriptions + front-panel photo. Two Rock publishes no schematic: stage internals are a reconstruction.", 12.5, col="#444")
row1 = [("INPUT","guitar"),("FET stage","FET switch · FET GAIN ±14 dB\n(below noon also cuts bass)"),("GAIN STRUCTURE","Down: scooped, less gain\nMiddle: hottest\nUp: classic Two-Rock"),("V1  12AX7","gain stage"),("GAIN pot","+ BRIGHT (cap across pot)\nmore gain adds bass"),("EQ1 / EQ2","EQ1: lower gain, fuller\nEQ2: tighter, more gain")]
row2 = [("TREBLE · MIDDLE · BASS","TMB stack · BYPASS = all up"),("MID switch","+4.5 dB around 650 Hz"),("DEEP switch","+5 dB low shelf, tighter mud"),("V4  12AX7","mix / recovery"),("REVERB","SEND → 12AT7 → spring → RETURN\nsend sets drive+decay"),("CONTOUR","CCW: darker, more low end\nCW: brighter, less low end")]
row3 = [("MASTER","pre-PI level"),("V5 12AX7 PI","long-tail pair + NFB (~4.5 dB)"),("4× 6L6GC","push-pull, fixed bias\n100 W / 50 W switch"),("Output transformer","+ speaker impedance model"),("Cab / IR","modelled cab or your IR"),("STANDBY","mutes output")]
def row(items, y):
    x = 24; w = 215
    for i,(t,sub) in enumerate(items):
        d.rect(x, y, w, 120, fill="#eef3f8" if i%2==0 else "#f6efe2")
        d.text(x+w/2, y+26, t, 13, "middle", bold=True)
        for k,l in enumerate(sub.split("\n")): d.text(x+w/2, y+52+k*16, l, 11, "middle", col="#333")
        if i < len(items)-1:
            d.line([(x+w, y+60), (x+w+22, y+60)]); d.add('<polygon points="%g,%g %g,%g %g,%g" fill="#222"/>' % (x+w+14,y+55,x+w+14,y+65,x+w+22,y+60))
        x += w + 22
row(row1, 90); row(row2, 260); row(row3, 430)
for yy in (210, 380):
    d.line([(1370, yy-10), (1370, yy+15), (130, yy+15), (130, yy+50)]) if False else None
d.text(24, 600, "Switch/knob effects are my interpretation of the manual's wording (e.g. exact shelf frequencies); the numbers live in AmpEngine::setControls() and are easy to retune.", 12, italic=True)
d.text(24, 620, "100 W model: solid-state rectifier (the tube rectifier is a 50 W-model feature). 50 W setting = two output tubes, doubled primary impedance, -3 dB.", 12, italic=True)
d.text(24, 640, "Pedal version: the JFET + op-amp pedal on drawing 04 maps to FET stage → V1 → stack → drive; reverb/contour/switches are not in the pedal.", 12, italic=True)
d.save("02_TwoRock_CRS_Signal_Flow.svg")
