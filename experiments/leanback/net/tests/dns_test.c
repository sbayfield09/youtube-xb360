#include "dns_wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned long random_state=1;
static unsigned rnd(void){random_state=random_state*1664525+1013904223;return (unsigned)random_state;}
int main(void){
    unsigned char p[1024],valid[1024],ip[4];size_t n=lb_dns_query(p,sizeof p,0x1234,"www.youtube.com"),q=n;unsigned i;
    static const unsigned char a[]={0xc0,12,0,1,0,1,0,0,0,30,0,4,142,250,10,20};
    assert(n && !lb_dns_query(p,12,1,"www.youtube.com"));
    p[2]=0x81;p[3]=0x80;p[7]=1;memcpy(p+n,a,sizeof a);n+=sizeof a;
    assert(lb_dns_answer(p,n,0x1234,"www.youtube.com",ip)==1 && ip[0]==142 && ip[3]==20);
    assert(lb_dns_answer(p,n,0x1235,"www.youtube.com",ip)==0);
    assert(lb_dns_answer(p,n,0x1234,"bad.youtube.com",ip)==0);
    p[2]|=2;assert(lb_dns_answer(p,n,0x1234,"www.youtube.com",ip)==-1);p[2]&=~2;
    p[n-4]=127;assert(lb_dns_answer(p,n,0x1234,"www.youtube.com",ip)==-1);p[n-4]=142;
    for(i=0;i<n;++i)assert(lb_dns_answer(p,i,0x1234,"www.youtube.com",ip)!=1);
    memcpy(valid,p,n);
    p[q]=0xc0;p[q+1]=(unsigned char)q;assert(lb_dns_answer(p,n,0x1234,"www.youtube.com",ip)==0);
    /* Compressed CNAME followed by its correctly named A record. */
    memcpy(p,valid,q);p[7]=2;
    {static const unsigned char c[]={0xc0,12,0,5,0,1,0,0,0,30,0,7,4,'e','d','g','e',0xc0,16};
     size_t alias=q+12;memcpy(p+q,c,sizeof c);n=q+sizeof c;memcpy(p+n,a,sizeof a);p[n+1]=(unsigned char)alias;n+=sizeof a;}
    assert(lb_dns_answer(p,n,0x1234,"www.youtube.com",ip)==1);
    /* Deterministic bounded malformed-packet exercise under ASan/UBSan. */
    for(i=0;i<20000;++i){size_t j,len=rnd()%sizeof p;for(j=0;j<len;++j)p[j]=(unsigned char)rnd();(void)lb_dns_answer(p,len,0x1234,"www.youtube.com",ip);}
    puts("PASS: DNS A/CNAME/compression, transaction/question match, truncation, malformed names, public-IP check, 20000 malformed packets");return 0;
}
