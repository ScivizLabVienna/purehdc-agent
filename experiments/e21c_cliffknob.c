/* experiments/e21c_cliffknob.c — can TAM *knobs* (threshold T, residual r) push the deletion cliff deeper,
 * without memory surgery? Grid over (T,r); eval depths 7..11; report R@1 + random-cue FP. */
#include "store/tam_bitpair.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint64_t x_; static uint64_t xs(void){ x_^=x_<<13; x_^=x_>>7; x_^=x_<<17; return x_; }
#define NA 1024
static int P_=16,QN_=16,CORE=2000;
static Set **A_,**B_;
static void makecue(Set*q,int idx,int d){ int keep=P_-d; char del[256]; memset(del,0,sizeof del); int made=0;
  while(made<d){int j=(int)(xs()%P_); if(!del[j]){del[j]=1;made++;}} int w=0;
  for(int k=0;k<P_;k++) if(!del[k]) q->a[w++]=A_[idx]->a[k]; q->p=keep; }
int main(void){ x_=8817264546ull;
  A_=malloc(CORE*sizeof(Set*)); B_=malloc(CORE*sizeof(Set*));
  for(int i=0;i<CORE;i++){A_[i]=Set_random(Set_new(NA),P_);B_[i]=Set_random(Set_new(NA),QN_);}
  printf("T r | R@1 d7 d8 d9 d10 d11 (n=200) | FP/50\n");
  int TS[]={6,7,8,9}, RS[]={0,4,8};
  for(unsigned ti=0;ti<4;ti++)for(unsigned ri=0;ri<3;ri++){
    Memory*m=Memory_new(NA,P_,NA,QN_); Memory_set_threshold(m,TS[ti]); Memory_set_residual(m,RS[ri]);
    for(int i=0;i<CORE;i++) Memory_write(m,A_[i],B_[i]);
    uint64_t base=x_; printf("%d %d |",TS[ti],RS[ri]);
    int DEP[]={7,8,9,10,11};
    for(int di=0;di<5;di++){ x_=base; int hit=0; Set*q=Set_new(NA);
      for(int t=0;t<200;t++){ int i=(int)(xs()%CORE); makecue(q,i,DEP[di]);
        Set*Y=Memory_read(m,q,Set_new(QN_)); int ok=Y->p>0;
        for(int k=0;k<B_[i]->p&&ok;k++){int f=0;for(int j=0;j<Y->p;j++) if(Y->a[j]==B_[i]->a[k]){f=1;break;} if(!f)ok=0;}
        hit+=ok;} printf(" %3d",hit); }
    x_=base^0x5555; int fp=0; Set*q=Set_new(NA);
    for(int t=0;t<50;t++){ Set_random(q,P_); Set*Y=Memory_read(m,q,Set_new(QN_)); if(Y->p>0)fp++; }
    printf(" | %d\n",fp);
  }
  return 0; }
