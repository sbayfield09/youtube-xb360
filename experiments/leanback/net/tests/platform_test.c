#include "platform.h"
#include <libfat/fat.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>
DISC_INTERFACE usb2mass_ops_0,usb2mass_ops_1,usb2mass_ops_2;
static unsigned mounts,unmounts,ms;
static int refuse;
static long clock_seconds;
uint32_t xf_monotonic_ms(void){return ms;}
uint64_t xf_monotonic_us(void){return (uint64_t)ms*1000;}
int fatMountSimple(const char *name,DISC_INTERFACE *disc){(void)name;(void)disc;++mounts;return !refuse;}
void fatUnmount(const char *name){(void)name;++unmounts;}
int __wrap_gettimeofday(struct timeval *tv,void *zone){(void)zone;tv->tv_sec=clock_seconds;tv->tv_usec=0;return 0;}
static void write_seed(unsigned n,unsigned value){FILE *f=fopen("uda:/leanback-seed.bin","wb");unsigned i;assert(f);for(i=0;i<n;++i)assert(fputc((int)value,f)!=EOF);assert(fclose(f)==0);}
int main(void){
    char error[160];unsigned char seed[64],check[64];
    assert(!lb_platform_prepare(error,sizeof error));
    write_seed(64,0);assert(!lb_platform_prepare(error,sizeof error));
    write_seed(63,7);assert(!lb_platform_prepare(error,sizeof error));
    write_seed(65,7);assert(!lb_platform_prepare(error,sizeof error));
    write_seed(64,7);clock_seconds=0;assert(!lb_platform_prepare(error,sizeof error));
    clock_seconds=1793318400L;assert(lb_platform_prepare(error,sizeof error));
    assert(lb_clock_epoch()==clock_seconds);ms=1000;assert(lb_clock_epoch()==clock_seconds+1);
    memset(seed,42,sizeof seed);assert(lb_seed_write(seed,sizeof seed)==64);
    assert(lb_seed_read(check,sizeof check)==64 && !memcmp(check,seed,64));
    assert(mounts && unmounts);assert(lb_seed_write(seed,63)<0);
    refuse=1;assert(lb_seed_write(seed,64)<0);
    puts("PASS: real platform seed length/zero/missing checks, clock gate, seed update/readback/remount, failure propagation");return 0;
}
