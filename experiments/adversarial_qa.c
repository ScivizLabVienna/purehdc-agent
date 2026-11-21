/* r7b ADVERSARIAL QA: inject confusable distractor rows key' = true_key XOR mask(delta bits), val=decoy.
 * Channel noise alone never defeats binary XOR nearest-neighbour (r7 finding); MEMORY INTERFERENCE does.
 * Prediction tested: always-top1 becomes confidently wrong as delta->noise level; margin gate detects tie -> abstains.
 * D=1024, facts 200x40=8000, DEV/TEST parity split. */
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
static uint64_t *HV;
#define SJ(i) (HV + (size_t)(i) * W)
#define RL(i) (HV + (size_t)(NSUBJ + (i)) * W)
#define OB(i) (HV + (size_t)(NSUBJ + NREL + (i)) * W)

int main(void) {
    HV = calloc(((size_t)NSUBJ + NREL + NOBJ) * W, 8);
    uint64_t hs = 51423ull;
    for (uint32_t i = 0; i < NSUBJ + NREL + NOBJ; i++) ph_random(HV + (size_t)i * W, W, &hs);
    double taus[4] = {0.02, 0.05, 0.10, 0.15};
    printf("delta\tflip\talways_acc\tgated cov/acc/cwr @tau .02/.05/.10/.15 (TEST)\tDEV@.05\n");
    for (uint32_t delta = 4; delta <= 28; delta += 4) {      /* distractor key distance in bits */
        for (uint32_t fb = 0; fb <= 24; fb += 8) {           /* query cue noise bits */
            uint64_t seed = 991ull + delta * 131ull + fb * 7ull;
            store_t mem; st_init(&mem, D, NSUBJ * NREL * 2u);
            for (uint32_t s = 0; s < NSUBJ; s++) for (uint32_t r = 0; r < NREL; r++) {
                uint32_t k = (uint32_t)(ph_splitmix64_next(&seed) % NOBJ); val[s][r] = k;
                uint64_t *key = calloc(W, 8); ph_xor(key, SJ(s), RL(r), W);
                st_put(&mem, key, OB(k));
                {   /* twin with decoy value for EVERY fact */
                    uint64_t *tw = calloc(W, 8); memcpy(tw, key, (size_t)W * 8);
                    for (uint32_t b = 0; b < delta; b++) { uint64_t w = ph_splitmix64_next(&seed) % W; tw[w] ^= 1ull << (ph_splitmix64_next(&seed) & 63u); }
                    st_put(&mem, tw, OB((k + 1u + (uint32_t)(ph_splitmix64_next(&seed) % (NOBJ - 1u))) % NOBJ));
                    free(tw);
                }
                free(key);
            }
            struct { uint64_t cov, acc, cw; } g[4] = {{0}}, gd[4] = {{0}};
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
                    for (uint32_t t = 0; t < 4; t++) {
                        int answer = (i0 >= 0 && m0 - m1 > taus[t]);      /* pure margin gate this time */
                        if (!answer) continue;
                        if (is_test) { g[t].cov++; if (correct) g[t].acc++; else g[t].cw++; }
                        else         { gd[t].cov++; if (correct) gd[t].acc++; else gd[t].cw++; }
                    }
                    free(cue); free(got);
                } }
            printf("%u\t%u\t%.4f", delta, fb, (double)ok / tot);
            for (uint32_t t = 0; t < 4; t++)
                printf("\t%.3f/%.4f/%.4f", (double)g[t].cov / tot, g[t].cov ? (double)g[t].acc / g[t].cov : -1.0, (double)g[t].cw / tot);
            printf("\t%.4f/%.4f\n", gd[1].cov ? (double)gd[1].acc / gd[1].cov : -1.0, (double)gd[1].cw / totd);
            st_free(&mem);
        }
    }
    free(HV);
    printf("ADVERSARIAL-QA RUN COMPLETE\n");
    return 0;
}
