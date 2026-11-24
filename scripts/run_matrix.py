#!/usr/bin/env python3
"""E19 Phase-0 reproduction matrix: wall time + peak RSS for every benchmark binary.
Research tooling (Python outside the agent runtime, per v2 mandate)."""
import subprocess, sys, time, psutil, platform, os, hashlib, glob

BINS = ["subst_qa","hard_qa","path_ghrr","bench_store","cap_curve","gridworld",
        "adversarial_qa","qa_kg","subst_gates2"]  # seeds_sweep excluded (long; run separately)

def peak_rss(cmd):
    t0 = time.perf_counter()
    p = psutil.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    peak = 0
    while p.poll() is None:
        try: peak = max(peak, p.memory_info().rss)
        except psutil.NoSuchProcess: break
        time.sleep(0.02)
    return (time.perf_counter() - t0, p.returncode, peak / 1e6)

print(f"# host={platform.node()} os={platform.platform()} py={platform.python_version()}")
for b in BINS:
    src = os.path.join("bin", b)
    if not os.path.exists(src): print(f"{b}: MISSING"); continue
    exe = os.path.join("bin", b + ".exe")  # Win CreateProcess refuses extension-less names
    import shutil; shutil.copyfile(src, exe)
    t, rc, mb = peak_rss([exe])
    print(f"{b:16s} {t*1000:9.1f} ms  rc={rc}  rssMax~20ms={mb:7.1f} MB")
