// AmpCore.h  -  circuit-level tube amp DSP (no JUCE dependency, header only)
//
// Every nonlinear stage is solved per sample with Newton-Raphson against the
// actual node equations (Koren tube models, trapezoidal companion models for
// every capacitor / inductor), rather than waveshaper curves.  Run it
// oversampled (2x or 4x) - the JUCE wrapper does that.
//
//   Preamp triodes : Koren triode + grid conduction + coupling cap/grid leak
//                    (blocking distortion) + Miller capacitance + cathode RC
//   Tone stack     : exact TMB transfer function (Yeh & Smith), bilinear
//   Phase inverter : long-tail pair (tail-current limited)
//   Power amp      : push-pull Koren beam tetrodes, fixed bias, solved against
//                    the output transformer (magnetising L) and a real
//                    speaker-impedance curve (Re, Le, cone resonance)
//   Supply         : rectifier/reservoir sag, tracked by preamp B+
//   NFB            : taken from the speaker node, with presence shaping
//   Spring reverb  : dispersive all-pass chains in feedback loops
//   Tremolo        : optical (lamp/LDR-like) amplitude modulation
#pragma once
#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include <cstring>

namespace amp
{
constexpr double kPi = 3.14159265358979323846;

inline double clampd (double x, double lo, double hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline double softplus (double x) { if (x > 30.0) return x; if (x < -30.0) return std::exp (x); return std::log1p (std::exp (x)); }
inline double sigmoid (double x) { return 1.0 / (1.0 + std::exp (-x)); }
inline double sanitize (double x) { return std::isfinite (x) ? x : 0.0; }

//==============================================================================
//  Tube models (Koren)
//==============================================================================
struct TriodeModel
{
    double mu, ex, kg1, kp, kvb;

    inline void eval (double vgk, double vpk, double& ip, double& gm, double& gp) const
    {
        if (vpk < 0.0) vpk = 0.0;
        const double s  = std::sqrt (kvb + vpk * vpk);
        const double z  = kp * (1.0 / mu + vgk / s);
        const double sp = softplus (z), sg = sigmoid (z);
        const double e1 = vpk / kp * sp;
        if (e1 <= 1.0e-12) { ip = gm = gp = 0.0; return; }
        const double pw  = std::pow (e1, ex);
        ip = pw / kg1;
        const double dip = ex * pw / e1 / kg1;
        gm = dip * vpk * sg / s;
        gp = dip * (sp / kp - vpk * vpk * vgk * sg / (s * s * s));
    }
};

struct PentodeModel   // Koren beam-tetrode form
{
    double mu, ex, kg1, kg2, kp, kvb;

    inline void eval (double vgk, double vpk, double vg2k, double& ip, double& ig2, double& gp) const
    {
        if (vpk < 0.0) vpk = 0.0;
        const double z  = kp * (1.0 / mu + vgk / vg2k);
        const double e1 = vg2k / kp * softplus (z);
        if (e1 <= 1.0e-12) { ip = ig2 = gp = 0.0; return; }
        const double pw = std::pow (e1, ex);
        const double q  = vpk / kvb;
        ip  = pw / kg1 * std::atan (q);
        ig2 = pw / kg2;
        gp  = pw / kg1 / kvb / (1.0 + q * q);
    }
};

// constants tuned to datasheet bias points (see TubeTest in the harness)
inline constexpr TriodeModel  kTube12AX7 { 100.0, 1.40, 1060.0, 600.0, 300.0 };
inline constexpr TriodeModel  kTube12AT7 {  60.0, 1.35,  460.0, 300.0, 300.0 };
inline constexpr PentodeModel kTube6L6GC {   8.7, 1.35,  820.0, 7700.0, 48.0, 12.0 };
inline constexpr PentodeModel kTube6V6   {  11.0, 1.35, 1010.0, 8800.0, 60.0, 14.0 };
inline constexpr PentodeModel kTubeEL34  {  11.0, 1.35,  650.0, 6500.0, 60.0, 16.0 };

//==============================================================================
//  Small filters
//==============================================================================
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline double process (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
    void set (double B0, double B1, double B2, double A0, double A1, double A2) { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }
    void lowpass  (double fs, double f, double q) { const double w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q); set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al); }
    void highpass (double fs, double f, double q) { const double w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q); set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al); }
    void peak (double fs, double f, double q, double dB)
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void highshelf (double fs, double f, double dB, double slope = 0.8)
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2 * kPi * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / 2 * std::sqrt ((A + 1 / A) * (1 / slope - 1) + 2), tA = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + tA), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - tA),
             (A + 1) - (A - 1) * c + tA, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - tA);
    }
    void lowshelf (double fs, double f, double dB, double slope = 0.8)
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2 * kPi * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / 2 * std::sqrt ((A + 1 / A) * (1 / slope - 1) + 2), tA = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + tA), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - tA),
             (A + 1) + (A - 1) * c + tA, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - tA);
    }
};

struct OnePole   // TPT one pole, LP and HP outputs
{
    double g = 0, s = 0, lp = 0, hp = 0;
    void setCutoff (double fs, double f) { const double w = std::tan (kPi * f / fs); g = w / (1.0 + w); }
    inline double lowpass (double x) { const double v = (x - s) * g; lp = v + s; s = lp + v; hp = x - lp; return lp; }
    void reset() { s = lp = hp = 0; }
};

