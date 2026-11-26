#include "../AmpCore.h"
#include <cstdio>
#include <complex>
using namespace amp;
int main(){
  double fs=96000;
  AmpEngine a; a.configure(Voicing::TwoRock100); a.prepare(fs);
  Controls c; c.volume=1.0; c.master=1.0; c.reverb=0; c.presence=0; c.treble=0; c.bass=0.0; a.setControls(c);
  int N=(int)(1.0*fs); std::vector<double> o(N);
  for(int i=0;i<N;i++){ double x=(i<30)?0.3:0.0; o[i]=a.process(x);
     if(i%9600==0) printf("t=%.2fs out=%.4f  s1.vp=%.1f s2.vp=%.1f s3.vp=%.1f  power.v=%.1f ip1=%.0f ip2=%.0f  Vb=%.1f g1.vg=%.2f g2.vg=%.2f\n",i/fs,o[i],a.s1.vp,a.s2.vp,a.s3.vp,a.power.v,a.power.ipk1*1e3,a.power.ipk2*1e3,a.power.Vb,a.power.g1.vg,a.power.g2.vg); }
  double best=0,bf=0; for(double f=20; f<20000; f*=1.02){ std::complex<double> acc=0; for(int i=N*3/4;i<N;i++) acc+=o[i]*std::polar(1.0,-2*kPi*f*i/fs); double m=std::abs(acc); if(m>best){best=m;bf=f;} }
  printf("dominant late freq %.0f Hz\n",bf);
}
