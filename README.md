# PURE-HDC Agent (E18)

**Published**: v1.0.1 DOI [10.5281/zenodo.24477493](https://doi.org/10.5281/zenodo.24477493) · concept DOI [10.5281/zenodo.24462795](https://doi.org/10.5281/zenodo.24462795) (PDF + code bundle) — source repo mirror: https://github.com/ScivizLabVienna/purehdc-agent

A minimal, fully operational agent built **entirely** from hyperdimensional computing
(binary HV algebra), a flat-scan topological associative memory, and bounded deterministic
control. No neural networks, no gradients, no GPU, no learned tokenizer, no LLM calls,
no external vector DB. Pure C11, CPU-only, deterministic (fixed seeds).

## What it does (all demonstrated by runnable benchmarks)

| Capability | Artifact | Headline result |
|---|---|---|
| Reversible compositional algebra (XOR bind/unbind, permute, bundle, cleanup) | `bin/test_algebra` | 26 invariants PASS incl. negative controls |
| Genuine GHRR (complex U(m) matrix binding, m=1/2/4) | `bin/test_ghrr` | unitary defect ~1e-16; noncommutative; bundling non-unitarity **recorded** |
| Byte n-gram SDR encoding (no learned vocab) | `bin/test_sdr` | order-sensitive (swap Jaccard 0.75 ≠ bag) |
| Associative store @40k entries, noisy cues | `bin/bench_store` | R@1 = 1.000; hashmap-with-hash-HV control collapses to 0.0 |
| Multi-hop relational QA (10-hop chains) | `bin/qa_kg` | acc 1.000 at every hop depth (= ceiling); abstain on unseen relations |
| Learned-transition planning (Gridworld) | `bin/gridworld` | gap 0 vs exact BFS oracle; 15% missing transitions → only detours, never failure |
| Persistent CLI agent (observe/learn/query/abstain/restart) | `bin/purehdc-agent` + `experiments/cli_e2e.sh` | cross-restart recall + calibrated abstention |
| Capacity curve D∈{256..2048} | `bin/cap_curve` | see `results/r8_capcurve_run1.txt` |
| Uncertainty calibration ablations | `bin/hard_qa`, `bin/adversarial_qa`, `bin/subst_qa` | margin gate survives channel noise but **cannot** detect row substitution (cwr≤0.5); dual independent view restores cwr=0 at cost of coverage |

## Build & run

```sh
# any C11 compiler; on this rig: zig cc 0.17
make CC="zig cc" test          # unit suites
make CC="zig cc"               # all benchmarks + agent
make CC="zig cc" demo          # one-command end-to-end demo (spec §11 deliverable 5)
bash build.sh                  # Windows/MSYS one-shot alternative (verified)
./bin/purehdc-agent memory.hlog  # interactive; commands: LEARN s r o | QUERY s r | BYE
```

Figures: `python3 scripts/make_figs.py` (stdlib-only) regenerates `figures/*.svg` from committed `results/`.

## Layout

`include/` public headers · `src/algebra|encoding|tam|agent/` runtime ·
`tests/` invariant suites · `experiments/` benchmarks · `results/` raw run outputs (append-only) ·
`findings.md` running research log · `task_plan.md` pre-registration + TODO.

## Reproducibility

Every experiment is deterministic from fixed splitmix64 seeds; repeated runs are
byte-identical (verified by digest). Raw outputs live in `results/` and are never edited,
only appended to. Negative results are first-class outputs — see LIMITATIONS.md.
