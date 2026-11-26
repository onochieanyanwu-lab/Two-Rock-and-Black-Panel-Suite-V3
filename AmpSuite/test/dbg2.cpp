#include "../AmpCore.h"
#include <cstdio>
using namespace amp;
int main(){
  double fs=96000;
  TriodeStage s; s.Bplus=285; s.Rp=100e3; s.Rk=1500; s.Ck=25e-6; s.grid.Cc=100e-9; s.grid.Rl=1e6; s.grid.Rg=68e3; s.grid.Rs=30e3; s.grid.Cm=100e-12;
  s.setup(fs);
  for(int k=0;k<8;k++){ for(int i=0;i<5000;i++) s.process(0.0); printf("after %d: ip=%.4fmA vk=%.4f vp=%.3f vg=%.5f\n",(k+1)*5000,s.ip*1e3,s.vk,s.vp,s.grid.vg);}
  s.vpIdle=s.vp; s.grid.reset();
  for(int i=0;i<4;i++) printf("post reset out=%.4f vg=%.5f\n", s.process(0.0), s.grid.vg);
}