//==============================================================================
//  Grid circuit : source -> Cc -> stopper -> grid (Miller C, leak, conduction)
//==============================================================================
struct GridCircuit
{
    double T = 1.0 / 96000.0;
    double Cc = 22e-9, Rl = 1e6, Rg = 68e3, Rs = 30e3, Cm = 100e-12, Vleak = 0.0;
    double Rgrid = 2500.0, Vgo = 0.2, Vsc = 0.2;       // grid-conduction diode (softplus knee)
    double vC = 0, iPrev = 0, vgPrev = 0, imPrev = 0, vg = 0;

    void reset() { vC = iPrev = vgPrev = imPrev = 0; vg = Vleak; }

    inline double solve (double vin, double vk)
    {
        const double kC  = T / (2.0 * Cc);
        const double vth = vin - vC - kC * iPrev;
        const double Rth = Rs + Rg + kC;
        const double Gm  = 2.0 * Cm / T, Hm = Gm * vgPrev + imPrev;
        double v = vg;
        for (int k = 0; k < 5; ++k)
        {
            const double x  = (v - vk - Vgo) / Vsc;
            const double ig = Vsc / Rgrid * softplus (x);
            const double gg = sigmoid (x) / Rgrid;
            const double f  = (vth - v) / Rth - ig - (v - Vleak) / Rl - Gm * v + Hm;
            const double df = -1.0 / Rth - gg - 1.0 / Rl - Gm;
            double dv = -f / df;
            dv = clampd (dv, -20.0, 20.0);
            v += dv;
            if (std::fabs (dv) < 1e-7) break;
        }
        vg = v;
        const double i = (vth - v) / Rth;
        vC += kC * (i + iPrev);
        iPrev = i;
        imPrev = Gm * (v - vgPrev) - imPrev;
        vgPrev = v;
        return v;
    }
};

//==============================================================================
//  Common-cathode triode gain stage
//==============================================================================
struct TriodeStage
{
    TriodeModel tube = kTube12AX7;
    double T = 1.0 / 96000.0;
    double Bplus = 250.0, Rp = 100e3, Rk = 1500.0, Ck = 25e-6;
    GridCircuit grid;
    double vk = 0, ick = 0, ip = 0, vp = 0, vpIdle = 0;

    double saved[9] = { 0 };

    void setup (double fs)
    {
        T = 1.0 / fs; grid.T = T;
        // DC operating point by bisection:  I = f(Vgk = -I*Rk, Vpk = B+ - I*(Rp+Rk))
        double lo = 0.0, hi = 0.02;
        for (int i = 0; i < 80; ++i)
        {
            const double mid = 0.5 * (lo + hi); double ipc, gm, gp;
            tube.eval (-mid * Rk, Bplus - mid * (Rp + Rk), ipc, gm, gp);
            if (ipc > mid) lo = mid; else hi = mid;
        }
        ip = 0.5 * (lo + hi); vk = ip * Rk; ick = 0.0; vp = Bplus - Rp * ip; vpIdle = vp;
        grid.reset(); grid.vg = 0.0;
    }
    void saveState()    { saved[0]=grid.vC; saved[1]=grid.iPrev; saved[2]=grid.vgPrev; saved[3]=grid.imPrev; saved[4]=grid.vg; saved[5]=vk; saved[6]=ick; saved[7]=ip; saved[8]=vp; }
    void restoreState() { grid.vC=saved[0]; grid.iPrev=saved[1]; grid.vgPrev=saved[2]; grid.imPrev=saved[3]; grid.vg=saved[4]; vk=saved[5]; ick=saved[6]; ip=saved[7]; vp=saved[8]; }

    inline double process (double vin)
    {
        const double Gck = 2.0 * Ck / T, Gk = 1.0 / Rk + Gck, Hk = Gck * vk + ick;
        const double vg = grid.solve (vin, vk);
        double I = ip;
        for (int k = 0; k < 6; ++k)
        {
            const double vkn = (I + Hk) / Gk;
            double ipc, gm, gp;
            tube.eval (vg - vkn, Bplus - Rp * I - vkn, ipc, gm, gp);
            const double f  = I - ipc;
            const double df = 1.0 + gm / Gk + gp * (Rp + 1.0 / Gk);
            double dI = -f / df;
            dI = clampd (dI, -0.01, 0.01);
            I += dI;
            if (I < 0.0) I = 0.0;
            if (std::fabs (dI) < 1e-10) break;
        }
        ip = I;
        const double vkn = (I + Hk) / Gk;
        ick = Gck * (vkn - vk) - ick;
        vk  = vkn;
        vp  = Bplus - Rp * I;
        return vp - vpIdle;
    }

    void settle (int n = 8000)
    {
        for (int i = 0; i < n; ++i) process (0.0);
        vpIdle = vp; saveState();
    }
};

//==============================================================================
//  Tone stack: Yeh & Smith general TMB transfer function
//==============================================================================
struct ToneStack
{
    double R1 = 250e3, R2 = 250e3, R3 = 10e3, R4 = 100e3, C1 = 250e-12, C2 = 100e-9, C3 = 47e-9;
    double b[4] = { 0, 0, 0, 0 }, a[4] = { 1, 0, 0, 0 }, s[3] = { 0, 0, 0 };
    double fs = 96000;

    static void polyMul (double* p, int& n, double c0, double c1)   // p *= (c0 + c1 z)
    {
        double r[5] = { 0, 0, 0, 0, 0 };
        for (int i = 0; i < n; ++i) { r[i] += p[i] * c0; r[i + 1] += p[i] * c1; }
        ++n; for (int i = 0; i < n; ++i) p[i] = r[i];
    }

