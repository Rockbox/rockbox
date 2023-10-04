/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2023 by William Wilgus
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

/*Plugin Includes*/

#include "plugin.h"
/* Redefinitons of ANSI C functions. */
#include "lib/wrappers.h"
#include "lib/helper.h"

static void thread_create(void);
static void thread(void); /* the thread running it all */
static void allocate_tempbuf(void);
static void free_tempbuf(void);
static bool do_timed_yield(void);
static void _log(const char *fmt, ...);
/*Aliases*/
#if 0
#ifdef ROCKBOX_HAS_LOGF
    #define logf         rb->logf
#else
    #define logf(...)   {}
#endif
#endif


#define logf _log
#define sleep rb->sleep
#define qsort rb->qsort

#define write(x,y,z)   rb->write(x,y,z)
#define ftruncate rb->ftruncate
#define remove rb->remove

#define vsnprintf    rb->vsnprintf
#define mkdir        rb->mkdir
#define filesize     rb->filesize

#define strtok_r     rb->strtok_r
#define strncasecmp  rb->strncasecmp
#define strcasecmp   rb->strcasecmp

#define current_tick (*rb->current_tick)
#define crc_32(x,y,z)   rb->crc_32(x,y,z)


#ifndef SIMULATOR
#define errno  (*rb->__errno())
#endif
#define ENOENT 2        /* No such file or directory */
#define EEXIST 17       /* File exists */
#define MAX_LOG_SIZE 16384

#define EV_EXIT        MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0xFF)
#define EV_ACTION      MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0x02)
#define EV_STARTUP     MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0x01)

/* communication to the worker thread */
static struct
{
    int user_index;
    bool exiting; /* signal to the thread that we want to exit */
    bool resume;
    unsigned int id; /* worker thread id */
    struct  event_queue queue; /* thread event queue */
    long    last_useraction_tick;
} gThread;


/*Support Fns*/
/* open but with a builtin printf for assembling the path */
int open_pathfmt(char *buf, size_t size, int oflag, const char *pathfmt, ...)
{
    va_list ap;
    va_start(ap, pathfmt);
    vsnprintf(buf, size, pathfmt, ap);
    va_end(ap);
    if ((oflag & O_PATH) == O_PATH)
        return -1;
    int handle = open(buf, oflag, 0666);
    //logf("Open: %s %d flag: %x", buf, handle, oflag);
    return handle;
}

static void sleep_yield(void)
{
    rb->queue_remove_from_head(&gThread.queue, EV_ACTION);
    rb->queue_post(&gThread.queue, EV_ACTION, 0);
    sleep(1);
    #undef yield
    rb->yield();
    #define yield sleep_yield
}

/* make sure tag can be displayed by font pf*/
static bool text_is_displayable(struct font *pf, unsigned char *src)
{
    unsigned short code;
    const unsigned char *ptr = src;
    while(*ptr)
    {
        ptr = rb->utf8decode(ptr, &code);

        if(!rb->font_get_bits(pf, code))
        {
            return false;
        }
    }
    return true;
}

/* callback for each tag if false returned tag will not be added */
bool user_check_tag(int index_type, char* build_idx_buf)
{
    static struct font *pf = NULL;
    if (!pf)
        pf = rb->font_get(FONT_UI);

    if (index_type == tag_artist || index_type == tag_album ||
        index_type == tag_genre || index_type == tag_title ||
        index_type == tag_composer || index_type == tag_comment ||
        index_type == tag_albumartist || index_type == tag_virt_canonicalartist)
    {
        /* this could be expanded with more rules / transformations */
        if (rb->utf8length(build_idx_buf) != strlen(build_idx_buf))
        {
            if (!text_is_displayable(pf, build_idx_buf))
            {
                logf("Can't display (%d) %s", index_type, build_idx_buf);
            }
        }
    }
    return true;
}

/* undef features we don't need */
#undef HAVE_DIRCACHE
#undef HAVE_TC_RAMCACHE
#undef HAVE_EEPROM_SETTINGS
/* paste the whole tagcache.c file */
#include "../tagcache.c"

static bool logdump(bool append);
static unsigned char logbuffer[MAX_LOG_SIZE + 1];
static int logindex;
static bool logwrap;
static bool logenabled = true;

static void check_logindex(void)
{
    if(logindex >= MAX_LOG_SIZE)
    {
        /* wrap */
        logdump(true);
        //logwrap = true;
        logindex = 0;
    }
}

static int log_push(void *userp, int c)
{
    (void)userp;

    logbuffer[logindex++] = c;
    check_logindex();

    return 1;
}

