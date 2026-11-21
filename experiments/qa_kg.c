/* G-C relational QA over synthetic chain KG in pure C.
 * Storage per spec L4-1: rows key = bind(s,r) = s XOR r ; value = object o.
 * Arms:  HDC associative store (ours), exact symbolic hashmap (deterministic ceiling), random floor.
 * Metrics: hop-wise accuracy, abstention via margin tau sweep, confidently-wrong rate. */
#include "store.h"
#include "purehdc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXN 600u          /* entities */
#define DEPTHMAX 10u
typedef struct { uint32_t cap; uint64_t *dg; uint32_t *idx; } hmap_t;
static void hm_init(hmap_t *h, uint32_t cap) { h->cap = cap; h->dg = calloc(cap, 8); h->idx = calloc(cap, 4); }
static int  hm_put(hmap_t *h, uint64_t dg, uint32_t i) { uint32_t sl = (uint32_t)(dg % h->cap); while (h->dg[sl]) sl = (sl + 1) % h->cap; h->dg[sl] = dg; h->idx[sl] = i; return 0; }
static int  hm_get(hmap_t *h, uint64_t dg) { uint32_t sl = (uint32_t)(dg % h->cap); while (h->dg[sl]) { if (h->dg[sl] == dg) return (int)h->idx[sl]; sl = (sl + 1) % h->cap; } return -1; }

int main(void) {
    const uint32_t D = 1024, W = ph_words(D);
    uint64_t seed = 93001u;
    int fails = 0;
#define CHECK(name, cond) do { if (!(cond)) { printf("FAIL %s\n", name); fails++; } else printf("ok   %s\n", name); } while (0)

    /* entity/relation HV pools */
    uint64_t *ent = calloc((size_t)MAXN * W, 8), *rel = calloc(3u * W, 8);
    for (uint32_t i = 0; i < MAXN; i++) ph_random(ent + (size_t)i * W, W, &seed);
    for (uint32_t i = 0; i < 3; i++)    ph_random(rel + (size_t)i * W, W, &seed);

    /* chain KG: entity i -> i+1 via rel r=i%3 ; ground truth object = i+1 */
    const uint32_t E = 500u;
    store_t st; st_init(&st, D, E); hmap_t hm; hm_init(&hm, 2u * E);
    uint64_t *key = calloc(W, 8), *valv = calloc(W, 8), *cue = calloc(W, 8), *ans = calloc(W, 8);
    for (uint32_t i = 0; i < E; i++) {
        ph_xor(key, ent + (size_t)i * W, rel + (size_t)(i % 3) * W, W);   /* bind(s,r) */
        st_put(&st, key, ent + (size_t)(i + 1) * W);                       /* value o=i+1 */
        hm_put(&hm, ph_digest(key, W), i + 1);
    }

    /* invariants */
    { const uint64_t *s = ent, *r0 = rel, *r1 = rel + W;
      uint64_t *a = calloc(W,8), *b = calloc(W,8); ph_xor(a, s, r0, W); ph_xor(b, s, r1, W);
      double simab = 0.0; /* random-ish: use hamming-based sim via store-free path */
      uint32_t pop = 0; for (uint32_t w = 0; w < W; w++) pop += __builtin_popcountll(a[w]^b[w]);
      simab = 1.0 - 2.0*pop/(double)D;
      printf("measure rel_swap_key_sim=%.4f\n", simab); CHECK("direction_rel_swap_distinguishable", simab < 0.5);
      ph_xor(b, r0, s, W); double p2 = 0; for (uint32_t w=0;w<W;w++) p2 += __builtin_popcountll(a[w]^b[w]);
      printf("measure bind_noncommute_sim=%.4f\n", 1.0-2.0*p2/D); /* XOR commutes: expect 1.0 — record honestly */
      free(a); free(b); }

    /* hop-wise chained retrieval, with margin-based abstention */
    const double taus[5] = {0.02, 0.05, 0.10, 0.15, 0.20};
    printf("hop\tstore_acc\thash_acc\trandom_floor\t(hop2 detail below)\n");
    for (uint32_t hop = 1; hop <= DEPTHMAX; hop++) {
        uint32_t succ = 0, hsucc = 0, Q = E - hop;
        double rnd = 1.0 / (E + 1); /* random guess among the E+1 known entities */
        for (uint32_t q = 0; q < Q; q++) {
            uint64_t *cur = ent + (size_t)q * W; int ok = 1;
            for (uint32_t hstep = 0; hstep < hop; hstep++) {
                ph_xor(cue, cur, rel + (size_t)((q + hstep) % 3) * W, W);
                double s0, s1; int i0 = st_retrieve(&st, cue, ans, &s0, &s1);
                if (!(i0 >= 0 && s0 - s1 > 0.05)) { ok = 0; break; }        /* abstain */
                /* decode: nearest entity to ans to get its index for next hop */
                double best = -2; uint32_t bi = 0;
                for (uint32_t e = 0; e < E + 1; e++) {
                    uint32_t pop = 0; const uint64_t *e_p = ent + (size_t)e * W;
                    for (uint32_t w = 0; w < W; w++) pop += __builtin_popcountll(ans[w]^e_p[w]);
                    double sim = 1.0 - 2.0*pop/D; if (sim > best) { best = sim; bi = e; }
                }
                if (!(best > 0.8 && bi == q + hstep + 1)) { ok = 0; break; }
                cur = ent + (size_t)bi * W;
            }
            { uint32_t hc = q; int hok = 1;
              for (uint32_t hstep = 0; hstep < hop && hok; hstep++) {
                  ph_xor(cue, ent + (size_t)hc * W, rel + (size_t)((q + hstep) % 3) * W, W);
                  int gi = hm_get(&hm, ph_digest(cue, W));
                  if (gi < 0 || (uint32_t)gi != q + hstep + 1) hok = 0; else hc = (uint32_t)gi;
              }
              if (hok) hsucc++; }
            if (ok) succ++;
        }
        printf("%u\t%.4f\t%.4f\t%.4f\n", hop, (double)succ/Q, (double)hsucc/Q, rnd);
    }

    /* abstention calibration on UNKNOWN queries (nonexistent edges) */
    { uint32_t U = 200; 
      for (int ti = 0; ti < 5; ti++) {
          double tau = taus[ti]; uint32_t cw = 0, ab = 0;
          for (uint32_t u = 0; u < U; u++) {                    /* query entity i -> rel 2 from chain mismatched: nonexistent (i, r) for shifted relation */
              uint32_t s_i = u % E; uint32_t r_j = (s_i + 2) % 3;   /* wrong relation => no edge stored for that (s,r) unless i%3==r_j */
              if ((s_i % 3) == r_j) continue;                    /* skip accidental existence */
              ph_xor(cue, ent + (size_t)s_i * W, rel + (size_t)r_j * W, W);
              double s0, s1; int i0 = st_retrieve(&st, cue, ans, &s0, &s1);
              (void)i0;
              if (s0 - s1 > tau) cw++; else ab++;                /* confident on unknown = wrongly confident */
          }
          printf("abstain_tau=%.2f  confidently_wrong_rate=%.4f  abstain_rate=%.4f\n", tau, (double)cw/U, (double)ab/U);
      }
    }

    st_free(&st); free(hm.dg); free(hm.idx); free(ent); free(rel); free(key); free(valv); free(cue); free(ans);
    printf("%s (%d fails)\n", fails ? "QC-FAIL" : "QC-PASS", fails);
    return fails ? 1 : 0;
}
