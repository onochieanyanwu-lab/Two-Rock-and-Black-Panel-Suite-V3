#include "../AmpCore.h"
#include <cstdio>
#include <complex>
using namespace amp;
int main(){
  double fs=96000;
  for (double Cw : {1e-9, 3e-9, 8e-9}) {
    AmpEngine a; a.configure(Voicing::BlackfaceTwin); a.power.Cw = Cw; a.prepare(fs);
    printf("Cw=%4.1f nF  power-section gain (PI-in volts -> primary-half volts), dB: ", Cw*1e9);
    for(double f : {50.,100.,200.,500.,1000.,2000.,5000.,10000.,15000.,20000.,30000.}){
      a.reset(); a.power.reset(); a.piLP.reset();
      int N=(int)(0.4*fs); std::complex<double> acc=0; int cnt=0; double amp=0.05;
      for(int i=0;i<N;i++){ double pin=amp*std::sin(2*kPi*f*i/fs); double y=a.piSwing*std::tanh(pin/(a.piSwing/a.piGain)); y=a.piLP.lowpass(y); double v=a.power.process(y,-y*a.piMismatch); if(i>N/2){acc+=v*std::polar(1.0,-2*kPi*f*i/fs);cnt++;} }
      printf("%6.1f", 20*std::log10(std::abs(acc)*2/cnt/amp));
    }
    printf("   @50,100,200,500,1k,2k,5k,10k,15k,20k,30k\n");
  }
}
