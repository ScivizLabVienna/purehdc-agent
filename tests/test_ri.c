/* tests/test_ri.c — gate RI primitive: bridge co-occurrence must pull a synonym toward taught nouns. */
#include "semantic/ri.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int tok(const ri_t *r, char *s, int *out){ int n=0;
    for(char *w=strtok(s," "); w && n<16; w=strtok(NULL," ")){ int id=ri_intern((ri_t*)r,w); if(id>=0) out[n++]=id; } return n; }

int main(void){
    ri_t *R = ri_new(1024, 32, 256, 64);
    const char *corp[] = {"a thing such as alpha","a thing such as beta","a thing such as gamma",
                          "a object such as delta","the zeta is on the kapa","the beta runs fast"};
    for(int i=0;i<6;i++){ char b[128]; strcpy(b,corp[i]); int ids[16]; int n=tok(R,b,ids); ri_observe(R,ids,n); }

    const char *ws[] = {"alpha","beta","gamma","delta","zeta","kapa"};
    char q[16]; strcpy(q,"thing"); int qi[1]; int qn=tok(R,q,qi);
    ri_embed(R,qi,qn,R->scratch_a);
    double cs[6];
    for(int i=0;i<6;i++){ char w[16]; strcpy(w,ws[i]); int id[1]; int n=tok(R,w,id); ri_embed(R,id,n,R->scratch_b); cs[i]=ri_cos(R->scratch_a,R->scratch_b,1024); }
    printf("cos(thing | alpha beta gamma delta zeta kapa) = %.3f %.3f %.3f %.3f %.3f %.3f\n",cs[0],cs[1],cs[2],cs[3],cs[4],cs[5]);
    for(int i=0;i<3;i++) assert(cs[i]>cs[4] && cs[i]>cs[5]);   /* thing-bridge beats untaught nouns */

    strcpy(q,"object"); qn=tok(R,q,qi); ri_embed(R,qi,qn,R->scratch_a);
    for(int i=0;i<6;i++){ char w[16]; strcpy(w,ws[i]); int id[1]; int n=tok(R,w,id); ri_embed(R,id,n,R->scratch_b); cs[i]=ri_cos(R->scratch_a,R->scratch_b,1024); }
    printf("cos(object | alpha beta gamma delta zeta kapa) = %.3f %.3f %.3f %.3f %.3f %.3f\n",cs[0],cs[1],cs[2],cs[3],cs[4],cs[5]);
    assert(cs[3]>cs[4] && cs[3]>cs[5]);                         /* delta reachable only via object bridge */
    printf("TEST_RI PASS words=%u bytes=%zu\n",R->n_words,ri_bytes(R)); ri_free(R); return 0;
}
