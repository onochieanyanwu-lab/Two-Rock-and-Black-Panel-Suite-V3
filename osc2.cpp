#include "../AmpCore.h"
#include <cstdio>
using namespace amp;
int main(){
  double fs=96000;
  for (double vol : {0.2, 1.0}) for (double pres : {0.0, 1.0}) for (double mst : {0.0, 1.0}) for (double tr : {0.0,1.0}) {
    AmpEngine a; a.configure(Voicing::TwoRock100); a.prepare(fs);
    Controls c; c.volume=vol; c.master=mst; c.reverb=0; c.presence=pres; c.treble=tr; c.bass=0.0; a.setControls(c);
    int N=(int)(0.8*fs); double rms=0, mx=0;
    for(int i=0;i<N;i++){ double x=(i<30)?0.3:0.0; double o=a.process(x); if(i>N*3/4){rms+=o*o;} mx=std::max(mx,std::fabs(o)); }
    printf("vol %.1f pres %.0f master %.0f treble %.0f : late rms %.2e peak %.2f\n",vol,pres,mst,tr,std::sqrt(rms/(N/4)),mx);
  }
}
