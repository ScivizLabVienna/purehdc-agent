/* SDR encoder implementation. shortcut: k=2 bits/gram fixed; upgrade when capacity curves demand tunable sparsity (r2+). */
#include "sdr.h"
#include "purehdc.h"
#include <stdlib.h>

#define GMIN 2u
#define GMAX 3u

static uint64_t gram_hash(const uint8_t *g, uint32_t n, uint32_t level) {
    uint64_t h = 0xcbf29ce484222325ull ^ level;
    for (uint32_t i = 0; i < n; i++) { h ^= g[i]; h *= 0x100000001b3ull; }
    return ph_splitmix64_next(&h);
}

static void set2(uint64_t *out, uint32_t words, uint64_t hv) {
    uint32_t b1 = (uint32_t)(hv % ((uint64_t)words * 64));
    uint32_t b2 = (uint32_t)(ph_splitmix64_next(&hv) % ((uint64_t)words * 64));
    out[b1 >> 6] |= 1ull << (b1 & 63);
    out[b2 >> 6] |= 1ull << (b2 & 63);
}

void sdr_encode(const uint8_t *data, uint32_t len, uint64_t *out, uint32_t words) {
    for (uint32_t w = 0; w < words; w++) out[w] = 0;
    for (uint32_t n = GMIN; n <= GMAX; n++)
        if (len >= n)
            for (uint32_t i = 0; i + n <= len; i++)
                set2(out, words, gram_hash(data + i, n, n));
}

/* distinct grams via tiny open-addressed set of hashes — exact count for diagnostics */
uint32_t sdr_gram_count(const uint8_t *data, uint32_t len) {
    uint32_t cap = 4096; uint64_t mask = cap - 1;
    uint64_t *tbl = calloc(cap, 8); if (!tbl) return 0; /* collision-curve inputs are tiny */
    uint32_t cnt = 0;
    for (uint32_t n = GMIN; n <= GMAX; n++)
        if (len >= n)
            for (uint32_t i = 0; i + n <= len; i++) {
                uint64_t h = gram_hash(data + i, n, n), slot = h & mask;
                while (tbl[slot] && tbl[slot] != h) slot = (slot + 1) & mask;
                if (!tbl[slot]) { tbl[slot] = h; cnt++; }
            }
    free(tbl);
    return cnt;
}

double sdr_jaccard(const uint64_t *a, const uint64_t *b, uint32_t words) {
    uint32_t inter = 0, uni = 0;
    for (uint32_t w = 0; w < words; w++) {
        uint64_t x = a[w] & b[w], u = a[w] | b[w];
        inter += (uint32_t)__builtin_popcountll(x); uni += (uint32_t)__builtin_popcountll(u);
    }
    return uni ? (double)inter / uni : 0.0;
}
