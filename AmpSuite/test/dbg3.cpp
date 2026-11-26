#include "../AmpCore.h"
#include <cstdio>
#include <complex>
using namespace amp;
int main(){
  double fs=96000;
  AmpEngine a; a.configure(Voicing::TwoRock100); a.prepare(fs);
  Controls c; c.volume=0.5; c.master=0.5; c.reverb=0; c.presence=0.5; c.treble=0.5; c.middle=0.5; c.bass=0.5; a.setControls(c);
  printf("nfb beta=%.4f  piPad=%.3f\n", a.nfbAmount, a.piPad);
  double r[7]={0}; int cnt=0; int N=(int)(0.5*fs);
  for(int i=0;i<N;i++){
    double v=0.063*std::sin(2*kPi*1000.0*i/fs)*a.inputScale;           // -24 dBFS
    double o1=a.s1.process(v); a.hp1.lowpass(o1); double h1=a.hp1.hp;
    double vol=a.bright1.process(h1);
    double o2=a.s2.process(vol); a.hp2.lowpass(o2); double h2=a.hp2.hp;
    double t=a.stack.process(h2)*6.0;
    double o3=a.s3.process(t); a.hp3.lowpass(o3); double h3=a.hp3.hp;
    double pin=h3*0.5*a.piPad;
    double y=a.piSwing*std::tanh(pin/(a.piSwing/a.piGain));
    double vh=a.power.process(y,-y*a.piMismatch);
    if(i>N/2){ r[0]+=h1*h1; r[1]+=vol*vol; r[2]+=h2*h2; r[3]+=t*t; r[4]+=h3*h3; r[5]+=y*y; r[6]+=vh*vh; cnt++; }
  }
  const char* nm[]={"s1 out","volume","s2 out","stack*6","s3 out","PI out","vhalf"};
  for(int k=0;k<7;k++) printf("%-8s rms %8.3f V\n", nm[k], std::sqrt(r[k]/cnt));
}
