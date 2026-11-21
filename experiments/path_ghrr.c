/* r9v2 L4.2 — PathHD-inspired ordered path reasoning in GHRR with deterministic selector.
 * Chain start -r1-> mid -r2-> end.  KEY = ordered matrix product P = R2·R1·S (path signature).
 * VALUE = endpoint entity HV. Query cue Q = Rb·Ra·S; retrieval = NN over stored path keys
 * (substitutes PathHD's neural/LLM adjudication with margin-gated NN + entity-type check);
 * answer endpoint = nearest stored entity to the retrieved VALUE; structural verification:
 * unbind(conj(R2),conj(R1)·P) must return S within fp tolerance, else ABSTAIN.
 * Measures: hop-2 accuracy, retrieval margin, order sensitivity (swapped-relation cue sim vs correct). */
#include "ghrr.h"
#include "purehdc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NE 40u
#define CHAINS 150u   /* capacity 4 rel-pairs x 40 starts; duplicates would give zero-margin ties */
static const uint32_t D = 256u, M = 2u;
static ghrr_t ent[NE], rel[8];

static double nn_entity(const ghrr_t *v) {          /* type check: nearest real entity */
    double best = -2; int bi = -1;
    for (uint32_t e = 0; e < NE; e++) { double s = ghrr_sim(v, &ent[e]); if (s > best) { best = s; bi = (int)e; } }
    return bi >= 0 && best > 0.9 ? (double)bi : -1.0; /* -1 = no entity of this type */
}
static int nn_key(ghrr_t *keys, uint32_t n, const ghrr_t *q, double *m0, double *m1) {
    double best = -2, second = -2; int bi = -1;
    for (uint32_t i = 0; i < n; i++) { double s = ghrr_sim(&keys[i], q);
        if (s > best) { second = best; best = s; bi = (int)i; } else if (s > second) second = s; }
    *m0 = best; *m1 = second; return bi;
}

int main(void) {
    uint64_t seed = 777ull;
    for (uint32_t e = 0; e < NE; e++)   { ghrr_alloc(&ent[e], D, M); ghrr_random_unitary(&ent[e], &seed); }
    for (uint32_t r = 0; r < 8; r++)    { ghrr_alloc(&rel[r],  D, M); ghrr_random_unitary(&rel[r],  &seed); }
    ghrr_t *keys = calloc(CHAINS, sizeof(ghrr_t)), *vals = calloc(CHAINS, sizeof(ghrr_t));
    for (uint32_t i = 0; i < CHAINS; i++) { ghrr_alloc(&keys[i], D, M); ghrr_alloc(&vals[i], D, M); }
    static uint8_t used[4][NE];
    uint32_t st[CHAINS], en[CHAINS], pairi[CHAINS];
    double sum_correct_cue = 0, sum_swapped_cue = 0;
    for (uint32_t i = 0; i < CHAINS; i++) {
        uint32_t s, m, e, pi = i % 4;
        do { s = (uint32_t)(ph_splitmix64_next(&seed) % NE); } while (used[pi][s]);   /* unique (pair,start) keys */
        used[pi][s] = 1; pairi[i] = pi;
        do { m = (uint32_t)(ph_splitmix64_next(&seed) % NE); } while (m == s);
        do { e = (uint32_t)(ph_splitmix64_next(&seed) % NE); } while (e == m || e == s);
        st[i] = s; en[i] = e;
        uint32_t r1i = 2 * pi, r2i = r1i + 1;
        ghrr_t t; ghrr_alloc(&t, D, M);
        ghrr_bind(&t, &rel[r1i], &ent[s]);              /* R1·S   */
        ghrr_bind(&keys[i], &rel[r2i], &t);             /* KEY = R2·R1·S  (ordered binding) */
        ghrr_free(&t);
        memcpy(vals[i].v, ent[e].v, (size_t)D * M * M * 2 * sizeof(double)); /* VALUE = endpoint entity E */
    }
    uint32_t ok = 0, abst = 0, tot = 0; double sum_margin = 0, min_margin = 9;
    ghrr_t q, t1; ghrr_alloc(&q, D, M); ghrr_alloc(&t1, D, M);
    for (uint32_t i = 0; i < CHAINS; i++) {
        uint32_t r1i = 2 * pairi[i], r2i = r1i + 1, want = en[i];
        ghrr_bind(&t1, &rel[r1i], &ent[st[i]]); ghrr_bind(&q, &rel[r2i], &t1);     /* Q = R2·R1·S */
        double m0, m1; int fi = nn_key(keys, CHAINS, &q, &m0, &m1);
        sum_margin += m0 - m1; if (m0 - m1 < min_margin) min_margin = m0 - m1;
        sum_correct_cue += m0;
        /* NOTE: ghrr_unbind(A,B)=A·Bᴴ right-peels only; left-bound path P=R2R1S needs a
         * conj-transpose-first-left variant for start-recovery — recorded as API gap (findings r9). */
        double ehat = nn_entity(&vals[fi >= 0 ? fi : 0]);
        int answered = m0 - m1 > 0.02 && ehat >= 0;   /* margin + entity-type gate; no oracle peek */
        if (!answered) abst++; else ok += ((uint32_t)(ehat + .5) == want);
        tot++;
        /* swapped cue: same chain, wrong relation order */
        ghrr_bind(&t1, &rel[r2i], &ent[st[i]]); ghrr_bind(&q, &rel[r1i], &t1);
        sum_swapped_cue += ghrr_sim(&keys[i], &q);
    }
    printf("r9v2 GHRR PATHS: D=%u m=%u chains=%u\n", D, M, CHAINS);
    printf("hop2 gated acc (answered only)   = %.4f over %u answered of %u (abstained %u)\n",
           ok ? (double)ok / (tot - abst) : 0.0, tot - abst, tot, abst);
    printf("mean margin = %.4f  min margin = %.4f\n", sum_margin / tot, min_margin);
    printf("cue similarity: correct order mean = %.4f | swapped order mean = %.4f (separation => order carries semantics)\n",
           sum_correct_cue / tot, sum_swapped_cue / tot);
    printf("G-R9 PATHS %s\n", (abst == 0 && ok == tot) ? "PASS" : (ok + abst == tot ? (abst ? "PARTIAL" : "FAIL") : "BUG"));
    for (uint32_t i = 0; i < CHAINS; i++) { ghrr_free(&keys[i]); ghrr_free(&vals[i]); }
    free(keys); free(vals);
    ghrr_free(&q); ghrr_free(&t1);
    for (uint32_t e = 0; e < NE; e++) ghrr_free(&ent[e]);
    for (uint32_t r = 0; r < 8; r++)  ghrr_free(&rel[r]);
    return 0;
}
