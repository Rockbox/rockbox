#include "kernel.h"
#include "thread.h"

#include <stdio.h>
#include "lcd.h"

extern void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

void timer4(void) {
	int i;
    /* Run through the list of tick tasks */
    for(i = 0; i < MAX_NUM_TICK_TASKS; i++)
    {
        if(tick_funcs[i])
        {
            tick_funcs[i]();
        }
    }

    current_tick++;

    /* following needs to be fixed.  */
    /*wake_up_thread();*/
}

