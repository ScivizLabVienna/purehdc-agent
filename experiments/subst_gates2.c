/* r11v2 GATE HEAD-TO-HEAD v2 on substitution corruption. Arms:
 *   1) margin(.10)       — HDUQ rank gap                       [expected blind: answers wrong]
 *   2) selflog           — triple present in agent's OWN append-only episodic log
 *                          (corruption is post-hoc over the whole world the agent recorded -> fooled)
 *   3) obsverify         — decoded claim checked against an INDEPENDENT environment record
 *                          (spec L5 VERIFY CONSTRAINTS: evidence not derivable from the retrieval path)
 *   4) dual-view         — independent permuted encoding decodes same object
 * NOTE proven geometrically by v1 (subst_gates_v1_blind.c, cov=0.0): forward+inverse through the
 * SAME store is a round-trip — unbind(retrieve(cue)) returns the row's own key, so self-consistency
 * gates on one view cannot flag substitution at all. Only non-derivable evidence can. */
#include "store.h"
#include "purehdc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NSUBJ 100u
#define NREL 20u
#define NOBJ 300u
static const uint32_t D = 1024u, W = D / 64;
static uint32_t val[NSUBJ][NREL];          /* environment ground truth (independent of semantic mem) */
static uint8_t logged[NSUBJ][NREL];        /* agent's episodic record of having observed the triple */
static uint64_t *HV, *sHp, *rHp;
#define SJ(i) (HV + (size_t)(i) * W)
#define RL(i) (HV + (size_t)(NSUBJ + (i)) * W)
#define OB(i) (HV + (size_t)(NSUBJ + NREL + (i)) * W)

static double sim(const uint64_t *a, const uint64_t *b) {
    uint32_t pop = 0; for (uint32_t w = 0; w < W; w++) pop += __builtin_popcountll(a[w] ^ b[w]);
    return 1.0 - 2.0 * pop / D;
}
static uint32_t dec(const uint64_t *got) {
    double best = -2; int bi = -1;
    for (uint32_t e = 0; e < NOBJ; e++) { double s = sim(got, OB(e)); if (s > best) { best = s; bi = (int)e; } }
    return bi >= 0 && best > 0.85 ? (uint32_t)bi : 0xffffffffu;
}

int main(void) {
    HV  = calloc(((size_t)NSUBJ + NREL + NOBJ) * W, 8);
    sHp = calloc((size_t)NSUBJ * W, 8); rHp = calloc((size_t)NREL * W, 8);
    uint64_t hs = 8080ull;
    for (uint32_t i = 0; i < NSUBJ + NREL + NOBJ; i++) ph_random(HV + (size_t)i * W, W, &hs);
    for (uint32_t i = 0; i < NSUBJ; i++) ph_permute(sHp + (size_t)i * W, SJ(i), W, D, 1u);
    for (uint32_t i = 0; i < NREL;  i++) ph_permute(rHp + (size_t)i * W, RL(i), W, D, 7u);
    printf("p_erase\tdelta\tmargin cov/acc/cwr\tselflog cov/acc/cwr\tobsverify cov/acc/cwr\tdual cov/acc/cwr   (TEST)\n");
    for (double p_e = 0.5; p_e <= 1.01; p_e += 0.5) {
      for (uint32_t delta = 8; delta <= 32; delta += 24) {
        uint64_t seed = 31337ull + (uint32_t)(p_e * 100) * 977ull + delta;
        store_t A, B; st_init(&A, D, NSUBJ * NREL * 2u); st_init(&B, D, NSUBJ * NREL);
        memset(logged, 0, sizeof logged);
        uint8_t erased[NSUBJ];
        for (uint32_t s = 0; s < NSUBJ; s++) { uint64_t u = ph_splitmix64_next(&seed); erased[s] = ((double)(u >> 11) / 9007199254740992.0) < p_e; }
        for (uint32_t s = 0; s < NSUBJ; s++) for (uint32_t r = 0; r < NREL; r++) {
            uint32_t k = (uint32_t)(ph_splitmix64_next(&seed) % NOBJ); val[s][r] = k; logged[s][r] = 1;
            uint64_t *ka = calloc(W, 8), *kb = calloc(W, 8); ph_xor(ka, SJ(s), RL(r), W); ph_xor(kb, rHp + r * W, sHp + s * W, W);
            if (erased[s]) { uint64_t *tw = calloc(W, 8); memcpy(tw, ka, (size_t)W * 8);   /* post-hoc semantic corruption */
                for (uint32_t b = 0; b < delta; b++) { uint64_t w = ph_splitmix64_next(&seed) % W; tw[w] ^= 1ull << (ph_splitmix64_next(&seed) & 63u); }
                st_put(&A, tw, OB((k + 1u + (uint32_t)(ph_splitmix64_next(&seed) % (NOBJ - 1u))) % NOBJ)); free(tw);
            } else st_put(&A, ka, OB(k));
            st_put(&B, kb, OB(k)); free(ka); free(kb);
        }
        uint64_t tot = 0;
        struct { uint64_t cov, acc, cw; } mg = {0}, sl = {0}, ov = {0}, du = {0};
        for (uint32_t s = 0; s < NSUBJ; s++) { if (s % 2u == 0) continue;
            for (uint32_t r = 0; r < NREL; r++) {
                uint64_t *cueA = calloc(W, 8), *cueB = calloc(W, 8), *gA = calloc(W, 8), *gB = calloc(W, 8);
                ph_xor(cueA, SJ(s), RL(r), W); ph_xor(cueB, rHp + r * W, sHp + s * W, W);
                double m0, m1; st_retrieve(&A, cueA, gA, &m0, &m1); st_retrieve(&B, cueB, gB, NULL, NULL);
                uint32_t oa = dec(gA), ob = dec(gB); int okA = (oa == val[s][r]); tot++;
                if (m0 - m1 > 0.10) { mg.cov++; if (okA) mg.acc++; else mg.cw++; }
                if (logged[s][r])   { sl.cov++; if (okA) sl.acc++; else sl.cw++; }
                if (oa != 0xffffffffu && val[s][r] == oa) { ov.cov++; if (okA) ov.acc++; else ov.cw++; }  /* env confirms the exact claim */
                if (oa != 0xffffffffu && oa == ob)        { du.cov++; if (okA) du.acc++; else du.cw++; }
                free(cueA); free(cueB); free(gA); free(gB);
            } }
        printf("%.1f\t%u\t%.3f/%.4f/%.4f\t%.3f/%.4f/%.4f\t%.3f/%.4f/%.4f\t%.3f/%.4f/%.4f\n", p_e, delta,
               (double)mg.cov/tot, mg.cov?(double)mg.acc/mg.cov:-1.0, (double)mg.cw/tot,
               (double)sl.cov/tot, sl.cov?(double)sl.acc/sl.cov:-1.0, (double)sl.cw/tot,
               (double)ov.cov/tot, ov.cov?(double)ov.acc/ov.cov:-1.0, (double)ov.cw/tot,
               (double)du.cov/tot, du.cov?(double)du.acc/du.cov:-1.0, (double)du.cw/tot);
        st_free(&A); st_free(&B);
      }
    }
    free(HV); free(sHp); free(rHp);
    printf("GATE-H2Hv2 COMPLETE\n");
    return 0;
}
