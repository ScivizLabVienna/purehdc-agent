/* GHRR implementation — naive blockwise complex algebra (correctness first). */
#include "ghrr.h"
#include "purehdc.h" /* splitmix64 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef PH_PI
#define PH_PI 3.14159265358979323846
#endif

#define MMAX 4u
/* complex double at (d,r,c): index = (((size_t)d*m + r)*m + c)*2; +0 re, +1 im */
static size_t bi(uint32_t m, uint32_t d, uint32_t r, uint32_t c) { return (((size_t)d * m + r) * m + c) * 2u; }

static double cgauss(uint64_t *seed) {
    uint64_t r1 = ph_splitmix64_next(seed);
    double u1 = ((double)(r1 >> 11) + 1.0) / 9007199254740993.0;
    uint64_t r2 = ph_splitmix64_next(seed);
    double u2 = ((double)(r2 >> 11)) / 9007199254740992.0;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * PH_PI * u2);
}

void ghrr_alloc(ghrr_t *h, uint32_t D, uint32_t m) {
    h->D = D; h->m = m;
    h->v = calloc((size_t)D * m * m * 2u, sizeof(double));
    if (!h->v) { fprintf(stderr, "OOM ghrr\n"); exit(2); }
}
void ghrr_free(ghrr_t *h) { free(h->v); h->v = NULL; }

void ghrr_identity(ghrr_t *h) {
    memset(h->v, 0, (size_t)h->D * h->m * h->m * 2u * sizeof(double));
    for (uint32_t d = 0; d < h->D; d++)
        for (uint32_t r = 0; r < h->m; r++) h->v[bi(h->m, d, r, r)] = 1.0;
}

void ghrr_random_unitary(ghrr_t *h, uint64_t *seed) {
    const uint32_t m = h->m;
    if (m > MMAX) exit(2);
    double W[MMAX*MMAX*2], Q[MMAX*MMAX*2]; /* element (row r, col j): (r*m+j)*2 */
    for (uint32_t d = 0; d < h->D; d++) {
        for (size_t i = 0; i < (size_t)m * m * 2u; i++) W[i] = cgauss(seed);
        memset(Q, 0, sizeof Q);
        for (uint32_t pass = 0; pass < 2; pass++) {              /* 2nd pass kills GS drift */
            for (uint32_t j = 0; j < m; j++) {
                for (uint32_t i = 0; i < j; i++) {               /* ip = <Q[:,i], W[:,j]> */
                    double ip_r = 0, ip_i = 0;
                    for (uint32_t t = 0; t < m; t++) {
                        double q_r = Q[(t*m+i)*2], q_i = Q[(t*m+i)*2+1];
                        double w_r = W[(t*m+j)*2], w_i = W[(t*m+j)*2+1];
                        ip_r += q_r*w_r + q_i*w_i;               /* conj(q)*w */
                        ip_i += q_r*w_i - q_i*w_r;
                    }
                    for (uint32_t t = 0; t < m; t++) {           /* W[:,j] -= ip * Q[:,i] */
                        double q_r = Q[(t*m+i)*2], q_i = Q[(t*m+i)*2+1];
                        W[(t*m+j)*2]   -= ip_r*q_r - ip_i*q_i;
                        W[(t*m+j)*2+1] -= ip_r*q_i + ip_i*q_r;
                    }
                }
                double nrm = 0;
                for (uint32_t t = 0; t < m; t++) { double wr = W[(t*m+j)*2], wi = W[(t*m+j)*2+1]; nrm += wr*wr + wi*wi; }
                double inv = 1.0 / sqrt(nrm);                    /* degenerate Gaussian draw: measure-zero under splitmix64 */
                for (uint32_t t = 0; t < m; t++) { Q[(t*m+j)*2] = W[(t*m+j)*2]*inv; Q[(t*m+j)*2+1] = W[(t*m+j)*2+1]*inv; }
            }
        }
        for (uint32_t r = 0; r < m; r++)
            for (uint32_t c = 0; c < m; c++) {
                h->v[bi(m,d,r,c)]     = Q[(r*m+c)*2];
                h->v[bi(m,d,r,c) + 1] = Q[(r*m+c)*2+1];
            }
    }
}

void ghrr_bind(ghrr_t *out, const ghrr_t *A, const ghrr_t *B) {
    const uint32_t m = A->m;
    for (uint32_t d = 0; d < A->D; d++)
        for (uint32_t r = 0; r < m; r++)
            for (uint32_t c = 0; c < m; c++) {
                double sr = 0, si = 0;
                for (uint32_t k = 0; k < m; k++) {
                    size_t a_ = bi(m,d,r,k), b_ = bi(m,d,k,c);
                    double ar = A->v[a_], ai = A->v[a_+1], br = B->v[b_], bi2 = B->v[b_+1];
                    sr += ar*br - ai*bi2;
                    si += ar*bi2 + ai*br;
                }
                out->v[bi(m,d,r,c)] = sr; out->v[bi(m,d,r,c)+1] = si;
            }
}

