/* G-D world model + planning (spec §5). 8x8 grid, actions N/S/E/W.
 * ONE-PASS learned transitions: row key = bind(state,action) -> next_state HV.
 * Planner: BFS over PREDICTED successors only (expansion budget = |S|; bounded by construction),
 * unknown-transition handling via retrieval margin + decode confidence -> fail = report UNKNOWN.
 * Baselines/metrics: exact symbolic BFS oracle lengths (gap), full vs 15%-missing model stress arm,
 * deterministic seeds for everything. */
#include "store.h"
#include "purehdc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIDE 8u
#define NS 64u
#define LOGMAX 40000u
static const char *G[SIDE] = {"........","..#.....","....#...",".....##.",".#......","........","...#....","........"};
static int blocked(uint32_t s) { return G[s / SIDE][s % SIDE] == '#'; }
static uint32_t step_env(uint32_t s, uint32_t a) { /* N0 S1 E2 W3; bump = self-loop */
    uint32_t x = s % SIDE, y = s / SIDE;
    if (a == 0 && y > 0 && !blocked(s - SIDE)) s -= SIDE;
    if (a == 1 && y + 1 < SIDE && !blocked(s + SIDE)) s += SIDE;
    if (a == 2 && x + 1 < SIDE && !blocked(s + 1)) s += 1;
    if (a == 3 && x > 0 && !blocked(s - 1)) s -= 1;
    return s; }

static uint32_t decode(const uint64_t *q, const uint64_t *pool, uint32_t W, uint32_t D, double *bs) {
    double best = -2; uint32_t bi = 0;
    for (uint32_t e = 0; e < NS; e++) {
        uint32_t pop = 0; const uint64_t *p = pool + (size_t)e * W;
        for (uint32_t w = 0; w < W; w++) pop += __builtin_popcountll(q[w] ^ p[w]);
        double sim = 1.0 - 2.0 * pop / D; if (sim > best) { best = sim; bi = e; }
    }
    if (bs) *bs = best; return bi;
}
static uint32_t bfs_oracle(uint32_t s0, uint32_t g, uint32_t dist[NS]) {
    static uint32_t que[NS]; for (uint32_t i = 0; i < NS; i++) dist[i] = 1u << 31;
    uint32_t head = 0, tail = 0; que[tail++] = s0; dist[s0] = 0;
    while (head < tail) { uint32_t s = que[head++];
        for (uint32_t a = 0; a < 4; a++) { uint32_t t = step_env(s, a);
            if (t != s && dist[t] == (1u << 31)) { dist[t] = dist[s] + 1; que[tail++] = t; } } }
    return dist[g];
}

/* BFS over learned predictions. returns path length or -1 (caller must abstain/report UNKNOWN). */
static int plan_model(const store_t *wm, const uint64_t *sH, const uint64_t *aH, uint32_t W, uint32_t D, uint32_t s0, uint32_t g) {
    static uint32_t ppar[NS], pact[NS]; static uint8_t vis[NS]; static uint32_t que[NS];
    memset(vis, 0, sizeof vis); (void)pact;
    uint32_t head = 0, tail = 0; que[tail++] = s0; vis[s0] = 1; ppar[s0] = NS; /* sentinel */
    if (s0 == g) return 0;
    uint64_t *cue = calloc(W, 8), *tmp = calloc(W, 8);
    while (head < tail) { /* budget <= NS pops by construction */
        uint32_t s = que[head++];
        for (uint32_t a = 0; a < 4; a++) {
            ph_xor(cue, sH + (size_t)s * W, aH + (size_t)a * W, W);
            double m0, m1; int i0 = st_retrieve(wm, cue, tmp, &m0, &m1);
            if (i0 < 0 || m0 - m1 < 0.02) continue;                    /* unknown transition -> prune */
            double ds; uint32_t pred = decode(tmp, sH, W, D, &ds);
            if (ds < 0.85) continue;                                   /* low-confidence successor -> prune */
            if (!vis[pred]) { vis[pred] = 1; ppar[pred] = s; pact[pred] = a; que[tail++] = pred;
                if (pred == g) { free(cue); free(tmp);
                    uint32_t len = 0, x = g; while (ppar[x] != NS) { x = ppar[x]; len++; } return (int)len; } }
        }
    }
    free(cue); free(tmp); return -1;
}

int main(void) {
    const uint32_t D = 1024, W = ph_words(D);
    uint64_t seedg = 4242u;
    uint64_t *sH = calloc((size_t)NS * W, 8), *aH = calloc(4 * W, 8);
    for (uint32_t i = 0; i < NS; i++) ph_random(sH + (size_t)i * W, W, &seedg);
    for (uint32_t i = 0; i < 4; i++)   ph_random(aH + (size_t)i * W, W, &seedg);

    /* replay with persistent log (enables stress rebuild) */
    static uint8_t os[LOGMAX], oa[LOGMAX], onx[LOGMAX];
    store_t wm; st_init(&wm, D, LOGMAX); store_t wm2; st_init(&wm2, D, LOGMAX);
    uint32_t s = 0, nlog = 0;
    for (uint64_t t = 0; t < LOGMAX; t++) {
        uint32_t a = (uint32_t)(ph_splitmix64_next(&seedg) % 4);
        uint32_t nx = step_env(s, a);
        uint64_t *key = calloc(W, 8); ph_xor(key, sH + (size_t)s * W, aH + (size_t)a * W, W);
        st_put(&wm, key, sH + (size_t)nx * W); free(key);
        os[nlog] = (uint8_t)s; oa[nlog] = (uint8_t)a; onx[nlog] = (uint8_t)nx; nlog++;
        s = nx;
    }
    /* stress model: drop 15% of distinct keys (deterministic digest rule) */
    uint64_t *ks = calloc(W, 8);
    for (uint32_t i = 0; i < nlog; i++) {
        ph_xor(ks, sH + (size_t)os[i] * W, aH + (size_t)oa[i] * W, W);
        uint64_t dg = ph_digest(ks, W);
        if ((dg % 100u) < 15u) continue;
        st_put(&wm2, ks, sH + (size_t)onx[i] * W);   /* copy made inside st_put */
    }
    free(ks);

    /* eval trials vs oracle */
    uint32_t dist[NS];
    for (int arm = 0; arm < 2; arm++) {
        store_t *M = arm ? &wm2 : &wm;
        uint32_t trials = 0, ok = 0, gapmax = 0; double gapsum = 0; uint32_t lensum = 0;
        for (uint32_t st0 = 0; st0 < NS; st0++) {
            if (blocked(st0) || st0 % 7u) continue;
            uint32_t g = (st0 * 13u + 11u) % NS; if (blocked(g)) continue;
            uint32_t opt = bfs_oracle(st0, g, dist); if (opt == (1u << 31)) continue; trials++;
            int len = plan_model(M, sH, aH, W, D, st0, g);
            if (len >= 0) { ok++; uint32_t gap = ((uint32_t)len > opt) ? (uint32_t)len - opt : 0; gapsum += gap; if (gap > gapmax) gapmax = gap; lensum += (uint32_t)len; }
        }
        printf("G-D arm=%s trials=%u success=%.4f mean_gap=%.2f max_gap=%u mean_len=%.1f  (failures -> abstain UNKNOWN)\n",
               arm ? "model-85%" : "model-full", trials, (double)ok / trials, ok ? gapsum / ok : -1.0, gapmax, ok ? (double)lensum / ok : 0.0);
    }
    st_free(&wm); st_free(&wm2); free(sH); free(aH);
    printf("G-D RUN COMPLETE\n");
    return 0;
}
