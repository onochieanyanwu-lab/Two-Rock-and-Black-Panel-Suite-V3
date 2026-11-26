import numpy as np, itertools
R1,R2,R3,R4=250e3,250e3,10e3,100e3; C1,C2,C3=250e-12,100e-9,47e-9
def formula(s,t,m,l):
    b1=t*C1*R1+m*C3*R3+l*(C1*R2+C2*R2)+(C1*R3+C2*R3)
    b2=t*(C1*C2*R1*R4+C1*C3*R1*R4)-m*m*(C1*C3*R3**2+C2*C3*R3**2)+m*(C1*C3*R1*R3+C1*C3*R3**2+C2*C3*R3**2)+l*(C1*C2*R1*R2+C1*C2*R2*R4+C1*C3*R2*R4)+l*m*(C1*C3*R2*R3+C2*C3*R2*R3)+(C1*C2*R1*R3+C1*C2*R3*R4+C1*C3*R3*R4)
    b3=l*m*(C1*C2*C3*R1*R2*R3+C1*C2*C3*R2*R3*R4)-m*m*(C1*C2*C3*R1*R3*R4+C1*C2*C3*R3**2*R4)+m*(C1*C2*C3*R3**2*R4+C1*C2*C3*R1*R3*R4)+t*C1*C2*C3*R1*R3*R4-t*m*C1*C2*C3*R1*R3*R4+t*l*C1*C2*C3*R1*R2*R4
    a1=(C1*R1+C1*R3+C2*R3+C2*R4+C3*R4)+m*C3*R3+l*(C1*R2+C2*R2)
    a2=m*(C1*C3*R1*R3-C2*C3*R3*R4+C1*C3*R3**2+C2*C3*R3**2)+l*m*(C1*C3*R2*R3+C2*C3*R2*R3)-m*m*(C1*C3*R3**2+C2*C3*R3**2)+l*(C1*C2*R2*R4+C1*C2*R1*R2+C1*C3*R2*R4+C2*C3*R2*R4)+(C1*C2*R1*R4+C1*C3*R1*R4+C1*C2*R3*R4+C1*C2*R1*R3+C1*C3*R3*R4+C2*C3*R3*R4)
    a3=l*m*(C1*C2*C3*R1*R2*R3+C1*C2*C3*R2*R3*R4)-m*m*(C1*C2*C3*R1*R3*R4+C1*C2*C3*R3**2*R4)+m*(C1*C2*C3*R3**2*R4+C1*C2*C3*R1*R3*R4-C1*C2*C3*R2*R3*R4)+l*C1*C2*C3*R1*R2*R4+C1*C2*C3*R1*R3*R4
    return (b1*s+b2*s**2+b3*s**3)/(1+a1*s+a2*s**2+a3*s**3)
# Fender TMB candidate (Duncan/Yeh): nodes 0=gnd 1=in 2=P 3=Q 4=S ; C1 in-P ; R1 P-Q wiper=out ; R4 in-S? variants below
def mna(s,t,m,l,var):
    # unknown node voltages for nodes 2..5, in=1V
    N=6; Y=np.zeros((N,N),complex); 
    def add(a,b,y):
        for (i,j,sg) in ((a,a,1),(b,b,1),(a,b,-1),(b,a,-1)):
            if i and j: Y[i,j]+=sg*y
            elif i and j==0 and i==j: pass
        # handled below
    def stamp(a,b,y):
        if a: Y[a,a]+=y
        if b: Y[b,b]+=y
        if a and b: Y[a,b]-=y; Y[b,a]-=y
    # nodes: 1 in(fixed) 2 P 3 Q 4 X 5 Z
    g=lambda r:1/r
    stamp(1,2,s*C1)                       # C1 in-P
    stamp(2,3,g(R1))                      # treble pot P-Q
    if var==0:
        stamp(1,3,g(R4))                  # slope in->Q
        stamp(3,4,s*C2); stamp(4,0,g(max(l*R2,1e-3)))   # C2 then bass rheostat
        stamp(3,5,s*C3); stamp(5,0,g(max(m*R3,1e-3)))   # C3 then mid rheostat
    elif var==1:
        stamp(1,5,g(R4))                  # slope in->Z
        stamp(5,3,s*C2) if False else None
        stamp(5,4,s*C3); stamp(4,0,g(max(m*R3,1e-3)))
        stamp(3,5,s*C2); stamp(3,0,g(max(l*R2,1e-3))) if False else None
        stamp(3,0,g(max(l*R2,1e-3)))
    elif var==2:
        # classic Fender: R4 in->Q ; C2 Q->bass pot top; bass pot (R2) top->bottom node Y; C3 from in? mid pot Y->gnd
        stamp(1,3,g(R4)); stamp(3,4,s*C2); stamp(4,5,g(l*R2+1e-3)); stamp(5,0,g(R3*m+1e-3)); stamp(3,5,s*C3) 
    elif var==3:
        stamp(1,3,g(R4)); stamp(3,4,s*C2); stamp(4,0,g(l*R2+1e-3)); stamp(3,5,s*C3); stamp(5,0,g(R3*m+1e-3)); stamp(3,0,g(R3*(1-m)+1e-3)) 
    # solve with in fixed at 1
    A=Y[2:,2:]; b=-Y[2:,1]*1.0
    v=np.linalg.solve(A+1e-18*np.eye(4),b)
    V=np.concatenate([[0,1],v])
    return t*V[2]+(1-t)*V[3]
