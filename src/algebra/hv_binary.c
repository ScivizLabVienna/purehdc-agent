/* L1 binary algebra implementation — scalar portable baseline. */
#include "purehdc.h"
#include <string.h>

uint64_t ph_splitmix64_next(uint64_t *s) {
    uint64_t z = (*s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

void ph_random(uint64_t *out, uint32_t words, uint64_t *seed) {
    for (uint32_t i = 0; i < words; i++) out[i] = ph_splitmix64_next(seed);
}

void ph_xor(uint64_t *dst, const uint64_t *a, const uint64_t *b, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) dst[i] = a[i] ^ b[i];
}

void ph_copy(uint64_t *dst, const uint64_t *src, uint32_t n) { memcpy(dst, src, n * 8); }

/* Circular bit shift LEFT by s within exactly D = words*64 bits. dst must not alias src. */
void ph_permute(uint64_t *dst, const uint64_t *src, uint32_t words, uint32_t d, uint32_t s) {
    (void)d;
    uint32_t ws = s / 64u, bs = s % 64u;
    memset(dst, 0, words * 8);
    if (bs == 0) {
        for (uint32_t i = 0; i < words; i++) dst[(i + ws) % words] |= src[i];
        return;
    }
    uint32_t inv = 64u - bs;
    for (uint32_t i = 0; i < words; i++) {
        dst[(i + ws) % words]         |= src[i] << bs;
        dst[(i + ws + 1) % words]     |= src[i] >> inv; /* bits shifted past the word boundary (and D wrap) */
    }
}

void ph_bundle(uint64_t *dst, const uint64_t **items, uint32_t k, uint32_t words, uint64_t *tie_count) {
    int32_t votes = 0;
    for (uint32_t w = 0; w < words; w++) {
        uint64_t outw = 0;
        for (uint32_t b = 0; b < 64; b++) {
            votes = 0;
            for (uint32_t i = 0; i < k; i++) votes += ((items[i][w] >> b) & 1ull) ? 1 : -1;
            if (votes > 0) outw |= 1ull << b;
            else if (votes == 0 && tie_count) (*tie_count)++;
        }
        dst[w] = outw;
    }
}

void ph_bundle_w(uint64_t *dst, const uint64_t **items, const int32_t *wt, uint32_t k, uint32_t words) {
    for (uint32_t w = 0; w < words; w++) {
        uint64_t outw = 0;
        for (uint32_t b = 0; b < 64; b++) {
            int32_t v = 0;
            for (uint32_t i = 0; i < k; i++) v += ((items[i][w] >> b) & 1ull) ? wt[i] : -wt[i];
            if (v > 0) outw |= 1ull << b;
        }
        dst[w] = outw;
    }
}

uint32_t ph_hamming(const uint64_t *a, const uint64_t *b, uint32_t words) {
    uint32_t h = 0;
    for (uint32_t i = 0; i < words; i++) h += (uint32_t)__builtin_popcountll(a[i] ^ b[i]);
    return h;
}

double ph_sim(const uint64_t *a, const uint64_t *b, uint32_t d, uint32_t words) {
    return 1.0 - 2.0 * (double)ph_hamming(a, b, words) / (double)d;
}

int ph_cleanup(const uint64_t *query, const uint64_t *const *rows, uint32_t k,
               double d_, int *i0, double *s0, int *i1, double *s1) {
    int b0 = -1, b1 = -1;
    double sbest = -2.0, second = -2.0;
    const uint32_t words = ph_words((uint32_t)d_);
    for (uint32_t i = 0; i < k; i++) {
        double s = ph_sim(query, rows[i], (uint32_t)d_, words);
        if (s > sbest) { second = sbest; b1 = b0; sbest = s; b0 = (int)i; }
        else if (s > second) { second = s; b1 = (int)i; }
    }
    if (i0) *i0 = b0; if (s0) *s0 = sbest; if (i1) *i1 = b1; if (s1) *s1 = second;
    return b0;
}

uint64_t ph_digest(const uint64_t *bits, uint32_t words) {
    uint64_t h = 0xcbf29ce484222325ull;
    const unsigned char *p = (const unsigned char *)bits;
    for (size_t i = 0, n = (size_t)words * 8; i < n; i++) { h ^= p[i]; h *= 0x100000001b3ull; }
    return h;
}
