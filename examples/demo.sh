#!/usr/bin/env bash
# Spec §11 deliverable 5 — one-command reproducible end-to-end demo:
# observe+learn (CLI s1) → multi-hop inference (qa_kg) → plan over learned transitions (gridworld)
# → act + persistent recall + calibrated abstention (CLI s2). Deterministic seeds; exits non-zero on any miss.
set -u
cd "$(dirname "$0")/.."
B=bin; fail=0
ck(){ if grep -qE "$2" "$1"; then echo "  OK   $3"; else echo "  MISS $3"; fail=1; fi; }

echo "[1/4] observe + learn (fresh agent session)"
rm -f /tmp/purehdc_demo.hlog results/demo_s1.txt
printf 'LEARN room_1 contains box_a\nLEARN box_a color red\nQUERY room_1 contains\nBYE\n' | ./$B/purehdc-agent.exe /tmp/purehdc_demo.hlog > results/demo_s1.txt 2>&1
ck results/demo_s1.txt '^ANSWER room_1 \|contains\| box_a' "learns and answers in-session (confidence+provenance)"

echo "[2/4] multi-hop inference, 10 hops"
./$B/qa_kg.exe > results/demo_qa.txt 2>&1; ck results/demo_qa.txt '^10[[:space:]]+1\.0000' "hop-10 accuracy 1.000"

echo "[3/4] planning over one-pass learned transitions"
./$B/gridworld.exe > results/demo_grid.txt 2>&1; ck results/demo_grid.txt 'model-full.*success=1\.0000' "planner success 1.00 (full knowledge)"

echo "[4/4] restart: persistent recall + abstain on unknown"
printf 'QUERY box_a color\nQUERY mars atmosphere\nBYE\n' | ./$B/purehdc-agent.exe /tmp/purehdc_demo.hlog > results/demo_s2.txt 2>&1
ck results/demo_s2.txt '^ANSWER box_a \|color\| red'  "cross-restart recall"
ck results/demo_s2.txt '^ABSTAIN'                     "abstains on never-learned fact"

[ $fail -eq 0 ] && echo "DEMO COMPLETE — pure HDC agent: learn → infer → plan → act → persist → abstain, zero neural components." || echo "DEMO FAIL"; exit $fail
