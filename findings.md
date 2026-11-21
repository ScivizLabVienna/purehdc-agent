# E18 findings — pure-HDC C agent

## §0 Lineage + binding negatives (read before any design choice)
- E17 (S:/research/e17-purehdc-agent, CLOSED NOGO 2026-11-21 r1): TAM bag-of-cue episodic recall R@10 5.03% vs bm25 54.28% (paired delta -0.492 CI95 [-0.517,-0.468], n=1531). Kernel innocent (df==1 cue->turn 79/80); aggregation structurally dead on LoCoMo-style evidence sharing. Novel-cue leak 4/10 natural cues at thr .7 -> 'no match->empty' FALSIFIED; abstention MUST be agreement-gated.
- E4 (substrate accepted): depth exact to m=8; capacity cliff n/D ~1/32..1/8 under rank-1 metric; per-bit winner FHRR-bit. P3/P3c: tabular multi-feature HV browsers falsified x3.
- nativehdc G2: GHRR-as-attention loses +8.3% ppl band at LM scale — E18 never bets on generation.
- E6: XOR-bundle dense-text memory ~random vs bm25.
- Claim ledger for the pasted v2.0 citations: see e17-purehdc-agent/findings.md §1 (same document family; 9/9 checkable ids VERIFIED exact-title there). New-to-v2 items verified by desk arithmetic: GHRR RAM formula bytes= D*m^2*8 correct as stated (cf32); TAM dense bits formula from paste is the hetero-variant to be re-derived from vendor source at L3 port time — do NOT trust until derived.

## §1 Round log
- 2026-11-21 r0: lane opened by operator paste of spec v2.0 (continuation directive: continuous autonomous rounds, persistent logs, no fabricated progress). Compiler absent -> Zig cc portable to S:/tools ($0). Scaffold + L1 algebra started same round.

## §2 Results tables (append-only; raw artifacts in results/)
