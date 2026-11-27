# tools/p3_multiseed.py — is the RI tie-break signal real or one lucky ctx draw? Replicate over seeds.
import math, random
M=(1<<64)-1
def sm64(st):
    st[0]=(st[0]+0x9E3779B97F4A7C15)&M; z=st[0]
    z=((z^(z>>30))*0xBF58476D1CE4E5B9)&M; z=((z^(z>>27))*0x94D049BB133111EB)&M; return z^(z>>31)
def fnv(s):
    h=1469598103934665603
    for c in s.encode(): h=((h^c)*1099511628211)&M
    return h

src=open('tools/p2_ri_eval2.py').read().split('def bm25_scores')[0]
exec(src)   # corpus+QUERIES, deterministic seed 42 — identical across draws

def run(D, nz, seed):
    rng=random.Random(seed); coords_of={}
    def ctx(w):
        if w not in coords_of:
            cl=set(); st=[fnv(w)^seed]
            while len(cl)<nz: cl.add(sm64(st)%D)
            cl=list(cl); signs={c:(-1 if (sm64(st)>>63)&1 else 1) for c in cl}
            coords_of[w]=(cl,signs)
        return coords_of[w]
    prof={}
    for d in FT+BRIDGE:
        for i,w in enumerate(d):
            ci,si=ctx(w); p=prof.setdefault(w,{})
            for j in range(max(0,i-2),min(len(d),i+3)):
                if i==j: continue
                cj,sj=ctx(d[j])
                cjs=set(cj)
                for a in ci:
                    if a in cjs: p[a]=p.get(a,0)+si[a]*sj[a]
    def emb(ts):
        acc={}
        for w in ts:
            for k,v in prof.get(w,{}).items(): acc[k]=acc.get(k,0.0)+v
        return acc
    def cos(a,b):
        n=sum(x*y for k,x in a.items() if k in b and (y:=b[k]))
        na=math.sqrt(sum(x*x for x in a.values())) or 1; nb=math.sqrt(sum(y*y for y in b.values())) or 1
        return n/(na*nb)
    FE=[emb(t) for t in FT]
    aucs=[]
    for item in QUERIES:
        if len(item)<4 or item[2]!='synx': continue
        q,golds,cand=item[0],item[1],item[3]
        e=emb(q); sc=[cos(e,f) for f in FE]
        c=tot=0
        for g in golds:
            for i2 in cand-golds:
                tot+=1; c+= 1.0 if sc[g]>sc[i2] else (0.5 if sc[g]==sc[i2] else 0.0)
        aucs.append(c/tot)
    return sum(aucs)/len(aucs)

for D,nz in [(1024,32),(1024,64),(4096,32)]:
    v=[run(D,nz,s) for s in range(1,11)]
    m=sum(v)/len(v); sd=math.sqrt(sum((x-m)**2 for x in v)/9)
    print(f"D={D} nz={nz:3d}: across-draw mean {m:.3f} sd {sd:.3f} min {min(v):.3f} max {max(v):.3f}")
