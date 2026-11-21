/* purehdc — L1B genuine GHRR: per-dimension m×m complex unitary binding matrices
 * (arXiv 2405.09689 semantics). C11, scalar, no neural deps.
 * HV layout: D blocks of m*m complex doubles, row-major: hv[((size_t)d*m + r)*m + c].
 * Binding = per-block matrix product O(D*m^3); unbind = product with conjugate transpose.
 * Bundled sums need NOT stay unitary — deviation is measured and reported by tests. */
#ifndef GHRR_H
#define GHRR_H
#include <stdint.h>

typedef struct { uint32_t D; uint32_t m; double *v; } ghrr_t; /* caller allocates v: D*m*m*2 doubles */

void   ghrr_alloc(ghrr_t *h, uint32_t D, uint32_t m);        /* mallocs v */
void   ghrr_free(ghrr_t *h);
/* random unitary blocks (QR/Gram-Schmidt of complex Gaussian), splitmix64 seed */
void   ghrr_random_unitary(ghrr_t *h, uint64_t *seed);
/* identity-unitary (all 1x1 phase=1 etc.) for control arms */
void   ghrr_identity(ghrr_t *h);
/* C = A·B blockwise; out may alias neither in. depth: number of matrix products per block, informational */
void   ghrr_bind(ghrr_t *out, const ghrr_t *A, const ghrr_t *B);
/* out = A · conjtrans(B)  (unbinding with unitary B is exact up to fp error) */
void   ghrr_unbind(ghrr_t *out, const ghrr_t *A, const ghrr_t *B);
/* elementwise sum bundle */
void   ghrr_bundle(ghrr_t *out, const ghrr_t **items, uint32_t k);
/* cos-type similarity: sum_d Re tr(A_d B_d^H) / (||A||_F ||B||_F) */
double ghrr_sim(const ghrr_t *A, const ghrr_t *B);
/* max over blocks of ||A^H A - I||_max ; unitarity defect. 0 for unitaries. */
double ghrr_unitarity_defect(const ghrr_t *h);
/* Frobenius norm of elementwise difference (for noncommutativity / associativity error) */
double ghrr_frob_diff(const ghrr_t *A, const ghrr_t *B);
uint64_t ghrr_digest(const ghrr_t *h); /* determinism manifest over raw doubles */
#endif
