# LIMITATIONS — measured, not hand-waved (E18)

Every item below is backed by a deterministic run in `results/`.

## L1 algebra / memory
1. **XOR-superposition recall is negative by design (M-1).** Bundling k pairs then
   unbinding the whole bundle returns cleanup against stored candidates, not the items —
   superposition noise grows with k. Correct usage is one-pair-at-a-time unbinding or
   cleanup over a candidate set. Recorded from round 1.
2. **GHRR bundled matrices are not unitary.** Unitarity defect measured ≈2.4 after
   bundling 5 random terms (generators themselves: ~4e-16). Unbinding through bundles is
   approximate; we never claim otherwise. GHRR cost is O(D·m³) naive.
3. **Flat-scan store is O(N·W) per query.** Fine to the sizes measured (40k rows p50
   ≤0.2 ms at D=1024); it is *not* a sublinear index. The hashmap control was included
   precisely to show what we are and are not buying.

## L3 uncertainty — the central negative result
4. **Channel noise cannot produce confident errors** in binary XOR NN with unique keys:
   always-top1 stayed at acc=1.0 under 12.5% cue bit-flips (r7). The margin gate's value
   there is coverage control, not error prevention.
5. **First-vs-second margin CANNOT detect row substitution.** With true rows replaced by
   mutated twins (decoy values), the gate returned cov=1.0 with confidently-wrong rate up
   to 0.50 — the twin scores high and the second-best is random, so the margin looks safe
   (r7c). *A single retrieval view's uncertainty is blind to corruption of that view.*
6. **Dual independent views fix it only via coverage.** Requiring both permuted-binding
   views to decode the same object: cwr→0.0000, but answers only survive where the
   corrupted view agrees — coverage fell to the clean fraction (0.52 at p_erase=0.5; 0 at
   p_erase=1). Detection-by-agreement abstains; it does not repair. No mechanism in this
   architecture distinguishes "twin" from "true" without a second source of evidence.

## L4/L5 reasoning & control
7. **Multi-hop accuracy is ceiling-limited.** In the synthetic 10-hop chain every hop was
   already solved at hop 1 (acc 1.0 = task ceiling); this demonstrates non-degradation,
   not super-baseline reasoning. The rel-swap negative control (sim −0.02) shows direction/role
   binding does work; it is not evidence of generalization beyond the generator grammar.
8. **Gridworld planning recombines learned transitions, not world physics.** Gap 0 vs BFS
   oracle holds only when every needed bind(state,action) row was observed in replay;
   with 15% of keys absent, plans remain goal-reaching but lengthen (mean gap 0.5, max 3).
9. **Persistence = deterministic log replay.** Identity across restarts requires identical
   bytes interned in identical order (same seed); binary snapshot save/load is not implemented
   because replay latency has not yet measured as a bottleneck at our N.
10. **Novelty detection is symbol-level** (interned-string freshness), not semantic: no
    open-domain paraphrase robustness is claimed anywhere (byte n-grams are order-sensitive
    but vocabulary-blind by construction).

## Scope exclusions (pre-registered, E18 §1)
11. Open-domain episodic QA vs bm25 is **out of scope** — that gate already failed in E17
    (TAM 5.0% vs bm25 54.3%, binding negative result); re-gating it here would be dishonest.
12. Stage B–D language (phrase retrieval, constrained composition, muse mode) is not
    implemented; only Stage A deterministic rendering exists. The paper claim set does not
    cover Stages B–D.

## Addendum — rounds r9–r11 (measured)
13. **GHRR path unbind is right-peel only.** `ghrr_unbind(A,B)=A·Bᴴ` recovers the *last*
    bound factor; a left-bound path P=R₂R₁S cannot yield S through the current API. The r9
    selector therefore verifies decode structurally via entity-type check (nearest stored
    entity sim > 0.9) rather than algebraic round-trip. A general peel is an API gap, not a
    demonstrated capability.
14. **Ordered path reasoning has a dimension floor.** At D=64 with 200 chains hop-2 acc
    collapsed to 0.575; at D=256 with 150 *unique* (relation-pair, start) keys it is 1.0 @
    cov 1.0. Duplicate keys produce margin-0 ties that a margin gate cannot break — key
    uniqueness is an architectural precondition of the confidence signal, not an implementation detail.
15. **An agent's own episodic log is not independent evidence.** Post-hoc substitution of
    world rows also corrupts what the log proves (r11 self-log arm: cwr 0.5, identical to no
    gate). Any verification channel must be non-derivable from the retrieval path — see §R6.
