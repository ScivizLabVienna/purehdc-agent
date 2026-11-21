#!/usr/bin/env bash
# G-E persistence gate: session1 learns+logs, session2 (fresh process) recalls + abstains.
set -u
cd "$(dirname "$0")/.."
rm -f agent.hlog
printf 'LEARN room_1 contains box_a\nLEARN box_a color red\nLEARN room_2 contains box_b\nQUERY room_1 contains\nQUERY moon orbits\nBYE\n' | ./bin/purehdc-agent.exe results/ge_cli_s1.hlog > results/ge_cli_s1.txt 2>&1; r1=$?
printf 'QUERY room_1 contains\nQUERY box_a color\nQUERY room_1 containsX\nBYE\n' | ./bin/purehdc-agent.exe results/ge_cli_s1.hlog > results/ge_cli_s2.txt 2>&1; r2=$?
echo "rc s1=$r1 s2=$r2"
grep -c '^ANSWER' results/ge_cli_s1.txt | sed 's/^/session1 answers=/g'
grep -c '^ABSTAIN' results/ge_cli_s1.txt | sed 's/^/session1 abstains=/g'
grep -c '^ANSWER' results/ge_cli_s2.txt | sed 's/^/session2 answers(after restart)=/g'
if grep -q 'ANSWER room_1 |contains| box_a' results/ge_cli_s2.txt && grep -q '^ABSTAIN' results/ge_cli_s1.txt && grep -q '^ABSTAIN' results/ge_cli_s2.txt; then echo "G-E PERSISTENCE-PASS"; else echo "G-E FAIL"; exit 1; fi
