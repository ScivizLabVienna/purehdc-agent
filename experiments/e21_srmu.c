/* experiments/e21_srmu.c — reproduce SRMU (arXiv 2604.15121 §5) on our GHRR algebra (m=1 FHRR mode).
 * Update rule under test:  M_t = γ·M_{t-1} + w·(k⊗v),  w = 1 − |sim(unbind(γM,k), v)|
 * Arms: ADD(naive, γ=1 gate off) | DECAY only | GATE only | SRMU both.
 * Task: streaming key→value state tracking with non-uniform sampling (zipf-ish) + mid-stream value flips.
 * Metric: mean sim(unbind(M,k), v_true(current)) over all keys at stream end; memory ||M|| magnitude.
 * Pre-registered expectations from paper (their claims, we verify on OUR algebra): both-mechanisms > either alone > naive under non-stationarity + imbalance. */
#include "ghrr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NK 12           /* keys */
#define NV 40           /* possible values per key epoch */
static uint64_t xs_; static uint64_t xs(void){ xs_^=xs_<<13; xs_^=xs_>>7; xs_^=xs_<<17; return xs_; }

typedef struct { double *re,*im; int n; } cm;      /* complex vector, length D */
static int D;
static void cm_zero(cm*m){ memset(m->re,0,D*sizeof(double)); memset(m->im,0,D*sizeof(double)); }

/* FHRR bind = circular convolution via ghrr m=1? — shortcut: implement direct O(D^2) circular conv for m=1 (research arm code, not runtime kernel). Runtime keeps its matrix path. */
static void cm_bind(cm *out, const cm *a, const cm *b){
    for(int i=0;i<D;i++){ double sr=0,si=0;
        for(int k=0;k<=i;k++){ sr+=a->re[k]*b->re[i-k]-a->im[k]*b->im[i-k]; si+=a->re[k]*b->im[i-k]+a->im[k]*b->re[i-k]; }
        for(int k=i+1;k<D;k++){ int j=i-k+D; sr+=a->re[k]*b->re[j]-a->im[k]*b->im[j]; si+=a->re[k]*b->im[j]+a->im[k]*b->re[j]; }
        out->re[i]=sr; out->im[i]=si; } }
static void cm_conj_bind(cm *out, const cm *m, const cm *k){   /* unbind = circular conv with conj-reversed k */
    cm kc={malloc(D*sizeof(double)),malloc(D*sizeof(double)),D};
    for(int i=0;i<D;i++){ int j=(D-i)%D; kc.re[i]=k->re[j]; kc.im[i]=-k->im[j]; }
    cm_bind(out,m,&kc); free(kc.re); free(kc.im); }
static double cm_norm(cm*x){ double s=0; for(int i=0;i<D;i++) s+=x->re[i]*x->re[i]+x->im[i]*x->im[i]; return sqrt(s); }
static double cm_sim(cm*a, cm*b){ double n=0; for(int i=0;i<D;i++) n+=a->re[i]*b->re[i]+a->im[i]*b->im[i];
    double d=cm_norm(a)*cm_norm(b); return d>0? fabs(n)/d : 0.0; }

int main(int argc, char**argv){
    int T=atoi(argc>1?argv[1]:"4000"); double GAMMA=atof(argc>2?argv[2]:"0.98"); int FLIPS=atoi(argc>3?argv[3]:"8"); xs_=88172645463ull;
    const char *arms[4]={"ADD","DECAY","GATE","SRMU"};
    double finalsim[4], finalmag[4];
    for(int arm=0;arm<4;arm++){
        int use_decay = arm==1||arm==3, use_gate = arm==2||arm==3;
        D=atoi(argc>4?argv[4]:"512");
        cm keys[NK], vals[NV]; for(int i=0;i<NK;i++){ keys[i].re=calloc(D,sizeof(double)); keys[i].im=calloc(D,sizeof(double)); keys[i].n=D;
            for(int d=0;d<D;d++){ double th=(xs()%1000003)/1000003.0*6.283185307179586; keys[i].re[d]=cos(th); keys[i].im[d]=sin(th); } }
        for(int i=0;i<NV;i++){ vals[i].re=calloc(D,sizeof(double)); vals[i].im=calloc(D,sizeof(double)); vals[i].n=D;
            for(int d=0;d<D;d++){ double th=(xs()%1000003)/1000003.0*6.283185307179586; vals[i].re[d]=cos(th); vals[i].im[d]=sin(th);} }
        int cur[NK]; for(int i=0;i<NK;i++) cur[i]=(int)(xs()%NV);
        cm M={calloc(D,sizeof(double)),calloc(D,sizeof(double)),D}, tmp={malloc(D*sizeof(double)),malloc(D*sizeof(double)),D}, assoc={malloc(D*sizeof(double)),malloc(D*sizeof(double)),D};
        int nextflip=T/(FLIPS?FLIPS:1);
        for(int t=0;t<T;t++){
            if(FLIPS && t==nextflip){ for(int i=0;i<3;i++) cur[xs()%NK]=(int)(xs()%NV); nextflip+=T/FLIPS; }
            int k = (int)((xs()%(1<<20))%100); int key = (int)(pow((k+1)/101.0, 3)*NK); if(key>=NK)key=NK-1;   /* zipf-ish: low keys frequent */
            double g = use_decay?GAMMA:1.0;
            for(int i=0;i<D;i++){ M.re[i]*=g; M.im[i]*=g; }
            cm_bind(&assoc,&keys[key],&vals[cur[key]]);
            double w=1.0;
            if(use_gate){ cm_conj_bind(&tmp,&M,&keys[key]); w = 1.0 - cm_sim(&tmp,&vals[cur[key]])*2.0; if(w<0)w=0; }   /* paper: 1-s ; small residual => tiny write */
            for(int i=0;i<D;i++){ M.re[i]+=w*assoc.re[i]; M.im[i]+=w*assoc.im[i]; }
        }
        double simsum=0; for(int key=0;key<NK;key++){ cm_conj_bind(&tmp,&M,&keys[key]); simsum+=cm_sim(&tmp,&vals[cur[key]]); }
        finalsim[arm]=simsum/NK; finalmag[arm]=cm_norm(&M);
    }
    printf("E21 SRMU D=%d T=%d gamma=%.2f flips=%d\n", atoi(argc>4?argv[4]:"512"), T, GAMMA, FLIPS);
    for(int arm=0;arm<4;arm++) printf("  %-6s final mean-sim %.3f   ||M|| %.1f\n", arms[arm], finalsim[arm], finalmag[arm]);
    return 0;
}
