/* r8 CAPACITY CURVE: R@1 vs memory size N at fixed D sweep — where does binary XOR flat-scan break?
 * Unique random HV keys/values, noiseless probe + 5% cue noise. Reports onset N for R@1>=0.99 per D. */
#include "store.h"
#include "purehdc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(void) {
    uint32_t Ds[4] = {256u, 512u, 1024u, 2048u};
    printf("D\tN\tR@1_clean\tR@1_5%%noise\tms_per_query_pair\n");
    for (int di = 0; di < 4; di++) {
        uint32_t D = Ds[di], W = ph_words(D);
        for (uint32_t N = 1000u; N <= 100000u; N *= 3) {
            if (N > 100000u) break;
            uint64_t seed = 555ull + D;
            store_t st; st_init(&st, D, N);
            uint64_t *keys = calloc((size_t)N * W, 8), *vals = calloc((size_t)N * W, 8);
            for (uint32_t i = 0; i < N; i++) { ph_random(keys + (size_t)i * W, W, &seed); ph_random(vals + (size_t)i * W, W, &seed); st_put(&st, keys + (size_t)i * W, vals + (size_t)i * W); }
            uint64_t okc = 0, okn = 0, probes = N < 2000u ? N : 2000u;
            uint64_t *got = calloc(W, 8);
            double t0 = (double)clock();
            for (uint64_t p = 0; p < probes; p++) {
                uint32_t i = (uint32_t)p % N;
                if (st_retrieve(&st, keys + (size_t)i * W, got, NULL, NULL) >= 0 && memcmp(got, vals + (size_t)i * W, (size_t)W * 8) == 0) okc++;
                uint64_t *cue = calloc(W, 8); memcpy(cue, keys + (size_t)i * W, (size_t)W * 8);
                for (uint32_t b = 0; b < D / 20u; b++) { uint64_t w = ph_splitmix64_next(&seed) % W; cue[w] ^= 1ull << (ph_splitmix64_next(&seed) & 63u); }
                if (st_retrieve(&st, cue, got, NULL, NULL) >= 0 && memcmp(got, vals + (size_t)i * W, (size_t)W * 8) == 0) okn++;
                free(cue);
            }
            double t1 = (double)clock();
            printf("%u\t%u\t%.4f\t%.4f\t%.3f\n", D, N, (double)okc / probes, (double)okn / probes,
                   1000.0 * (t1 - t0) / CLOCKS_PER_SEC / (double)probes);
            st_free(&st); free(keys); free(vals); free(got);
        }
    }
    printf("CAP-CURVE COMPLETE\n");
    return 0;
}
