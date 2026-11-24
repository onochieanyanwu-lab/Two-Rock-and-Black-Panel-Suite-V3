#include "../AmpCore.h"
#include <cstdio>
#include <complex>
using namespace amp;
int main(){
  double fs=96000;
  // 1) closed-loop frequency response, clean, typical settings
  for(int v=0; v<3; v++){
    Voicing vc = v==0?Voicing::BlackfaceTwin: v==1?Voicing::Blackface22:Voicing::TwoRock100;
    const char* nm = v==0?"Twin  ":v==1?"Deluxe":"TwoRock";
    AmpEngine a; a.configure(vc); a.prepare(fs);
    Controls c; c.volume=0.35; c.treble=0.6; c.middle=0.5; c.bass=0.5; c.reverb=0; c.master=0.7; c.presence=0.5; a.setControls(c);
    printf("%-7s clean response (rel. 1k):", nm); double ref=0; double vals[10]; int k=0;
    for(double f: {60.,100.,200.,400.,800.,1000.,2000.,4000.,8000.,12000.}){
      a.reset(); int N=(int)(0.45*fs); std::complex<double> acc=0; int cnt=0; double amp=0.004;
      for(int i=0;i<N;i++){ double o=a.process(amp*std::sin(2*kPi*f*i/fs)); if(i>N/2){acc+=o*std::polar(1.0,-2*kPi*f*i/fs);cnt++;} }
      vals[k]=20*std::log10(std::abs(acc)*2/cnt/amp+1e-12); if(f==1000.) ref=vals[k]; k++; }
    for(int i=0;i<10;i++) printf(" %5.1f",vals[i]-ref); printf("   dB @ 60,100,200,400,800,1k,2k,4k,8k,12k\n");
  }
  // 2) spring reverb tail
  {
    AmpEngine a; a.configure(Voicing::BlackfaceTwin); a.prepare(fs);
    Controls c; c.volume=0.4; c.reverb=0.6; c.master=1; a.setControls(c);
    int N=(int)(3.0*fs); std::vector<double> o(N);
    for(int i=0;i<N;i++){ double x=(i<40)?0.05:0.0; o[i]=a.process(x); }
    printf("reverb tail energy (dB re first 100ms):");
    double e0=0; for(int i=0;i<(int)(0.1*fs);i++) e0+=o[i]*o[i];
    for(double t : {0.1,0.3,0.6,1.0,1.5,2.0,2.5}){ double e=0; int s=(int)(t*fs), n=(int)(0.1*fs); for(int i=s;i<s+n;i++) e+=o[i]*o[i]; printf(" %.1fs:%5.1f", t, 10*std::log10(e/e0+1e-20)); }
    printf("\n");
  }
  // 3) tremolo depth
  {
    AmpEngine a; a.configure(Voicing::BlackfaceTwin); a.prepare(fs);
    Controls c; c.volume=0.3; c.reverb=0; c.vibDepth=1.0; c.vibSpeed=0.5; a.setControls(c);
    int N=(int)(2.0*fs); double mn=1e9,mx=0; int blk=(int)(0.005*fs); double acc=0; int n=0;
    for(int i=0;i<N;i++){ double o=a.process(0.02*std::sin(2*kPi*500.0*i/fs)); acc+=o*o; if(++n==blk){ double r=std::sqrt(acc/n); if(i>fs*0.5){mn=std::min(mn,r);mx=std::max(mx,r);} acc=0;n=0; } }
    printf("tremolo: level swing %.1f dB at depth 1.0\n", 20*std::log10(mx/mn));
  }
}
