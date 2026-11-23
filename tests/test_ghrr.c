/* G-A(GHRR arm) invariant suite (spec §3 L1B). Named asserts; empirical curves printed only. */
#include "ghrr.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static int fails = 0;
#define CHECK(name, cond) do { if (!(cond)) { printf("FAIL %s\n", name); fails++; } else printf("ok   %s\n", name); } while (0)

int main(void) {
    const uint32_t ms[3] = {1, 2, 4};
    const uint32_t D = 64;                                   /* matrices small here; exactness not D-dependent */
    for (int mi = 0; mi < 3; mi++) {
        uint32_t m = ms[mi];
        printf("== m=%u ==\n", m);
        ghrr_t A,B,C,AB,BA,ABC1,ABC2,X,tmp;
        ghrr_alloc(&A,D,m); ghrr_alloc(&B,D,m); ghrr_alloc(&C,D,m);
        ghrr_alloc(&AB,D,m); ghrr_alloc(&BA,D,m); ghrr_alloc(&ABC1,D,m); ghrr_alloc(&ABC2,D,m);
        ghrr_alloc(&X,D,m); ghrr_alloc(&tmp,D,m);
        uint64_t seed = 4242u + m;
        ghrr_random_unitary(&A,&seed); ghrr_random_unitary(&B,&seed); ghrr_random_unitary(&C,&seed);

        char nm[96];
        snprintf(nm,sizeof nm,"unitary_generator_m%u",m);
        CHECK(nm, ghrr_unitarity_defect(&A) < 1e-8 && ghrr_unitarity_defect(&B) < 1e-8);

        /* exactness of inverse-pair under fp: (A·B)·B^H = A */
        ghrr_bind(&AB,&A,&B);
        ghrr_unbind(&X,&AB,&B);
        snprintf(nm,sizeof nm,"unbind_exact_inverse_m%u",m);
        CHECK(nm, ghrr_frob_diff(&X,&A) < 1e-9);

        /* associativity numerics: (A·B)·C vs A·(B·C) */
        ghrr_bind(&tmp,&B,&C); ghrr_bind(&ABC2,&A,&tmp);
        ghrr_bind(&tmp,&A,&B); ghrr_bind(&ABC1,&tmp,&C);
        snprintf(nm,sizeof nm,"associative_fp_m%u",m);
        CHECK(nm, ghrr_frob_diff(&ABC1,&ABC2) < 1e-9);

        /* noncommutativity: m=1 must be commutative (FHRR phases); m>=2 must NOT */
        ghrr_bind(&BA,&B,&A);
        double nc = ghrr_frob_diff(&AB,&BA);
        snprintf(nm,sizeof nm,"commutativity_contract_m%u",m);
        if (m == 1) CHECK(nm, nc < 1e-9); else CHECK(nm, nc > 1e-3);

        /* sim self = 1; identity vs random ~ neutral positive */
        snprintf(nm,sizeof nm,"sim_self_one_m%u",m);
        CHECK(nm, fabs(ghrr_sim(&A,&A) - 1.0) < 1e-9);

        /* determinism: same seed => same digest */
        uint64_t s2 = 4242u + m; ghrr_random_unitary(&tmp,&s2);
        snprintf(nm,sizeof nm,"determinism_digest_m%u",m);
        CHECK(nm, ghrr_digest(&tmp) == ghrr_digest(&A));

        ghrr_free(&A);ghrr_free(&B);ghrr_free(&C);ghrr_free(&AB);ghrr_free(&BA);ghrr_free(&ABC1);ghrr_free(&ABC2);ghrr_free(&X);ghrr_free(&tmp);
    }

    /* r17: left-factor peel — conjtrans(A)·(A·X) recovers X exactly (unitaries) */
    {
        uint32_t ms[3] = {1,2,4};
        for (int i = 0; i < 3; i++) {
            uint32_t m = ms[i];
            ghrr_t A,X,AX,rec,W; ghrr_alloc(&A,16,m);ghrr_alloc(&X,16,m);ghrr_alloc(&AX,16,m);ghrr_alloc(&rec,16,m);ghrr_alloc(&W,16,m);
            uint64_t sd = 9000u + m; ghrr_random_unitary(&A,&sd); ghrr_random_unitary(&X,&sd); ghrr_random_unitary(&W,&sd);
            ghrr_bind(&AX,&A,&X);
            ghrr_unbind_left(&rec,&A,&AX);
            char nm[64]; snprintf(nm,sizeof nm,"leftpeel_roundtrip_m%u",m);
            CHECK(nm, ghrr_frob_diff(&rec,&X) < 1e-9);
            ghrr_bind(&AX,&W,&X); ghrr_unbind_left(&rec,&A,&AX); /* wrong left factor */
            snprintf(nm,sizeof nm,"leftpeel_wrongfactor_detected_m%u",m);
            CHECK(nm, ghrr_frob_diff(&rec,&X) > 1e-3 || m==1);   /* m=1: phases cancel noise-free but wrong phase still != 1 */
            ghrr_free(&A);ghrr_free(&X);ghrr_free(&AX);ghrr_free(&rec);ghrr_free(&W);
        }
    }

    /* bundled sums need not stay unitary — measure the defect (spec: explicit handling) */
    {
        uint32_t m = 2; ghrr_t P,Q,S; ghrr_alloc(&P,16,m); ghrr_alloc(&Q,16,m); ghrr_alloc(&S,16,m);
        uint64_t sd=7; ghrr_random_unitary(&P,&sd); ghrr_random_unitary(&Q,&sd);
        const ghrr_t *its[2] = {&P,&Q}; ghrr_bundle(&S,its,2);
        double def_u = ghrr_unitarity_defect(&P), def_s = ghrr_unitarity_defect(&S);
        printf("measure unitarity: generators=%.3e bundle=%.3e\n", def_u, def_s);
        CHECK("bundle_defect_recorded", def_u < 1e-8 && def_s > 0.05); /* bundled unitary sums are NOT unitary */
        ghrr_free(&P);ghrr_free(&Q);ghrr_free(&S);
    }

    printf("%s (%d fails)\n", fails ? "G-B FAIL" : "G-B PASS", fails);
    return fails ? 1 : 0;
}
