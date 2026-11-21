#!/usr/bin/env bash
# Build+run G-A/G-B suites with the portable S:/tools compiler. $0.
set -e
cd "$(dirname "$0")"
ZIG=$(ls /s/tools/zig-x86_64-windows-*/zig.exe 2>/dev/null | head -1)
CXX="${CC:-$ZIG cc}"
CFLAGS="-O2 -std=c11 -Wall -Iinclude"
mkdir -p bin
$CXX $CFLAGS -o bin/test_algebra.exe tests/test_algebra.c src/algebra/hv_binary.c -lm
$CXX $CFLAGS -o bin/test_ghrr.exe   tests/test_ghrr.c   src/algebra/ghrr.c src/algebra/hv_binary.c -lm
$CXX $CFLAGS -o bin/test_sdr.exe     tests/test_sdr.c    src/encoding/sdr.c src/algebra/hv_binary.c -lm
$CXX $CFLAGS -o bin/bench_store.exe  experiments/bench_store.c src/tam/store.c src/algebra/hv_binary.c -lm
$CXX $CFLAGS -o bin/qa_kg.exe        experiments/qa_kg.c    src/tam/store.c src/algebra/hv_binary.c -lm
$CXX $CFLAGS -o bin/gridworld.exe      experiments/gridworld.c   src/tam/store.c src/algebra/hv_binary.c -lm
$CXX $CFLAGS -o bin/purehdc-agent.exe  src/agent/main.c          src/tam/store.c src/algebra/hv_binary.c -lm
$CXX $CFLAGS -o bin/hard_qa.exe         experiments/hard_qa.c     src/tam/store.c src/algebra/hv_binary.c -lm
$CXX $CFLAGS -o bin/adversarial_qa.exe experiments/adversarial_qa.c src/tam/store.c src/algebra/hv_binary.c -lm
$CXX $CFLAGS -o bin/subst_qa.exe     experiments/subst_qa.c      src/tam/store.c src/algebra/hv_binary.c -lm
$CXX $CFLAGS -o bin/cap_curve.exe     experiments/cap_curve.c       src/tam/store.c src/algebra/hv_binary.c -lm
$CXX $CFLAGS -o bin/subst_gates2.exe    experiments/subst_gates2.c      src/tam/store.c src/algebra/hv_binary.c -lm
echo BUILT
