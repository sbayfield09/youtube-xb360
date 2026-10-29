#ifndef LB_DNS_WIRE_H
#define LB_DNS_WIRE_H
#include <stddef.h>
#include <stdint.h>
size_t lb_dns_query(unsigned char *, size_t, uint16_t, const char *);
/* 1: valid public A answer; 0: malformed/unrelated; -1: server error/truncated/no A. */
int lb_dns_answer(const unsigned char *, size_t, uint16_t, const char *, unsigned char[4]);
#endif
