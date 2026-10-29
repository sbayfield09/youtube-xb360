#include <assert.h>
#include <stdio.h>
#include "input.h"
static unsigned codes[16], downs[16], count;
static void record(void *unused, unsigned code, int down) {
    (void)unused; assert(count<16); codes[count]=code; downs[count++]=(unsigned)down;
}
int main(void) {
    lb_input input = {0};
    lb_input_poll(&input, 1, 1, record, 0);
    lb_input_poll(&input, 1, 1, record, 0);
    assert(count==1 && codes[0]==32768 && downs[0]==1);
    lb_input_poll(&input, 4, 1, record, 0);
    assert(count==3 && codes[1]==32768 && !downs[1] && codes[2]==32780 && downs[2]);
    lb_input_poll(&input, 4, 0, record, 0);
    assert(count==4 && codes[3]==32780 && !downs[3] && input.held==0);
    puts("PASS: press, hold, simultaneous release/press, disconnect release");
    return 0;
}
