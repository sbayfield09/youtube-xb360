#ifndef LB_PLATFORM_H
#define LB_PLATFORM_H
#include <stddef.h>
#include <stdint.h>
int lb_platform_prepare(char *,size_t);
const char *lb_volume(void);
int lb_seed_read(unsigned char *,size_t);
int lb_seed_write(unsigned char *,size_t);
long long lb_clock_epoch(void);
#endif
