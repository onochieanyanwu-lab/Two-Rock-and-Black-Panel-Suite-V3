#include "../AmpCore.h"
#include <cstdio>
using namespace amp;
static double fs=96000;
// guitar-ish test: decaying chord-like burst, report peak & rms (dBFS) of output for -18 dBFS-peak DI input
void meas(AmpEngine&a,double&pk,double&rms){ a.reset(); int N=(int)(1.0*fs); double s2=0; int n=0; pk=0;
  for(int i=0;i<N;i++){ double t=i/fs; double env=std::exp(-3*t); double x=0.126*env*(0.6*std::sin(2*kPi*196*t)+0.4*std::sin(2*kPi*392*t+1)+0.3*std::sin(2*kPi*587*t)+0.2*std::sin(2*kPi*1175*t)); double o=a.process(x*1.0/0.6/1.0*0.6);
    if(i>0.1*fs){ pk=std::max(pk,std::fabs(o)); s2+=o*o; n++; } }
  pk=20*std::log10(pk+1e-9); rms=10*std::log10(s2/n+1e-12); }
int main(){
  printf("master\\gain  ");
  for(double g:{0.3,0.5,0.7,1.0}) printf("   gain %.1f (pk/rms)",g); printf("\n");
  for(double m:{0.2,0.3,0.4,0.5,0.6,0.7,0.8,1.0}){
    printf("master %.1f: ",m);
    for(double g:{0.3,0.5,0.7,1.0}){ AmpEngine a; a.configure(Voicing::TwoRockCRS); a.prepare(fs); Controls c; c.fetOn=false; c.ret=0; c.gain=g; c.master=m; a.setControls(c); double pk,rms; meas(a,pk,rms); printf("  %6.1f /%6.1f   ",pk,rms); }
    printf("\n"); }
}
