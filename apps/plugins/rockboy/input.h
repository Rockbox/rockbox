/*
 * input.h
 *
 * Definitions for input device stuff - buttons, keys, etc.
 */

typedef struct event_s
{
    int type;
    int code;
} event_t;

#define EV_NONE 0
#define EV_PRESS 1
#define EV_RELEASE 2

int ev_postevent(event_t *ev);
int ev_getevent(event_t *ev);


