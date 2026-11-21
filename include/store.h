/* L3-lite per-entry associative store (M-1 lesson: never superpose). Open addressing
 * is NOT used for retrieval — content-addressed linear scan over rows (TAM-like flat
 * index), so partial-cue recall works. RAM = N * 2*D/8 bytes, byte-exact from layout. */
#ifndef STORE_H
#define STORE_H
#include <stdint.h>
typedef struct { uint32_t D; uint32_t W; uint32_t cap; uint32_t n; uint64_t *keys; uint64_t *vals; uint8_t *used; } store_t;
void     st_init(store_t *s, uint32_t D, uint32_t cap);
void     st_free(store_t *s);
int      st_put(store_t *s, const uint64_t *key, const uint64_t *val);      /* -1 if full */
/* retrieve: best row by sim(query,key), copy its val out; returns index or -1. top-2 sims for abstention margins. */
int      st_retrieve(const store_t *s, const uint64_t *query, uint64_t *val_out, double *s0, double *s1);
size_t   st_bytes(const store_t *s);
#endif
