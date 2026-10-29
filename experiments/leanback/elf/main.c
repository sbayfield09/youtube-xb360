/* SPDX-License-Identifier: GPL-2.0-or-later
 * Standalone XeLL diagnostic. Reuses XenonFin's unchanged text/network helpers
 * and adds a separate Mbed TLS HTTPS path. No sound/video decoder or web engine. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <console/console.h>
#include <input/input.h>
#include <usb/usbmain.h>
#include <xenos/xenos.h>
#include <ppc/timebase.h>
#include <time/time.h>
#include "console_batch.h"
#include "protocol.h"
#include "timing.h"
#include "native_net.h"
#include "probe.h"
#include "platform.h"
#include "../input.h"

static unsigned page, events, last_code;
static int last_down, dirty=1, exiting;
uint64_t xf_monotonic_us(void) {
    uint32_t high,low,check;
    do {
        __asm__ volatile("mftbu %0":"=r"(high));
        __asm__ volatile("mftbl %0":"=r"(low));
        __asm__ volatile("mftbu %0":"=r"(check));
    } while(high!=check);
    return xf_ticks_to_us(((uint64_t)high<<32)|low,PPC_TIMEBASE_FREQ);
}
uint32_t xf_monotonic_ms(void) {return (uint32_t)(xf_monotonic_us()/1000);}
static void key(void *context, unsigned code, int down) {
    (void)context;
    ++events; last_code=code; last_down=down; dirty=1;
    if (!down) return;
    if(code==32783 || code==32781) page=(page+1)%3;
    if(code==32782 || code==32780) page=(page+2)%3;
    if(code==32769) { if(page) page=0; else {lb_probe_cancel();exiting=1;} }
    if(code==32768 && page==1) {
        lb_probe_start(xf_monotonic_ms());
    }
}
static void draw(void) {
    console_set_colors(0xff101624,0xffe6eaf4);
    xf_console_begin();
    xf_console_printf("\n  YOUTUBE LEANBACK - XeLL HTTPS DIAGNOSTIC 0.2\n\n");
    xf_console_printf("  DIAGNOSTIC ONLY: NO WEB UI OR VIDEO PLAYBACK\n");
    xf_console_printf("  D-pad: pages | A: HTTPS test | X: cancel | B: back/exit\n\n");
    xf_console_printf("  Page %u/3: %s\n\n",page+1,
        page==0?"Feasibility status":page==1?"Direct DNS / HTTPS":"Controller events");
    if(page==0) {
        xf_console_printf("  Standalone PowerPC ELF for XeLL, not XEX.\n\n");
        xf_console_printf("  Reuses XenonFin's text presenter and input driver.\n");
        xf_console_printf("  Audio/video source and binaries remain unchanged.\n\n");
        xf_console_printf("  Page 2 tests DHCP, DNS, TLS and GET /tv.\n");
        xf_console_printf("  Requires writable FAT USB with a private NV seed.\n\n");
        xf_console_printf("  Required work before Leanback can run:\n");
        xf_console_printf("    1. Hardware verification of the new HTTPS path\n");
        xf_console_printf("    2. Compatible JS/DOM/CSS browser engine\n");
        xf_console_printf("    3. MSE/media bridge and suitable native codecs\n\n");
        xf_console_printf("  No PC/proxy/server is used by the console.\n");
    } else if(page==1) {
        xf_console_printf("  Target: https://www.youtube.com/tv\n\n");
        const lb_probe_status *s=lb_probe_state();
        xf_console_printf("  Stage: %s%s\n",s->stage,s->active?" (running)":"");
        xf_console_printf("  Network: %.65s\n",xf_network_status());
        xf_console_printf("  DHCP: %s | IP: %s\n",xf_dhcp_state(),s->address[0]?s->address:"unresolved");
        xf_console_printf("  %.65s\n",s->detail);
        if(strlen(s->detail)>65)xf_console_printf("  %.65s\n",s->detail+65);
        xf_console_printf("\n  Certificate verified: %s | flags %08lx\n",s->verified?"YES":"NO",(unsigned long)s->verify_flags);
        xf_console_printf("  TLS error: %d | HTTP status: %d\n",s->tls_error,s->http_status);
        xf_console_printf("  TLS bytes: %lu | page bytes: %lu\n",s->received,s->body_bytes);
        xf_console_printf("  Cipher: %.64s\n",s->cipher);
        if(lb_clock_epoch()>0) {
            time_t epoch=(time_t)lb_clock_epoch();struct tm *utc=gmtime(&epoch);
            if(utc)xf_console_printf("  Console UTC: %04d-%02d-%02d %02d:%02d:%02d\n",utc->tm_year+1900,utc->tm_mon+1,utc->tm_mday,utc->tm_hour,utc->tm_min,utc->tm_sec);
        }
        xf_console_printf("\n  TLS verifies GTS roots, hostname and certificate dates.\n");
        xf_console_printf("  A download is not web rendering or playback.\n");
    } else {
        xf_console_printf("  A/B/D-pad normalized to gamepad key codes.\n");
        xf_console_printf("  Press/release pairs, no synthetic hold repeats.\n\n");
        xf_console_printf("  Events: %u | last key: %u | %s\n",events,last_code,last_down?"DOWN":"UP");
        xf_console_printf("  These events are not connected to a browser DOM.\n\n");
        xf_console_printf("  Use A to observe down/up without changing pages.\n");
    }
    xf_console_printf("\n  v0.1 boot/input confirmed; new HTTPS path unverified.\n");
    xf_console_end();
}
int main(void) {
    struct controller_data_s pad;
    lb_input input={0};
    uint32_t drawn=0;
    int previous_x=0;
    xenos_init(VIDEO_MODE_AUTO); console_init(); usb_init();
    if(!xf_console_init()) { puts("Unsupported framebuffer geometry");return 1; }
    while(!exiting) {
        uint32_t now=xf_monotonic_ms();
        unsigned bits;
        usb_do_poll(); memset(&pad,0,sizeof pad); get_controller_data(&pad,0);
        xf_network_tick(now);lb_probe_tick(now);
        if(pad.x && !previous_x){lb_probe_cancel();dirty=1;}
        previous_x=pad.x;
        bits=(pad.a?1u:0u)|(pad.b?2u:0u)|(pad.up?4u:0u)|(pad.down?8u:0u)|
            (pad.left?16u:0u)|(pad.right?32u:0u);
        /* A zeroed sample releases stale keys; see pinned driver semantics. */
        lb_input_poll(&input,bits,1,key,0);
        xf_console_poll(now);
        if(xf_console_ready() && (dirty || (uint32_t)(now-drawn)>=250)) {
            draw();drawn=now;dirty=0;
        }
        udelay(1000);
    }
    xf_console_shutdown();
    return 0;
}
