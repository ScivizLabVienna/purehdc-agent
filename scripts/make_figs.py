#!/usr/bin/env python3
"""Figures for E18 — stdlib-only SVG from committed results/ artifacts. Offline orchestration only (spec-allowed)."""
import os
R = os.path.join(os.path.dirname(__file__), "..", "results")
OUT = os.path.join(os.path.dirname(__file__), "..", "figures")
os.makedirs(OUT, exist_ok=True)

def rows(name):
    out = []
    for ln in open(os.path.join(R, name), encoding="utf-8").read().splitlines():
        if not ln.strip() or ln.lower().startswith(("d\t", "p_erase")) or "complete" in ln.lower() or not ln[0].isdigit(): continue
        out.append(ln.split("\t"))
    return out

def svg(w, h, body, title):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" font-family="sans-serif">'
            f'<text x="12" y="20" font-size="14">{title}</text>' + body + "</svg>\n")

def lines(name, dvals, xlabel, ylabel):
    data = rows(name)
    series = {dv: [(float(r[1]), float(r[4])) for r in data if int(r[0]) == dv] for dv in dvals}
    allx = [x for s in series.values() for x, _ in s]; ally = [y for s in series.values() for _, y in s]
    W, H, ml, mb, mt = 520, 300, 55, 40, 34
    lo, hi = min(allx), max(allx); ymax = max(ally) * 1.05
    def X(v): return ml + (v - lo) / (hi - lo) * (W - ml - 12)
    def Y(v): return H - mb - v / ymax * (H - mb - mt)
    b = ""
    for y in (0, ymax / 2, ymax):
        b += f'<line x1="{ml}" y1="{Y(y):.0f}" x2="{W-12}" y2="{Y(y):.0f}" stroke="#999"/><text x="6" y="{Y(y)+4:.0f}" font-size="10">{y:.1f}</text>'
    for r in data:
        if int(r[0]) == dvals[0]: b += f'<text x="{X(float(r[1]))-8:.0f}" y="{H-mb+14}" font-size="9">{int(float(r[1])):,}</text>'
    cols = ["#2c7fb8", "#d95f0e", "#555"]
    for i, dv in enumerate(dvals):
        pts = " ".join(f"{X(x):.1f},{Y(y):.1f}" for x, y in series[dv])
        b += f'<polyline points="{pts}" fill="none" stroke="{cols[i]}" stroke-width="2"/>'
        b += f'<rect x="{W-105}" y="{mt+i*16}" width="10" height="3" fill="{cols[i]}"/><text x="{W-91}" y="{mt+i*16+5}" font-size="10">D={dv}</text>'
    return svg(W, H, b + f'<text x="{(ml+W)/2-40:.0f}" y="{H-8}" font-size="11">{xlabel}</text>',
               "Latency per query pair vs store size (R@1 = 1.0 on every point)")

def gates():
    data = rows("r11_gateh2h_run2.txt")[:2]   # p_erase=0.5 arms, delta 8 & 32
    names = ["margin", "self-log", "env-verify", "dual-view"]
    W, H, ml, mb, mt = 560, 260, 45, 55, 34
    b = ""
    for y in (0, .25, .5):
        yy = H - mb - y / .55 * (H - mb - mt)
        b += f'<line x1="{ml}" y1="{yy:.0f}" x2="{W-12}" y2="{yy:.0f}" stroke="#999"/><text x="6" y="{yy+4:.0f}" font-size="10">{y}</text>'
    for di, row in enumerate(data):
        base = ml + 30 + di * 250
        for gi, g in enumerate(range(2, 6)):
            cov, acc, cwr = (float(x) for x in row[g].split("/"))
            cwr = max(cwr, 0.0); hgt = cwr / .55 * (H - mb - mt)
            x = base + gi * 52
            b += (f'<rect x="{x}" y="{H-mb-hgt:.0f}" width="34" height="{hgt:.0f}" fill="#c0392b"/>' if cwr > 0 else
                  f'<rect x="{x}" y="{H-mb-3}" width="34" height="3" fill="#27ae60"/>')
            b += f'<text x="{x+17}" y="{H-mb-hgt-4:.0f}" font-size="9" text-anchor="middle">{cwr:.2f}</text>'
        b += f'<text x="{base+95}" y="{H-mb+16}" font-size="11" text-anchor="middle">p_erase=0.5, delta={row[1]} bits</text>'
    for gi, n in enumerate(names):
        b += f'<text x="{ml+30+gi*52+17}" y="{H-8}" font-size="9" text-anchor="middle">{n}</text>'
    return svg(W, H, b, "Confidently-wrong rate under substitution corruption (red = harm)")

open(os.path.join(OUT, "fig_latency.svg"), "w", encoding="utf-8").write(
    lines("r8_capcurve_run2.txt", [256, 1024, 2048], "entries N", "ms / query pair"))
open(os.path.join(OUT, "fig_gates.svg"), "w", encoding="utf-8").write(gates())
print("wrote", sorted(os.listdir(OUT)))
