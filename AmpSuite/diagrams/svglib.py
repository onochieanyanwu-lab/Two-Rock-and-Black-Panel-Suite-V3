# tiny schematic drawing helpers -> SVG
class Dwg:
    def __init__(s, w, h, bg="#fbfaf6"):
        s.w, s.h = w, h; s.e = []; s.bg = bg
    def add(s, x): s.e.append(x)
    def line(s, pts, w=1.6, col="#222", dash=None):
        d = ' stroke-dasharray="%s"' % dash if dash else ''
        s.add('<polyline points="%s" fill="none" stroke="%s" stroke-width="%s" stroke-linejoin="round"%s/>' % (" ".join("%g,%g" % p for p in pts), col, w, d))
    def text(s, x, y, t, size=12, anchor="start", col="#111", bold=False, italic=False):
        s.add('<text x="%g" y="%g" font-family="Helvetica,Arial,sans-serif" font-size="%g" text-anchor="%s" fill="%s"%s%s>%s</text>' % (x, y, size, anchor, col, ' font-weight="700"' if bold else '', ' font-style="italic"' if italic else '', t))
    def dot(s, x, y, r=3): s.add('<circle cx="%g" cy="%g" r="%g" fill="#222"/>' % (x, y, r))
    def rect(s, x, y, w, h, fill="none", col="#222", sw=1.4, rx=6, dash=None):
        d = ' stroke-dasharray="%s"' % dash if dash else ''
        s.add('<rect x="%g" y="%g" width="%g" height="%g" rx="%g" fill="%s" stroke="%s" stroke-width="%g"%s/>' % (x, y, w, h, rx, fill, col, sw, d))
    def circle(s, x, y, r, fill="none", col="#222", sw=1.6): s.add('<circle cx="%g" cy="%g" r="%g" fill="%s" stroke="%s" stroke-width="%g"/>' % (x, y, r, fill, col, sw))
    # ---- components. (x,y) = first terminal; vertical goes down, horizontal goes right. return far terminal.
    def Rv(s, x, y, label="", val="", L=46, lx=10):
        a = y + 8; b = y + L - 8; zz = [(x, y), (x, a)]
        n = 6; step = (b - a) / n
        for i in range(n): zz.append((x + (7 if i % 2 == 0 else -7), a + step * (i + 0.5)))
        zz += [(x, b), (x, y + L)]; s.line(zz)
        if label or val: s.text(x + lx + 4, y + L / 2 - 2, label, 11, bold=True); s.text(x + lx + 4, y + L / 2 + 11, val, 11)
        return (x, y + L)
    def Rh(s, x, y, label="", val="", L=46):
        a = x + 8; b = x + L - 8; zz = [(x, y), (a, y)]
        n = 6; step = (b - a) / n
        for i in range(n): zz.append((a + step * (i + 0.5), y + (7 if i % 2 == 0 else -7)))
        zz += [(b, y), (x + L, y)]; s.line(zz)
        if label: s.text(x + L / 2, y - 12, label, 11, "middle", bold=True)
        if val: s.text(x + L / 2, y + 22, val, 11, "middle")
        return (x + L, y)
    def Cv(s, x, y, label="", val="", L=40, pol=False):
        m = y + L / 2; s.line([(x, y), (x, m - 4)]); s.line([(x, m + 4), (x, y + L)])
        s.line([(x - 11, m - 4), (x + 11, m - 4)], 2.4); s.line([(x - 11, m + 4), (x + 11, m + 4)], 2.4)
        if pol: s.text(x - 20, m - 6, "+", 11)
        if label or val: s.text(x + 16, m - 1, label, 11, bold=True); s.text(x + 16, m + 12, val, 11)
        return (x, y + L)
    def Ch(s, x, y, label="", val="", L=40):
        m = x + L / 2; s.line([(x, y), (m - 4, y)]); s.line([(m + 4, y), (x + L, y)])
        s.line([(m - 4, y - 11), (m - 4, y + 11)], 2.4); s.line([(m + 4, y - 11), (m + 4, y + 11)], 2.4)
        if label: s.text(m, y - 16, label, 11, "middle", bold=True)
        if val: s.text(m, y + 26, val, 11, "middle")
        return (x + L, y)
    def gnd(s, x, y):
        s.line([(x, y), (x, y + 6)]); s.line([(x - 10, y + 6), (x + 10, y + 6)]); s.line([(x - 6, y + 10), (x + 6, y + 10)]); s.line([(x - 2, y + 14), (x + 2, y + 14)])
    def rail(s, x, y, label):
        s.line([(x - 12, y), (x + 12, y)], 2.4); s.text(x, y - 6, label, 11, "middle", bold=True)
    def triode(s, cx, cy, name="", kind="12AX7"):
        s.circle(cx, cy, 24); s.line([(cx, cy - 36), (cx, cy - 12)]); s.line([(cx - 11, cy - 12), (cx + 11, cy - 12)], 2.2)
        s.line([(cx - 11, cy), (cx - 6, cy)], 2); s.line([(cx - 2, cy), (cx + 3, cy)], 2); s.line([(cx + 7, cy), (cx + 11, cy)], 2); s.line([(cx - 36, cy), (cx - 11, cy)])
        s.line([(cx - 11, cy + 12), (cx + 11, cy + 12)], 2.2); s.line([(cx, cy + 12), (cx, cy + 36)])
        if name: s.text(cx + 30, cy - 2, name, 11, bold=True); s.text(cx + 30, cy + 11, kind, 10, col="#555")
        return dict(plate=(cx, cy - 36), grid=(cx - 36, cy), cath=(cx, cy + 36))
    def pentode(s, cx, cy, name="", kind="6L6GC"):
        s.circle(cx, cy, 26); s.line([(cx, cy - 38), (cx, cy - 14)]); s.line([(cx - 11, cy - 14), (cx + 11, cy - 14)], 2.2)
        for dy in (-4, 4):
            for k in range(-11, 11, 8): s.line([(cx + k, cy + dy), (cx + k + 4, cy + dy)], 2)
        s.line([(cx - 38, cy + 4), (cx - 11, cy + 4)]); s.line([(cx + 11, cy - 4), (cx + 38, cy - 4)])
        s.line([(cx - 11, cy + 14), (cx + 11, cy + 14)], 2.2); s.line([(cx, cy + 14), (cx, cy + 38)])
        if name: s.text(cx + 32, cy + 22, name, 11, bold=True); s.text(cx + 32, cy + 35, kind, 10, col="#555")
        return dict(plate=(cx, cy - 38), g1=(cx - 38, cy + 4), g2=(cx + 38, cy - 4), cath=(cx, cy + 38))
    def pot_h(s, x, y, label="", val="", L=70, wiper=True):
        s.Rh(x, y, "", "", L)
        if wiper:
            s.line([(x + L / 2, y - 24), (x + L / 2, y - 7)]); s.add('<polygon points="%g,%g %g,%g %g,%g" fill="#222"/>' % (x + L / 2 - 4, y - 12, x + L / 2 + 4, y - 12, x + L / 2, y - 6))
        if label: s.text(x + L / 2, y + 22, label, 11, "middle", bold=True)
        if val: s.text(x + L / 2, y + 34, val, 11, "middle")
        return (x + L / 2, y - 24)
    def opamp(s, x, y, label="", flip=True):
        # triangle; flip=True: NON-inverting input on top, inverting at bottom (output right)
        s.add('<polygon points="%g,%g %g,%g %g,%g" fill="#fff" stroke="#222" stroke-width="1.8"/>' % (x, y - 34, x, y + 34, x + 62, y))
        top, bot = ("+", "−") if flip else ("−", "+")
        s.text(x + 6, y - 10, top, 14); s.text(x + 6, y + 22, bot, 14)
        if label: s.text(x + 34, y + 4, label, 10, "middle", col="#555")
        if flip: return dict(non=(x, y - 14), inv=(x, y + 14), out=(x + 62, y))
        return dict(inv=(x, y - 14), non=(x, y + 14), out=(x + 62, y))
    def jfet(s, cx, cy, label=""):
        # N-channel JFET: gate left, drain top, source bottom
        s.line([(cx, cy - 22), (cx, cy + 22)], 2.6); s.line([(cx, cy - 16), (cx + 18, cy - 16), (cx + 18, cy - 36)]); s.line([(cx, cy + 16), (cx + 18, cy + 16), (cx + 18, cy + 36)])
        s.line([(cx - 28, cy), (cx - 3, cy)]); s.add('<polygon points="%g,%g %g,%g %g,%g" fill="#222"/>' % (cx - 3, cy, cx - 12, cy - 5, cx - 12, cy + 5))
        s.circle(cx + 6, cy, 30, sw=1.2)
        if label: s.text(cx + 40, cy + 4, label, 11, bold=True)
        return dict(gate=(cx - 28, cy), drain=(cx + 18, cy - 36), src=(cx + 18, cy + 36))
    def diode(s, x, y, label="", L=40, up=False):  # vertical, anode top->cathode bottom
        m = y + L / 2; s.line([(x, y), (x, m - 8)]); s.line([(x, m + 8), (x, y + L)])
        s.add('<polygon points="%g,%g %g,%g %g,%g" fill="#222"/>' % (x - 8, m - 8, x + 8, m - 8, x, m + 8)); s.line([(x - 9, m + 8), (x + 9, m + 8)], 2.2)
        if label: s.text(x + 14, m + 4, label, 11, bold=True)
        return (x, y + L)
    def diode_h(s, x, y, label="", L=40, rev=False):
        m = x + L / 2; s.line([(x, y), (m - 8, y)]); s.line([(m + 8, y), (x + L, y)])
        if not rev: s.add('<polygon points="%g,%g %g,%g %g,%g" fill="#222"/>' % (m - 8, y - 8, m - 8, y + 8, m + 8, y)); s.line([(m + 9, y - 9), (m + 9, y + 9)], 2.2)
        else: s.add('<polygon points="%g,%g %g,%g %g,%g" fill="#222"/>' % (m + 8, y - 8, m + 8, y + 8, m - 8, y)); s.line([(m - 9, y - 9), (m - 9, y + 9)], 2.2)
        if label: s.text(m, y - 13, label, 10.5, "middle", bold=True)
        return (x + L, y)
    def ot(s, x, y, h=120, label="", sec=True):
        # transformer: primary coil on left of core, secondary right. (x,y) top-left of coil column
        for side, cxp in (("L", x), ("R", x + 34)):
            n = 5; seg = h / n
            for i in range(n):
                yy = y + i * seg
                if side == "L": s.add('<path d="M %g %g a %g %g 0 0 1 0 %g" fill="none" stroke="#222" stroke-width="1.8"/>' % (cxp, yy, seg / 2, seg / 2, seg))
                else: s.add('<path d="M %g %g a %g %g 0 0 0 0 %g" fill="none" stroke="#222" stroke-width="1.8"/>' % (cxp, yy, seg / 2, seg / 2, seg))
        s.line([(x + 15, y), (x + 15, y + h)], 1.6); s.line([(x + 19, y), (x + 19, y + h)], 1.6)
    def box(s, x, y, w, h, title, sub="", fill="#eef3f8"):
        s.rect(x, y, w, h, fill=fill); s.text(x + w / 2, y + h / 2 - (4 if sub else -4), title, 12, "middle", bold=True)
        if sub: s.text(x + w / 2, y + h / 2 + 12, sub, 10.5, "middle", col="#444")
    def panel(s, x, y, w, h, title):
        s.rect(x, y, w, h, fill="#ffffff", col="#9aa", sw=1.2, rx=10); s.text(x + 12, y + 20, title, 13, bold=True, col="#234")
    def save(s, path):
        body = "\n".join(s.e)
        open(path, "w").write('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d"><rect width="100%%" height="100%%" fill="%s"/>\n%s\n</svg>' % (s.w, s.h, s.w, s.h, s.bg, body))