/* our logf function */
static void _log(const char *fmt, ...)
{
    if (!logenabled)
    {
        rb->splash(10, "log not enabled");
        return;
    }


    #ifdef USB_ENABLE_SERIAL
    int old_logindex = logindex;
    #endif
    va_list ap;

    va_start(ap, fmt);

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
    char buf[1024];
    vsnprintf(buf, sizeof buf, fmt, ap);
    DEBUGF("%s\n", buf);
    /* restart va_list otherwise the result if undefined when vuprintf is called */
    va_end(ap);
    va_start(ap, fmt);
#endif

    rb->vuprintf(log_push, NULL, fmt, ap);
    va_end(ap);

    /* add trailing zero */
    log_push(NULL, '\0');
}


int compute_nb_lines(int w, struct font* font)
{
    int i, nb_lines;
    int cur_x, delta_x;

    if(logindex>= MAX_LOG_SIZE || (logindex == 0 && !logwrap))
        return 0;

    if(logwrap)
        i = logindex;
    else
        i = 0;

    cur_x = 0;
    nb_lines = 0;

    do {
        if(logbuffer[i] == '\0')
        {
            cur_x = 0;
            nb_lines++;
        }
        else
        {
            /* does character fit on this line ? */
            delta_x = rb->font_get_width(font, logbuffer[i]);

            if(cur_x + delta_x > w)
            {
                cur_x = 0;
                nb_lines++;
            }

            /* update pointer */
            cur_x += delta_x;
        }

        i++;
        if(i >= MAX_LOG_SIZE)
            i = 0;
    } while(i != logindex);

    return nb_lines;
}

static bool logdisplay(void)
{

    int w, h, i, index;
    int fontnr;
    static int delta_y = -1;
    int cur_x, cur_y, delta_x;
    struct font* font;

    char buf[2];

    fontnr = FONT_FIRSTUSERFONT;
    font = rb->font_get(fontnr);
    buf[1] = '\0';
    w = LCD_WIDTH;
    h = LCD_HEIGHT;

    if (delta_y < 0) /* init, get the horizontal size of each line */
    {
        rb->font_getstringsize("A", NULL, &delta_y, fontnr);
        /* start at the end of the log */
        gThread.user_index = compute_nb_lines(w, font) - h/delta_y -1;
        /* user_index will be number of the first line to display (warning: line!=log entry) */
        /* if negative, will be set 0 to zero later */
    }


    rb->lcd_clear_display();

    if(gThread.user_index < 0 || gThread.user_index >= MAX_LOG_SIZE)
        gThread.user_index = 0;


    if(logwrap)
        i = logindex;
    else
        i = 0;

    index = 0;
    cur_x = 0;
    cur_y = 0;

    /* nothing to print ? */
    if(logindex == 0 && !logwrap)
        goto end_print;

    do {
        if(logbuffer[i] == '\0')
        {
            /* should be display a newline ? */
            if(index >= gThread.user_index)
                cur_y += delta_y;
            cur_x = 0;
            index++;
        }
        else
        {
            /* does character fit on this line ? */
            delta_x = rb->font_get_width(font, logbuffer[i]);

            if(cur_x + delta_x > w)
            {
                /* should be display a newline ? */
                if(index >= gThread.user_index)
                    cur_y += delta_y;
                cur_x = 0;
                index++;
            }

            /* should we print character ? */
            if(index >= gThread.user_index)
            {
                buf[0] = logbuffer[i];
                rb->lcd_putsxy(cur_x, cur_y, buf);
            }

            /* update pointer */
            cur_x += delta_x;
        }
        i++;
        /* did we fill the screen ? */
        if(cur_y > h - delta_y)
        {
            if (TIME_AFTER(current_tick, gThread.last_useraction_tick + HZ))
                gThread.user_index++;
            break;
        }

        if(i >= MAX_LOG_SIZE)
            i = 0;
    } while(i != logindex);

    end_print:
    rb->lcd_update();

    return false;
}

static bool logdump(bool append)
{
    int fd;
    int flags = O_CREAT|O_WRONLY|O_TRUNC;
    /* nothing to print ? */
    if(!logenabled || (logindex == 0 && !logwrap))
    {
        /* nothing is logged just yet */
        return false;
    }
    if (!append)
    {
        flags = O_CREAT|O_WRONLY|O_APPEND;
    }

    fd = open(ROCKBOX_DIR "/db_commit_log.txt", flags, 0666);
    logenabled = false;
    if(-1 != fd) {
        int i;

        if(logwrap)
            i = logindex;
        else
            i = 0;

        do {
            if(logbuffer[i]=='\0')
                rb->fdprintf(fd, "\n");
            else
                rb->fdprintf(fd, "%c", logbuffer[i]);

            i++;
            if(i >= MAX_LOG_SIZE)
                i = 0;
        } while(i != logindex);

        close(fd);
    }

    logenabled = true;

    return false;
}

