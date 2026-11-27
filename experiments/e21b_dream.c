/* experiments/e21b_dream.c — REM-style generative replay using ONLY the agent's own memory as generator.
 * Mechanism (ordinary algorithm + bitpair TAM): each dream cycle, sample stored A sets, delete a random
 * subset to fabricate a partial cue, READ own memory for pseudo-answer, WRITE cue'->Y back (self-rehearsal).
 * Pre-registered: P1 deep-deletion R@1 improves with dream cycles; P2 exact/shallow-cue accuracy must NOT
 * degrade (distortion check) + random-cue FP should stay 0; P3 cost per cycle reported. */
#include "store/tam_bitpair.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t xs1_, xs2_; 
static uint64_t xsq(void){ xs1_^=xs1_<<13; xs1_^=xs1_>>7; xs1_^=xs1_<<17; return xs1_; }   /* eval stream */
static uint64_t xsd(void){ xs2_^=xs2_<<13; xs2_^=xs2_>>7; xs2_^=xs2_<<17; return xs2_; }   /* dream stream (disjoint!) */

#define NA 1024
static int P_=16, QN_=16, CORE;
static Set **A_,**B_;
static Memory *M_;

/* cue = stored A minus d random elements (keep sorted) */
static void makecue(Set *q, int idx, int d, uint64_t (*rnd)(void)){
    int keep=P_-d; char del[256]; memset(del,0,sizeof del);
    int made=0; while(made<d){ int j=(int)(rnd()%P_); if(!del[j]){del[j]=1;made++;} }
    int w=0; for(int k=0;k<P_;k++) if(!del[k]) q->a[w++]=A_[idx]->a[k];
    q->p=keep;
}
static int readok(Set *q, int idx){ Set *Y=Memory_read(M_,q,Set_new(QN_)); /* NOTE: leaks Y per call (bounded experiment) */
    int ok=Y->p>0; for(int k=0;k<B_[idx]->p&&ok;k++){ int f=0; for(int j=0;j<Y->p;j++) if(Y->a[j]==B_[idx]->a[k]){f=1;break;} if(!f)ok=0; }
    return ok; }

int main(int argc,char**argv){
    CORE=atoi(argc>1?argv[1]:"200"); int SAMPLES=atoi(argc>2?argv[2]:"2000"); int MAXCYC=atoi(argc>3?argv[3]:"20");
    xs1_=8817264546ull; xs2_=123456789ull;   /* disjoint streams so dream cues != eval cues structurally */
    A_=malloc(CORE*sizeof(Set*)); B_=malloc(CORE*sizeof(Set*));
    M_=Memory_new(NA,P_,NA,QN_);
    for(int i=0;i<CORE;i++){ A_[i]=Set_random(Set_new(NA),P_); B_[i]=Set_random(Set_new(NA),QN_); Memory_write(M_,A_[i],B_[i]); }

    const int DEPTHS[]={7,8}; const int DQ=200;   // transfer window right at the native cliff edge
    printf("E21b core=%d samples/cycle=%d\n",CORE,SAMPLES);
    for(int cyc=0;cyc<=MAXCYC;cyc++){
        if(cyc>0){   /* one dream cycle: generative replay from own memory only */
            clock_t t0=clock(); Set *q=Set_new(NA);
            int wrotes=0;
            for(int s=0;s<SAMPLES;s++){
                int i=(int)(xsd()%CORE); int d=(int)(xsd()%11);         /* depth 0..10 — rehearse across the cliff */
                if(d>=P_){ continue; }
                makecue(q,i,d,xsd);
                Set *Y=Memory_read(M_,q,Set_new(QN_));
                if(Y->p>0){ Memory_write(M_,q,Y); wrotes++; }           /* rehearse self-generated pair */
            }
            printf("  dream cycle %2d: wrote %d/%d pseudo-episodes  %.1f ms\n",cyc,wrotes,SAMPLES,
                   (double)(clock()-t0)/CLOCKS_PER_SEC*1e3);
        }
        /* eval: fresh cues from xs1 stream (identical query draws each cyc — paired comparison) */
        uint64_t snap=xs1_;   /* reset eval stream so queries identical across cycles */
        xs1_=8817264546ull;
        for(int di=0;di<3;di++){ int hit=0; Set *q=Set_new(NA);
            for(int t=0;t<DQ;t++){ int i=(int)(xsq()%CORE); makecue(q,i,DEPTHS[di],xsq); hit+=readok(q,i); }
            printf("  R@1 depth-%d %3d/%d |",DEPTHS[di],hit,DQ);
        }
        { Set *q=Set_new(NA); int fp=0; for(int t=0;t<50;t++){ Set_random(q,P_); Set*Y=Memory_read(M_,q,Set_new(QN_)); if(Y->p>0)fp++; } printf(" FP %d/50\n",fp); }
        xs1_=snap; xsq(); /* advance eval stream one step per cycle so draws still progress? no—paired: keep fixed. (kept for clarity) */
    }
    return 0;
}