    void setControls (double t, double m, double l, double sampleRate)   // t,m linear 0..1  l already tapered
    {
        fs = sampleRate;
        t = clampd (t, 0.001, 0.999); m = clampd (m, 0.001, 0.999); l = clampd (l, 0.001, 0.999);
        // Exact coefficients for the standard Fender TMB stack, derived symbolically (sympy, MNA) from this topology:
        //   C1: in->P   R1(treble pot, wiper=out): P-Q   R4(slope): in->Q   C2: Q->N4   l*R2(bass): N4->N5
        //   m*R3(mid): N5->gnd   C3: Q->N5
        const double bs1 = C1*R1 + C1*R4 + C2*R2*l + C2*R3*m + C3*R3*m;
        const double bs2 = C1*C2*R1*R2*l + C1*C2*R1*R3*m + C1*C2*R1*R4*t + C1*C2*R2*R4*l + C1*C2*R3*R4*m + C1*C3*R1*R3*m
                         + C1*C3*R1*R4*t + C1*C3*R3*R4*m + C2*C3*R2*R3*l*m;
        const double bs3 = C1*C2*C3*R2*l*(R1*R3*m + R1*R4*t + R3*R4*m);
        const double as1 = C1*R1 + C1*R4 + C2*R2*l + C2*R3*m + C2*R4 + C3*R3*m + C3*R4;
        const double as2 = C1*C2*R1*R2*l + C1*C2*R1*R3*m + C1*C2*R1*R4 + C1*C2*R2*R4*l + C1*C2*R3*R4*m + C1*C3*R1*R3*m
                         + C1*C3*R1*R4 + C1*C3*R3*R4*m + C2*C3*R2*R3*l*m + C2*C3*R2*R4*l;
        const double as3 = C1*C2*C3*R2*l*(R1*R3*m + R1*R4 + R3*R4*m);

        const double bn[4] = { 1.0, bs1, bs2, bs3 }, an[4] = { 1.0, as1, as2, as3 };
        const double K = 2.0 * fs;
        double B[4] = { 0, 0, 0, 0 }, A[4] = { 0, 0, 0, 0 };
        for (int k = 0; k < 4; ++k)
        {
            double p[5] = { 1, 0, 0, 0, 0 }; int n = 1;
            for (int i = 0; i < k; ++i) polyMul (p, n, 1.0, -1.0);
            for (int i = 0; i < 3 - k; ++i) polyMul (p, n, 1.0, 1.0);
            const double Kk = std::pow (K, k);
            for (int i = 0; i < 4; ++i) { B[i] += bn[k] * Kk * p[i]; A[i] += an[k] * Kk * p[i]; }
        }
        for (int i = 0; i < 4; ++i) { b[i] = B[i] / A[0]; a[i] = A[i] / A[0]; }
    }

    inline double process (double x)
    {
        const double y = b[0] * x + s[0];
        s[0] = b[1] * x - a[1] * y + s[1];
        s[1] = b[2] * x - a[2] * y + s[2];
        s[2] = b[3] * x - a[3] * y;
        return y;
    }
    void reset() { s[0] = s[1] = s[2] = 0; }
};

//==============================================================================
//  Push-pull power stage with output transformer + speaker load
//==============================================================================
struct PowerStage
{
    PentodeModel tube = kTube6L6GC;
    double T = 1.0 / 96000.0;
    int    parallel = 2;            // tubes per side
    double Vb0 = 440.0, Vscreen0 = 430.0, Vbias = 52.0, idleTarget = 0.035;
    double Zpp = 2000.0, Rnom = 4.0, Lm = 12.0, Cw = 3.0e-9;   // Cw: primary winding capacitance
    double Re = 0.8, Le = 0.00012, Rres = 3.0, fres = 100.0, Qres = 3.5;   // per-ohm-of-Rnom values, set in prepare()
    GridCircuit g1, g2;

    // reactive state
    double iLe = 0, vLeprev = 0, vm = 0, iC = 0, iLb = 0, iLm = 0, vprev = 0, icw = 0;
    // derived
    double Reff, LeEff, Rb, Reff_res, Cm_, Lm_res, kappa;
    double Vb = 440.0, Vg2 = 430.0, ipk1 = 0, ipk2 = 0, supplyI = 0, v = 0, idleI = 0, accel = 0;

    void prepare (double fs)
    {
        T = 1.0 / fs;
        kappa = (Zpp / 4.0) / Rnom;
        Reff = kappa * Re * Rnom; LeEff = kappa * Le * Rnom; Rb = 3.0 * Reff;   // eddy-loss resistor caps the HF impedance rise
        Reff_res = kappa * Rres * Rnom;
        const double w = 2.0 * kPi * fres;
        Cm_ = Qres / (w * Reff_res) ; Lm_res = Reff_res / (w * Qres);
        // pick bias for requested idle current
        double lo = 5.0, hi = 120.0;
        for (int i = 0; i < 60; ++i)
        {
            const double mid = 0.5 * (lo + hi); double ip, ig2, gp;
            tube.eval (-mid, Vb0, Vscreen0, ip, ig2, gp);
            if (ip * parallel > idleTarget * parallel) lo = mid; else hi = mid;
        }
        Vbias = 0.5 * (lo + hi);
        for (auto* g : { &g1, &g2 }) { g->T = T; g->Cc = 100e-9; g->Rl = 220e3; g->Rg = 1500.0; g->Rs = 40e3; g->Cm = 60e-12; g->Vleak = -Vbias; g->Vgo = 0.0; g->Vsc = 0.25; g->Rgrid = 600.0; }
        { double ip, ig2, gp; tube.eval (-Vbias, Vb0, Vscreen0, ip, ig2, gp); idleI = 2.0 * parallel * (ip + ig2); }
        Vb = Vb0; Vg2 = Vscreen0;
        reset();
    }

