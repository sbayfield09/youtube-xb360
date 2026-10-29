/* SPDX-License-Identifier: GPL-2.0-or-later
 * Standalone XeLL diagnostic. Reuses XenonFin's unchanged text presenter and
 * endpoint parser. No sound/video decoder, TLS stack or web engine is linked. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <console/console.h>
#include <input/input.h>
#include <usb/usbmain.h>
#include <xenos/xenos.h>
#include <ppc/timebase.h>
#include <time/time.h>
#include "console_batch.h"
#include "protocol.h"
#include "timing.h"
#include "../input.h"

static unsigned page, events, last_code;
static int last_down, dirty=1, exiting;
static char transport[160]="Not tested. Press A on the transport page.";
uint32_t xf_monotonic_ms(void) {
    uint32_t high,low,check;
    do {
        __asm__ volatile("mftbu %0":"=r"(high));
        __asm__ volatile("mftbl %0":"=r"(low));
        __asm__ volatile("mftbu %0":"=r"(check));
    } while(high!=check);
    return (uint32_t)(xf_ticks_to_us(((uint64_t)high<<32)|low,PPC_TIMEBASE_FREQ)/1000);
}
static void key(void *context, unsigned code, int down) {
    (void)context;
    ++events; last_code=code; last_down=down; dirty=1;
    if (!down) return;
    if(code==32783 || code==32781) page=(page+1)%3;
    if(code==32782 || code==32780) page=(page+2)%3;
    if(code==32769) { if(page) page=0; else exiting=1; }
    if(code==32768 && page==1) {
        xf_endpoint endpoint;
        char error[128]={0};
        if(!xf_endpoint_parse("https://www.youtube.com/tv",&endpoint,error,sizeof error))
            snprintf(transport,sizeof transport,"%s",error);
        else snprintf(transport,sizeof transport,"Endpoint accepted; network/TLS not tested.");
    }
}
static void draw(void) {
    console_set_colors(0xff101624,0xffe6eaf4);
    xf_console_begin();
    xf_console_printf("\n  YOUTUBE LEANBACK - XeLL DIAGNOSTIC 0.1\n\n");
    xf_console_printf("  DIAGNOSTIC ONLY: NO WEB UI OR VIDEO PLAYBACK\n");
    xf_console_printf("  D-pad: pages | A: transport test | B: back/exit\n\n");
    xf_console_printf("  Page %u/3: %s\n\n",page+1,
        page==0?"Feasibility status":page==1?"Native transport gate":"Controller events");
    if(page==0) {
        xf_console_printf("  Standalone PowerPC ELF for XeLL, not XEX.\n\n");
        xf_console_printf("  Reuses XenonFin's text presenter and input driver.\n");
        xf_console_printf("  Audio/video source and binaries remain unchanged.\n\n");
        xf_console_printf("  Required work before direct YouTube can run:\n");
        xf_console_printf("    1. Verified HTTPS and DNS\n");
        xf_console_printf("    2. Compatible JS/DOM/CSS browser engine\n");
        xf_console_printf("    3. MSE/media bridge and suitable native codecs\n\n");
        xf_console_printf("  This application makes no network requests.\n");
    } else if(page==1) {
        xf_console_printf("  Target: https://www.youtube.com/tv\n\n");
        xf_console_printf("  A calls XenonFin's actual native endpoint parser.\n");
        xf_console_printf("  A parser rejection occurs BEFORE DNS/TCP/TLS.\n");
        xf_console_printf("  It is not an HTTP response from YouTube.\n\n");
        /* Keep lines short enough for the lowest supported console geometry. */
        if(strstr(transport,"HTTPS unavailable")) {
            xf_console_printf("  BLOCKED: HTTPS unavailable on this native build;\n");
            xf_console_printf("  no insecure TLS fallback.\n");
        } else xf_console_printf("  %.65s\n",transport);
    } else {
        xf_console_printf("  A/B/D-pad normalized to gamepad key codes.\n");
        xf_console_printf("  Press/release pairs, no synthetic hold repeats.\n\n");
        xf_console_printf("  Events: %u | last key: %u | %s\n",events,last_code,last_down?"DOWN":"UP");
        xf_console_printf("  These events are not connected to a browser DOM.\n\n");
        xf_console_printf("  Use A to observe down/up without changing pages.\n");
    }
    xf_console_printf("\n  Build verified on host; console behavior unverified.\n");
    xf_console_end();
}
int main(void) {
    struct controller_data_s pad;
    lb_input input={0};
    uint32_t drawn=0;
    xenos_init(VIDEO_MODE_AUTO); console_init(); usb_init();
    if(!xf_console_init()) { puts("Unsupported framebuffer geometry");return 1; }
    while(!exiting) {
        uint32_t now=xf_monotonic_ms();
        unsigned bits;
        usb_do_poll(); memset(&pad,0,sizeof pad); get_controller_data(&pad,0);
        bits=(pad.a?1u:0u)|(pad.b?2u:0u)|(pad.up?4u:0u)|(pad.down?8u:0u)|
            (pad.left?16u:0u)|(pad.right?32u:0u);
        /* A zeroed sample releases stale keys; see pinned driver semantics. */
        lb_input_poll(&input,bits,1,key,0);
        xf_console_poll(now);
        if(xf_console_ready() && (dirty || (uint32_t)(now-drawn)>=1000)) {
            draw();drawn=now;dirty=0;
        }
        udelay(1000);
    }
    xf_console_shutdown();
    return 0;
}
