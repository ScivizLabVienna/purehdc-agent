#ifndef RI_H
#define RI_H
/* Random-indexing word profiles — complement to exact-first retrieval (E19 r6 GO).
 * Sparse per-word profile maps; deterministic ±1 context vectors. Bounded: max_words,
 * max_entries/word (overflow drops new coords; shortcut: upgrade to eviction if vocab >> cap). */
#include <stdint.h>
typedef struct { uint16_t coord; float val; } ri_ent;
typedef struct { char *name; ri_ent *ents; uint32_t n, cap; } ri_word;
typedef struct {
    uint32_t D, nz, max_words, ent_cap;
    ri_word *words; uint32_t n_words;
    float *scratch_a, *scratch_b;      /* dense embed scratch, D floats each */
} ri_t;
ri_t  *ri_new(uint32_t D, uint32_t nz, uint32_t max_words, uint32_t ent_cap);
void   ri_free(ri_t *r);
int    ri_intern(ri_t *r, const char *w);                  /* id or -1 if table full */
void   ri_observe(ri_t *r, const int *ids, int ntok);      /* window ±2, symmetric pairs (mirrors Python tool) */
void   ri_embed(const ri_t *r, const int *ids, int n, float *out); /* sum profiles into out[D] (zeroed) */
double ri_cos(const float *a, const float *b, uint32_t D);
size_t ri_bytes(const ri_t *r);
#endif
