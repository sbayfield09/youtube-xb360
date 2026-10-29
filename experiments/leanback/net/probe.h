#ifndef LB_NET_PROBE_H
#define LB_NET_PROBE_H
#include <stdint.h>
typedef struct {
    const char *stage;
    char detail[160],address[32],cipher[64];
    unsigned long received,body_bytes;
    int active,verified,http_status,complete,tls_error;
    uint32_t verify_flags;
} lb_probe_status;
void lb_probe_start(uint32_t);
void lb_probe_tick(uint32_t);
void lb_probe_cancel(void);
const lb_probe_status *lb_probe_state(void);
#endif
