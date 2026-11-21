/* L2 — streaming byte/n-gram SDR encoder. No learned vocabulary: raw bytes ->
 * deterministic hashed sparse distributed representation (bit occupancy set).
 * n-grams of size NGRAMS levels {2,3} per position; each distinct gram sets k=2 bits
 * via splitmix64 avalanche of FNV1a(gram||level). Similarity = Jaccard over bits. */
#ifndef SDR_H
#define SDR_H
#include <stdint.h>
void     sdr_encode(const uint8_t *data, uint32_t len, uint64_t *out, uint32_t words); /* zero-fills out */
uint32_t sdr_gram_count(const uint8_t *data, uint32_t len);  /* distinct grams seen (info) */
double   sdr_jaccard(const uint64_t *a, const uint64_t *b, uint32_t words);
#endif
