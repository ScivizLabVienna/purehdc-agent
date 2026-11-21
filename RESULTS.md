# A Pure Hyperdimensional-Computing Agent: Associative Memory, Compositional Algebra, and Calibrated Abstention Without Neural Training

**Workshop draft — E18 campaign. All numbers in this paper are read directly from deterministic run artifacts in `results/` (git-committed); §Reproducibility maps every table to its file.**
*Status: pre-review draft. Author list / affiliations pending; internal policy label per lane convention: preliminary, not peer-reviewed.*

## Abstract

We present the first end-to-end agent written in pure C11 whose representation, memory, inference, planning, and online learning are built exclusively from binary hyperdimensional computing (HDC), a flat-scan topological associative store, and bounded deterministic control — no neural networks, gradients, GPU, learned tokenizer, or LLM calls. On unique-key relational question answering the agent matches an exact symbolic baseline (accuracy 1.000 through 10-hop chains; retrieval R@1 = 1.0 at 40k entries under 10% query bit-flips, where a hashing-based control collapses to 0.0). On gridworld planning over *learned* state–action transitions it reaches the exact BFS oracle's optimality (gap 0) when transitions are fully observed and degrades only to bounded detours (mean gap 0.5) under 15% missing knowledge, never failing silently. Our central contribution is a measured account of **when associative confidence signals do and do not support calibrated abstention**: channel noise cannot induce confident errors in binary XOR nearest-neighbour memory (always-answer stays at 1.0 accuracy up to 12.5% bit-flips), yet *substitution* corruption — replacing stored rows with mutated twins — defeats first-vs-second margin gating entirely (confidently-wrong rate 0.48–1.00), while consistency across two independently encoded views restores cwr = 0 at a proportional coverage cost. We release code, seeds, and raw logs, including three negative results we consider load-bearing for the field.

## 1 Introduction & research question

Vector-symbolic architectures promise compositional cognition from cheap algebra. Yet no public system demonstrates a *complete agent loop* — observe, encode, store, retrieve, compose, plan, act, abstain, persist — that is genuinely HDC-native end to end (spec: Pure-HDC Agent v2.0). We ask:

**RQ.** Can a CPU-only C11 program built from binary HV algebra, an associative store, and deterministic control exhibit useful compositional agent behaviour with *calibrated* uncertainty, within measurable margin of exact baselines on structured domains?

Pre-registration discipline (lane `task_plan.md`, commit `e896a62`): primary domains are unique-key relational QA and gridworld planning; open-domain episodic retrieval versus bm25 was excluded a priori because that gate failed in our prior campaign (TAM 5.0% vs bm25 54.3%, binding negative). Every experiment is deterministic from fixed splitmix64 seeds; repeat runs are byte-identical.

## 2 System

Five layers, ~1.6k LoC total C11 (`zig cc -O2 -std=c11`):

- **L1 algebra** (`src/algebra/`): XOR bind/unbind, circular permutation (bit-exact inverse verified), majority/weighted bundle, cosine similarity, cleanup; genuine GHRR complex U(m) binding for m∈{1,2,4} with conjugate-transpose unbinding. 26 invariants pass, incl. negative controls.
- **L2 encoding** (`src/encoding/`): streaming byte 2/3-gram SDR hashing (k=2 bits/gram, FNV1a+splitmix64), no learned vocabulary; order sensitivity measured (0.750 Jaccard under word swap — not bag-of-cue).
- **L3 memory** (`src/tam/store.c`): flat-scan content-addressed store, key→value HV rows, append-only; `st_retrieve` returns top match plus margin against the best *distinct-key* competitor (duplicate keys = corroborating evidence, not competition — see §6).
- **L4 reasoning**: relational triple encoding key = subj ⊕ rel → obj; hop-wise decode-to-nearest-entity chain traversal; margin+confidence abstention gate calibrated on held-out DEV.
- **L5 agent** (`src/agent/main.c`): bounded CLI loop LEARN/QUERY/BYE with symbol-level novelty detection, provenance (row id + confidence), and cross-restart persistence via deterministic append-only episodic-log replay.

