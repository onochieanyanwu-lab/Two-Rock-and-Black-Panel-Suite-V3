#include "../AmpCore.h"
#include <cstdio>
using namespace amp;
static double fs=96000;
void meas(AmpEngine&a,double&pk,double&rms){ a.reset(); int N=(int)(1.0*fs); double s2=0; int n=0; pk=0;
  for(int i=0;i<N;i++){ double t=i/fs; double env=std::exp(-3*t); double x=0.126*env*(0.6*std::sin(2*kPi*196*t)+0.4*std::sin(2*kPi*392*t+1)+0.3*std::sin(2*kPi*587*t)+0.2*std::sin(2*kPi*1175*t)); double o=a.process(x);
    if(i>0.1*fs){ pk=std::max(pk,std::fabs(o)); s2+=o*o; n++; } } pk=20*std::log10(pk+1e-9); rms=10*std::log10(s2/n+1e-12); }
int main(){
  for(int v=0;v<2;v++){ Voicing vc=v?Voicing::Blackface22:Voicing::BlackfaceTwin; printf("%s  pk/rms dBFS vs volume (treble.6 mid.5 bass.4)\n", v?"Deluxe":"Twin");
    for(double vol:{0.2,0.3,0.4,0.5,0.6,0.7,0.8,1.0}){ AmpEngine a; a.configure(vc); a.prepare(fs); Controls c; c.volume=vol; c.treble=.6; c.middle=.5; c.bass=.4; c.reverb=0; a.setControls(c); double pk,rms; meas(a,pk,rms); printf("  vol %.1f: %6.1f / %6.1f\n",vol,pk,rms);} }
}
