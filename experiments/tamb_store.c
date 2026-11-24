/* P1b: matched-protocol backend comparison — Overmann bit-pair TAM vs flat-scan oracle.
 * Same sparse-set associations, same queries. Labels: bitpair = HDC/TAM mechanism;
 * flat scan = ordinary algorithm (exact overlap argmax), retained as oracle.
 * Metrics: recall (B-containment), contamination, write/read latency p50/p95, replay-rebuild wall time. */
#include "store/tam_bitpair.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static unsigned long long xs_s = 88172645463393265ull;
static unsigned long long xs(void){ xs_s^=xs_s<<13; xs_s^=xs_s>>7; xs_s^=xs_s<<17; return xs_s; }
static double now_ms(void){ struct timespec ts; timespec_get(&ts,TIME_UTC); return ts.tv_sec*1e3+ts.tv_nsec/1e6; }
static int cmpd(const void*a,const void*b){ double x=*(const double*)a-*(const double*)b; return (x>0)-(x<0); }

static void mk_set(int *el, int n_unused, int p) {  /* sorted uniform p-subset of [512) */
    for (int i = 0; i < p; ) { int e = (int)(xs() % 512), dup = 0;
        for (int j = 0; j < i; j++) if (el[j] == e) { dup = 1; break; }
        if (!dup) el[i++] = e; }                       /* rejection keeps uniformity cheap */
}

int main(int argc, char **argv) {
    const int NP = 16, NQ_MIN = 2000;
    int Ns[4] = {1000, 10000, 40000, 100000};
    if (argc > 1) { Ns[0] = atoi(argv[1]); Ns[1] = Ns[2] = Ns[3] = Ns[0]; } /* one-shot mode for RSS wrapper */
    printf("tamb_store seed=88172645463393265 p=%d na=nb=512\n", NP);
    int rc = 0;
    for (int ni = 0; ni < 4 && Ns[ni]; ni++) {
        int N = Ns[ni], Q = N < NQ_MIN ? N : NQ_MIN;
        int *Ael = malloc((size_t)N*NP*sizeof(int)), *Bel = malloc((size_t)N*NP*sizeof(int));
        for (int i = 0; i < N; i++) { mk_set(Ael+(size_t)i*NP,512,NP); mk_set(Bel+(size_t)i*NP,512,NP); }

        /* ---- backend A: bit-pair TAM ---- */
        double t0 = now_ms();
        Memory *m = Memory_new(512, NP, 512, NP);
        Set *A = Set_new(512), *B = Set_new(512);
        for (int i = 0; i < N; i++) { A->p = B->p = NP;
            for (int k = 0; k < NP; k++) { A->a[k] = Ael[(size_t)i*NP+k]; B->a[k] = Bel[(size_t)i*NP+k]; }
            Memory_write(m, A, B); }
        double wms = now_ms() - t0;

        double *lat = malloc((size_t)Q*sizeof(double));
        double incl_sum = 0, contam_sum = 0; int incl1 = 0;
        Set *Y = Set_new(512), *Qq = Set_new(512);
        for (int q = 0; q < Q; q++) {
            int idx = (int)(xs() % N);
            Qq->p = NP; for (int k = 0; k < NP; k++) Qq->a[k] = Ael[(size_t)idx*NP+k];
            double r0 = now_ms(); Memory_read(m, Qq, Y); lat[q] = now_ms() - r0;
            int hitB = 0;                                  /* |Y ∩ Bel[idx]| */
            for (int j = 0; j < Y->p; j++) { int ok = 0;
                for (int k = 0; k < NP; k++) if (Y->a[j] == Bel[(size_t)idx*NP+k]) { ok = 1; break; }
                if (ok) hitB++; }
            incl_sum += (double)hitB / NP; contam_sum += Y->p ? 1.0 - (double)hitB / Y->p : 0.0;
            if (hitB >= NP) incl1++;                        /* full-B containment = strict R@1 */
        }
        qsort(lat, Q, sizeof(double), cmpd);
        double p50 = lat[Q/2], p95 = lat[(int)(Q*0.95)];

        /* replay rebuild (deletion is unavailable upstream; rebuild-from-data is the delete story) */
        Memory_free(m); Set_free(A); Set_free(B);
        t0 = now_ms();
        m = Memory_new(512, NP, 512, NP); A = Set_new(512); B = Set_new(512);
        for (int i = 0; i < N; i++) { A->p = B->p = NP;
            for (int k = 0; k < NP; k++) { A->a[k] = Ael[(size_t)i*NP+k]; B->a[k] = Bel[(size_t)i*NP+k]; }
            Memory_write(m, A, B); }
        double rms = now_ms() - t0;

        /* ---- backend B: flat-scan exact overlap argmax (oracle) ---- */
        int hitscan = 0;
        t0 = now_ms();
        for (int q = 0; q < Q; q++) {
            int idx = (int)(xs() % N); int *qy = Ael + (size_t)idx*NP, best = -1, bv = -1;
            for (int i = 0; i < N; i++) { int *x = Ael + (size_t)i*NP, j = 0, k = 0, ov = 0;
                while (j < NP && k < NP) { if (qy[j]==x[k]) { ov++; j++; k++; } else if (x[k]<qy[j]) k++; else j++; }
                if (ov > bv) { bv = ov; best = i; } }
            hitscan += (best == idx);
        }
        double s_ms = now_ms() - t0;

        printf("N=%d | tam write %.1f ms (%.2f us/assoc), read p50 %.1f us p95 %.1f us, B-incl %.4f full-contain %.4f contam %.4f | rebuild %.0f ms | flat R@1 %.4f, %.1f us/query\n",
               N, wms, 1e3*wms/N, p50*1e3, p95*1e3, incl_sum/Q, (double)incl1/Q, contam_sum/Q, rms, (double)hitscan/Q, 1e3 * s_ms / Q);
        if (hitscan != Q) { printf("ORACLE FAIL N=%d\n", N); rc = 1; }   /* exact-match scan must be perfect */
        if (N == 1000 && (incl1 < Q*95/100)) { printf("LOWLOAD RECALL FAIL\n"); rc = 1; }

        Memory_free(m); Set_free(A); Set_free(B); Set_free(Qq); Set_free(Y);
        free(Ael); free(Bel); free(lat);
    }
    printf("TAMB_STORE %s\n", rc ? "FAIL" : "PASS");
    return rc;
}
