#include "../AmpCore.h"
#include <cstdio>
#include <vector>
#include <complex>
#include <chrono>
using namespace amp;

static double fsOS = 96000.0;

// goertzel-ish single-bin DFT
static std::complex<double> bin (const std::vector<double>& x, double f, size_t start)
{
    std::complex<double> acc = 0; size_t n = x.size() - start;
    for (size_t i = 0; i < n; ++i) acc += x[start + i] * std::polar (1.0, -2 * kPi * f * (double) i / fsOS);
    return acc * (2.0 / (double) n);
}

struct Result { double fund, thd, peak, rms; };
static Result runSine (AmpEngine& a, double amp_in, double f = 1000.0, double secs = 0.6)
{
    a.reset();
    size_t N = (size_t) (secs * fsOS); std::vector<double> y (N);
    // integer number of cycles in analysis window
    for (size_t i = 0; i < N; ++i)
    {
        double ramp = std::min (1.0, (double) i / (0.02 * fsOS));
        y[i] = a.process (ramp * amp_in * std::sin (2 * kPi * f * (double) i / fsOS));
    }
    size_t start = N / 2; size_t nn = N - start;
    size_t cyc = (size_t) std::floor ((double) nn * f / fsOS); nn = (size_t) ((double) cyc * fsOS / f);
    std::vector<double> w (y.begin() + (long) start, y.begin() + (long) (start + nn));
    double s1 = std::abs (bin (w, f, 0)), h = 0;
    for (int k = 2; k <= 10; ++k) { double m = std::abs (bin (w, f * k, 0)); h += m * m; }
    double pk = 0, rms = 0; for (double v : w) { pk = std::max (pk, std::fabs (v)); rms += v * v; }
    return { s1, std::sqrt (h) / std::max (s1, 1e-12), pk, std::sqrt (rms / (double) w.size()) };
}

int main()
{
    // ---- tubes
    double ip, gm, gp, ig2;
    kTube12AX7.eval (-2, 250, ip, gm, gp); printf ("12AX7 Vp250 Vg-2 : %.2f mA  gm %.2f mA/V\n", ip * 1e3, gm * 1e3);
    kTube6L6GC.eval (-14, 250, 250, ip, ig2, gp); printf ("6L6GC 250/250/-14: %.1f mA (ds 72)  Ig2 %.1f mA (ds 5)\n", ip * 1e3, ig2 * 1e3);
    kTube6L6GC.eval (0, 250, 250, ip, ig2, gp);   printf ("6L6GC 250/250/0  : %.1f mA  (ds curves ~ 190-230)\n", ip * 1e3);
    kTube6V6.eval (-12.5, 250, 250, ip, ig2, gp); printf ("6V6   250/250/-12.5: %.1f mA (ds 45)  Ig2 %.1f mA (ds 4.5)\n", ip * 1e3, ig2 * 1e3);

    // ---- tone stack
    for (int tcase = 0; tcase < 2; ++tcase)
    {
        ToneStack ts; double t = tcase ? 0.5 : 1.0, m = tcase ? 0.5 : 1.0, l = tcase ? 0.18 : 1.0;
        ts.setControls (t, m, l, fsOS);
        printf ("Fender stack  T%.1f M%.1f B%.2f :", t, m, l);
        for (double f : { 50., 100., 200., 400., 800., 1600., 3200., 6400., 12000. })
        {
            std::complex<double> z = std::polar (1.0, -2 * kPi * f / fsOS), zz = z * z, zzz = zz * z;
            auto H = (ts.b[0] + ts.b[1] * z + ts.b[2] * zz + ts.b[3] * zzz) / (1.0 + ts.a[1] * z + ts.a[2] * zz + ts.a[3] * zzz);
            printf (" %5.1f", 20 * std::log10 (std::abs (H)));
        }
        printf ("  dB @ 50..12k\n");
    }

    struct Case { const char* name; Voicing v; double Zref; } cases[] = {
        { "BlackfaceTwin", Voicing::BlackfaceTwin, 0 }, { "Blackface22", Voicing::Blackface22, 0 }, { "TwoRock100", Voicing::TwoRock100, 0 } };
    for (auto& cs : cases)
    {
        AmpEngine a; a.configure (cs.v); a.prepare (fsOS);
        printf ("\n=== %s  bias -%.1f V  idle/tube %.1f mA  Zpp %.0f  outRef %.0f V\n", cs.name, a.power.Vbias, a.power.idleI / (2.0 * a.power.parallel) * 1e3, a.power.Zpp, a.outRef);
        printf ("stage idle: V1 Vp=%.0f Ip=%.2fmA | V2 Vp=%.0f Ip=%.2fmA | V3 Vp=%.0f\n", a.s1.vpIdle, a.s1.ip * 1e3, a.s2.vpIdle, a.s2.ip * 1e3, a.s3.vpIdle);
        Controls c; c.treble = 0.6; c.middle = 0.5; c.bass = 0.5; c.reverb = 0.0; c.master = 1.0; c.sag = 0.5; c.presence = 0.5;
        for (double vol : { 0.3, 0.5, 0.7, 1.0 })
        {
            c.volume = vol; a.setControls (c);
            printf (" vol %.1f: ", vol);
            for (double inDb : { -30., -24., -18., -12., -6., 0. })
            {
                double A = std::pow (10.0, inDb / 20.0);
                auto r = runSine (a, A);
                printf (" [%+3.0fdBFS in: out %6.1f dBFS THD %5.1f%%]", inDb, 20 * std::log10 (r.fund + 1e-9), 100 * r.thd);
            }
            printf ("\n");
        }
    }
    // ---- stability / timing / reverb
    {
        AmpEngine a; a.configure (Voicing::BlackfaceTwin); a.prepare (fsOS);
        Controls c; c.volume = 1; c.reverb = 0.6; c.vibDepth = 0.5; c.bright = true; a.setControls (c);
        size_t N = (size_t) (4 * fsOS); bool bad = false; double mx = 0; auto t0 = std::chrono::steady_clock::now();
        for (size_t i = 0; i < N; ++i) { double in = (i < fsOS * 0.1) ? 1.0 * std::sin (2 * kPi * 110 * (double) i / fsOS) : 0.0; double o = a.process (in); if (!std::isfinite (o)) bad = true; mx = std::max (mx, std::fabs (o)); }
        double dt = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        printf ("\nstability (max-drive burst + reverb tail): finite=%s peak=%.2f   speed: %.2fx realtime-cost at 96k (cpu = %.0f%% of one core)\n", bad ? "NO" : "yes", mx, dt / 4.0, 100 * dt / 4.0);
    }
    return 0;
}
