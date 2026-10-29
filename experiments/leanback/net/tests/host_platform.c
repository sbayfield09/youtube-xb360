/* OS-backed seed/clock for tests only; does not validate libfat or SMC. */
#define _POSIX_C_SOURCE 200809L
#include "platform.h"
#include <mbedtls/platform.h>
#include <mbedtls/platform_time.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
uint32_t xf_monotonic_ms(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint32_t)((uint64_t)t.tv_sec*1000+t.tv_nsec/1000000);}
mbedtls_ms_time_t mbedtls_ms_time(void){return xf_monotonic_ms();}
const char *lb_volume(void){return "fixture";}
long long lb_clock_epoch(void){return time(NULL);}
int lb_seed_read(unsigned char *p,size_t n){FILE *f=fopen("seed.bin","rb");size_t got;if(!f)return -1;got=fread(p,1,n,f);fclose(f);return got==n?(int)n:-1;}
int lb_seed_write(unsigned char *p,size_t n){FILE *f;size_t wrote;int fail;if(getenv("LB_TEST_SEED_WRITE_FAIL"))return -1;f=fopen("seed.bin","wb");if(!f)return -1;wrote=fwrite(p,1,n,f);fail=fflush(f);if(fsync(fileno(f)))fail=1;if(fclose(f))fail=1;return wrote==n && !fail?(int)n:-1;}
int lb_platform_prepare(char *error,size_t n){(void)error;(void)n;mbedtls_platform_set_nv_seed(lb_seed_read,lb_seed_write);return 1;}