    void reset()
    {
        g1.reset(); g2.reset(); g1.vg = g2.vg = -Vbias; g1.vC = g2.vC = Vbias; g1.vgPrev = g2.vgPrev = -Vbias;
        iLe = vLeprev = vm = iC = iLb = iLm = vprev = icw = 0; v = 0; accel = 0; ipk1 = ipk2 = idleI * 0.25;
    }

    // e1,e2 : drive voltages from the phase inverter.  returns primary half-winding voltage (+ve when tube 1 pulls harder)
    inline double process (double e1, double e2)
    {
        const double vg1 = g1.solve (e1, 0.0), vg2 = g2.solve (e2, 0.0);
        // reactive companion coefficients
        const double GC = 2.0 * Cm_ / T, GL = T / (2.0 * Lm_res), den = GC + 1.0 / Reff_res + GL;
        const double a  = 1.0 / den, bq = ((GC - GL) * vm + iC - iLb) / den;
        const double gL = T / (2.0 * LeEff), a2 = 1.0 / (1.0 / Rb + gL), b2 = -a2 * (iLe + gL * vLeprev);
        const double z0 = Reff + a2 + a;
        const double hist = bq + b2;
        const double gm_ = T / (2.0 * Lm);
        const double Gw  = 2.0 * Cw / T;
        const double scale = (double) parallel;

        double vv = v;
        double ig2s = 0, ip1 = 0, ip2 = 0;
        for (int k = 0; k < 8; ++k)
        {
            double i1, s1, gp1, i2, s2, gp2;
            tube.eval (vg1, Vb - vv, Vg2, i1, s1, gp1);
            tube.eval (vg2, Vb + vv, Vg2, i2, s2, gp2);
            i1 *= scale; i2 *= scale; gp1 *= scale; gp2 *= scale;
            const double id = i1 - i2;
            const double f  = vv * (1.0 + z0 * (gm_ + Gw)) - z0 * id + z0 * (iLm + (gm_ - Gw) * vprev - icw) - hist;
            const double df = (1.0 + z0 * (gm_ + Gw)) + z0 * (gp1 + gp2);
            double dv = -f / df;
            dv = clampd (dv, -150.0, 150.0);
            vv += dv;
            ip1 = i1; ip2 = i2; ig2s = (s1 + s2) * scale;
            if (std::fabs (dv) < 1e-6) break;
        }
        {   // final consistent currents
            double i1, s1, gp1, i2, s2, gp2;
            tube.eval (vg1, Vb - vv, Vg2, i1, s1, gp1);
            tube.eval (vg2, Vb + vv, Vg2, i2, s2, gp2);
            ip1 = i1 * scale; ip2 = i2 * scale; ig2s = (s1 + s2) * scale;
        }
        const double id = ip1 - ip2;
        const double iLm_new = iLm + gm_ * (vv + vprev);
        const double icw_new = Gw * (vv - vprev) - icw;
        const double iL = id - iLm_new - icw_new;
        // update load states
        const double vLe = a2 * iL + b2;
        const double vmn = a * iL + bq;
        const double iCn = GC * (vmn - vm) - iC;
        const double iLbn = iLb + GL * (vmn + vm);
        accel = (vmn - vm) / T;                 // cone acceleration ~ d(motional voltage)/dt : what the ear hears
        iLe += gL * (vLe + vLeprev); vLeprev = vLe; vm = vmn; iC = iCn; iLb = iLbn; iLm = iLm_new; icw = icw_new; vprev = vv;
        v = vv; ipk1 = ip1; ipk2 = ip2;
        supplyI = ip1 + ip2 + ig2s;
        return vv;
    }
};

//==============================================================================
//  Spring reverb (two dispersive springs)
//==============================================================================
struct SpringReverb
{
    struct Spring
    {
        std::vector<double> buf; int w = 0, len = 1;
        std::vector<double> apX, apY; int stages = 48; double apA = -0.62;
        OnePole lp; double fb = 0.8, fb0 = 0.8;
        double process (double x, double drive)
        {
            const double d = buf[(size_t) w];                 // oldest sample
            double in = x + fb * lp.lowpass (d);
            if (drive > 0) in = std::tanh (in * drive) / drive;
            // dispersive all-pass chain
            double sig = in;
            for (int i = 0; i < stages; ++i)
            {
                const double y = apA * sig + apX[(size_t) i] - apA * apY[(size_t) i];
                apX[(size_t) i] = sig; apY[(size_t) i] = y; sig = y;
            }
            buf[(size_t) w] = sig; if (++w >= len) w = 0;
            return d;
        }
    };
    Spring s1, s2; Biquad hpIn, lpOut; double fs = 96000; double drive = 1.0;

    void prepare (double sampleRate, double rt60 = 2.4)
    {
        fs = sampleRate; const double k = fs / 48000.0;
        const int l1 = (int) (1550 * k), l2 = (int) (1810 * k);
        for (auto* sp : { &s1, &s2 })
        {
            const int L = (sp == &s1) ? l1 : l2;
            sp->buf.assign ((size_t) L, 0.0); sp->len = L; sp->w = 0;
            sp->stages = 48; sp->apX.assign (48, 0.0); sp->apY.assign (48, 0.0);
            sp->apA = -0.62;
            sp->lp.setCutoff (fs, 3800.0); sp->lp.reset();
            // delay loop length ~ L samples (+ dispersion), gain for requested RT60
            const double loopSec = (double) L / fs + 0.004;
            sp->fb0 = sp->fb = std::pow (10.0, -3.0 * loopSec / rt60);
        }
        hpIn.highpass (fs, 110.0, 0.7); lpOut.lowpass (fs, 4200.0, 0.7);
    }
    void setDecayScale (double sc) { s1.fb = std::pow (s1.fb0, 1.0 / sc); s2.fb = std::pow (s2.fb0, 1.0 / sc); }
    inline double process (double x)
    {
        const double in = hpIn.process (x);
        return lpOut.process (0.5 * (s1.process (in, drive) + s2.process (in * 0.85, drive)));
    }
    void reset() { for (auto* sp : { &s1, &s2 }) { std::fill (sp->buf.begin(), sp->buf.end(), 0.0); std::fill (sp->apX.begin(), sp->apX.end(), 0.0); std::fill (sp->apY.begin(), sp->apY.end(), 0.0); sp->lp.reset(); } hpIn.reset(); lpOut.reset(); }
};

