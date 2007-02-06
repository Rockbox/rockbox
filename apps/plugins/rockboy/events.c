/*
 * events.c
 *
 * Event queue.
 */


#include "rockmacros.h"
#include "input.h"

#define MAX_EVENTS 32

static event_t eventqueue[MAX_EVENTS];
static int eventhead, eventpos;


int ev_postevent(event_t *ev)
{
    int nextevent;
    nextevent = (eventhead+1)%MAX_EVENTS;
    if (nextevent == eventpos)
        return 0;
    eventqueue[eventhead] = *ev;
    eventhead = nextevent;
    return 1;
}

int ev_getevent(event_t *ev)
{
    if (eventpos == eventhead)
    {
        ev->type = EV_NONE;
        return 0;
    }
    *ev = eventqueue[eventpos];
    eventpos = (eventpos+1)%MAX_EVENTS;
    return 1;
}
