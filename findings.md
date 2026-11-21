# E18 findings — pure-HDC C agent

## §0 Lineage + binding negatives (read before any design choice)
- E17 (S:/research/e17-purehdc-agent, CLOSED NOGO 2026-11-21 r1): TAM bag-of-cue episodic recall R@10 5.03% vs bm25 54.28% (paired delta -0.492 CI95 [-0.517,-0.468], n=1531). Kernel innocent (df==1 cue->turn 79/80); aggregation structurally dead on LoCoMo-style evidence sharing. Novel-cue leak 4/10 natural cues at thr .7 -> 'no match->empty' FALSIFIED; abstention MUST be agreement-gated.
- E4 (substrate accepted): depth exact to m=8; capacity cliff n/D ~1/32..1/8 under rank-1 metric; per-bit winner FHRR-bit. P3/P3c: tabular multi-feature HV browsers falsified x3.
- nativehdc G2: GHRR-as-attention loses +8.3% ppl band at LM scale — E18 never bets on generation.
- E6: XOR-bundle dense-text memory ~random vs bm25.
- Claim ledger for the pasted v2.0 citations: see e17-purehdc-agent/findings.md §1 (same document family; 9/9 checkable ids VERIFIED exact-title there). New-to-v2 items verified by desk arithmetic: GHRR RAM formula bytes= D*m^2*8 correct as stated (cf32); TAM dense bits formula from paste is the hetero-variant to be re-derived from vendor source at L3 port time — do NOT trust until derived.

## §1 Round log
- 2026-11-21 r0: lane opened by operator paste of spec v2.0 (continuation directive: continuous autonomous rounds, persistent logs, no fabricated progress). Compiler absent -> Zig cc portable to S:/tools ($0). Scaffold + L1 algebra started same round.

## §2 Rounds r0/r1 measured (2026-11-21)
- Toolchain: Zig 0.17.1 x86_64-windows at S:/tools/zig-x86_64-windows-0.17.1/ (zig cc = clang, full C11, $0). Pitfalls hit: ziglang index.json key is `tarball` not `url`; git-bash GNU tar treats `S:\...` as remote host (colon) -> extract with python zipfile; M_PI absent under strict c11.
- G-A binary algebra PASS 8/8 named invariants @D=1024: xor bind/unbind exact, perm inverse bit-exact (all tested shifts), bundle k=1 identity, weighted-bundle smoke, determinism digest, orthogonality worst|sim| <= 4/sqrt(D) over 200 pairs. Determinism x2: ga_run1.txt == ga_run2.txt byte-identical (cmp).
- G-B GHRR PASS 18/18 over m in {1,2,4} @D=64 blocks: generator unitarity defect <= 4.4e-16, unbind exact inverse RMS<1e-9, associativity fp-stable, commutativity contract (m=1 commutes to fp; m>=2 noncommutes >1e-3), sim(self)=1, digest determinism. Bundled sums non-unitary MEASURED: defect(P)=4.4e-16 vs defect(P+Q)=2.37 -> GHRR bundling must be followed by cleanup/normalization in reasoning layers; recorded per spec requirement.
- M-1 NEGATIVE (L1 curve): flat XOR-superposition pair-bind memory recall at chance — top1: D=1024 K=8/32/128/512 -> 0.0/0.0/0.0/0.002; D=4096 -> 0.125(3/24 noise)/0.031/0.0/0.004 (chance=1/K). Mechanism: XOR of >=1 uniform HVs is itself uniform-like, so unbinding cannot correlate with the stored item. Consequence: exact-recall substrate must be per-entry store (TAM/lamport index), NOT vector superposition — consistent with E4/E6 lineage; strengthens L3 design choice rather than weakening it. Bundling capacity of MAJORITY type vs K is tested at G-B store gate, not here.

- r2 SDR encoder (byte 2/3-grams, k=2 bits/gram hashed): SDR PASS. order_swap Jaccard 0.750 (not bag-of-cue), one-char Jaccard 0.886; identical string Jaccard=1.0 exact. Collision curve degenerate-by-construction (short repeated ids) — honest note: no hash-induced collisions found beyond identical inputs at D=4096 for these corpora.

- r3 G-B STORE GATE PASS: flat-scan per-entry store (value==key sanity) R@1=1.0000 for N={1k,10k,40k} x flip={0,2,5,10%}; 257 B/pattern exact (=formula 2*D/8+1); latency p50 0.004/0.040/0.159 ms at N=1k/10k/40k (p95 <=0.254ms). Exact-digest hashmap control: R@1 1.0 flip-free, 0.0 at ANY noise -> brittleness contrast quantifies where associative tolerance matters. Flat O(N*W) scan fine to ~40k; sharding deferred until measured limit (spec §3 L3 rule honored).

- r4 G-C RELATIONAL QA PASS: chain-KG unique-key retrieval, hop-wise store_acc=1.0000 for ALL hops 1..10 == exact-symbolic hash ceiling (margin of zero), random floor 0.002. Architecture detail: per-hop decode-to-nearest-entity (re-normalization) prevents noise compounding across hops — required, raw HV chaining would degrade. Direction check rel-swap sim -0.02 (distinguishable); XOR commutes by design (honest note; noncommutativity lives in GHRR arm G-B). Abstention calibration on nonexistent edges: tau=0.05 -> confidently-wrong 0.0%, abstain 100%. CAVEAT for r5+: chain is unique-key easy-mode; distractor/multi-value arms pending.

## §3 Results tables (append-only; raw artifacts in results/)