//==============================================================================
//  Optical tremolo ("vibrato" on Fender amps)
//==============================================================================
struct OpticalTremolo
{
    double fs = 96000, phase = 0, rate = 5, depth = 0, ldr = 0;
    void prepare (double sampleRate) { fs = sampleRate; phase = 0; ldr = 0; }
    inline double gain()
    {
        phase += rate / fs; if (phase >= 1.0) phase -= 1.0;
        const double lfo = 0.5 + 0.5 * std::sin (2.0 * kPi * phase);       // lamp drive 0..1
        const double a = lfo > ldr ? 1.0 - std::exp (-1.0 / (0.0035 * fs)) : 1.0 - std::exp (-1.0 / (0.020 * fs));  // fast attack, slower decay
        ldr += a * (lfo - ldr);
        return 1.0 - depth * ldr;
    }
};

//==============================================================================
//  The amplifier
//==============================================================================
enum class Voicing { Blackface22, BlackfaceTwin, TwoRock100, TwoRockCRS };

struct Controls
{
    double volume = 0.5, treble = 0.5, middle = 0.5, bass = 0.5, reverb = 0.2, master = 1.0, presence = 0.5;
    double vibSpeed = 0.5, vibDepth = 0.0, sag = 0.5;
    bool   bright = false;
    // Two Rock Classic Reverb Signature panel
    double fetGain = 0.5, gain = 0.5, send = 0.4, ret = 0.3, contour = 0.5;
    bool   fetOn = true, midSw = false, deep = false, eq2 = true, bypass = false;
    int    gainStructure = 2;     // 0 = down, 1 = middle, 2 = up
};

class AmpEngine
{
public:
    Voicing voicing = Voicing::BlackfaceTwin;
    double fs = 96000;

    TriodeStage    s1, s2, s3;
    ToneStack      stack;
    PowerStage     power;
    SpringReverb   spring;
    OpticalTremolo trem;
    Controls       c;

    // calibration (set by configure())
    double inputScale = 2.0;                 // volts at digital full scale
    double piPad = 0.05, piGain = 30.0, piSwing = 62.0, piMismatch = 0.94;
    double accRef = 1.0, nfbAmount = 0.006, nfbReductionDb = 6.0, nfbCornerHz = 3200.0, presenceDepth = 0.0, outRef = 300.0;
    double reverbDrive = 4.0, reverbReturn = 1.0;
    double preSagTrack = 0.35, sagR = 120.0;
    double presenceBase = 0.0;
    double Vb0 = 440.0, preB1 = 250.0, preB2 = 250.0, preB3 = 250.0;
    bool   masterBeforePI = false, tubeRect = false;

    // runtime state
    OnePole hp1, hp2, hp3, fbSplit, piLP, fbLP;
    Biquad  bright1, bright2;
    double  volPos = 0.5, fbPrev = 0.0;
    // Classic Reverb Signature extras
    OnePole fetHP; Biquad gsEq, eqShape1, eqShape2, midSw, deepSw1, deepSw2, contLo, contHi, gainShelf;
    double  fetGain = 1.0, fetLimit = 3.0, gsGain = 1.0, eqPad = 1.0, sendGain = 0.06, retGain = 0.0;
    double  ratedPower = 100.0; bool halfPower = false;
    double  outMakeup = 1.0;        // loudness compensation (Classic Reverb Signature)
    double  idleSupply = 0.2, sagLP = 0.2, sagAtt = 0.0, sagRel = 0.0, railDrop = 0.0, railCoef = 0.0;