best=None
for var in range(4):
    err=0
    for (t,m,l) in [(0.5,0.5,0.2),(1,1,1),(0.2,0.7,0.5),(0.8,0.3,0.1)]:
        for f in [50,200,800,3000,10000]:
            s=2j*np.pi*f
            try: err+=abs(20*np.log10(abs(formula(s,t,m,l)))-20*np.log10(abs(mna(s,t,m,l,var))+1e-12))
            except Exception as e: err+=999
    print("variant",var,"total dB error",round(err,1))

print("--- search")
import itertools
def mna2(s,t,m,l,opt):
    c3to, bassmode, midmode, r4to = opt
    # nodes: 1 in, 2 P, 3 Q, 4 n4, 5 n5, 6 w(mid wiper)
    N=7; Y=np.zeros((N,N),complex)
    def st(a,b,y):
        if a: Y[a,a]+=y
        if b: Y[b,b]+=y
        if a and b: Y[a,b]-=y; Y[b,a]-=y
    e=1e-3
    st(1,2,s*C1); st(2,3,1/R1)
    st(1,3 if r4to==0 else 4,1/R4)
    st(3,4,s*C2)
    # bass pot between n4 and n5
    if bassmode==0: st(4,5,1/(l*R2+e))
    else: st(4,5,1/((1-l)*R2+e)) 
    # mid pot between n5 and gnd with wiper node w=6
    if midmode==0: st(5,6,1/((1-m)*R3+e)); st(6,0,1/(m*R3+e))
    else: st(5,6,1/(m*R3+e)); st(6,0,1/((1-m)*R3+e))
    tgt={0:4,1:5,2:6,3:3}[c3to]
    st(3 if c3to!=3 else 1,tgt,s*C3) if c3to!=3 else st(1,6,s*C3)
    A=Y[2:,2:]; b=-Y[2:,1]
    v=np.linalg.solve(A+1e-18*np.eye(N-2),b)
    V=np.concatenate([[0,1],v])
    return t*V[2]+(1-t)*V[3]
res=[]
for opt in itertools.product(range(4),range(2),range(2),range(2)):
    err=0
    for (t,m,l) in [(0.5,0.5,0.2),(1,1,1),(0.2,0.7,0.5),(0.8,0.3,0.1)]:
        for f in [50,200,800,3000,10000]:
            s=2j*np.pi*f
            try: err+=abs(20*np.log10(abs(formula(s,t,m,l)))-20*np.log10(abs(mna2(s,t,m,l,opt))+1e-12))
            except Exception: err+=999
    res.append((err,opt))
res.sort(); print(res[:5])

print("--- formula variants vs topology variant 2")
def formula2(s,t,m,l,v):
    b1=t*C1*R1+m*C3*R3+l*(C1*R2+C2*R2)+(C1*R3+C2*R3)
    extra_m = (C1*C2*R3*R4+C1*C3*R3*R4) if v&1 else 0
    extra_l = (C2*C3*R2*R4) if v&2 else 0
    b2=t*(C1*C2*R1*R4+C1*C3*R1*R4)-m*m*(C1*C3*R3**2+C2*C3*R3**2)+m*(C1*C3*R1*R3+C1*C3*R3**2+C2*C3*R3**2+extra_m)+l*(C1*C2*R1*R2+C1*C2*R2*R4+C1*C3*R2*R4+extra_l)+l*m*(C1*C3*R2*R3+C2*C3*R2*R3)+(C1*C2*R1*R3+C1*C2*R3*R4+C1*C3*R3*R4)
    b3=l*m*(C1*C2*C3*R1*R2*R3+C1*C2*C3*R2*R3*R4)-m*m*(C1*C2*C3*R1*R3*R4+C1*C2*C3*R3**2*R4)+m*(C1*C2*C3*R3**2*R4+C1*C2*C3*R1*R3*R4)+t*C1*C2*C3*R1*R3*R4-t*m*C1*C2*C3*R1*R3*R4+t*l*C1*C2*C3*R1*R2*R4
    a1=(C1*R1+C1*R3+C2*R3+C2*R4+C3*R4)+m*C3*R3+l*(C1*R2+C2*R2)
    a2=m*(C1*C3*R1*R3-C2*C3*R3*R4+C1*C3*R3**2+C2*C3*R3**2)+l*m*(C1*C3*R2*R3+C2*C3*R2*R3)-m*m*(C1*C3*R3**2+C2*C3*R3**2)+l*(C1*C2*R2*R4+C1*C2*R1*R2+C1*C3*R2*R4+C2*C3*R2*R4)+(C1*C2*R1*R4+C1*C3*R1*R4+C1*C2*R3*R4+C1*C2*R1*R3+C1*C3*R3*R4+C2*C3*R3*R4)
    a3=l*m*(C1*C2*C3*R1*R2*R3+C1*C2*C3*R2*R3*R4)-m*m*(C1*C2*C3*R1*R3*R4+C1*C2*C3*R3**2*R4)+m*(C1*C2*C3*R3**2*R4+C1*C2*C3*R1*R3*R4-C1*C2*C3*R2*R3*R4)+l*C1*C2*C3*R1*R2*R4+C1*C2*C3*R1*R3*R4
    return (b1*s+b2*s**2+b3*s**3)/(1+a1*s+a2*s**2+a3*s**3)
for v in range(4):
  for var in (2,):
    err=0
    for (t,m,l) in [(0.5,0.5,0.2),(1,1,1),(0.2,0.7,0.5),(0.8,0.3,0.1)]:
        for f in [50,200,800,3000,10000]:
            s=2j*np.pi*f
            err+=abs(20*np.log10(abs(formula2(s,t,m,l,v)))-20*np.log10(abs(mna(s,t,m,l,var))+1e-12))
    print("formula variant",v,"vs topology",var,"err",round(err,2))
# exact brute-force: derive the true transfer function numerically from topology var2 and compare formula pieces
