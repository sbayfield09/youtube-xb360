#include "dns_wire.h"
#include <string.h>
#include <ctype.h>
static unsigned u16(const unsigned char *p) { return (unsigned)p[0]*256+p[1]; }
size_t lb_dns_query(unsigned char *p,size_t cap,uint16_t id,const char *host) {
    size_t at=12;
    const char *label=host,*end;
    if(cap<18 || !host || !*host || strlen(host)>253) return 0;
    memset(p,0,12);p[0]=(unsigned char)(id>>8);p[1]=(unsigned char)id;p[2]=1;p[5]=1;
    do {
        size_t n;
        end=strchr(label,'.');n=end?(size_t)(end-label):strlen(label);
        if(!n || n>63 || at+n+6>cap)return 0;
        p[at++]=(unsigned char)n;memcpy(p+at,label,n);at+=n;
        label=end?end+1:NULL;
    } while(label);
    p[at++]=0;p[at++]=0;p[at++]=1;p[at++]=0;p[at++]=1;return at;
}
static int name(const unsigned char *p,size_t n,size_t *cursor,char out[256]) {
    size_t at=*cursor,used=0,end=0;unsigned hops=0;
    while(at<n && ++hops<=128) {
        unsigned len=p[at++];size_t i;
        if((len&192)==192) {
            if(at>=n)return 0;
            if(!end)end=at+1;
            at=((len&63)<<8)|p[at];continue;
        }
        if(len&192 || len>63 || at+len>n)return 0;
        if(!len) {out[used]=0;*cursor=end?end:at;return 1;}
        if(used && used<255)out[used++]='.';
        if(used+len>253)return 0;
        for(i=0;i<len;++i) {
            unsigned c=p[at++];
            if(!(isalnum(c)||c=='-'||c=='_'))return 0;
            out[used++]=(char)tolower(c);
        }
    }
    return 0;
}
int lb_dns_answer(const unsigned char *p,size_t n,uint16_t id,const char *host,unsigned char ip[4]) {
    struct record {char owner[256],alias[256];unsigned char ip[4];unsigned type;} r[16];
    size_t at=12;char question[256],wanted[256];unsigned count,i,step;
    if(n<12 || u16(p)!=id || !(p[2]&128) || (p[2]&120) || u16(p+4)!=1)return 0;
    if((p[2]&2) || (p[3]&15))return -1;
    count=u16(p+6);if(count>16)return -1;
    if(!name(p,n,&at,question) || strcmp(question,host) || at+4>n || u16(p+at)!=1 || u16(p+at+2)!=1)return 0;
    at+=4;memset(r,0,sizeof r);
    for(i=0;i<count;++i) {
        unsigned len,klass;size_t data,end;
        if(!name(p,n,&at,r[i].owner) || at+10>n)return 0;
        r[i].type=u16(p+at);klass=u16(p+at+2);len=u16(p+at+8);at+=10;data=at;end=at+len;
        if(end>n)return 0;
        if(klass!=1)r[i].type=0;
        if(r[i].type==1) {if(len!=4)return 0;memcpy(r[i].ip,p+at,4);}
        if(r[i].type==5 && (!name(p,n,&data,r[i].alias) || data!=end))return 0;
        at=end;
    }
    if(strlen(host)>253)return 0;
    strcpy(wanted,host);
    for(step=0;step<16;++step) {
        int changed=0;
        for(i=0;i<count;++i)if(!strcmp(wanted,r[i].owner)) {
            if(r[i].type==1) {
                /* This fixed public service probe never targets a LAN host. */
                unsigned char *a=r[i].ip;
                if(!a[0] || a[0]==10 || a[0]==127 || a[0]>=224 ||
                   (a[0]==169 && a[1]==254) || (a[0]==172 && a[1]>=16 && a[1]<=31) ||
                   (a[0]==192 && a[1]==168))return -1;
                memcpy(ip,a,4);return 1;
            }
            if(r[i].type==5) {strcpy(wanted,r[i].alias);changed=1;break;}
        }
        if(!changed)return -1;
    }
    return -1;
}
