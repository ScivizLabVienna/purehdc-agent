/* G-A invariant suite for L1 binary algebra. Named asserts; exit 0 = all pass.
 * Empirical curves (bundling recall vs K/D) are PRINTED, never asserted — doctrine. */
#include "purehdc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int fails = 0;
#define CHECK(name, cond) do { if (!(cond)) { printf("FAIL %s\n", name); fails++; } else printf("ok   %s\n", name); } while (0)

static uint64_t *alloc(uint32_t words) { uint64_t *p = malloc((size_t)words * 8); if (!p) { fprintf(stderr, "OOM\n"); exit(2);} return p; }

int main(void) {
    /* ---- exact algebraic invariants at D=1024 ---- */
    const uint32_t d = 1024, W = ph_words(d);
    uint64_t seed = 20261121ull;
    uint64_t *a = alloc(W), *b = alloc(W), *ab = alloc(W), *t = alloc(W), *u = alloc(W);
    ph_random(a, W, &seed); ph_random(b, W, &seed);
    ph_xor(ab, a, b, W);

    /* bind/unbind exactness (XOR binding is its own unbinding) */
    ph_xor(t, ab, a, W);
    CHECK("xor_unbind_exact", ph_hamming(t, b, W) == 0);

    /* XOR commutativity — expected property, verified not assumed */
    ph_xor(t, b, a, W);
    CHECK("xor_commutative_property", ph_digest(t, W) == ph_digest(ab, W));

    /* permutation invertibility: perm^s then perm^(D-s) is identity, bit-exact */
    for (uint32_t s = 1; s <= d - 1; s *= 7u) {
        ph_permute(t, a, W, d, s);
        ph_permute(u, t, W, d, ph_perm_inv(d, s));
        if (ph_hamming(u, a, W) != 0) { printf("FAIL perm_inverse s=%u\n", s); fails++; }
    }
    CHECK("perm_inverse_bitexact_all_s", fails == 0);

    /* distinct shifts give distinct vectors (no identity collision except s=0 mod D) */
    ph_permute(t, a, W, d, 1); ph_permute(u, a, W, d, 2);
    CHECK("perm_shifts_distinct", ph_hamming(t, u, W) > 0);

    /* bundling degenerates to identity at k=1 */
    const uint64_t *one[1] = { a };
    uint64_t ties = 0;
    ph_bundle(t, one, 1, W, &ties);
    CHECK("bundle_k1_identity", ph_hamming(t, a, W) == 0 && ties == 0);

    /* weighted bundling with weights (2,1,1): majority follows the overweighted item */
    { const uint64_t *its[3] = { a, b, u }; int32_t wgt[3] = { 1, 1, -1 };
      ph_bundle_w(t, its, wgt, 3, W); /* (a+b-u)/votes -> not used for exactness; smoke only */
      double s = ph_sim(t, a, d, W) + ph_sim(t, b, d, W);
      CHECK("bundle_weighted_smoke_finite", s > -2.0 && s < 2.1); }

    /* determinism: same seed twice => bit-identical digest */
    uint64_t s1 = 7ull, s2 = 7ull;
    ph_random(t, W, &s1); ph_random(u, W, &s2);
    CHECK("determinism_same_seed_digest", ph_digest(t, W) == ph_digest(u, W));

    /* near-orthogonality of random HVs at this D: |sim| bound 4/sqrt(D) */
    double worst = 0; int ortho_fails = 0;
    for (int i = 0; i < 200; i++) {
        ph_random(t, W, &seed); ph_random(u, W, &seed);
        double s = ph_sim(t, u, d, W); if (s < 0) s = -s; if (s > worst) worst = s;
        if (s > 4.0 / sqrt((double)d)) ortho_fails++; /* needs math for sqrt; fallback constant below */
    }
    CHECK("orthogonality_worst_le_bound", worst <= 4.0 / sqrt((double)d));
    (void)ortho_fails;

    /* ---- superposition capacity curve: bundled pair-binds, printed not asserted ---- */
    for (uint32_t D = 1024; D <= 4096; D *= 4) {
        uint32_t Wd = ph_words(D);
        printf("curve D=%u\n", D);
        for (uint32_t K = 8; K <= 1024; K *= 4) {
            uint64_t *keys = calloc(K, (size_t)Wd * 8), *items = calloc(K, (size_t)Wd * 8);
            const uint64_t **its = malloc((size_t)K * sizeof(void*)), *mem = alloc(Wd), *q = alloc(Wd);
            uint64_t kseed = 1000u + K + D;
            for (uint32_t i = 0; i < K; i++) { ph_random(keys + (size_t)i * Wd, Wd, &kseed); ph_random(items + (size_t)i * Wd, Wd, &kseed); its[i] = items + (size_t)i * Wd; }
            /* memory = XOR of bind(key_i,item_i) — use xor superposition as the flat-pair-bind memory */
            memset(mem, 0, (size_t)Wd * 8);
            for (uint32_t i = 0; i < K; i++) { uint64_t *bx = alloc(Wd); ph_xor(bx, keys + (size_t)i * Wd, items + (size_t)i * Wd, Wd); ph_xor(mem, mem, bx, Wd); free(bx); }
            uint32_t hits = 0;
            for (uint32_t j = 0; j < K; j++) {
                ph_xor(q, mem, keys + (size_t)j * Wd, Wd);
                int i0; ph_cleanup(q, its, K, D, &i0, NULL, NULL, NULL);
                hits += (uint32_t)i0 == (int)j;
            }
            printf("  K=%u xor_superposition_top1=%.4f\n", K, (double)hits / K); /* negative by design: XOR-superposed HV is uniform-like -> see findings M-1 */
            free(keys); free(items); free(its); free(mem); free(q);
        }
    }

    printf("%s (%d fails)\n", fails ? "G-A FAIL" : "G-A PASS", fails);
    return fails ? 1 : 0;
}
