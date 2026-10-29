#define _POSIX_C_SOURCE 200809L
#include "probe.h"
#include "native_net.h"
#include "driver_guard.h"
#include <stdio.h>
#include <time.h>
int main(void){
    const lb_probe_status *s;unsigned start=xf_monotonic_ms();
    lb_probe_start(start);
    do {struct timespec wait={0,1000000};shim_poll();lb_probe_tick(xf_monotonic_ms());nanosleep(&wait,NULL);
        s=lb_probe_state();if((uint32_t)(xf_monotonic_ms()-start)>10000)break;
    } while(s->active);
    printf("stage=%s\nverified=%d\nflags=%u\nstatus=%d\ncomplete=%d\nbody=%lu\ndetail=%s\n",s->stage,s->verified,s->verify_flags,s->http_status,s->complete,s->body_bytes,s->detail);
    lb_probe_cancel();return 0;
}
