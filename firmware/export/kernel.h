/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <stdbool.h>

/* wrap-safe macros for tick comparison */
#define TIME_AFTER(a,b)         ((long)(b) - (long)(a) < 0)
#define TIME_BEFORE(a,b)        TIME_AFTER(b,a)

#define HZ      100 /* number of ticks per second */

#define MAX_NUM_TICK_TASKS 4

#define QUEUE_LENGTH 16 /* MUST be a power of 2 */
#define QUEUE_LENGTH_MASK (QUEUE_LENGTH - 1)

/* System defined message ID's, occupying the top 5 bits of the event ID */
#define SYS_EVENT                 (long)0x80000000 /* SYS events are negative */
#define SYS_USB_CONNECTED         ((SYS_EVENT | ((long)1 << 27)))
#define SYS_USB_CONNECTED_ACK     ((SYS_EVENT | ((long)2 << 27)))
#define SYS_USB_DISCONNECTED      ((SYS_EVENT | ((long)3 << 27)))
#define SYS_USB_DISCONNECTED_ACK  ((SYS_EVENT | ((long)4 << 27)))
#define SYS_TIMEOUT               ((SYS_EVENT | ((long)5 << 27)))
#define SYS_MMC_INSERTED          ((SYS_EVENT | ((long)6 << 27)))
#define SYS_MMC_EXTRACTED         ((SYS_EVENT | ((long)7 << 27)))
#define SYS_POWEROFF              ((SYS_EVENT | ((long)8 << 27)))
#define SYS_FS_CHANGED            ((SYS_EVENT | ((long)9 << 27)))
#define SYS_CHARGER_CONNECTED     ((SYS_EVENT | ((long)10 << 27)))
#define SYS_CHARGER_DISCONNECTED  ((SYS_EVENT | ((long)11 << 27)))

struct event
{
    long id;
    void *data;
};

struct event_queue
{
    struct event events[QUEUE_LENGTH];
    unsigned int read;
    unsigned int write;
};

struct mutex
{
    bool locked;
};

/* global tick variable */
extern long current_tick;

#ifdef SIMULATOR
#define sleep(x) sim_sleep(x)
#endif

/* kernel functions */
extern void kernel_init(void);
extern void yield(void);
extern void sleep(int ticks);
int tick_add_task(void (*f)(void));
int tick_remove_task(void (*f)(void));

extern void queue_init(struct event_queue *q);
extern void queue_wait(struct event_queue *q, struct event *ev);
extern void queue_wait_w_tmo(struct event_queue *q, struct event *ev, int ticks);
extern void queue_post(struct event_queue *q, long id, void *data);
extern bool queue_empty(const struct event_queue* q);
void queue_clear(struct event_queue* q);
extern int queue_broadcast(long id, void *data);

extern void mutex_init(struct mutex *m);
extern void mutex_lock(struct mutex *m);
extern void mutex_unlock(struct mutex *m);
extern void tick_start(unsigned int interval_in_ms);

#endif
