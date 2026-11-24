# tools/p2_ri_eval2.py — P2c: discriminating eval with GROUND-TRUTH queries + per-arm score dumps.
# Fixes r4/r5 flaws: (1) labeled queries by construction, (2) stopword-component diagnosis,
# (3) arms: bm25 | jaccard | RI-raw | RI-stopword-removed | RI-idf-weighted. Deterministic.
import math, random, re
from collections import Counter

rng = random.Random(42)
D = 1024
ADJ = [f"c{i}" for i in range(1200)]; NOUN = [f"n{i}" for i in range(60)]
REL = [f"r{i}" for i in range(8)]
SYNTH_N = 600
FACTS, TUP = [], []
for _ in range(SYNTH_N):
    a, n1, r, n2 = rng.choice(ADJ), rng.choice(NOUN), rng.choice(REL), rng.choice(NOUN)
    TUP.append((a, n1, r, n2)); FACTS.append(f"the {a} {n1} is {r} the {n2}")
SYN = {"thing": set(random.Random(7).sample(NOUN, 18)), "object": set(random.Random(9).sample(NOUN, 18))}

# ---- labeled queries: adj used exactly once in corpus, noun has a synonym ----
adjcount = Counter(t[0] for t in TUP)
uniq_facts = [(i,t) for i,t in enumerate(TUP) if adjcount[t[0]]==1 and t[1] in SYN["thing"]]
rng.shuffle(uniq_facts)
QUERIES = []                                   # (query_tokens, gold_fact_idx, kind)
for i,(a,n1,r,n2) in uniq_facts[:30]:
    QUERIES.append((f"which thing is {a}".split(), i, "syn_adj"))   # sanity: adj present -> lexical trivial
lex = [(i,t) for i,t in enumerate(TUP) if adjcount[t[0]]==1]
rng.shuffle(lex)
for i,(a,n1,r,n2) in lex[:30]:
    QUERIES.append((f"the {a} {n1}".split(), i, "lex"))

# synx: adjective WITHHELD — lexical arms tie within candidate set; only thing->noun bridge breaks ties.
from collections import defaultdict
by_rn = defaultdict(list)
for i,(a,n1,r,n2) in enumerate(TUP): by_rn[(r,n2)].append(i)
nqx = 0
for (r,n2), idxs in by_rn.items():
    if nqx >= 60: break
    golds = [i for i in idxs if TUP[i][1] in SYN["thing"]]
    dis   = [i for i in idxs if TUP[i][1] not in SYN["thing"]]
    if len(idxs) >= 3 and golds and dis:
        QUERIES.append((f"which thing is {r} the {n2}".split(), set(golds), "synx", set(idxs)))
        nqx += 1
tok = lambda s: re.findall(r"\w+", s.lower())
FT = [tok(f) for f in FACTS]
BRIDGE = [[f"a", s, "such", "as", w] for s in SYN for w in sorted(SYN[s])]
CORPUS = FT + BRIDGE

def bm25_scores(q, k1=1.5, b=0.75):
    N=len(FT); dl=[len(d) for d in FT]; avgdl=sum(dl)/N
    df=Counter(w for d in FT for w in set(d))
    out=[]
    for d,L in zip(FT,dl):
        tf=Counter(d); s=0.0
        for w in set(q):
            if w not in tf: continue
            idf=math.log(1+(N-df[w]+.5)/(df[w]+.5))
            s+=idf*tf[w]*(k1+1)/(tf[w]+k1*(1-b+b*L/avgdl))
        out.append(s)
    return out

def jacc(q): 
    Q=set(q); return [len(Q&e)/max(1,len(Q|e)) for e in map(set,FT)]

_ctx={}
def gc(w):
    if w not in _ctx:
        r=random.Random("c"+w); v=[0]*D
        for i in r.sample(range(D),32): v[i]=r.choice((-1,1))
        _ctx[w]=v
    return _ctx[w]

N=len(CORPUS)
DF=Counter(w for d in CORPUS for w in set(d))
STOP={w for w,c in DF.items() if c > N*0.5}          # df>50% = common-component suspects
IDF={w: math.log(1+(N-c+.5)/(c+.5)) for w,c in DF.items()}

def build_profiles(drop_stop=False, idf_w=False):
    prof={}
    for d in CORPUS:
        for i,w in enumerate(d):
            if drop_stop and (w in STOP): continue
            a=gc(w)
            for j in range(max(0,i-2), min(len(d),i+3)):
                if i==j: continue
                v=d[j]
                if drop_stop and v in STOP: continue
                b_=gc(v); p=prof.setdefault(w,{})
                wt = IDF.get(v,1.0) if idf_w else 1.0
                for k,(x,y) in enumerate(zip(a,b_)):          # CRITICAL: keys are COORDINATE INDICES (r4/r5 bug: values-as-keys made every doc embed identically)
                    if x and y: p[k]=p.get(k,0)+wt*x*y
    return prof

def mk_arm(prof):
    def embed(tokens):
        acc=[0.0]*D
        for w in tokens:
            for k,v in prof.get(w,{}).items(): acc[k]+=v
        return acc
    FE=[embed(t) for t in FT]
    def sc(q):
        e=embed(q)
        ne=math.sqrt(sum(x*x for x in e)) or 1
        out=[]
        for f in FE:
            nf=math.sqrt(sum(x*x for x in f)) or 1
            out.append(sum(x*y for x,y in zip(e,f))/(ne*nf))
        return out
    return sc

ARMS={"bm25":bm25_scores,"jaccard":jacc,
      "ri_raw":mk_arm(build_profiles()),
      "ri_stopdrop":mk_arm(build_profiles(drop_stop=True)),
      "ri_idf":mk_arm(build_profiles(idf_w=True))}

print(f"p2c corpus={len(FT)} D={D} nz=32 queries={len(QUERIES)} stopset={sorted(STOP)}")
BR = random.Random(1234)
for name,fn in ARMS.items():
    cnt = {k:[0,0] for k in ("syn_adj","lex","synx")}
    aucs=[]
    for item in QUERIES:
        q,gold,kind = item[0],item[1],item[2]
        golds = gold if isinstance(gold,set) else {gold}
        s=fn(q); b=max(range(len(FT)),key=lambda i:s[i])
        cnt[kind][1]+=1; cnt[kind][0]+= (b in golds)
        if kind=="synx":
            cand=item[3]; gs=[s[i] for i in golds]; ds=[s[i] for i in cand-golds]
            cmp_=sum(1.0 if g>d else 0.5 if g==d else 0.0 for g in gs for d in ds)
            aucs.append(cmp_/(len(gs)*len(ds)))
    m=sum(aucs)/len(aucs); boot=[]
    for _ in range(2000):
        rs=[aucs[BR.randrange(len(aucs))] for _ in aucs]; boot.append(sum(rs)/len(rs))
    boot.sort(); lo,hi=boot[50],boot[1949]
    parts=" ".join(f"{k} {v[0]}/{v[1]}" for k,v in cnt.items())
    print(f"{name:12s} {parts}  synx AUC {m:.3f} [95%CI {lo:.3f},{hi:.3f}] (chance=0.500)")
