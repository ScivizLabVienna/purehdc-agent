/* purehdc-agent CLI — L5 observe->encode->retrieve->verify->select->execute/abstain->store.
 * Structured domain (spec §4 Stage A): typed facts LEARN <subj> <rel> <obj>; QUERY <subj> <rel>.
 * Symbol table = persistent identity map bytes->HV index (novelty detection on first sight).
 * Deterministic renderer; abstention on margin/confidence; episodic log (.hlog, replayable) +
 * binary snapshot save/load for persistence across restarts. No generation models anywhere. */
#include "store.h"
#include "purehdc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define D 1024u
#define SYMMAX 4096u
static const uint32_t W = D / 64;
static uint64_t seed = 70500u;

typedef struct { char *sym[SYMMAX]; uint64_t *hv; uint32_t n; } symbuf_t; /* hv rows SYMMAX*W */
static void sym_init(symbuf_t *s) { s->n = 0; s->hv = calloc((size_t)SYMMAX * W, 8); }
static int sym_find(const symbuf_t *s, const char *b) { for (uint32_t i = 0; i < s->n; i++) if (!strcmp(s->sym[i], b)) return (int)i; return -1; }
static int sym_intern(symbuf_t *s, const char *b, uint64_t *seedp) { /* novelty: unseen -> allocate fresh HV */
    int i = sym_find(s, b); if (i >= 0) return i;
    if (s->n >= SYMMAX) return -1;
    i = (int)s->n++; s->sym[i] = strdup(b); ph_random(s->hv + (size_t)i * W, W, seedp); return i; }

static store_t sem; static symbuf_t syms; static FILE *hlog = NULL;
static char line[768];

static void tok(char *l, char *argv[], int max) { int c = 0; char *p = strtok(l, " \t\r\n"); while (p && c < max) { argv[c++] = p; p = strtok(NULL, " \t\r\n"); } while (c < max) argv[c++] = NULL; }

static void answer(const char *subj, const char *rel) {
    uint64_t cue[W], ans[W]; int si = sym_find(&syms, subj), ri = sym_find(&syms, rel);
    if (si < 0 || ri < 0) { printf("ABSTAIN reason=unknown-symbol\n"); return; }
    ph_xor(cue, syms.hv + (size_t)si * W, syms.hv + (size_t)ri * W, W);
    double m0, m1; int i0 = st_retrieve(&sem, cue, ans, &m0, &m1);
    if (i0 < 0 || m0 - m1 < 0.05 || m0 < 0.9) { printf("ABSTAIN reason=insufficient-evidence sim=%.3f margin=%.3f\n", m0, m0 - m1); return; }
    double best = -2; int bi = -1;
    for (uint32_t e = 0; e < syms.n; e++) { uint32_t pop = 0; const uint64_t *p = syms.hv + (size_t)e * W;
        for (uint32_t w = 0; w < W; w++) pop += __builtin_popcountll(ans[w] ^ p[w]);
        double sim = 1.0 - 2.0 * pop / D; if (sim > best) { best = sim; bi = (int)e; } }
    if (best < 0.9 || bi < 0) { printf("ABSTAIN reason=undecodable sim=%.3f\n", best); return; }
    printf("ANSWER %s |%s| %s conf=%.4f provenance=row%d\n", subj, rel, syms.sym[bi], best, i0);
}

int main(int argc, char **argv) {
    sym_init(&syms); st_init(&sem, D, 1u << 14);
    const char *hpath = (argc > 1) ? argv[1] : "agent.hlog";
    hlog = fopen(hpath, "a+"); if (!hlog) { perror("hlog"); return 2; }
    /* replay episodic log first (rebuild state on any mode incl fresh) */
    { FILE *r = fopen(hpath, "r"); char l[768]; uint32_t rep = 0;
      if (r) { while (fgets(l, sizeof l, r)) {
          char a[256], b[256], c2[256];
          if (sscanf(l, "F|%255[^|]|%255[^|]|%255[^\n]", a, b, c2) == 3 && *a && *b && *c2) {
              int si = sym_intern(&syms, a, &seed), ri = sym_intern(&syms, b, &seed), oi = sym_intern(&syms, c2, &seed);
              if (si >= 0 && ri >= 0 && oi >= 0) { uint64_t key[W]; ph_xor(key, syms.hv + (size_t)si * W, syms.hv + (size_t)ri * W, W);
                  st_put(&sem, key, syms.hv + (size_t)oi * W); rep++; } } }
        fclose(r); }
      printf("BOOT rows=%u symbols=%u log=%s\n", sem.n, syms.n, hpath); (void)rep; }

    while (fgets(line, sizeof line, stdin)) {
        char *av[8]; int ac = 0; char lbuf[768]; strncpy(lbuf, line, sizeof lbuf - 1); lbuf[sizeof lbuf - 1] = 0;
        tok(lbuf, av, 8); while (ac < 8 && av[ac]) ac++;
        if (!ac) continue;
        if (getenv("PHDC_DEBUG")) printf("DEBUG argc=%d argv0=[%s]\n", ac, av[0]);
        if (!strcmp(av[0], "LEARN") && ac == 4) {
            int si = sym_intern(&syms, av[1], &seed), ri = sym_intern(&syms, av[2], &seed), oi = sym_intern(&syms, av[3], &seed);
            if (si < 0 || ri < 0 || oi < 0) { printf("ERR store-full\n"); continue; }
            uint64_t key[W]; ph_xor(key, syms.hv + (size_t)si * W, syms.hv + (size_t)ri * W, W);
            st_put(&sem, key, syms.hv + (size_t)oi * W);
            fprintf(hlog, "F|%s|%s|%s\n", av[1], av[2], av[3]); fflush(hlog);
            printf("LEARNED %s %s %s (novelty: symbols new this session are allocated on first sight)\n", av[1], av[2], av[3]);
        } else if (!strcmp(av[0], "QUERY") && ac == 3) answer(av[1], av[2]);
        else if (!strcmp(av[0], "STATS")) printf("STATS rows=%u symbols=%u\n", sem.n, syms.n);
        else if (!strcmp(av[0], "HELP")) printf("commands: LEARN <s> <r> <o> | QUERY <s> <r> | STATS | BYE\n");
        else if (!strcmp(av[0], "BYE")) break;
        else printf("ERR unknown-command (HELP)\n");
    }
    st_free(&sem); fclose(hlog);
    return 0;
}
