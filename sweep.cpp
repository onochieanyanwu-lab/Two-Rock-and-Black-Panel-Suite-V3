#include "../AmpCore.h"
#include <cstdio>
#include <complex>
using namespace amp;
// small-signal frequency response of the whole amp (in -> speaker volts), with feedback on/off, plus ringing check
int main(int argc,char**argv){
  double fs=96000;
  for(int v=0; v<3; v++){
    Voicing vc = v==0?Voicing::BlackfaceTwin: v==1?Voicing::Blackface22:Voicing::TwoRock100;
    const char* nm = v==0?"BlackfaceTwin":v==1?"Blackface22":"TwoRock100";
    for(int fbOn=1; fbOn>=0; --fbOn){
      AmpEngine a; a.configure(vc); double nf=a.nfbAmount; if(!fbOn) a.nfbAmount=0; a.prepare(fs);
      Controls c; c.volume=0.4; c.treble=0.7; c.middle=0.5; c.bass=0.5; c.reverb=0; c.master=0.6; c.presence=0.5; a.setControls(c);
      printf("%-14s NFB %s:", nm, fbOn?"on ":"off");
      for(double f: {60.,120.,250.,500.,1000.,2000.,4000.,8000.,12000.}){
        a.reset(); int N=(int)(0.5*fs); std::complex<double> acc=0; int cnt=0; double amp=0.0005;
        for(int i=0;i<N;i++){ double o=a.process(amp*std::sin(2*kPi*f*i/fs)); if(i>N/2){ acc+=o*std::polar(1.0,-2*kPi*f*i/fs); cnt++; } }
        printf(" %6.1f", 20*std::log10(std::abs(acc)*2/cnt/amp+1e-12));
      }
      printf("  dB(out/in) @60..12k\n");
    }
  }
}
