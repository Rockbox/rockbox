#ifndef _CLOCK_MESSAGE_
#define _CLOCK_MESSAGE_
#include "clock.h"
#include "plugin.h"
#include "lib/picture.h"

struct counter{
    int ticks_at_last_unpause;/* to count the time from last pause to now */
    int ticks_since_started;/* accumulated time */
    bool paused;
};

void counter_init(struct counter* counter);
void counter_toggle(struct counter* counter);
#define counter_reset(counter) counter_init(counter)
void counter_pause(struct counter* counter, bool paused);
void counter_get_elapsed_time(struct counter* counter, struct time* elapsed_time);

#endif /* _CLOCK_MESSAGE_ */
