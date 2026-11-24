#include "../AmpCore.h"
#include <cstdio>
using namespace amp;
int main(){ double fs=96000; for(int h=0;h<2;h++){ AmpEngine a; a.halfPower=h; a.configure(Voicing::TwoRockCRS); a.prepare(fs); Controls c; c.fetOn=false; c.gain=1; c.master=1; c.ret=0; a.setControls(c);
 double pk=0; for(int i=0;i<(int)(0.4*fs);i++){ double o=a.process(0.3*std::sin(2*kPi*110*i/fs)); if(i>fs*0.2) pk=std::max(pk,std::fabs(o)); }
 printf("%s: peak at full drive %.2f (rated-sine ref 0.70) -> ", h?"50W ":"100W", pk); }
 printf("\n"); }
