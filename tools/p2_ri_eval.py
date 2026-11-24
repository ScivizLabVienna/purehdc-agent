# tools/p2_ri_eval.py — P2a smoke: does random indexing beat BM25 on compositional retrieval?
# Research tooling ONLY (mandate: Python allowed outside the C runtime). Deterministic.
import math, random, re
from collections import Counter

random.seed(88172645463)
D = 1024
COLORS = ["red","blue","green","yellow"]
SHAPE  = ["sphere","cube","cone"]
PLACE  = ["table","shelf","window","door"]
REL    = ["above","beside","under"]

rng = random.Random(1000003)
FACTS = [f"the {c} {s} is {rng.choice(REL)} the {p}" for c in COLORS for s in SHAPE for p in PLACE]  # 48, each combo once
QUERIES = ["which sphere is red?", "what is above the shelf?", "the green cube where?",
           "red thing beside door", "where is the blue cone?", "yellow object under?"]
tok = lambda t: re.findall(r"[a-z]+", t.lower())

def bm25_scores(docs, q, k1=1.5, b=0.75):
    N = len(docs); dl = [len(d) for d in docs]; avgdl = sum(dl)/N
    df = Counter(w for d in docs for w in set(d))
    out = []
    for d, L in zip(docs, dl):
        tf = Counter(d); s = 0.0
        for w in set(q):
            if w not in tf: continue
            idf = math.log(1 + (N - df[w] + .5)/(df[w] + .5))
            s += idf * tf[w]*(k1+1)/(tf[w] + k1*(1-b+b*L/avgdl))
        out.append(s)
    return out

_ctx = {}
def get_ctx(w):                                  # deterministic 4-nonzero ±1 vector per word
    if w not in _ctx:
        r = random.Random("ctx:" + w); v = [0]*D
        for i in r.sample(range(D), 4): v[i] = r.choice((-1, 1))
        _ctx[w] = v
    return _ctx[w]

def ri_embed(words, window=3):                   # symmetric RI: sum_{|i-j|<=W} c_i ⊙ c_j
    acc = [0.0]*D
    for i, w in enumerate(words):
        ci = get_ctx(w)
        for j in range(max(0,i-window), min(len(words), i+window+1)):
            if j == i: continue
            cj = get_ctx(words[j])
            for k in range(D): acc[k] += ci[k]*cj[k]
    return acc   # real-valued (binarisation here collapsed D=1024 signal to all-ties)

def cos(a, b):
    num = sum(x*y for x,y in zip(a,b))
    na = math.sqrt(sum(x*x for x in a)) or 1; nb = math.sqrt(sum(y*y for y in b)) or 1
    return num/(na*nb)

FACT_TOK = [tok(f) for f in FACTS]
RI_FACT  = [ri_embed(t) for t in FACT_TOK]
EXACT    = [set(t) for t in FACT_TOK]

arms = {"bm25": lambda q: bm25_scores(FACT_TOK, q),
        "ri_bundle": lambda q: [cos(ri_embed(q), f) for f in RI_FACT],
        "jaccard_exact": lambda q: [len(set(q)&e)/max(1,len(set(q)|e)) for e in EXACT]}

print(f"p2_ri_eval D={D} facts={len(FACTS)} (synthetic grounded triples, seed 88172645463)")
for name, fn in arms.items():
    print(f"== {name}")
    for q in QUERIES:
        s = fn(tok(q)); b = max(range(len(FACTS)), key=lambda i: s[i])
        print(f"  Q: {q:30s} -> {FACTS[b]:38s} (margin {s[b]-sorted(s)[-2]:+.4f})")