def stage(d, x, y, name, kind, Bp, Rp, Rk, Ck, inlab, outlab, Rleak="1M", Rstop="68k", Cc="22nF"):
    """common-cathode stage. rail at (x,y). returns plate-out point"""
    d.rail(x, y, Bp)
    d.Rv(x, y, "Rp", Rp, 50); d.dot(x, y + 50)
    t = d.triode(x, y + 86, name, kind)
    pl = t['plate']; d.line([pl, (x, y + 50)])
    # output coupling
    d.line([(x, y + 50), (x + 40, y + 50)]); d.Ch(x + 40, y + 50, "", "", 30); d.line([(x + 70, y + 50), (x + 110, y + 50)])
    d.text(x + 112, y + 46, outlab, 11, bold=True)
    # cathode network
    cy = t['cath'][1]; d.dot(x, cy)
    d.Rv(x, cy, "Rk", Rk, 44); d.gnd(x, cy + 44)
    d.line([(x, cy + 6), (x + 36, cy + 6)]); d.Cv(x + 36, cy + 6, "Ck", Ck, 34, pol=True); d.line([(x + 36, cy + 40), (x, cy + 40)]); d.dot(x, cy + 40) if False else None
    # grid side
    gx, gy = t['grid']
    d.line([(gx, gy), (gx - 26, gy)]); d.Rh(gx - 72, gy, "", "", 46); d.text(gx - 49, gy - 12, "Rg " + Rstop, 11, "middle", bold=True)
    d.line([(gx - 26, gy), (gx - 26, gy)])
    d.dot(gx - 72, gy); d.Rv(gx - 72, gy, "", "", 46); d.text(gx - 58, gy + 20, "RL " + Rleak, 11, bold=True); d.gnd(gx - 72, gy + 46)
    d.line([(gx - 72, gy), (gx - 112, gy)]); d.Ch(gx - 152, gy, "Cc", Cc, 40)
    d.text(gx - 156, gy - 6, inlab, 11, "end", bold=True)
    return (x + 110, y + 50)
