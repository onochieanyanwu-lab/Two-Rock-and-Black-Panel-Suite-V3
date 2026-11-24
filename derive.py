import sympy as sp
s,t,m,l=sp.symbols('s t m l'); R1,R2,R3,R4,C1,C2,C3=sp.symbols('R1 R2 R3 R4 C1 C2 C3',positive=True)
P,Q,N4,N5=sp.symbols('P Q N4 N5')
Vin=1
# KCL (currents leaving node = 0)
eqs=[ (P-Vin)*s*C1 + (P-Q)/R1,
      (Q-P)/R1 + (Q-Vin)/R4 + (Q-N4)*s*C2 + (Q-N5)*s*C3,
      (N4-Q)*s*C2 + (N4-N5)/(l*R2),
      (N5-N4)/(l*R2) + (N5-Q)*s*C3 + N5/(m*R3) ]
sol=sp.solve(eqs,[P,Q,N4,N5],dict=True)[0]
H=sp.simplify(t*sol[P]+(1-t)*sol[Q])
num,den=sp.fraction(sp.together(H))
num=sp.Poly(sp.expand(num),s); den=sp.Poly(sp.expand(den),s)
d0=den.coeff_monomial(1)
print("den deg",den.degree(),"num deg",num.degree())
def c(poly,k): return sp.factor(poly.coeff_monomial(s**k)/d0)
import json
out={}
for name,poly in (('b',num),('a',den)):
    for k in range(0,4):
        out[f'{name}{k}']=sp.expand(sp.simplify(poly.coeff_monomial(s**k)/d0)) if poly.degree()>=k else 0
# numeric self-check vs MNA at a point
vals={R1:250e3,R2:250e3,R3:10e3,R4:100e3,C1:250e-12,C2:100e-9,C3:47e-9,t:0.5,m:0.5,l:0.2}
sv=2j*3.14159265*800
Hn=complex(H.subs(vals).subs(s,sv)); print("H(800Hz) mag dB",20*sp.log(abs(Hn),10).evalf())
with open('test/ts_coeffs.txt','w') as f:
    for k,v in out.items():
        f.write(f"{k} = {sp.ccode(sp.simplify(v))};\n")
print(open('test/ts_coeffs.txt').read()[:1500])
