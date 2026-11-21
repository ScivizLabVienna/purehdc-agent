/* r12a seed sensitivity: N=4k unique random pairs x 8 seeds x D sweep -> R@1 mean/min, clean + 5% cue noise.
 * Answers the reviewer question "is 1.000 a lucky seed?" for store + margin-gate arms. Deterministic per seed. */
#include "store.h"
#include "purehdc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t rs; /* running seed state */
#define N 4000u
static void flip(uint64_t *v, uint32_t W, double p) { for (uint32_t w=0;w<W;w++) for (uint32_t b=0;b<64;b++){ rs^=rs<<13; rs^=rs>>7; rs^=rs<<17; if((double)(rs&0xffff)/65536.0<p) v[w]^=1ull<<b; } }

int main(void){
    printf("D\tseeds\tR@1_clean_mean\tR@1_clean_min\tR@1_noise_mean\tnoise_cwr_tau.05\n");
    for (uint32_t D = 256; D <= 2048; D <<= 1){
        uint32_t W = D/64;
        double cmin = 1.0, csum = 0, nsum = 0; double ncwr = 0; int nseeds = 8;
        for (int s = 0; s < nseeds; s++){
            rs = 0x9E3779B97F4A7C15ull * (s + 1) + D; /* deterministic per (seed,D) */
            uint64_t *keys = malloc((size_t)N*W*8), *vals = malloc((size_t)N*W*8);
            for (uint32_t i=0;i<N;i++){ ph_random(keys+(size_t)i*W,W,&rs); ph_random(vals+(size_t)i*W,W,&rs); }
            store_t st; st_init(&st, D, N*2u);
            for (uint32_t i=0;i<N;i++) st_put(&st, keys+(size_t)i*W, vals+(size_t)i*W);
            uint64_t cue[2048/64], got[2048/64]; long okc=0, okn=0, wrongans=0, answered=0;
            for (uint32_t i=0;i<N;i++){
                double m0,m1; int r;
                r = st_retrieve(&st, keys+(size_t)i*W, got, &m0,&m1); okc += (r==(int)i && !memcmp(got,vals+(size_t)i*W,W*8));
                memcpy(cue, keys+(size_t)i*W, (size_t)W*8); flip(cue,W,0.05);
                r = st_retrieve(&st, cue, got, &m0,&m1); okn += (r==(int)i && !memcmp(got,vals+(size_t)i*W,W*8));
                if (m0-0.05 >= m1) { answered++; if (r!=(int)i) wrongans++; } /* gate answers; count confident errors */
            }
            double rc=(double)okc/N, rn=(double)okn/N;
            csum+=rc; nsum+=rn; if(rc<cmin)cmin=rc; ncwr += (double)wrongans/(answered?answered:1);
            st_free(&st); free(keys); free(vals);
        }
        printf("%u\t%d\t%.4f\t%.4f\t%.4f\t%.5f\n", D, nseeds, csum/nseeds, cmin, nsum/nseeds, ncwr/nseeds);
    }
    printf("SEED-SWEEP COMPLETE\n"); return 0;
}
