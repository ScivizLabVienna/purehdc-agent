#include "store.h"
#include "purehdc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void st_init(store_t *s, uint32_t D, uint32_t cap) {
    s->D = D; s->W = ph_words(D); s->cap = cap; s->n = 0;
    size_t bits = (size_t)cap * s->W;
    s->keys = calloc(bits, 8); s->vals = calloc(bits, 8); s->used = calloc(cap, 1);
    if (!s->keys || !s->vals || !s->used) { fprintf(stderr, "OOM store\n"); exit(2); }
}
void st_free(store_t *s) { free(s->keys); free(s->vals); free(s->used); s->keys = NULL; }

int st_put(store_t *s, const uint64_t *key, const uint64_t *val) {
    if (s->n >= s->cap) return -1;
    uint32_t i = s->n++;
    memcpy(s->keys + (size_t)i * s->W, key, (size_t)s->W * 8);
    memcpy(s->vals + (size_t)i * s->W, val, (size_t)s->W * 8);
    s->used[i] = 1;
    return (int)i;
}

int st_retrieve(const store_t *s, const uint64_t *query, uint64_t *val_out, double *s0, double *s1) {
    int b0 = -1; double sbest = -2.0;
    for (uint32_t i = 0; i < s->n; i++) {
        double sim = ph_sim(query, s->keys + (size_t)i * s->W, s->D, s->W);
        if (sim > sbest) { sbest = sim; b0 = (int)i; }
    }
    /* second-best over rows with DISTINCT key content (duplicates of best row are evidence, not competition) */
    int b1 = -1; double second = -2.0;
    if (b0 >= 0) {
        const uint64_t *kbest = s->keys + (size_t)b0 * s->W;
        for (uint32_t i = 0; i < s->n; i++) {
            if (i == (uint32_t)b0) continue;
            const uint64_t *ki = s->keys + (size_t)i * s->W;
            if (memcmp(ki, kbest, (size_t)s->W * 8) == 0) continue;
            double sim = ph_sim(query, ki, s->D, s->W);
            if (sim > second) { second = sim; b1 = (int)i; }
        }
    }
    if (s0) *s0 = sbest; if (s1) *s1 = second; (void)b1;
    if (b0 >= 0 && val_out) memcpy(val_out, s->vals + (size_t)b0 * s->W, (size_t)s->W * 8);
    return b0;
}

size_t st_bytes(const store_t *s) { return (size_t)s->cap * s->W * 16u + s->cap; }