    void configure (Voicing v)
    {
        voicing = v;
        s1.tube = s2.tube = s3.tube = kTube12AX7;
        s1.Rp = s2.Rp = s3.Rp = 100e3; s1.Rk = s2.Rk = s3.Rk = 1500.0; s1.Ck = s2.Ck = s3.Ck = 25e-6;
        // input stage grid: guitar source, 68k stopper, 1M leak
        s1.grid.Cc = 100e-9; s1.grid.Rl = 1e6; s1.grid.Rg = 68e3; s1.grid.Rs = 30e3; s1.grid.Cm = 100e-12;
        s2.grid.Cc = 22e-9;  s2.grid.Rl = 1e6; s2.grid.Rg = 10e3; s2.grid.Rs = 50e3; s2.grid.Cm = 90e-12;
        s3.grid.Cc = 22e-9;  s3.grid.Rl = 1e6; s3.grid.Rg = 10e3; s3.grid.Rs = 50e3; s3.grid.Cm = 90e-12;
        power.parallel = 2;
        switch (v)
        {
            case Voicing::BlackfaceTwin:
                // AB763/AA270 family: 4x 6L6GC, solid-state rectifier, 820R NFB
                Vb0 = 440; power.Vb0 = 440; power.Vscreen0 = 428; power.tube = kTube6L6GC; power.parallel = 2;
                power.Zpp = 2000; power.Rnom = 4.0; power.Lm = 14.0; power.idleTarget = 0.032;
                preB1 = 285; preB2 = 270; preB3 = 270;
                stack.R1 = 250e3; stack.R2 = 250e3; stack.R3 = 10e3; stack.R4 = 100e3;
                stack.C1 = 250e-12; stack.C2 = 100e-9; stack.C3 = 47e-9;
                piPad = 0.040; nfbReductionDb = 6.0; presenceDepth = 0.0; sagR = 70.0; tubeRect = false;
                ratedPower = 85.0; break;
            case Voicing::Blackface22:
                // Deluxe Reverb style: 2x 6V6, 8 ohm, 820R NFB
                Vb0 = 420; power.Vb0 = 420; power.Vscreen0 = 410; power.tube = kTube6V6; power.parallel = 1;
                power.Zpp = 8000; power.Rnom = 8.0; power.Lm = 25.0; power.idleTarget = 0.030;
                preB1 = 270; preB2 = 255; preB3 = 255;
                stack.R1 = 250e3; stack.R2 = 250e3; stack.R3 = 6.8e3; stack.R4 = 100e3;
                stack.C1 = 250e-12; stack.C2 = 100e-9; stack.C3 = 47e-9;
                piPad = 0.050; nfbReductionDb = 6.0; presenceDepth = 0.0; sagR = 150.0; tubeRect = false;
                ratedPower = 22.0; break;
            case Voicing::TwoRock100:
                // reconstructed topology (no official schematic): 4x 6L6GC, tube rectifier, Dumble-ish stack
                Vb0 = 470; power.Vb0 = 470; power.Vscreen0 = 462; power.tube = kTube6L6GC; power.parallel = 2;
                power.Zpp = 2000; power.Rnom = 4.0; power.Lm = 16.0; power.idleTarget = 0.038;
                preB1 = 300; preB2 = 285; preB3 = 285;
                stack.R1 = 250e3; stack.R2 = 250e3; stack.R3 = 25e3; stack.R4 = 47e3;
                stack.C1 = 470e-12; stack.C2 = 47e-9; stack.C3 = 22e-9;
                s1.grid.Cc = 47e-9;
                piPad = 0.040; nfbReductionDb = 4.5; presenceDepth = 0.8; sagR = 220.0; tubeRect = true; ratedPower = 100.0;
                break;
            case Voicing::TwoRockCRS:
                // Classic Reverb Signature 100 W: FET front end -> 12AX7 V1 -> stack -> 12AX7 mix stage -> master -> 12AX7 PI -> 4x 6L6GC,
                // solid-state rectifier on the 100 W model (tube rectifier is 50 W only). Stack values reconstructed.
                Vb0 = 470; power.Vb0 = 470; power.Vscreen0 = 462; power.tube = kTube6L6GC; power.parallel = 2;
                power.Zpp = 2000; power.Rnom = 4.0; power.Lm = 16.0; power.idleTarget = 0.036;
                preB1 = 300; preB2 = 285; preB3 = 285;
                stack.R1 = 250e3; stack.R2 = 250e3; stack.R3 = 25e3; stack.R4 = 47e3;
                stack.C1 = 470e-12; stack.C2 = 47e-9; stack.C3 = 22e-9;
                s1.grid.Cc = 47e-9;
                piPad = 0.040; nfbReductionDb = 4.5; presenceDepth = 0.0; sagR = 110.0; tubeRect = false; ratedPower = 100.0;
                if (halfPower) { power.parallel = 1; power.Zpp *= 2.0; ratedPower = 50.0; power.idleTarget = 0.034; }
                break;
        }
        s1.Bplus = preB1; s2.Bplus = preB2; s3.Bplus = preB3;
    }

