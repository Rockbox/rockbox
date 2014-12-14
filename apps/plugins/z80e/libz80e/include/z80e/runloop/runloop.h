#ifndef RUNLOOP_H
#define RUNLOOP_H

typedef struct runloop_state runloop_state_t;
#include <z80e/ti/asic.h>


runloop_state_t *runloop_init(asic_t *);

void runloop_tick_cycles(runloop_state_t *, int);
void runloop_tick(runloop_state_t *);
#endif
