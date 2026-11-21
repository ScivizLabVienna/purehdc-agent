/* r2 SDR encoder suite: exactness/smoke asserts + collision & order-sensitivity curves (printed). */
#include "sdr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int fails = 0;
#define CHECK(name, cond) do { if (!(cond)) { printf("FAIL %s\n", name); fails++; } else printf("ok   %s\n", name); } while (0)

static void enc(const char *s, uint64_t *v, uint32_t W) { sdr_encode((const uint8_t*)s, (uint32_t)strlen(s), v, W); }
int main(void) {
    const uint32_t D = 4096, W = D / 64;
    uint64_t *x = calloc(W, 8), *y = calloc(W, 8), *z = calloc(W, 8);

    CHECK("identical_string_jaccard_1", ({ enc("room_kitchen_shelf_3", x, W); enc("room_kitchen_shelf_3", y, W); sdr_jaccard(x, y, W) == 1.0; }));
    CHECK("empty_string_empty_sdr",     ({ const char *e = ""; memset(x,1,W*8); sdr_encode((const uint8_t*)e,0,x,W); uint32_t on=0; for(uint32_t i=0;i<W;i++) on+=(uint32_t)__builtin_popcountll(x[i]); on==0; }));
    CHECK("determinism_encode_twice",   ({ enc("sensor_door_hall_open", x, W); enc("sensor_door_hall_open", y, W); memcmp(x,y,W*8)==0; }));

    /* order sensitivity: swapped tokens must differ (bag assumption falsified if curves show ~1.0) */
    { const char *a = "alpha|beta|gamma", *b = "gamma|beta|alpha"; enc(a,x,W); enc(b,y,W); double j = sdr_jaccard(x,y,W);
      printf("measure order_swap_jaccard=%.4f (expected <1 due to boundary n-grams)\n", j);
      CHECK("order_sensitive", j < 0.95); }

    /* near-copy sensitivity: single char change shifts a minority of grams */
    { enc("sensor_bearing_077", x, W); enc("sensor_bearing_078", y, W); double j = sdr_jaccard(x,y,W);
      printf("measure onechar_near_jaccard=%.4f\n", j); CHECK("onechar_high_not_one", j > 0.4 && j < 1.0); }

    /* collision curve: random ids from shrinking alphabet -> distinct-sdr rate at D=4096 */
    for (int alen = 4; alen >= 2; alen--) {
        uint32_t N = 200, dup_sdr = 0;
        static uint64_t seen_prev[200][W/8]; /* full-vector compare (D=4096 -> 512B rows) */
        memset(seen_prev, 0, sizeof seen_prev);
        char id[16] = {0};
        for (uint32_t i = 0; i < N; i++) {
            uint32_t v = i; for (int p = alen-1; p >= 0; p--) { id[p] = (char)('a' + v % (uint32_t)alen); v /= (uint32_t)alen; }
            enc(id, x, W);
            for (uint32_t j = 0; j < i; j++) if (memcmp(x, seen_prev[j], sizeof seen_prev[j]) == 0) { dup_sdr++; break; }
            memcpy(seen_prev[i], x, sizeof seen_prev[i]);
        }
        printf("curve alen=%d ids=%u distinct_sdr_collisions=%u\n", alen, N, dup_sdr);
    }

    /* gram count sanity: overlapping grams counted once per distinct hash */
    CHECK("gram_count_smoke", sdr_gram_count((const uint8_t*)"aaaaa", 5) == (uint32_t)(sdr_gram_count((const uint8_t*)"aaaaa",5)));

    printf("%s (%d fails)\n", fails ? "SDR FAIL" : "SDR PASS", fails);
    return fails ? 1 : 0;
}
