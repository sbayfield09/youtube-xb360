/* USB NV seed is a strong source only when uniquely provisioned from an OS
 * CSPRNG, kept private and never cloned/restored. No timing-based entropy. */
#include "platform.h"
#include <libfat/fat.h>
#include <diskio/disc_io.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <mbedtls/platform.h>
#include <mbedtls/platform_time.h>
extern DISC_INTERFACE usb2mass_ops_0,usb2mass_ops_1,usb2mass_ops_2;
extern uint32_t xf_monotonic_ms(void);
extern uint64_t xf_monotonic_us(void);
mbedtls_ms_time_t mbedtls_ms_time(void) {return (mbedtls_ms_time_t)(xf_monotonic_us()/1000);}
static DISC_INTERFACE *discs[]={&usb2mass_ops_0,&usb2mass_ops_1,&usb2mass_ops_2};
static const char *names[]={"uda","udb","udc"};
static int selected=-1;
static unsigned mounted;
static long long epoch;
static uint32_t clock_at;
const char *lb_volume(void) {return selected<0?NULL:names[selected];}
long long lb_clock_epoch(void) {return epoch?epoch+(uint32_t)(xf_monotonic_ms()-clock_at)/1000:0;}
static mbedtls_time_t clock_now(mbedtls_time_t *out) {
    mbedtls_time_t t=(mbedtls_time_t)lb_clock_epoch();if(out)*out=t;return t;
}
int lb_seed_read(unsigned char *data,size_t n) {
    FILE *f;char path[80];size_t got;unsigned any=0;size_t i;int extra,closed;
    if(selected<0 || n!=64)return -1;
    snprintf(path,sizeof path,"%s:/leanback-seed.bin",names[selected]);
    f=fopen(path,"rb");if(!f)return -1;
    got=fread(data,1,n,f);extra=fgetc(f);closed=fclose(f);
    for(i=0;i<got;++i)any|=data[i];
    return got==n && extra==EOF && !closed && any?(int)n:-1;
}
int lb_seed_write(unsigned char *data,size_t n) {
    FILE *f;char path[80];unsigned char check[64];size_t wrote;int flushed,closed;
    if(selected<0 || n!=64)return -1;
    snprintf(path,sizeof path,"%s:/leanback-seed.bin",names[selected]);
    f=fopen(path,"rb+");if(!f)return -1;
    wrote=fwrite(data,1,n,f);flushed=fflush(f);closed=fclose(f);
    if(wrote!=n || flushed || closed)return -1;
    /* Flush libfat's filesystem cache and re-read after remount before the
     * entropy module is allowed to release bytes to the TLS DRBG. */
    fatUnmount(names[selected]);mounted&=~(1u<<selected);
    if(!fatMountSimple(names[selected],discs[selected]))return -1;
    mounted|=1u<<selected;
    if(lb_seed_read(check,n)<0 || memcmp(check,data,n)) {memset(check,0,sizeof check);return -1;}
    memset(check,0,sizeof check);return (int)n;
}
int lb_platform_prepare(char *error,size_t cap) {
    unsigned i;struct timeval tv;unsigned char seed[64];
    selected=-1;
    for(i=0;i<3;++i) {
        if(!(mounted&(1u<<i))) {
            if(!fatMountSimple(names[i],discs[i]))continue;
            mounted|=1u<<i;
        }
        selected=(int)i;
        if(lb_seed_read(seed,sizeof seed)==64)break;
        selected=-1;
    }
    memset(seed,0,sizeof seed);
    if(selected<0) {snprintf(error,cap,"Need private 64-byte leanback-seed.bin on writable FAT USB");return 0;}
    if(gettimeofday(&tv,NULL) || tv.tv_sec<1767225600L || tv.tv_sec>2114380800L) {
        snprintf(error,cap,"Console UTC invalid (need 2026-2036); set dashboard date/time");return 0;
    }
    epoch=tv.tv_sec;clock_at=xf_monotonic_ms();
    mbedtls_platform_set_time(clock_now);
    mbedtls_platform_set_nv_seed(lb_seed_read,lb_seed_write);
    return 1;
}