    // measure the open-loop gain of PI + power stage at 1 kHz and set the NFB fraction for the requested gain reduction
    void calibrateNFB()
    {
        OnePole lp; lp.setCutoff (fs, 22000.0);
        const double f = 1000.0, amp = 0.05; const int N = (int) (0.12 * fs);
        double accRe = 0, accIm = 0; int cnt = 0;
        const double vs = piSwing / piGain;
        for (int i = 0; i < N; ++i)
        {
            const double ph = 2.0 * kPi * f * (double) i / fs;
            double y = piSwing * std::tanh (amp * std::sin (ph) / vs); y = lp.lowpass (y);
            const double vh = power.process (y, -y * piMismatch);
            if (i > N / 2) { accRe += vh * std::sin (ph); accIm += vh * std::cos (ph); ++cnt; }
        }
        const double G = 2.0 * std::sqrt (accRe * accRe + accIm * accIm) / ((double) cnt * amp);
        const double reduction = std::pow (10.0, nfbReductionDb / 20.0);
        nfbAmount = (reduction - 1.0) / std::max (G, 1.0);
    }

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        power.prepare (fs);
        calibrateNFB();
        power.reset();
        for (auto* s : { &s1, &s2, &s3 }) { s->setup (fs); }
        s1.settle(); s2.settle(); s3.settle();
        spring.prepare (fs); spring.drive = reverbDrive;
        trem.prepare (fs);
        hp1.setCutoff (fs, 6.0); hp2.setCutoff (fs, 6.0); hp3.setCutoff (fs, 6.0); fetHP.setCutoff (fs, 30.0);
        fbSplit.setCutoff (fs, 2200.0); piLP.setCutoff (fs, 22000.0); fbLP.setCutoff (fs, nfbCornerHz);
        const double idleI = power.idleI + 0.012;
        idleSupply = idleI; sagLP = idleI;
        railCoef = 1.0 - std::exp (-1.0 / (0.25 * fs));   // preamp rails sit behind big RC filters
        sagAtt = 1.0 - std::exp (-1.0 / (0.004 * fs)); sagRel = 1.0 - std::exp (-1.0 / ((tubeRect ? 0.090 : 0.045) * fs));
        // output reference: full rated sine = 0.7 FS
        const double z4 = power.Zpp / 4.0;
        const double ratedW = ratedPower;
        outRef = std::sqrt (2.0 * z4 * ratedW) / 0.7;
        accRef = (std::sqrt (2.0 * ratedW / z4) / power.Cm_) / 0.7;
        if (halfPower) accRef *= 1.41421356;      // 50 W mode is 3 dB quieter than 100 W at the same drive
        reset();
        setControls (c);
    }

    void reset()
    {
        for (auto* s : { &s1, &s2, &s3 }) { s->restoreState(); }
        stack.reset(); power.reset(); spring.reset();
        hp1.reset(); hp2.reset(); hp3.reset(); fbSplit.reset(); piLP.reset(); fbLP.reset();
        bright1.reset(); bright2.reset(); fbPrev = 0; sagLP = idleSupply; railDrop = 0.0;
    }

    static double audioTaper (double x) { return (std::exp (3.0 * x) - 1.0) / (std::exp (3.0) - 1.0); }

    void setControls (const Controls& nc)
    {
        c = nc;
        const bool crs = (voicing == Voicing::TwoRockCRS);
        if (crs && c.bypass) stack.setControls (1.0, 1.0, audioTaper (1.0), fs);          // BYPASS = stack controls fully up
        else stack.setControls (c.treble, c.middle, audioTaper (c.bass), fs);
        volPos = clampd (audioTaper (crs ? c.gain : c.volume), 0.001, 1.0);
        // bright cap across top of the volume pot (120pF on 1M) - first order, exact for an unloaded pot
        const double Rpot = 1.0e6, Cb = 120e-12, Rtop = (1.0 - volPos) * Rpot, Rbot = volPos * Rpot;
        if (c.bright && voicing != Voicing::TwoRock100)
        {
            const double K = 2.0 * fs, k = Rbot * Rtop * Cb;
            const double n0 = Rbot + K * k, n1 = Rbot - K * k, d0 = (Rbot + Rtop) + K * k, d1 = (Rbot + Rtop) - K * k;
            bright1.b0 = n0 / d0; bright1.b1 = n1 / d0; bright1.b2 = 0; bright1.a1 = d1 / d0; bright1.a2 = 0;
        }
        else { bright1.b0 = volPos; bright1.b1 = 0; bright1.b2 = 0; bright1.a1 = 0; bright1.a2 = 0; }
        trem.rate = 1.5 + 9.0 * c.vibSpeed; trem.depth = 0.85 * c.vibDepth;
        (void) 0;
        if (crs)
        {
            // FET: below noon cuts gain AND bass, above noon boosts (+/-14 dB)
            fetGain = std::pow (10.0, (c.fetGain - 0.5) * 2.0 * 14.0 / 20.0);
            fetHP.setCutoff (fs, c.fetGain < 0.5 ? 30.0 + (0.5 - c.fetGain) * 2.0 * 220.0 : 30.0);
            // Gain structure: down = scooped blackface-like, middle = hottest, up = classic Two-Rock
            if (c.gainStructure == 0)      { gsGain = 0.8; gsEq.peak (fs, 600.0, 0.7, -4.0); }
            else if (c.gainStructure == 1) { gsGain = 1.9; gsEq.peak (fs, 1200.0, 0.8, 2.5); }
            else                           { gsGain = 1.0; gsEq.peak (fs, 800.0, 0.7, 1.0); }
            // EQ1: lower gain, more bass/mid body.  EQ2: tighter, more gain
            if (c.eq2) { eqPad = 1.0; eqShape1.lowshelf (fs, 180.0, 0.0); eqShape2.peak (fs, 900.0, 0.8, 0.0); }
            else       { eqPad = 0.63; eqShape1.lowshelf (fs, 180.0, 2.0); eqShape2.peak (fs, 900.0, 0.8, 1.5); }
            gainShelf.lowshelf (fs, 150.0, 4.0 * c.gain);                              // GAIN adds gain and bass
            midSw.peak (fs, 650.0, 0.7, c.midSw ? 4.5 : 0.0);
            deepSw1.lowshelf (fs, 110.0, c.deep ? 5.0 : 0.0); deepSw2.peak (fs, 280.0, 0.8, c.deep ? -2.0 : 0.0);
            const double t = (c.contour - 0.5) * 2.0;                                  // CCW: darker + more low end, CW: brighter + less low end
            contLo.lowshelf (fs, 250.0, -6.0 * t); contHi.highshelf (fs, 2200.0, 6.0 * t);
            sendGain = 0.02 * std::pow (10.0, c.send); spring.drive = 1.0 + 5.0 * c.send;
            spring.setDecayScale (0.7 + 0.6 * c.send);
            retGain = std::pow (c.ret, 1.5) * 14.0;
            // Level compensation: the real Master pot throws away up to 25 dB before the power stage; give most of it back
            // (80 %) plus a +4 dB base lift so typical DI levels land around -8 dBFS peak instead of -24.
            const double taperDb = 20.0 * std::log10 (std::max (audioTaper (c.master), 1.0e-4));
            outMakeup = std::pow (10.0, (0.8 * (-taperDb) + 4.0) / 20.0);
        }
        // Two Rock bright: input-stage treble lift
        if (voicing == Voicing::TwoRock100)
        {
            if (c.bright) bright2.highshelf (fs, 1800.0, 6.0); else { bright2 = Biquad(); }
        }
    }

    // x : input in "digital full scale" units; returns normalised speaker voltage (1.0 ~ full scale)
    double process (double x)
    {
        // supply sag: tracks smoothed current draw
        {
            const double a = power.supplyI + 0.012 > sagLP ? sagAtt : sagRel;
            sagLP += a * (power.supplyI + 0.012 - sagLP);
            const double drop = c.sag * sagR * (sagLP - idleSupply);
            const double vb = Vb0 - drop;
            power.Vb = vb; power.Vg2 = vb - (Vb0 - power.Vscreen0);
            railDrop += railCoef * ((Vb0 - vb) - railDrop);                   // slow: only sustained sag reaches the preamp
            const double rel = 1.0 - railDrop / Vb0 * preSagTrack * 3.0;
            s1.Bplus = preB1 * rel; s2.Bplus = preB2 * rel; s3.Bplus = preB3 * rel;
        }

        double v = x * inputScale;
        double sig;
        if (voicing == Voicing::TwoRockCRS)
        {
            if (c.fetOn)                                                             // FET front end (clean boost / cut)
            {
                fetHP.lowpass (v); const double f = fetHP.hp * fetGain;
                v = fetLimit * std::tanh (f / fetLimit + 0.08) - fetLimit * std::tanh (0.08);
            }
            v = gsEq.process (v * gsGain);                                           // gain-structure switch
            sig = s1.process (v); hp1.lowpass (sig); sig = hp1.hp;                   // V1 12AX7
            sig = bright1.process (sig);                                             // GAIN pot (+ bright)
            sig = gainShelf.process (sig);
            sig = eqShape2.process (eqShape1.process (sig)) * eqPad;                 // EQ1 / EQ2
            sig = stack.process (sig);                                               // treble / middle / bass (bypass = all up)
            sig = deepSw2.process (deepSw1.process (midSw.process (sig)));           // MID and DEEP switches
            sig = s2.process (sig); hp2.lowpass (sig); sig = hp2.hp;                 // mix / recovery 12AX7
            sig += spring.process (sig * sendGain) * retGain;                        // SEND / RETURN reverb
            sig = contHi.process (contLo.process (sig));                             // CONTOUR
        }
        else
        {
            if (voicing == Voicing::TwoRock100) v = bright2.process (v);             // input-stage bright (treble lift)
            sig = s1.process (v); hp1.lowpass (sig); sig = hp1.hp;                   // V1A
            sig = stack.process (sig);                                               // 3-band stack
            sig = bright1.process (sig);                                             // volume pot (+ bright cap on blackface)
            sig = s2.process (sig); hp2.lowpass (sig); sig = hp2.hp;                 // V1B
            const double send = sig * 0.08;
            sig += spring.process (send) * reverbReturn * (c.reverb * c.reverb * 14.0);
        }
        // optical tremolo (Fender "vibrato")
        if ((voicing == Voicing::Blackface22 || voicing == Voicing::BlackfaceTwin) && c.vibDepth > 0.001) sig *= trem.gain();
        // master volume before the phase inverter
        sig *= (voicing == Voicing::TwoRock100 || voicing == Voicing::TwoRockCRS) ? audioTaper (c.master) : 1.0;

        // negative feedback from the speaker node, presence shapes how much HF is fed back
        const double fbIn = fbLP.lowpass (fbPrev);
        const double fbLow = fbSplit.lowpass (fbIn), fbHighPart = fbSplit.hp;
        const double presence = presenceBase + presenceDepth * c.presence;
        const double fb = nfbAmount * (fbLow + (1.0 - presence) * fbHighPart);

        // long-tail-pair phase inverter
        double pin = sig * piPad - fb;
        const double vs = piSwing / piGain;
        double y = piSwing * std::tanh (pin / vs);
        y = piLP.lowpass (y);
        const double e1 = y, e2 = -y * piMismatch;
        const double vhalf = power.process (e1, e2);
        fbPrev = vhalf;
        return sanitize (power.accel / accRef * outMakeup);
    }
};

