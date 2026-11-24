#include "../AmpCore.h"
#include <cstdio>
using namespace amp;
int main(){
  double fs=96000;
  AmpEngine a; a.configure(Voicing::BlackfaceTwin); a.prepare(fs);
  Controls c; c.volume=0.5; c.reverb=0; a.setControls(c);
  printf("idle: s1 ip=%.3fmA vk=%.2f vp=%.1f | power v=%.3f ip1=%.1fmA ip2=%.1fmA supply=%.0fmA Vb=%.1f\n",a.s1.ip*1e3,a.s1.vk,a.s1.vp,a.power.v,a.power.ipk1*1e3,a.power.ipk2*1e3,a.power.supplyI*1e3,a.power.Vb);
  double r1=0,r2=0,r3=0,r4=0,rpi=0,rv=0; int N=(int)(0.3*fs);
  for(int i=0;i<N;i++){
    double x=0.125*std::sin(2*kPi*1000.0*i/fs)*a.inputScale;
    double o1=a.s1.process(x); a.hp1.lowpass(o1); double h1=a.hp1.hp;
    double t=a.stack.process(h1); double v=a.bright1.process(t);
    double o2=a.s2.process(v); a.hp2.lowpass(o2); double h2=a.hp2.hp;
    double pin=h2*a.piPad; double y=a.piSwing*std::tanh(pin/(a.piSwing/a.piGain));
    double vh=a.power.process(y,-y*a.piMismatch);
    if(i>N/2){r1+=h1*h1;r2+=t*t;r3+=h2*h2;rpi+=y*y;rv+=vh*vh;r4++;}
    if(i<6||i%20000==0) printf("i=%d in=%.3f s1=%.3f stack=%.4f vol=%.4f s2=%.3f pi=%.3f vhalf=%.3f ip1=%.1f ip2=%.1f\n",i,x,h1,t,v,h2,y,vh,a.power.ipk1*1e3,a.power.ipk2*1e3);
  }
  printf("RMS: s1out %.3f  stack %.4f  s2out %.3f  PI %.3f  vhalf %.3f\n",sqrt(r1/r4),sqrt(r2/r4),sqrt(r3/r4),sqrt(rpi/r4),sqrt(rv/r4));
}
