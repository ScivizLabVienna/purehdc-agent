/* purehdc — L1 binary/sparse HDC algebra core (C11, CPU-only, no neural deps).
 * Dense bipolar hypervectors stored bit-packed; word = uint64_t. All ops
 * in-place-safe only when documented. Deterministic: splitmix64 PRNG seeded
 * by caller; identical seed + D => bit-identical vectors across runs. */
#ifndef PUREHDC_H
#define PUREHDC_H
#include <stdint.h>
#include <stddef.h>

/* word count for dimension D (D must be a multiple of 64) */
static inline uint32_t ph_words(uint32_t d) { return d / 64u; }

/* PRNG: splitmix64 — deterministic across platforms, no host entropy. */
uint64_t ph_splitmix64_next(uint64_t *state);

void     ph_random(uint64_t *out, uint32_t words, uint64_t *seed);          /* random HV (uniform bits) */
void     ph_xor(uint64_t *dst, const uint64_t *a, const uint64_t *b, uint32_t words);
void     ph_copy(uint64_t *dst, const uint64_t *src, uint32_t words);
/* circular bit permutation of a D-bit vector by s bits (0 < s < d). dst may alias src only if dst==src is disallowed; use tmp. */
void     ph_permute(uint64_t *dst, const uint64_t *src, uint32_t words, uint32_t d, uint32_t s);
static inline uint32_t ph_perm_inv(uint32_t d, uint32_t s) { return d - (s % d); } /* inverse shift */
/* majority bundling of k item vectors; ties (even k) resolve to 0. tie_count optional out. */
void     ph_bundle(uint64_t *dst, const uint64_t **items, uint32_t k, uint32_t words, uint64_t *tie_count);
/* weighted majority: vote += (bit? +w : -w), positive => 1. */
void     ph_bundle_w(uint64_t *dst, const uint64_t **items, const int32_t *w, uint32_t k, uint32_t words);
/* hamming distance (bits differing) */
uint32_t ph_hamming(const uint64_t *a, const uint64_t *b, uint32_t words);
/* cosine similarity of bipolar vectors in [-1,1] */
double   ph_sim(const uint64_t *a, const uint64_t *b, uint32_t d, uint32_t words);
/* cleanup: argmax sim over k candidate vectors given as an array of row pointers. */
int      ph_cleanup(const uint64_t *query, const uint64_t *const *rows, uint32_t k,
                    double d_, int *i0, double *s0, int *i1, double *s1);
/* FNV-1a digest over a bit array — for determinism manifests */
uint64_t ph_digest(const uint64_t *bits, uint32_t words);
#endif
