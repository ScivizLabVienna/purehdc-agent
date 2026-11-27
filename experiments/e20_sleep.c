/* experiments/e20_sleep.c — E20: "sleep" = offline replay consolidating by repetition count
 * (filter one-shot episodes, promote episodes seen >=K times) vs wake-state verbatim storage.
 * Pre-registered BEFORE run: P1 junk-heavy -> sleep recall>=wake & sleep contamination<wake;
 * P2 zero junk -> arms equal; P3 one-shot-only stream -> sleep forgets, wake recalls (tradeoff). */
#include "store/tam_bitpair.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t xs_; static uint64_t xs(void){ xs_^=xs_<<13; xs_^=xs_>>7; xs_^=xs_<<17; return xs_; }
typedef struct { int kind, idx; } Epi;
static Set **A0,**B0,**AJ,**BJ; static int na_,p_,nb_,pb_;
static Set *getA(Epi e){ if(!e.kind) return A0[e.idx]; if(!AJ[e.idx]) AJ[e.idx]=Set_random(Set_new(na_),p_); return AJ[e.idx]; }
static Set *getB(Epi e){ if(!e.kind) return B0[e.idx]; if(!BJ[e.idx]) BJ[e.idx]=Set_random(Set_new(nb_),pb_); return BJ[e.idx]; }

typedef struct { uint64_t key; int cnt; } Cnt;           /* generic occurrence tally, open addressing */
static uint64_t setfnv(Set *A){ uint64_t h=1469598103934665603ull;
    for(int i=0;i<A->p;i++){ h^=(uint64_t)A->a[i]; h*=1099511628211ull; } return h; }
static int cnt_find(Cnt*t,int cap,uint64_t k,int inc){ uint32_t i=(uint32_t)(k%(uint32_t)cap);
    while(t[i].key&&t[i].key!=k) i=(i+1)%cap;
    if(!t[i].key){ t[i].key=k; t[i].cnt=0; }
    if(inc) t[i].cnt++; return t[i].cnt; }

static int contains(Set *Y, Set *B){ for(int k=0;k<B->p;k++){ int ok=0;
        for(int j=0;j<Y->p;j++) if(Y->a[j]==B->a[k]){ok=1;break;} if(!ok) return 0; } return Y->p>0; }

int main(int argc,char**argv){
    int CORE=50, REP=atoi(argc>1?argv[1]:"8"), JUNK=atoi(argc>2?argv[2]:"1000");
    const int K=2, NOISE=atoi(argc>3?argv[3]:"4");
    na_=atoi(argc>4?argv[4]:"1024"); p_=16; nb_=na_; pb_=16; xs_=88172645463ull;
    A0=malloc(CORE*sizeof(Set*)); B0=malloc(CORE*sizeof(Set*));
    for(int i=0;i<CORE;i++){ A0[i]=Set_random(Set_new(na_),p_); B0[i]=Set_random(Set_new(nb_),pb_); }
    AJ=calloc(JUNK?JUNK:1,sizeof(Set*)); BJ=calloc(JUNK?JUNK:1,sizeof(Set*));
    int N=CORE*REP+JUNK; Epi *lg=malloc(N*sizeof(Epi));
    for(int i=0;i<CORE;i++) for(int r=0;r<REP;r++){ lg[i*REP+r].kind=0; lg[i*REP+r].idx=i; }
    for(int j=0;j<JUNK;j++){ lg[CORE*REP+j].kind=1; lg[CORE*REP+j].idx=j; }
    for(int i=N-1;i>0;i--){ int t=(int)(xs()%(uint32_t)(i+1)); Epi e=lg[i];lg[i]=lg[t];lg[t]=e; }

    clock_t t0=clock();
    Memory *MW=Memory_new(na_,p_,nb_,pb_);               /* WAKE: every episode verbatim */
    for(int i=0;i<N;i++) Memory_write(MW,getA(lg[i]),getB(lg[i]));
    double wake_ms=(double)(clock()-t0)/CLOCKS_PER_SEC*1e3;

    t0=clock();                                          /* SLEEP: tally pass + promote pass (>=K, dedup) */
    int TC=2*N+64; Cnt *tally=calloc(TC,sizeof(Cnt)), *wrote=calloc(TC,sizeof(Cnt));
    for(int i=0;i<N;i++) cnt_find(tally,TC,setfnv(getA(lg[i])),1);
    Memory *MS=Memory_new(na_,p_,nb_,pb_); int promoted=0;
    for(int i=0;i<N;i++){ uint64_t k=setfnv(getA(lg[i]));
        if(cnt_find(tally,TC,k,0)>=K && cnt_find(wrote,TC,k,1)==1){ Memory_write(MS,getA(lg[i]),getB(lg[i])); promoted++; } }
    double sleep_ms=(double)(clock()-t0)/CLOCKS_PER_SEC*1e3;

    const int Q=200; int hitW=0,hitS=0,contW=0,contS=0,fpW=0,fpS=0;
    Set *q=Set_new(na_),*Y=Set_new(nb_);
    for(int t=0;t<Q;t++){
        int i=(int)(xs()%(uint32_t)CORE);
        Set_copy(q,A0[i]); q->p = p_-NOISE;                       /* cue = stored A minus last NOISE */
        Memory_read(MW,q,Y); { int h=contains(Y,B0[i]); hitW+=h; contW+=(!h&&Y->p>0); }
        Memory_read(MS,q,Y); { int h=contains(Y,B0[i]); hitS+=h; contS+=(!h&&Y->p>0); }
    }
    for(int t=0;t<50;t++){                                         /* negative control */
        Set_copy(q,A0[0]); q->a[0]=(q->a[0]+517)%na_;              /* perturbed cue: expect abstain/wrong, just measure both equally */
        Memory_read(MW,q,Y); if(Y->p>0) fpW++;
        Memory_read(MS,q,Y); if(Y->p>0) fpS++;
    }
    printf("E20 rep=%d junk=%d noise=%d N=%d promoted=%d | ms wake %.1f sleep %.1f | R@1 W %d/200 S %d/200"
           " | contam W %.3f S %.3f | answered-cue FP W %d/50 S %d/50 | 1-bits W %lld S %lld\n",
        REP,JUNK,NOISE,N,promoted,wake_ms,sleep_ms,hitW,hitS,contW/200.0,contS/200.0,fpW,fpS,
        (long long)Memory_count(MW),(long long)Memory_count(MS));
    return 0;
}
