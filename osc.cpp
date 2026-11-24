#include "../AmpCore.h"
#include <cstdio>
#include <complex>
using namespace amp;
int main(){
  for (double fs : {88200.0, 96000.0, 192000.0, 384000.0})
  for (int v=0; v<3; v++){
    Voicing vc = v==0?Voicing::BlackfaceTwin: v==1?Voicing::Blackface22:Voicing::TwoRock100;
    const char* nm = v==0?"Twin":v==1?"Deluxe":"TwoRock";
    AmpEngine a; a.configure(vc); a.prepare(fs);
    double worst=0;
    for (double vol : {0.2, 1.0}) for (double pres : {0.0, 1.0}) {
      Controls c; c.volume=vol; c.master=1.0; c.reverb=0; c.presence=pres; c.treble=1.0; c.bass=0.0; a.setControls(c); a.reset();
      int N=(int)(0.8*fs); double rms=0; 
      for(int i=0;i<N;i++){ double x=(i<30)?0.3:0.0; double o=a.process(x); if(i>N*3/4) rms+=o*o; }
      worst=std::max(worst,std::sqrt(rms/(N/4)));
    }
    printf("fs %6.0f %-7s beta=%.4f  worst late-rms after click = %.2e  %s\n", fs, nm, a.nfbAmount, worst, worst>1e-4?"** UNSTABLE **":"stable");
  }
}
