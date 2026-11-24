/* E19-P1: verify vendored Overmann bit-pair TAM (tam_bitpair, MIT) against an
   exact-lookup oracle + noise/negative controls. Deterministic xorshift; upstream
   Set_random/Set_noise/FORGETTING deliberately unused (rand()-based).
   Hypothesis: within capacity, recall = 1.0 for exact queries and tolerant up to
   residual deletions; random out-of-codebook queries must not produce a match. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "store/tam_bitpair.h"

static unsigned long long S = 88172645463393265ull; /* fixed seed, logged */
static unsigned int xs(void) {
    S ^= S << 13; S ^= S >> 7; S ^= S << 17;
    return (unsigned int)(S >> 11);
}
static void fill_set(Set *z, int p) {           /* uniform p-subset, ordered */
    z->p = 0;
    while (z->p < p) { int e = xs() % z->n, dup = 0;
        for (int i = 0; i < z->p; i++) if (z->a[i] == e) { dup = 1; break; }
        if (!dup) z->a[z->p++] = e; }
    for (int i = 1; i < z->p; i++) { int k = z->a[i], j = i - 1; while (j >= 0 && z->a[j] > k) { z->a[j+1] = z->a[j]; j--; } z->a[j+1] = k; }
}
int main(int argc, char **argv) {
    const int na = 512, pa = 16, nb = 512, pb = 16;
    int N = argc > 1 ? atoi(argv[1]) : 1000;      /* scaling axis for P1 cost curve */
    printf("tam_bitpair verify seed=88172645463393265 na=%d pa=%d nb=%d pb=%d N=%d\n", na, pa, nb, pb, N);
    Memory *m = Memory_new(na, pa, nb, pb);
    printf("auto threshold T=%d residual=%d\n", Memory_get_threshold(m), Memory_get_residual(m));

    short *Ael = malloc((size_t)N * pa * sizeof(short)), *Bel = malloc((size_t)N * pb * sizeof(short));  /* exact-lookup oracle copy */
    Set *A = Set_new(na), *B = Set_new(nb);
    for (int i = 0; i < N; i++) {
        fill_set(A, pa); fill_set(B, pb);
        Memory_write(m, A, B);
        for (int k = 0; k < pa; k++) Ael[i * pa + k] = A->a[k];
        for (int k = 0; k < pb; k++) Bel[i * pb + k] = B->a[k];
    }
    Set *Q = Set_new(na), *Y = Set_new(nb);
    int fails[4] = {0}, fp_neg = 0, ntq = 200, M = N / 2;
    for (int d = 0; d <= 3; d++)                  /* query = stored A minus d smallest */
        for (int q = 0; q < M; q++) {
            int idx = xs() % N;
            Q->p = pa - d; for (int k = 0; k < pa - d; k++) Q->a[k] = Ael[idx * pa + k + d];
            Memory_read(m, Q, Y);
            /* oracle expectation: returned Y contains the stored B exactly */
            int hit = Y->p > 0;
            for (int k = 0; k < pb && hit; k++) { int ok = 0;
                for (int j = 0; j < Y->p; j++) if (Y->a[j] == Bel[idx * pb + k]) { ok = 1; break; }
                if (!ok) hit = 0; }
            if (!hit) fails[d]++;
        }
    for (int q = 0; q < ntq; q++) { fill_set(Q, pa); Memory_read(m, Q, Y); if (Y->p > pb) fp_neg++; }

    printf("recall fails — exact %d/%d | -1 %d/%d | -2 %d/%d | -3 %d/%d\n",
           fails[0], M, fails[1], M, fails[2], M, fails[3], M);
    printf("negative-control false positives=%d/%d (threshold Y->p>%d)\n", fp_neg, ntq, pb);
    /* query-cost independence from N: time reads now */
    clock_t t0 = clock(); int R = 1000;
    for (int q = 0; q < R; q++) { fill_set(A, pa); Memory_read(m, A, Y); }
    printf("%d reads on %d-assoc memory: %.2f us/read\n", R, N, 1e6 * (double)(clock() - t0) / CLOCKS_PER_SEC / R);
    /* flat-scan oracle cost at the same N (exact subset match, ordered two-pointer) */
    t0 = clock(); int hits = 0;
    for (int q = 0; q < M && q < 200; q++) {
        int idx = xs() % N; Q->p = pa; for (int k = 0; k < pa; k++) Q->a[k] = Ael[idx * pa + k];
        for (int i = 0; i < N; i++) { int j = 0, kk = 0;   /* Ael[i] superset-of check */
            while (j < pa && kk < pa) { if (Ael[i * pa + kk] == Q->a[j]) { j++; kk++; } else if (Ael[i * pa + kk] < Q->a[j]) kk++; else break; }
            if (j == pa) hits++; }
    }
    printf("flat-scan oracle: %.2f us/query (N=%d, %d probes, hits=%d expected>=probes)\n",
           1e6 * (double)(clock() - t0) / CLOCKS_PER_SEC / (M < 200 ? M : 200), N, M < 200 ? M : 200, hits);
    Memory_free(m); Set_free(A); Set_free(B); Set_free(Q); Set_free(Y);
    int rc = fails[0] || fails[1] || fp_neg;
    printf("TAMBp VERIFY %s (%d gated fails)\n", rc ? "FAIL" : "PASS", fails[0] + fails[1] + fp_neg);
    return rc;
}
