#include "clock_counter.h"
#include "clock_bitmap_strings.h"

void counter_init(struct counter* counter){
    counter->ticks_since_started=0;
    counter->ticks_at_last_unpause=0;
    counter->paused=true;
}

static int counter_get_ticks_since_last_pause(struct counter* counter){
    if(!counter->paused)
        return(*rb->current_tick - counter->ticks_at_last_unpause);
    return(0);
}

void counter_toggle(struct counter* counter){
    counter_pause(counter, !counter->paused);
}

void counter_pause(struct counter* counter, bool pause){
    if(pause){
        counter->ticks_since_started+=counter_get_ticks_since_last_pause(counter);
    }else{
        counter->ticks_at_last_unpause=*rb->current_tick;
    }
    counter->paused=pause;
}

void counter_get_elapsed_time(struct counter* counter, struct time* elapsed_time){
    int total_time=counter_get_ticks_since_last_pause(counter);
    total_time+=counter->ticks_since_started;
    total_time/=HZ;/* converts ticks to seconds */

    elapsed_time->second =  total_time%60;
    elapsed_time->minute = (total_time%3600) / 60;
    elapsed_time->hour   =  total_time / 3600;
    /* not yet ! */
    elapsed_time->day=0;
    elapsed_time->month=0;
    elapsed_time->year=0;
}