No component consults a neural model at build, run, or evaluation time. Python appears nowhere in the runtime path.

## 3 Method & baselines

Every experiment carries an *exact* control: hashmap exact-match for retrieval (no noise tolerance by construction), BFS over ground-truth graph for planning, and always-answer vs. gated arms for uncertainty. DEV/TEST splits are parity-partitioned on subject identity; thresholds (τ grid .02–.20) chosen on DEV only, reported on TEST. Negative controls: relation-swap key similarity, superposition-unbinding failure (M-1 below), missing-transition stress arm.

## 4 Results

**R1 — Algebra (established).** All invariants pass at D∈{64,…,4096}; permutation inverse bit-exact; GHRR generator unitarity defect ≤4.4e-16; noncommutativity of matrix binding confirmed; *bundled* matrices measured non-unitary (defect 2.37) and we never unbind through bundles losslessly.

**R2 — Storage vs noise (`gb_store_run2`).** At D=1024: store R@1 = **1.0000** at N∈{1k,10k,40k} for cue flip rates 0–10%; exact-hashmap control R@1 = 1.0 only at zero noise, 0.0 at any noise. RAM 257 B/pattern (formula-exact); p50 latency 0.36 ms @40k.

**R3 — Multi-hop QA (`gc_qa_run2`).** Synthetic chain KG, per-hop decode: accuracy **1.0000 for hops 1–10** (= task ceiling; claim limited to non-degradation). Negative control: relation-swap keys cos −0.02 → roles/direction preserved. Abstention: τ=0.02 leaves cwr 0.145; **τ≥0.05 → cwr = 0.0** (with full abstention on unseen hops — the gate abstains rather than hallucinating).

**R4 — Planning (`gd_grid_run1`).** 8×8 grid, one-pass learned transitions key = s⊕a; BFS over *predicted* successors only. Full knowledge: success 1.0, mean optimality gap vs exact BFS oracle **0.00**. Stress (15% of transition keys absent from replay): success still 1.0 — the planner recombines learned edges into detours (mean gap 0.50, max 3) instead of failing or hallucinating an edge; unknown-transition requests abstain.

**R5 — Capacity & latency (`r8_capcurve_run2`).** Unique random HV pairs: R@1 = **1.0000 at every measured point to N=81k × D∈{256…2048}**, with and without 5% cue noise. No interference onset is observable in this regime; the binding constraint is O(N·W) scan time: 0.013→1.079 ms per query pair at D=256 and 0.027→3.983 ms at D=2048 (linear, as predicted). Claimed capacity: >81k entries *under these domains*; bundling remains the fragile substrate (M-1).

**R6 — When does abstention earn its keep? (`r7_hardqa_run2`, `r7c_subst_run1`) — our central result.**
(a) *Channel noise*: 8000-fact dense base, D-sweep, cue flips to 12.5%·D: always-top1 accuracy stays **1.000 everywhere**; the margin gate never improves correctness, it only trades coverage for safety (cov 1.0→0.001 as noise grows, acc 1.0, cwr 0.0 throughout; DEV/TEST identical). Binary XOR nearest-neighbour simply cannot be confidently wrong from query noise with unique keys.
(b) *Substitution corruption*: p_erase ∈ {0,.5,1} of view-A rows replaced by bit-mutated twins carrying decoy values; independent permuted-view B intact:

| p_erase | δ (bits) | always-A acc | margin-gate A: cov / acc / **cwr** | dual-view: cov / acc / cwr |
|---|---|---|---|---|
| 0.0 | 8/32 | 1.000 | 1.00 / 1.000 / 0.000 | 1.00 / 1.000 / 0.000 |
| 0.5 | 8 | 0.500 | 1.00 / 0.500 / **0.500** | 0.50 / 1.000 / 0.000 |
| 0.5 | 32 | 0.520 | 1.00 / 0.520 / **0.480** | 0.52 / 1.000 / 0.000 |
| 1.0 | 8/32 | 0.000 | 1.00 / 0.000 / **1.000** | 0.00 / — / 0.000 (full abstention) |

