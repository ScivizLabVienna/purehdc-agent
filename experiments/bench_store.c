/* G-B store gate bench: unique-key associative store, R@1 vs flip-noise, RAM + latency.
 * Arms: flat-scan content-addressed store (ours), exact digest hash map (upper control). */
#include "store.h"
#include "purehdc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct { uint32_t cap; uint64_t *dg; uint32_t *idx; } hmap_t;
static void hm_init(hmap_t *h, uint32_t cap) { h->cap = cap; h->dg = calloc(cap, 8); h->idx = calloc(cap, 4); }
static int  hm_put(hmap_t *h, uint64_t dg, uint32_t i) { uint32_t sl = (uint32_t)(dg % h->cap); while (h->dg[sl]) sl = (sl + 1) % h->cap; h->dg[sl] = dg; h->idx[sl] = i; return 0; }
static int  hm_get(hmap_t *h, uint64_t dg) { uint32_t sl = (uint32_t)(dg % h->cap); while (h->dg[sl]) { if (h->dg[sl] == dg) return (int)h->idx[sl]; sl = (sl + 1) % h->cap; } return -1; }

static double now_ms(void) { struct timespec ts; timespec_get(&ts, TIME_UTC); return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6; }
static int cmpd(const void *a, const void *b) { double x = *(const double*)a - *(const double*)b; return (x > 0) - (x < 0); }

int main(void) {
    const uint32_t D = 1024, W = ph_words(D);
    const uint32_t Ns[3] = {1000u, 10000u, 40000u};
    const double flips[4] = {0.0, 0.02, 0.05, 0.10};
    for (int ni = 0; ni < 3; ni++) {
        uint32_t N = Ns[ni];
        store_t st; st_init(&st, D, N); hmap_t hm; hm_init(&hm, N * 2u);
        uint64_t seed = 77000u + N;
        uint64_t *keys = calloc((size_t)N * W, 8);
        for (uint32_t i = 0; i < N; i++) {
            uint64_t *k = keys + (size_t)i * W; ph_random(k, W, &seed);
            st_put(&st, k, k);                  /* value==key: R@1 = correct-row recall */
            hm_put(&hm, ph_digest(k, W), i);
        }
        printf("N=%u  RAM=%.2f MB  %.0f B/pattern (formula 2*D/8+1)\n", N, st_bytes(&st)/1048576.0, (double)st_bytes(&st)/N);
        for (int fi = 0; fi < 4; fi++) {
            double p = flips[fi];
            uint32_t Q = N < 2000u ? N : 2000u;
            uint64_t hits = 0, hmh = 0;
            double *lat = malloc((size_t)Q * sizeof(double));
            uint64_t qs = 51000u + N + (uint32_t)(p * 100.0);
            uint64_t *cue = calloc(W, 8);
            for (uint32_t q = 0; q < Q; q++) {
                uint32_t j = (uint32_t)(ph_splitmix64_next(&qs) % N);
                memcpy(cue, keys + (size_t)j * W, (size_t)W * 8);
                if (p > 0.0) { uint64_t ns = qs; uint32_t nf = (uint32_t)(p * D);
                    for (uint32_t f = 0; f < nf; f++) { uint32_t b = (uint32_t)(ph_splitmix64_next(&ns) % D); cue[b >> 6] ^= 1ull << (b & 63); } }
                if (hm_get(&hm, ph_digest(cue, W)) == (int)j) hmh++;   /* hash arm: exact-digest only */
                double t0 = now_ms();
                int i0; double s0, s1;
                i0 = st_retrieve(&st, cue, NULL, &s0, &s1);
                lat[q] = now_ms() - t0;
                if (i0 == (int)j) hits++;
            }
            qsort(lat, Q, sizeof(double), cmpd);
            printf("flip=%.2f  store R@1=%.4f  hashmap_exact R@1=%.4f  p50=%.3fms p95=%.3fms\n",
                   p, (double)hits/Q, (double)hmh/Q, lat[Q/2], lat[(size_t)(Q*0.95)]);
            free(lat); free(cue);
        }
        st_free(&st); free(hm.dg); free(hm.idx); free(keys);
    }
    return 0;
}
