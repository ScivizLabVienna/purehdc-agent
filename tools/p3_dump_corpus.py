# tools/p3_dump_corpus.py — freeze P2c corpus+queries to TSV for the C runtime port.
src = open("tools/p2_ri_eval2.py").read().split('def bm25_scores')[0]
exec(src)  # gives FACTS, FT, BRIDGE, QUERIES, D, SYN...
with open("results/p3_corpus.tsv", "w") as f:
    f.write(f"#meta D={D} NZ=8 NF={len(FT)} NQ={len(QUERIES)}\n")
    for t in FT:      f.write("F\t" + " ".join(t) + "\n")
    for t in BRIDGE:  f.write("B\t" + " ".join(t) + "\n")
    for q, gold, kind, *rest in QUERIES:
        g = ",".join(map(str, sorted(gold))) if isinstance(gold, set) else str(gold)
        c = ",".join(map(str, sorted(rest[0]))) if rest else "-"
        f.write(f"Q\t{kind}\t{g}\t{c}\t" + " ".join(q) + "\n")
print("dumped", len(FT), "facts", len(BRIDGE), "bridges", len(QUERIES), "queries")