//==============================================================================
//  Modelled guitar cabinet (parametric approximation of a 2x12; use an IR for exact mic'd cabs)
//==============================================================================
struct CabSim
{
    Biquad f[7]; int n = 0;
    void prepare (double fs, int type)   // 0 = Jensen-style open back (Fender), 1 = Celestion-style (Two Rock)
    {
        n = 0;
        if (type == 0)
        {
            f[n++].highpass (fs, 78.0, 0.75);  f[n++].peak (fs, 115.0, 1.1, 3.0);  f[n++].peak (fs, 1500.0, 0.9, -2.5);
            f[n++].peak (fs, 3100.0, 1.4, 4.5); f[n++].lowpass (fs, 5600.0, 0.62); f[n++].lowpass (fs, 6200.0, 0.62);
        }
        else
        {
            f[n++].highpass (fs, 85.0, 0.75);  f[n++].peak (fs, 125.0, 1.0, 2.0);  f[n++].peak (fs, 800.0, 0.8, 1.5);
            f[n++].peak (fs, 2500.0, 1.5, 5.0); f[n++].peak (fs, 4200.0, 2.0, -3.0); f[n++].lowpass (fs, 5200.0, 0.62); f[n++].lowpass (fs, 5600.0, 0.62);
        }
    }
    inline double process (double x) { for (int i = 0; i < n; ++i) x = f[i].process (x); return x; }
    void reset() { for (auto& b : f) b.reset(); }
};
} // namespace amp
