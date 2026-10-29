/* Platform-independent edges. Native adapters supply six normalized bits.
 * No repeat: deliver down/up pairs and let the browser own held-key repeat.
 * Sink must dispatch into the page's actual JS realm, not draw a fake UI. */
#ifndef LEANBACK_INPUT_H
#define LEANBACK_INPUT_H
typedef void (*lb_key_sink)(void *, unsigned code, int down);
typedef struct { unsigned held; } lb_input;
static void lb_input_poll(lb_input *state, unsigned buttons, int connected,
                          lb_key_sink sink, void *context) {
    /* A, B, Up, Down, Left, Right -> standard gamepad indices + 32768.
     * See VacuumTube controller-support.js; actual Leanback mapping unverified. */
    static const unsigned codes[6] = {32768,32769,32780,32781,32782,32783};
    unsigned i, changed;
    buttons = connected ? buttons & 63u : 0u;
    changed = state->held ^ buttons;
    for (i=0; i<6; ++i) if (changed & (1u<<i))
        sink(context, codes[i], !!(buttons & (1u<<i)));
    state->held = buttons;
}
#endif
