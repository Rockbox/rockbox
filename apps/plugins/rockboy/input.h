/*
 * input.h
 *
 * Definitions for input device stuff - buttons, keys, etc.
 */


#define MAX_KEYS 10

typedef struct event_s
{
	int type;
	int code;
} event_t;

#define EV_NONE 0
#define EV_PRESS 1
#define EV_RELEASE 2
#define EV_REPEAT 3

int ev_postevent(event_t *ev);
int ev_getevent(event_t *ev);


