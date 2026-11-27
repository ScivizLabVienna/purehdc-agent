/* ri.c — random-indexing word profiles (see ri.h). House style: init/free/pure ops, no hidden state. */
#include "semantic/ri.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
static uint64_t sm64(uint64_t *s){ uint64_t z=(*s+=0x9E3779B97F4A7C15ull);
  z=(z^(z>>30))*0xBF58476D1CE4E5B9ull; z=(z^(z>>27))*0x94D049BB133111EBull; return z^(z>>31); }
static uint64_t fnv(const char *s){ uint64_t h=1469598103934665603ull; for(;*s;s++){h^=(uint8_t)*s;h*=1099511628211ull;} return h; }

ri_t *ri_new(uint32_t D, uint32_t nz, uint32_t max_words, uint32_t ent_cap){
    ri_t *r = calloc(1, sizeof *r); if(!r) return NULL;
    r->D=D; r->nz=nz; r->max_words=max_words; r->ent_cap=ent_cap;
    r->words = calloc(max_words, sizeof(ri_word));
    r->scratch_a = calloc(D, sizeof(float));
    r->scratch_b = calloc(D, sizeof(float));
    return r;
}
void ri_free(ri_t *r){ if(!r)return;
    for(uint32_t i=0;i<r->n_words;i++){ free(r->words[i].name); free(r->words[i].ents); }
    free(r->words); free(r->scratch_a); free(r->scratch_b); free(r); }

int ri_intern(ri_t *r, const char *w){
    for(uint32_t i=0;i<r->n_words;i++) if(strcmp(r->words[i].name,w)==0) return (int)i;
    if(r->n_words>=r->max_words) return -1;                 /* bounded */
    ri_word *wd=&r->words[r->n_words];
    wd->name=strdup(w); wd->cap=r->ent_cap; wd->ents=calloc(wd->cap,sizeof(ri_ent)); wd->n=0;
    return (int)r->n_words++;
}

/* context coords/signs: deterministic stream from word hash (collisions = rare benign noise) */
typedef struct { uint16_t c[32]; int8_t s[32]; } ri_ctx;   /* nz=32 to match P2c tool (D=1024) */
static void ctx_of(uint32_t D, uint32_t nz, const char *name, ri_ctx *cx){
    uint64_t seed=fnv(name)^0xA5A5A5A5ull;
    for(uint32_t j=0;j<nz;j++){ uint64_t v=sm64(&seed); uint16_t c=(uint16_t)(v%D);
        /* sample WITHOUT replacement: collisions in one word's own ctx flip RI signal (measured AUC 0.385 vs 0.667) */
        for(uint32_t t=0;;t++){ uint32_t k=0; while(k<j && cx->c[k]!=c) k++; if(k==j)break; c=(uint16_t)((c+1u)%D); }
        cx->c[j]=c; cx->s[j]=(v>>63)?-1:1; }
}

void ri_observe(ri_t *r, const int *ids, int ntok){
    for(int i=0;i<ntok;i++){
        if(ids[i]<0) continue;
        ri_ctx ci; ctx_of(r->D,r->nz,r->words[ids[i]].name,&ci);
        for(int j=(i-2<0?0:i-2); j<ntok && j<=i+2; j++){
            if(j==i || ids[j]<0) continue;
            ri_ctx cj; ctx_of(r->D,r->nz,r->words[ids[j]].name,&cj);
            for(uint32_t a=0;a<r->nz;a++) for(uint32_t b=0;b<r->nz;b++) if(ci.c[a]==cj.c[b]){
                float prod=(float)ci.s[a]*cj.s[b];
                ri_word *wd=&r->words[ids[i]];
                uint32_t k=0; while(k<wd->n && wd->ents[k].coord!=ci.c[a]) k++;
                if(k==wd->n){ if(wd->n>=wd->cap) continue;      /* overflow: drop (documented) */
                    wd->ents[wd->n].coord=ci.c[a]; wd->ents[wd->n].val=0; wd->n++; }
                wd->ents[k].val+=prod;
            }
        }
    }
}

void ri_embed(const ri_t *r, const int *ids, int n, float *out){
    memset(out,0,r->D*sizeof(float));
    for(int i=0;i<n;i++){ if(ids[i]<0)continue; const ri_word*wd=&r->words[ids[i]];
        for(uint32_t k=0;k<wd->n;k++) out[wd->ents[k].coord]+=wd->ents[k].val; }
}
double ri_cos(const float *a, const float *b, uint32_t D){
    double num=0,na=0,nb=0;
    for(uint32_t i=0;i<D;i++){ num+=(double)a[i]*b[i]; na+=(double)a[i]*a[i]; nb+=(double)b[i]*b[i]; }
    return (na>0&&nb>0)? num/sqrt(na*nb) : 0.0;
}
size_t ri_bytes(const ri_t *r){
    size_t t=sizeof *r + (size_t)r->max_words*sizeof(ri_word)+2*(size_t)r->D*sizeof(float);
    for(uint32_t i=0;i<r->n_words;i++) t+=strlen(r->words[i].name)+1+(size_t)r->words[i].cap*sizeof(ri_ent);
    return t; }
