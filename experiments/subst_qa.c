/* r7c SUBSTITUTION-CORRUPTION QA + dual-view verification (spec L4.5 "dispersion across independently encoded evidence").
 * Setup: facts stored in view A key=S^R and independent view B key=perm1(R)^perm2(S).
 * For p-erased subjects, VIEW A's true row is REPLACED by a mutated twin (decoy value) — corruption, not noise; view B intact.
 * Arms: always-A | margin-gate-A | dual-view-consistent(A==B decode). Expectation from r7b: margin gate CANNOT detect
 * substitution (twin scores high, second-best random) -> dual evidence does. Deterministic seeds. */
#include "store.h"
#include "purehdc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NSUBJ 100u
#define NREL 20u
#define NOBJ 300u
static const uint32_t D = 1024u, W = D / 64;
static uint32_t val[NSUBJ][NREL];
static uint64_t *HV, *sHp, *rHp;   /* permuted views */
#define SJ(i) (HV + (size_t)(i) * W)
#define RL(i) (HV + (size_t)(NSUBJ + (i)) * W)
#define OB(i) (HV + (size_t)(NSUBJ + NREL + (i)) * W)

static uint32_t dec(const uint64_t *got) {
    double best = -2; int bi = -1;
    for (uint32_t e = 0; e < NOBJ; e++) { uint32_t pop = 0; const uint64_t *p = OB(e);
        for (uint32_t w = 0; w < W; w++) pop += __builtin_popcountll(got[w] ^ p[w]);
        double sim = 1.0 - 2.0 * pop / D; if (sim > best) { best = sim; bi = (int)e; } }
    return bi >= 0 && best > 0.85 ? (uint32_t)bi : 0xffffffffu;
}

int main(void) {
    HV  = calloc(((size_t)NSUBJ + NREL + NOBJ) * W, 8);
    sHp = calloc((size_t)NSUBJ * W, 8); rHp = calloc((size_t)NREL * W, 8);
    uint64_t hs = 8080ull;
    for (uint32_t i = 0; i < NSUBJ + NREL + NOBJ; i++) ph_random(HV + (size_t)i * W, W, &hs);
    for (uint32_t i = 0; i < NSUBJ; i++) ph_permute(sHp + (size_t)i * W, SJ(i), W, D, 1u);
    for (uint32_t i = 0; i < NREL;  i++) ph_permute(rHp + (size_t)i * W, RL(i), W, D, 7u);
    printf("p_erase\tdelta\talwaysA_acc\tgateA(.10) cov/acc/cwr\tdual cov/acc/cwr   (TEST)\n");
    for (double p_e = 0.0; p_e <= 1.01; p_e += 0.5) {
      for (uint32_t delta = 8; delta <= 32; delta += 24) {
        uint64_t seed = 31337ull + (uint32_t)(p_e * 100) * 977ull + delta;
        store_t A, B; st_init(&A, D, NSUBJ * NREL * 2u); st_init(&B, D, NSUBJ * NREL);
        uint8_t erased[NSUBJ];
        for (uint32_t s = 0; s < NSUBJ; s++) { uint64_t u = ph_splitmix64_next(&seed); erased[s] = ((double)(u >> 11) / 9007199254740992.0) < p_e; }
        for (uint32_t s = 0; s < NSUBJ; s++) for (uint32_t r = 0; r < NREL; r++) {
            uint32_t k = (uint32_t)(ph_splitmix64_next(&seed) % NOBJ); val[s][r] = k;
            uint64_t *ka = calloc(W, 8), *kb = calloc(W, 8); ph_xor(ka, SJ(s), RL(r), W); ph_xor(kb, rHp + r * W, sHp + s * W, W);
            if (erased[s]) {   /* view A corrupted: true row absent, mutated decoy-twin present */
                uint64_t *tw = calloc(W, 8); memcpy(tw, ka, (size_t)W * 8);
                for (uint32_t b = 0; b < delta; b++) { uint64_t w = ph_splitmix64_next(&seed) % W; tw[w] ^= 1ull << (ph_splitmix64_next(&seed) & 63u); }
                st_put(&A, tw, OB((k + 1u + (uint32_t)(ph_splitmix64_next(&seed) % (NOBJ - 1u))) % NOBJ)); free(tw);
            } else st_put(&A, ka, OB(k));
            st_put(&B, kb, OB(k)); free(ka); free(kb);
        }
        uint64_t tot = 0, oka = 0;
        struct { uint64_t cov, acc, cw; } g = {0}, du = {0};
        for (uint32_t s = 0; s < NSUBJ; s++) { if (s % 2u == 0) continue;             /* TEST = odd subjects */
            for (uint32_t r = 0; r < NREL; r++) {
                uint64_t *cueA = calloc(W, 8), *cueB = calloc(W, 8), *gA = calloc(W, 8), *gB = calloc(W, 8);
                ph_xor(cueA, SJ(s), RL(r), W); ph_xor(cueB, rHp + r * W, sHp + s * W, W);
                double m0, m1; st_retrieve(&A, cueA, gA, &m0, &m1); st_retrieve(&B, cueB, gB, NULL, NULL);
                uint32_t oa = dec(gA), ob = dec(gB); int correctA = (oa == val[s][r]);
                tot++; oka += (uint64_t)correctA;
                if (m0 - m1 > 0.10) { g.cov++; if (correctA) g.acc++; else g.cw++; }              /* gate on view A only */
                if (oa != 0xffffffffu && oa == ob) { du.cov++; if (oa == val[s][r]) du.acc++; else du.cw++; }  /* dual consistency */
                free(cueA); free(cueB); free(gA); free(gB);
            } }
        printf("%.1f\t%u\t%.4f\t%.3f/%.4f/%.4f\t%.3f/%.4f/%.4f\n", p_e, delta, (double)oka / tot,
               (double)g.cov / tot, g.cov ? (double)g.acc / g.cov : -1.0, (double)g.cw / tot,
               (double)du.cov / tot, du.cov ? (double)du.acc / du.cov : -1.0, (double)du.cw / tot);
        st_free(&A); st_free(&B);
      }
    }
    free(HV); free(sHp); free(rHp);
    printf("SUBST-QA RUN COMPLETE\n");
    return 0;
}
