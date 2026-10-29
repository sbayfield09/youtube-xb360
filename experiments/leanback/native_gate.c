/* Executes XenonFin's real endpoint parser, without modifying or copying it. */
#include <stdio.h>
#include "protocol.h"
int main(void) {
    xf_endpoint endpoint;
    char error[256] = {0};
    if (!xf_endpoint_parse("https://www.youtube.com/tv", &endpoint, error, sizeof error)) {
        printf("BLOCKED before DNS/TCP/TLS: %s\n", error);
        return 2;
    }
    fprintf(stderr, "Parser accepted endpoint; transport still needs separate validation.\n");
    return 3;
}