/* out = A · conjtranspose(B): (r,c) = sum_k A[r,k] * conj(B[c,k]) */
void ghrr_unbind(ghrr_t *out, const ghrr_t *A, const ghrr_t *B) {
    const uint32_t m = A->m;
    for (uint32_t d = 0; d < A->D; d++)
        for (uint32_t r = 0; r < m; r++)
            for (uint32_t c = 0; c < m; c++) {
                double sr = 0, si = 0;
                for (uint32_t k = 0; k < m; k++) {
                    size_t a_ = bi(m,d,r,k), b_ = bi(m,d,c,k);
                    double ar = A->v[a_], ai = A->v[a_+1], br = B->v[b_], bi2 = B->v[b_+1];
                    sr += ar*br + ai*bi2;                       /* *conj(B) */
                    si += ai*br - ar*bi2;
                }
                out->v[bi(m,d,r,c)] = sr; out->v[bi(m,d,r,c)+1] = si;
            }
}

/* out = conjtrans(A) · B: (r,c) = sum_k conj(A[k,r]) * B[k,c] */
void ghrr_unbind_left(ghrr_t *out, const ghrr_t *A, const ghrr_t *B) {
    const uint32_t m = A->m;
    for (uint32_t d = 0; d < A->D; d++)
        for (uint32_t r = 0; r < m; r++)
            for (uint32_t c = 0; c < m; c++) {
                double sr = 0, si = 0;
                for (uint32_t k = 0; k < m; k++) {
                    size_t a_ = bi(m,d,k,r), b_ = bi(m,d,k,c);
                    double ar = A->v[a_], ai = A->v[a_+1], br = B->v[b_], bi2 = B->v[b_+1];
                    sr += ar*br + ai*bi2;                       /* conj(A)*B */
                    si += ar*bi2 - ai*br;
                }
                out->v[bi(m,d,r,c)] = sr; out->v[bi(m,d,r,c)+1] = si;
            }
}

void ghrr_bundle(ghrr_t *out, const ghrr_t **items, uint32_t k) {
    size_t n = (size_t)out->D * out->m * out->m * 2u;
    memset(out->v, 0, n * sizeof(double));
    for (uint32_t i = 0; i < k; i++)
        for (size_t x = 0; x < n; x++) out->v[x] += items[i]->v[x];
}

double ghrr_sim(const ghrr_t *A, const ghrr_t *B) {
    size_t n = (size_t)A->D * A->m * A->m * 2u;
    double ip = 0, na = 0, nb = 0;
    for (size_t x = 0; x < n; x += 2) {                         /* Re tr(A B^H) */
        ip += A->v[x]*B->v[x] + A->v[x+1]*B->v[x+1];
        na += A->v[x]*A->v[x] + A->v[x+1]*A->v[x+1];
        nb += B->v[x]*B->v[x] + B->v[x+1]*B->v[x+1];
    }
    return ip / (sqrt(na) * sqrt(nb));
}

double ghrr_unitarity_defect(const ghrr_t *h) {
    const uint32_t m = h->m; double worst = 0;
    for (uint32_t d = 0; d < h->D; d++)
        for (uint32_t i = 0; i < m; i++)
            for (uint32_t j = 0; j < m; j++) {
                double sr = 0, si = 0;                          /* M = A^H A */
                for (uint32_t k = 0; k < m; k++) {
                    size_t ki = bi(m,d,k,i), kj = bi(m,d,k,j);
                    double ar = h->v[ki], ai = h->v[ki+1], br = h->v[kj], bj = h->v[kj+1];
                    sr += ar*br + ai*bj;                        /* conj(A[k,i])*A[k,j] */
                    si += ar*bj - ai*br;
                }
                double er = fabs(sr - (i==j ? 1.0 : 0.0)), ei = fabs(si);
                if (er > worst) worst = er;
                if (ei > worst) worst = ei;
            }
    return worst;
}

double ghrr_frob_diff(const ghrr_t *A, const ghrr_t *B) {
    size_t n = (size_t)A->D * A->m * A->m * 2u; double s = 0;
    for (size_t x = 0; x < n; x++) { double df = A->v[x] - B->v[x]; s += df*df; }
    return sqrt(s / (double)(n/2u));                            /* RMS per complex element */
}

uint64_t ghrr_digest(const ghrr_t *h) {
    uint64_t hh = 0xcbf29ce484222325ull; const unsigned char *p = (const unsigned char *)h->v;
    for (size_t i = 0, n = (size_t)h->D*h->m*h->m*2u*sizeof(double); i < n; i++) { hh ^= p[i]; hh *= 0x100000001b3ull; }
    return hh;
}
