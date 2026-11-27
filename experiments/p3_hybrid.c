/* experiments/p3_hybrid.c — P3a gate: RI profiles + exact-first hybrid in the C runtime.
 * Reads results/p3_corpus.tsv (frozen P2c corpus). Arms: exact-overlap | ri-cos | hybrid(lexicographic composite).
 * Prints meta, R@1 per kind, per-synx-query AUCs for CI via tools script. */
#include "semantic/ri.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAXF 2048
typedef struct { int *ids; int n; } toks_t;
static toks_t facts[MAXF]; static int nf;
static toks_t qs[512]; static char qkind[512][8]; static int qgold[512][64], qcand[512][256];
static int ngold[512], ncandn[512], nq;

static int parse_ints(char *s, int *out){ int n=0; for(char *p=strtok(s,","); p && n<64; p=strtok(NULL,",")){ if(*p=='-')break; out[n++]=atoi(p);} return n; }

int main(int argc, char **argv){
    FILE *fp=fopen(argc>1?argv[1]:"results/p3_corpus.tsv","r"); if(!fp){printf("no tsv\n");return 2;}
    char line[4096]; uint32_t D=1024;    ri_t *R=ri_new(D,32,4096,512);   /* cap must exceed nz*(2*window): 64 overflowed function-word profiles (measured) */
    toks_t corppool[8192]; int ncorp=0;
    while(fgets(line,sizeof line,fp)){
        char *nl=strchr(line,'\n'); if(nl)*nl=0; if(line[0]=='#')continue;
        char *type=line; char *rest=strchr(line,'\t'); if(!rest) continue; *rest++=0;
        if(strcmp(type,"Q")==0){
            char *k=rest; char *g=strchr(k,'\t'); *g++=0;
            char *gs=g; char *c=strchr(g,'\t'); *c++=0; char *tk=strchr(c,'\t'); *tk++=0;
            strncpy(qkind[nq],k,7);
            ngold[nq]=parse_ints(gs,qgold[nq]);
            if(*c!='-') ncandn[nq]=parse_ints(c,qcand[nq]);
            char *save=NULL; int ids[64], m=0;
            for(char *w=strtok_r(tk," ",&save); w && m<64; w=strtok_r(NULL," ",&save)) ids[m++]=ri_intern(R,w);
            qs[nq].ids=malloc(m*sizeof(int)); memcpy(qs[nq].ids,ids,m*sizeof(int)); qs[nq].n=m; nq++;
        } else {
            char *save=NULL; int ids[64], m=0;
            for(char *w=strtok_r(rest," ",&save); w && m<64; w=strtok_r(NULL," ",&save)){
                int id=ri_intern(R,w); if(id>=0) ids[m++]=id; }
            toks_t t; t.n=m; t.ids=malloc(m*sizeof(int)); memcpy(t.ids,ids,m*sizeof(int));
            if(strcmp(type,"F")==0){ facts[nf++]=t; }
            corppool[ncorp++]=t;
        }
    }
    fclose(fp);
    clock_t t0=clock();
    for(int i=0;i<ncorp;i++) ri_observe(R,corppool[i].ids,corppool[i].n);   /* facts+bridges in file order */
    double obs_ms=(double)(clock()-t0)/CLOCKS_PER_SEC*1e3;

    int hitE=0,hitR=0,hitH=0, nlex=0, nsynx=0, nsynadj=0;
    printf("meta nf=%d nq=%d words=%u bytes=%zu observe_ms=%.1f\n",nf,nq,R->n_words,ri_bytes(R),obs_ms);
    for(int q=0;q<nq;q++){
        float *sa=R->scratch_a,*sb=R->scratch_b; ri_embed(R,qs[q].ids,qs[q].n,sa);
        static int ov[MAXF]; static double rc[MAXF], hc[MAXF];
        /* overlap via id sets */
        for(int i=0;i<nf;i++){ int c=0; for(int a=0;a<qs[q].n;a++) for(int b=0;b<facts[i].n;b++) if(qs[q].ids[a]==facts[i].ids[b]){c++;break;} ov[i]=c; }
        double bestE=-1e9,bestR=-2,bestH=-1e9; int bE=0,bR=0,bH=0;
        for(int i=0;i<nf;i++){
            ri_embed(R,facts[i].ids,facts[i].n,sb); rc[i]=ri_cos(sa,sb,D); hc[i]=ov[i]*4.0+(rc[i]+1.0);
            if(hc[i]>bestH){bestH=hc[i];bH=i;} if(rc[i]>bestR){bestR=rc[i];bR=i;}
            if(ov[i]>bestE){bestE=ov[i];bE=i;}   /* first-index tie-break like Python max */
        }
        int g[64], ngg=ngold[q]; memcpy(g,qgold[q],ngg*sizeof(int));
        int hE=0,hR=0,hH=0; for(int i=0;i<ngg;i++){ hE|= bE==g[i]; hR|= bR==g[i]; hH|= bH==g[i]; }
        double aE=-1,aR=-1,aH=-1;
        if(strcmp(qkind[q],"synx")==0){
            nsynx++; int candset[MAXF]={0}; for(int i=0;i<ncandn[q];i++) candset[qcand[q][i]]=1;
            static int isgold[MAXF]; for(int i=0;i<nf;i++) isgold[i]=0; for(int i=0;i<ngg;i++) isgold[g[i]]=1;
            #define AUC(SC,OUT) do{ double c=0; int tot=0; \
                for(int gi=0;gi<ngg;gi++) for(int i=0;i<nf;i++) if(candset[i]&&!isgold[i]){ \
                    tot++; c += (SC)[g[gi]]>(SC)[i]?1.0:((SC)[g[gi]]==(SC)[i]?0.5:0.0); } \
                OUT = tot?c/tot:-1; }while(0)
            AUC(ov,aE); AUC(rc,aR); AUC(hc,aH);
        } else if(strcmp(qkind[q],"lex")==0){ nlex++; } else nsynadj++;
        hitE+=hE; hitR+=hR; hitH+=hH;
        printf("Q %s gold %d E %d R %d H %d aE %.3f aR %.3f aH %.3f\n",qkind[q],ngg,hE,hR,hH,aE,aR,aH);
    }
    int nall=nq;
    printf("R1 all  exact %d/%d ri %d/%d hybrid %d/%d\n",hitE,nall,hitR,nall,hitH,nall);
    printf("R1 synx exact-ri-hybrid counts above per-line; nsynx=%d nlex=%d nsynadj=%d\n",nsynx,nlex,nsynadj);
    ri_free(R); return 0;
}
