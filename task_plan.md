# E18 pure-HDC C agent — task plan (lane OPEN 2026-11-21)

Origin: operator re-pasted 'PURE-HDC AGENT spec v2.0' 2026-11-21 (verbatim refs/operator-plan-v2.txt).
Successor to E17 (CLOSED NOGO same day — its r1 negatives are BINDING INPUTS, never re-gated here).

## Central research question (frozen)
Can a minimal CPU-only C11 agent built purely from binary HDC + GHRR algebra and associative
memory demonstrate useful compositional behavior on STRUCTURED domains — unique-key relational
QA with calibrated abstention AND gridworld planning over learned transitions — landing within a
measured margin of exact deterministic baselines while abstaining on unknowns? Open-domain
episodic memory vs bm25 is EXCLUDED a priori (killed by E17 r1 + E6/E4 P3c). Paper = first pure-C
end-to-end agent + capacity/interference/hop curves + honest negatives.

## Ground rules (inherited, binding)
- $0 compute, CPU only. No neural anything in the HDC path. Frozen evaluator per gate; spec commit BEFORE gate run; `E18 round-K: <verdict>` commits.
- Anti-goals do NOT reopen: E17 P1 TAM-vs-bm25 episodic; bundled tabular triage (E4 P3/P3c x3 FALSIFIED); XOR-dense-text memory (E6 ~random); LM-replacement bets (nativehdc G2). Model-layer thesis dead — never cite GHRR-as-attention IEEE item (unlocated).
- E17 survivor facts to EXPLOIT: df==1 unique-key associative recall near-perfect; forward+inverse agreement gate = only credible abstention (bare TAM leaks 4/10 novel cues).
- Memory ceiling 80 GiB working set; GHRR D=10000 m=4 cf32 = 1.28 MB/HV (spec formula desk-verified: 10000*4*4*8B); start D=1024/4096, scale only after curves.
- No C compiler on box at lane open; toolchain = portable Zig-cc (clang front end) under S:/tools, never C:.

## Gates (pre-frozen; each committed before its run)
- G-A algebra: bind/unbind exact (~1.0) on disjoint items; perm invert exact; GHRR m in {1,2,4} invertibility + noncommutativity verified; bundling recall curve vs K at D=1024/4096; determinism x2 (bit-identical logs). [round 1]
- G-B store: >=10k unique-key associations in C item-store/TAM-lite; R@1 vs K, bytes/pattern from byte-exact layout formula, p50/p95 latency. Baselines: exact hash map (upper), random (lower). [round 2-3]
- G-C relational QA: synthetic unique-key KG chains depth 1..10 (depth-aware chain build per skill rule); abstain via forward+inverse agreement. Gate: answerable top-1 >= 95% of exact graph-search arm AND unanswerable abstain >= 0.90 at false-refuse <= 0.10, held-out seeds x3.
- G-D world model: 6x6 gridworld transitions learned one-pass; bounded beam planning vs BFS oracle on unseen layouts; report plan validity/optimal-rate + unknown-transition detection. No number invented before pilot; pilot runs after spec commit of metric definitions only.
- G-E persistence/recovery: save/load byte-identical, kill+restart resumes demo (assert exit 0).

## Rounds
- [x] r0 2026-11-21: lane open pre-reg, claim ledger pointer to E17 §1 (same document family; re-verify only items new to v2.0), toolchain bring-up, scaffold, L1 binary algebra + invariant tests.
- [x] r1 2026-11-21: GHRR m=1/2/4 PASS (G-B suite in tests/test_ghrr.c); G-A incl. M-1 negative recorded.
- [x] r2 2026-11-21: streaming byte 2/3-gram SDR encoder PASS (order-sensitive 0.75, onechar 0.886); see findings §2.
- [ ] r3: assoc store L3-lite (C), capacity/RAM/latency tables vs hash-map control.
- [ ] r4: relational layer + G-C run.  - [ ] r5: gridworld planner + G-D.  - [ ] r6: CLI agent loop + persistence G-E.  - [ ] r7+: ablations, TAM vendor-parity port (L3 full, after E4 R1 anchor), paper draft (tectonic at S:/nativehdc/tools/tectonic.exe; lab-level authors + non-peer-review notice per publication policy).