static void allocate_tempbuf(void)
{
    tempbuf_size = 0;
    tempbuf = rb->plugin_get_audio_buffer(&tempbuf_size);
    tempbuf_size &= ~0x03;

}

static void free_tempbuf(void)
{
    if (tempbuf_size == 0)
        return ;

    rb->plugin_release_audio_buffer();
    tempbuf = NULL;
    tempbuf_size = 0;
}

static bool do_timed_yield(void)
{
    /* Sorting can lock up for quite a while, so yield occasionally */
    static long wakeup_tick = 0;
    if (TIME_AFTER(current_tick, wakeup_tick))
    {

        yield();

        wakeup_tick = current_tick + (HZ/5);
        return true;
    }
    return false;
}

/*-----------------------------------------------------------------------------*/
/******* plugin_start *******                                                  */
/*-----------------------------------------------------------------------------*/

enum plugin_status plugin_start(const void* parameter)
{
    (void) parameter;

    /* Turn off backlight timeout */
    backlight_ignore_timeout();

    memset(&tc_stat, 0, sizeof(struct tagcache_stat));
    memset(&current_tcmh, 0, sizeof(struct master_header));
    filenametag_fd = -1;

    strlcpy(tc_stat.db_path, rb->global_settings->tagcache_db_path, sizeof(tc_stat.db_path));
    if (!rb->dir_exists(tc_stat.db_path)) /* on fail use default DB path */
        strlcpy(tc_stat.db_path, ROCKBOX_DIR, sizeof(tc_stat.db_path));
    tc_stat.initialized = true;

    memset(&gThread, 0, sizeof(gThread));
    logf("started");
    logdump(false);

    thread_create();
    gThread.user_index = 0;
    logdisplay(); /* get something on the screen while user waits */

    allocate_tempbuf();

    commit();

    if (tc_stat.ready)
        rb->tagcache_commit_finalize();

    free_tempbuf();

    logdump(true);
    gThread.user_index++;
    logdisplay();
    rb->thread_wait(gThread.id);
    rb->queue_delete(&gThread.queue);

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();
    return PLUGIN_OK;
}

/****************** main thread + helper ******************/
static void thread(void)
{
    struct queue_event ev;
    while (!gThread.exiting)
    {
        rb->queue_wait_w_tmo(&gThread.queue, &ev, 1);
        switch (ev.id)
        {
            case SYS_USB_CONNECTED:
                rb->usb_acknowledge(SYS_USB_CONNECTED_ACK);
                logenabled = false;
                break;
            case SYS_USB_DISCONNECTED:
                logenabled = true;
                /*fall through*/
            case EV_STARTUP:
                logf("Thread Started");
                break;
            case EV_EXIT:
                return;
            default:
                break;
        }
        logdisplay();

        int action = rb->get_action(CONTEXT_STD, HZ/10);

        switch( action )
        {
            case ACTION_NONE:
                break;
            case ACTION_STD_NEXT:
            case ACTION_STD_NEXTREPEAT:
                gThread.last_useraction_tick = current_tick;
                gThread.user_index++;
                break;
            case ACTION_STD_PREV:
            case ACTION_STD_PREVREPEAT:
                gThread.last_useraction_tick = current_tick;
                gThread.user_index--;
                break;
            case ACTION_STD_OK:
                gThread.last_useraction_tick = current_tick;
                gThread.user_index = 0;
                break;
            case SYS_POWEROFF:
            case ACTION_STD_CANCEL:
                gThread.exiting = true;
                return;
#ifdef HAVE_TOUCHSCREEN
            case ACTION_TOUCHSCREEN:
            {
                gThread.last_useraction_tick = current_tick;
                short x, y;
                static int prev_y;

                action = action_get_touchscreen_press(&x, &y);

                if(action & BUTTON_REL)
                    prev_y = 0;
                else
                {
                    if(prev_y != 0)
                        gThread.user_index += (prev_y - y) / delta_y;

                    prev_y = y;
                }
            }
#endif
            default:
                break;
        }


        yield();
    }
}

static void thread_create(void)
{
    /* put the thread's queue in the bcast list */
    rb->queue_init(&gThread.queue, true);
    /*Note: tagcache_stack is defined in apps/tagcache.c */
    gThread.last_useraction_tick = current_tick;
    gThread.id = rb->create_thread(thread, tagcache_stack, sizeof(tagcache_stack),
                                      0, "db_commit"
                                      IF_PRIO(, PRIORITY_USER_INTERFACE)
                                      IF_COP(, CPU));
    rb->queue_post(&gThread.queue, EV_STARTUP, 0);
    yield();
}
