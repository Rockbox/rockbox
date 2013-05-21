#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>

#include "locking.h"

#define SYS_TIMEOUT (1<<31)

#define HAVE_EXTENDED_MESSAGING_AND_NAME

#define MAX_NUM_QUEUES 32
#define QUEUE_LENGTH 16 /* MUST be a power of 2 */
#define QUEUE_LENGTH_MASK (QUEUE_LENGTH - 1)

#define TIMEOUT_BLOCK   -1
#define TIMEOUT_NOBLOCK  0

struct queue_event
{
    long     id;
    intptr_t data;
};

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
struct queue_sender_list
{
    /* If non-NULL, there is a thread waiting for the corresponding event */
    /* Must be statically allocated to put in non-cached ram. */
    //~ struct thread_entry *senders[QUEUE_LENGTH]; /* message->thread map */
    struct thread_entry *senders[QUEUE_LENGTH]; /* message->thread map */
    struct thread_entry *list;                  /* list of senders in map */
    /* Send info for last message dequeued or NULL if replied or not sent */
    struct thread_entry * volatile curr_sender;
//~ #ifdef HAVE_PRIORITY_SCHEDULING
    //~ struct blocker blocker;
//~ #endif
};
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

//~ #ifdef HAVE_PRIORITY_SCHEDULING
//~ #define QUEUE_GET_THREAD(q) \
    //~ (((q)->send == NULL) ? NULL : (q)->send->blocker.thread)
//~ #else
/* Queue without priority enabled have no owner provision _at this time_ */
#define QUEUE_GET_THREAD(q) \
    (NULL)
//~ #endif

struct event_queue
{
    struct thread_entry *queue;         /* waiter list */
    struct queue_event events[QUEUE_LENGTH]; /* list of events */
    unsigned int volatile read;         /* head of queue */
    unsigned int volatile write;        /* tail of queue */
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    struct queue_sender_list * volatile send; /* list of threads waiting for
                                           reply to an event */
//~ #ifdef HAVE_PRIORITY_SCHEDULING
    //~ struct blocker *blocker_p;          /* priority inheritance info
                                           //~ for sync message senders */
//~ #endif
#endif
    //~ IF_COP( struct corelock cl; )       /* multiprocessor sync */
    IF_COP( struct corelock cl; )       /* multiprocessor sync */
};
extern void queue_init(struct event_queue *q, bool register_queue);
extern void queue_delete(struct event_queue *q);
extern void queue_wait(struct event_queue *q, struct queue_event *ev);
extern void queue_wait_w_tmo(struct event_queue *q, struct queue_event *ev,
                             int ticks);
extern void queue_post(struct event_queue *q, long id, intptr_t data);
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
extern void queue_enable_queue_send(struct event_queue *q,
                                    struct queue_sender_list *send,
                                    unsigned int owner_id);
extern intptr_t queue_send(struct event_queue *q, long id, intptr_t data);
extern void queue_reply(struct event_queue *q, intptr_t retval);
extern bool queue_in_queue_send(struct event_queue *q);
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */
extern bool queue_empty(const struct event_queue* q);
extern bool queue_peek(struct event_queue *q, struct queue_event *ev);

#define QPEEK_FILTER_COUNT_MASK (0xffu) /* 0x00=1 filter, 0xff=256 filters */
#define QPEEK_FILTER_HEAD_ONLY  (1u << 8) /* Ignored if no filters */
#define QPEEK_REMOVE_EVENTS     (1u << 9) /* Remove or discard events */
extern bool queue_peek_ex(struct event_queue *q,
                          struct queue_event *ev,
                          unsigned int flags,
                          const long (*filters)[2]);

extern void queue_clear(struct event_queue* q);
extern void queue_remove_from_head(struct event_queue *q, long id);
extern int queue_count(const struct event_queue *q);
extern int queue_broadcast(long id, intptr_t data);

#endif
