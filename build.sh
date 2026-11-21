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
echo BUILT
