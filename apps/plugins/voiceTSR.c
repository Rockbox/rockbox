/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 *
 * Copyright (C) 2003-2005 Jörg Hohensohn
 * Copyright (C) 2020 BILGUS
 *
 * 
 *
 * Usage: Start plugin, it will stay in the background.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"

//#include "system.h"

/****************** constants ******************/
#define TM_OFF 0

#define LINES    8
#define COLUMNS 32 /* can't really tell for proportional font */

/****************** prototypes ******************/
void print_scroll(char* string); /* implements a scrolling screen */

int get_playtime(void); /* return the current track time in seconds */
int get_tracklength(void); /* return the total length of the current track */
int get_track(void); /* return the track number */
void get_playmsg(void); /* update the play message with Rockbox info */

void thread(void); /* the thread running it all */
int main(const void* parameter); /* main loop */
enum plugin_status plugin_start(const void* parameter); /* entry */


/****************** data types ******************/

/****************** globals ******************/
/* communication to the worker thread */
struct 
{
    bool foreground; /* set as long as we're owning the UI */
    bool exiting; /* signal to the thread that we want to exit */
    unsigned int thread; /* worker thread id */
} gThread;

/****************** helper fuctions ******************/


void print_scroll(char* string)
{
    static char screen[LINES][COLUMNS+1]; /* visible strings */
    static unsigned pos = 0; /* next print position */
    static unsigned screentop = 0; /* for scrolling */

    if (!gThread.foreground)
        return; /* just to protect careless callers */

    if (pos >= LINES)
    {   /* need to scroll first */
        int i;
        rb->lcd_clear_display();
        screentop++;
        for (i=0; i<LINES-1; i++)
            rb->lcd_puts(0, i, screen[(i+screentop) % LINES]);

        pos = LINES-1;
    }
    
    /* no strncpy avail. */
    rb->snprintf(screen[(pos+screentop) % LINES], sizeof(screen[0]), "%s", string);

    rb->lcd_puts(0, pos, screen[(pos+screentop) % LINES]);
    rb->lcd_update();
    pos++;
}

/****************** communication with Rockbox playback ******************/

/* return the current track time in seconds */
int get_playtime(void)
{
    struct mp3entry* p_mp3entry;

    p_mp3entry = rb->audio_current_track();
    if (p_mp3entry == NULL)
        return 0;

    return p_mp3entry->elapsed / 1000;
}

/* return the total length of the current track */
int get_tracklength(void)
{
    struct mp3entry* p_mp3entry;

    p_mp3entry = rb->audio_current_track();
    if (p_mp3entry == NULL)
        return 0;

    return p_mp3entry->length / 1000;
}


/* return the track number */
int get_track(void)
{
    //if (rb->audio_status() == AUDIO_STATUS_PLAY)
    //if (rb->audio_status() == (AUDIO_STATUS_PLAY | AUDIO_STATUS_PAUSE))
    struct mp3entry* p_mp3entry;

    p_mp3entry = rb->audio_current_track();
    if (p_mp3entry == NULL)
        return 0;

    return p_mp3entry->index + 1; /* track numbers start with 1 */
}

/****************** main thread + helper ******************/
/* the thread running it all */
void thread(void)
{
    const long interval = HZ * 60;
    long last_tick = *rb->current_tick; /* for 1 sec tick */
    struct tm *cur_tm;

    do
    {
        if(gThread.foreground)
        {

        }

        if (*rb->current_tick - last_tick >= interval)
        {
            last_tick += interval;
            rb->splash(0, "triggered");
            cur_tm = rb->get_time();
            if (cur_tm)
                rb->talk_time(cur_tm, true);
        }
        rb->yield();
    } while (!gThread.exiting);
}

/* callback to end the TSR plugin, called before a new one gets loaded */
static bool exit_tsr(bool reenter)
{
    if (reenter)
        return false; /* dont let it start again */
    gThread.exiting = true; /* tell the thread to end */
    rb->thread_wait(gThread.thread);  /* wait until it did */

    return true;
}

/****************** main ******************/

int plugin_main(const void* parameter)
{
    (void)parameter;
#ifdef DEBUG
    int button;
#endif
    size_t buf_size;
    ssize_t stacksize;
    void* stack;

    rb->splash(HZ/5, "Voice TSR"); /* be quick on autostart */

    /* init the worker thread */
    stack = rb->plugin_get_buffer(&buf_size); /* use the rest as stack */
    stacksize = buf_size;
    stack = (void*)(((long)stack + 100) & ~3); /* a bit away, 32 bit align */
    stacksize = (stacksize - 100) & ~3;
    if (stacksize < DEFAULT_STACK_SIZE)
    {
        rb->splash(HZ*2, "Out of memory");
        return -1;
    }

    rb->memset(&gThread, 0, sizeof(gThread));
    gThread.foreground = true;
    gThread.thread = rb->create_thread(thread, stack, stacksize, 0, "vTSR"
                                      IF_PRIO(, PRIORITY_BACKGROUND)
                                      IF_COP(, CPU));

#ifdef DEBUG
    do
    {
        button = rb->button_get(true);
    } while (button & BUTTON_REL);
#endif

    gThread.foreground = false; /* we're in the background now */
    rb->plugin_tsr(exit_tsr); /* stay resident */
    
#ifdef DEBUG
    return rb->default_event_handler(button);
#else
    return 0;
#endif
}


/***************** Plugin Entry Point *****************/


enum plugin_status plugin_start(const void* parameter)
{
    /* now go ahead and have fun! */
    return (plugin_main(parameter)==0) ? PLUGIN_OK : PLUGIN_ERROR;
}
