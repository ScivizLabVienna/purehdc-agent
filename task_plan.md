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
- [x] r3 2026-11-21: G-B store gate PASS (R@1=1.0 @40k, 257B/pat, p50<=0.16ms; hashmap brittle under noise). findings §2.
- [x] r4 2026-11-21: relational layer + G-C PASS (findings §2).
- [x] r5 2026-11-21: G-D PASS full gap 0.00 / stress mean_gap 0.50 (findings §2; st_retrieve distinct-key margin fix).
- [x] r6 2026-11-21: G-E CLI agent PASS — cross-restart log replay, ANSWER+ABSTAIN preserved (ge_cli_run1).
- [x] r7 2026-11-21: abstention ablations — channel noise benign; substitution defeats single-view margin gate (cwr≤.5); dual view restores cwr=0 (r7a/b/c findings §2). KEY PAPER RESULT.
- [x] r8 2026-11-21: capacity curve R@1=1.0 to 81k×D2048 no onset + latency linear; Makefile verified; README/LIMITATIONS/references.bib (provenance-gated) + RESULTS.md manuscript draft v1.
- [x] r9 2026-11-21: GHRR PathHD-style hop-2 PASS acc 1.0 cov 1.0 D=256 m=2; order sensitivity 1.0 vs .248 sim; unbind left-peel API gap logged (findings r9).
- [x] r10/r15 2026-11-21 DONE: stdlib-SVG figures (scripts/make_figs.py) + full LaTeX paper paper/main.tex compiled with S:/nativehdc/tools/tectonic.exe -> paper/main.pdf (two-column, pgfplots figures re-keyed to raw artifacts; latency coords corrected against r8 file during review).
- [x] r9b: RESULTS.md R7 GHRR path row + persistence renumbered R8.
- [x] r11: gate head-to-head DONE (r11_gateh2h_run2): margin blind; self-log fooled identically; only non-derivable evidence (dual-view / env-verify) works. v1 blind-gate kept as evidence.
- [~] r12 PARTIAL: r12a seed robustness DONE (8 seeds × 4 D, min R@1 1.0, gated cwr 0). OPEN stretch: TAM vendor-parity port; PDF/submission-format pass.
- [x] r13: make-all coverage + examples/demo.sh end-to-end PASS + LIMITATIONS L13-15 + findings §4 predicted-vs-achieved + cli_e2e arity bugfix (ABSTAIN path now truly gated).
