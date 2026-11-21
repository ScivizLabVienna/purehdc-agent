/* r7 HARD QA ablation v2: dense fact base (200 subj x 40 rel = 8000 rows, values collide),
 * query-cue bit-flip noise, DEV/TEST parity split, gated vs always-answer arms.
 * Sweeps D in {256,512,1024} — capacity-vs-dimension curve where abstention must earn its keep. */
#include "store.h"
#include "purehdc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NSUBJ 200u
#define NREL 40u
#define NOBJ 400u
static uint32_t D, W;
static uint32_t val[NSUBJ][NREL];
static uint64_t *HV;
#define SJ(i) (HV + (size_t)(i) * W)
#define RL(i) (HV + (size_t)(NSUBJ + (i)) * W)
#define OB(i) (HV + (size_t)(NSUBJ + NREL + (i)) * W)

static void run_once(uint64_t seed0) {
    HV = realloc(HV, ((size_t)NSUBJ + NREL + NOBJ) * W * 8);
    uint64_t hs = seed0;
    for (uint32_t i = 0; i < NSUBJ + NREL + NOBJ; i++) ph_random(HV + (size_t)i * W, W, &hs);
    store_t mem; st_init(&mem, D, NSUBJ * NREL);
    uint64_t seed = seed0 ^ 0xBEEF01ull;
    for (uint32_t s = 0; s < NSUBJ; s++) for (uint32_t r = 0; r < NREL; r++) {
        uint32_t k = (uint32_t)(ph_splitmix64_next(&seed) % NOBJ); val[s][r] = k;
        uint64_t *key = calloc(W, 8); ph_xor(key, SJ(s), RL(r), W); st_put(&mem, key, OB(k)); free(key); }
    double taus[5] = {0.02, 0.05, 0.10, 0.15, 0.20};
    printf("== D=%u facts=%u ==\nflip%%\talways_acc\tgated cov/acc/cwr @tau .02/.05/.10/.15/.20 (TEST)\tDEV@.10\n", D, NSUBJ * NREL);
    for (uint32_t fb = 0; fb <= D / 8u; fb += D / 64u) {           /* 0 .. 12.5% of bits */
        struct { uint64_t cov, acc, cw; } g[5] = {{0}}, gd[5] = {{0}};
        uint64_t tot = 0, ok = 0, totd = 0, okd = 0;
        for (uint32_t s = 0; s < NSUBJ; s++) { int is_test = (int)(s % 2u);
            for (uint32_t r = 0; r < NREL; r++) {
                uint64_t *cue = calloc(W, 8), *got = calloc(W, 8);
                ph_xor(cue, SJ(s), RL(r), W);
                for (uint32_t b = 0; b < fb; b++) { uint64_t w = ph_splitmix64_next(&seed) % W; cue[w] ^= 1ull << (ph_splitmix64_next(&seed) & 63u); }
                double m0, m1; int i0 = st_retrieve(&mem, cue, got, &m0, &m1);
                double best = -2; int bi = -1;
                for (uint32_t e = 0; e < NOBJ; e++) { uint32_t pop = 0; const uint64_t *p = OB(e);
                    for (uint32_t w = 0; w < W; w++) pop += __builtin_popcountll(got[w] ^ p[w]);
                    double sim = 1.0 - 2.0 * pop / D; if (sim > best) { best = sim; bi = (int)e; } }
                int correct = (i0 >= 0 && (uint32_t)bi == val[s][r]);
                if (is_test) { tot++; ok += (uint64_t)correct; } else { totd++; okd += (uint64_t)correct; }
                for (uint32_t t = 0; t < 5; t++) {
                    int answer = (i0 >= 0 && m0 > 0.85 && m0 - m1 > taus[t] && best > 0.85);
                    if (!answer) continue;
                    if (is_test) { g[t].cov++; if (correct) g[t].acc++; else g[t].cw++; }
                    else         { gd[t].cov++; if (correct) gd[t].acc++; else gd[t].cw++; }
                }
                free(cue); free(got);
            } }
        printf("%.2f\t%.4f", 100.0 * fb / D, (double)ok / tot);
        for (uint32_t t = 0; t < 5; t++)
            printf("\t%.3f/%.4f/%.4f", (double)g[t].cov / tot, g[t].cov ? (double)g[t].acc / g[t].cov : -1.0, (double)g[t].cw / tot);
        uint32_t t2 = 2;
        printf("\t%.4f/%.4f\n", gd[t2].cov ? (double)gd[t2].acc / gd[t2].cov : -1.0, totd ? (double)gd[t2].cw / totd : -1.0);
    }
    st_free(&mem);
}

int main(void) {
    uint32_t Ds[3] = {256u, 512u, 1024u};
    for (int i = 0; i < 3; i++) { D = Ds[i]; W = ph_words(D); run_once(777001ull + (uint64_t)i * 7919ull); }
    free(HV);
    printf("HARD-QA RUN COMPLETE\n");
    return 0;
}
