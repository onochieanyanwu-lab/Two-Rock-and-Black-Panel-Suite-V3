#include "../AmpCore.h"
#include <cstdio>
#include <complex>
using namespace amp;
static double fs = 96000;
struct R { double lvl, thd; };
R run(AmpEngine& a, double dbin, double f=1000.0){
  a.reset(); int N=(int)(0.5*fs); std::vector<double> y(N); double A=std::pow(10,dbin/20);
  for(int i=0;i<N;i++){ double r=std::min(1.0,i/(0.02*fs)); y[i]=a.process(r*A*std::sin(2*kPi*f*i/fs)); }
  int st=N/2; int cyc=(int)((N-st)*f/fs); int n=(int)(cyc*fs/f);
  auto bin=[&](double ff){ std::complex<double> acc=0; for(int i=0;i<n;i++) acc+=y[st+i]*std::polar(1.0,-2*kPi*ff*i/fs); return std::abs(acc)*2/n; };
  double f1=bin(f),h=0; for(int k=2;k<=10;k++){ double m=bin(f*k); h+=m*m; }
  return {20*std::log10(f1+1e-12), 100*std::sqrt(h)/std::max(f1,1e-12)};
}
AmpEngine mk(bool half=false){ AmpEngine a; a.halfPower=half; a.configure(Voicing::TwoRockCRS); a.prepare(fs); return a; }
int main(){
  { AmpEngine a=mk(); Controls c; c.fetOn=false; c.reverb=0; c.ret=0; c.gain=0.5; c.master=0.6; a.setControls(c);
    printf("baseline (gain5 master6 EQ2 up): ");
    for(double d: {-36.,-30.,-24.,-18.,-12.,-6.}){ auto r=run(a,d); printf(" [%+.0f in: %.1f dBFS %.1f%%]",d,r.lvl,r.thd);} printf("\n"); }
  auto level=[&](const char* nm, Controls c, double din=-24, double f=1000){ AmpEngine a=mk(); a.setControls(c); auto r=run(a,din,f); printf("%-34s out %.1f dBFS THD %.1f%%\n",nm,r.lvl,r.thd); };
  Controls b; b.fetOn=false; b.ret=0; b.master=0.6;
  level("base",b);
  { Controls c=b; c.fetOn=true; c.fetGain=0.5; level("FET on, noon (unity)",c); }
  { Controls c=b; c.fetOn=true; c.fetGain=1.0; level("FET fully up (+14 dB)",c); }
  { Controls c=b; c.fetOn=true; c.fetGain=0.0; level("FET fully down (-14 dB, bass cut)",c); }
  { Controls c=b; c.gainStructure=0; level("structure DOWN",c); }
  { Controls c=b; c.gainStructure=1; level("structure MIDDLE",c); }
  { Controls c=b; c.eq2=false; level("EQ1",c); }
  { Controls c=b; c.bypass=true; level("BYPASS stack",c); }
  { Controls c=b; c.midSw=true; level("MID switch @700Hz",c,-24,700); }
  { Controls c=b; level("no MID switch @700Hz",c,-24,700); }
  { Controls c=b; c.deep=true; level("DEEP @100Hz",c,-24,100); }
  { Controls c=b; level("no DEEP @100Hz",c,-24,100); }
  { Controls c=b; c.contour=0.0; level("contour CCW @4kHz",c,-24,4000); }
  { Controls c=b; c.contour=1.0; level("contour CW @4kHz",c,-24,4000); }
  { Controls c=b; c.contour=0.0; level("contour CCW @100Hz",c,-24,100); }
  { Controls c=b; c.contour=1.0; level("contour CW @100Hz",c,-24,100); }
  // reverb taper: tail energy for return 0.1..1.0 at send 0.4
  printf("reverb wet level (dB re dry click energy) vs RETURN:");
  for(double r: {0.1,0.2,0.3,0.5,0.7,1.0}){ AmpEngine a=mk(); Controls c=b; c.ret=r; c.send=0.4; a.setControls(c);
    int N=(int)(2*fs); double ed=0, ew=0; for(int i=0;i<N;i++){ double x=(i<40)?0.01:0.0; double o=a.process(x); if(i<(int)(0.15*fs)) ed+=o*o; else if(i<(int)(1.2*fs)) ew+=o*o; }
    printf(" %.1f:%+.1f", r, 10*std::log10(ew/ed+1e-12)); } printf("\n");
  { AmpEngine a=mk(true); Controls c=b; a.setControls(c); printf("50W: idle/tube %.1f mA, outRef %.0f\n", a.power.idleI/(2.0*a.power.parallel)*1e3, a.outRef);
    printf("   "); for(double d:{-24.,-12.,-6.,0.}){ auto r=run(a,d); printf(" [%+.0f: %.1f dBFS %.1f%%]",d,r.lvl,r.thd);} printf("\n");
    AmpEngine f=mk(false); f.setControls(c); printf("100W:"); for(double d:{-24.,-12.,-6.,0.}){ auto r=run(f,d); printf(" [%+.0f: %.1f dBFS %.1f%%]",d,r.lvl,r.thd);} printf("\n"); }
  // stability after click at extremes
  { AmpEngine a=mk(); Controls c; c.gain=1;c.master=1;c.fetGain=1;c.gainStructure=1;c.eq2=true;c.ret=1;c.send=1; a.setControls(c); double late=0; int N=(int)(1.5*fs);
    for(int i=0;i<N;i++){ double o=a.process(i<30?0.3:0.0); if(!std::isfinite(o)) {printf("NaN!\n");return 1;} if(i>N*3/4) late+=o*o; } printf("extreme click late rms %.2e\n", std::sqrt(late/(N/4))); }
}