The first-vs-second margin is *blind* to substitution: the twin scores high, runners-up are random, so τ-gating answers with maximal confidence and maximal error (cwr 0.48–1.00). Two further arms (`r11_gateh2h_run2`) sharpen the boundary: (i) an **episodic self-log** — facts the agent itself recorded at LEARN time — fails *identically* to the margin gate (cov 1.0, cwr 0.5), because post-hoc world corruption retroactively corrupts the evidentiary provenance of everything the agent stored; and (ii) forward+inverse consistency through a single XOR store is algebraically a round-trip (unbind of the retrieved value returns the row's own key), so such self-consistency gates flag nothing (measured coverage 0.0). Only evidence *not derivable from the retrieval path* works: independent permuted-view decoding and environment verification both achieve cov = clean-fraction, acc 1.0000, cwr 0.0000. **Uncertainty in HDC must be evidence-dispersal across independent channels — not single-index rank gaps, and not re-reading your own memory.**

**R7 — GHRR ordered path reasoning (`r9_ghrrpath_run3`).** PathHD-style 2-hop chains encoded as left-ordered matrix products P = R₂R₁S (D=256, m=2), retrieved by NN + margin gate + entity-type check — a fully deterministic selector in place of the published method's LLM adjudication: accuracy **1.0000 at coverage 1.0** (mean margin 0.94, min 0.92 over 150 chains); relation-order semantics quantified: correct cue sim 1.0000 vs swapped-relation cue 0.2476.

**R8 — Persistence.** Two-session CLI test (`ge_cli_run1`): fresh process replays append-only log (identical seed + interning order), answers both known queries with provenance, still abstains on the never-learned one.

## 5 Negative results (first-class)

- **N1 (M-1)**: bundling k XOR pairs then unbinding recovers essentially nothing (top1 ≤0.03 for k≥8; k=512,D=4096 → 0.004). Cleanup against candidates is the only safe readout of superpositions.
- **N2**: additive near-duplicate distractor rows never displace an exact-key NN (`r7b`) — interference ≠ substitution; memory *absence*, not crowding, creates the dangerous regime.
- **N3**: margin-gate blindness to substitution (R6b). We report the gate's confidently-wrong rate rather than hiding it behind a threshold tuned on benign noise.

## 6 Design notes with teeth

The distinct-key margin fix: replayed/learned duplicates of the top match are corroboration; counting them as "second candidate" zeroes every margin and collapses the gate (found via gridworld, where all transitions pruned). One-line semantic change, measurable consequences — retrieval *uncertainty semantics* deserve their own ablation in VSA toolkits.

## 7 Threats to validity

All domains are synthetic generators with unique keys by construction; claims are ceilings-relative and generator-scoped. No claim of open-domain language competence is made or implied (that gate already failed elsewhere). Single machine, `-O2`, one process. Latency figures exclude OS jitter bounds we did not measure.

## 8 Reproducibility

`make CC="zig cc" test` builds/runs all suites; `bash build.sh` equivalent on MSYS. Determinism: identical digests across reruns (checked). Artifacts: results/{ga_run1, gb_store_run2, gc_qa_run2, gd_grid_run1, ge_cli_run1, r7_hardqa_run2, r7b_advqa_run1, r7c_subst_run1, r8_capcurve_run2}.txt. Pre-registration `e896a62`; findings log append-only in `findings.md`; limitations enumerated in `LIMITATIONS.md` (12 measured items).

## 9 Conclusion

A few thousand lines of C11 with XOR and a linear scan already give: exact-margin relational QA, oracle-optimal recombining planning, >81k-entry noise-tolerant storage, persistent online learning — and a precise, quantified boundary where associative confidence stops being trustworthy. That boundary (rank gaps survive noise but not substitution; independent-view consistency restores it at coverage cost) is, we believe, the paper's most portable finding.
