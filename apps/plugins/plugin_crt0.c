/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2010 by Thomas Martitz
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
#include <setjmp.h>

PLUGIN_HEADER

/*
 * EXIT_MAGIC magic, because 0 cannot be used due to setjmp()
 * must be > 0
 */                      
#define EXIT_MAGIC 0x0CDEBABE

extern enum plugin_status plugin_start(const void*);
extern unsigned char plugin_bss_start[];
extern unsigned char plugin_end_addr[];

static jmp_buf __exit_env;
/* only 1 atexit handler for now, chain in the exit handler if you need more */
static void (*atexit_handler)(void);

int rb_atexit(void (*fn)(void))
{
    if (atexit_handler)
        return -1;
    atexit_handler = fn;
    return 0;
}

void exit(int status)
{   /* jump back in time to before starting the plugin */
    longjmp(__exit_env, status != 0 ? status : EXIT_MAGIC);
}

void _exit(int status)
{   /* don't call exit handler */
    atexit_handler = NULL;
    exit(status);
}

enum plugin_status plugin__start(const void *param)
{
    int exit_ret;
    enum plugin_status ret;

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)

/* IRAM must be copied before clearing the BSS ! */
#ifdef PLUGIN_USE_IRAM
    extern char iramcopy[], iramstart[], iramend[], iedata[], iend[];
    size_t iram_size = iramend - iramstart;
    size_t ibss_size = iend - iedata;
    if (iram_size > 0 || ibss_size > 0)
    {
        /* We need to stop audio playback in order to use codec IRAM */
        rb->audio_stop();
        rb->memcpy(iramstart, iramcopy, iram_size);
        rb->memset(iedata, 0, ibss_size);
        /* make the icache (if it exists) up to date with the new code */
        rb->commit_discard_idcache();

        /* barrier to prevent reordering iram copy and BSS clearing,
         * because the BSS segment alias the IRAM copy.
         */
        asm volatile ("" ::: "memory");
    }
#endif /* PLUGIN_USE_IRAM */

    /* zero out the bss section */
    rb->memset(plugin_bss_start, 0, plugin_end_addr - plugin_bss_start);

    /* Some parts of bss may be used via a no-cache alias (at least
     * portalplayer has this). If we don't clear the cache, those aliases
     * may read garbage */
    rb->commit_dcache();
#endif

    /* we come back here if exit() was called or the plugin returned normally */
    exit_ret = setjmp(__exit_env);
    if (exit_ret == 0)
    {   /* start the plugin */
        ret = plugin_start(param);
    }
    else
    {   /* plugin exit via exit() */
        if (exit_ret == EXIT_MAGIC)
        {   /* exit(EXIT_SUCCESS) */
            ret = PLUGIN_OK;
        }
        else if (exit_ret < INTERNAL_PLUGIN_RETVAL_START)
        {   /* exit(EXIT_FAILURE) */
            ret = PLUGIN_ERROR;
        }
        else
        {   /* exit(PLUGIN_XXX) */
            ret = (enum plugin_status)exit_ret;
        }
    }

    /* before finishing, call the exit handler if there was one */
    if (atexit_handler != NULL)
        atexit_handler();

    return ret;
}

static void cleanup_wrapper(void *param)
{
    (void)param;
    if (atexit_handler)
        atexit_handler();
}

void exit_on_usb(int button)
{   /* the default handler will call the exit handler before
     * showing the usb screen; after that we don't want the exit handler
     * to be called a second time, hence _exit()
     *
     * if not usb, then the handler will only be called if powering off
     * if poweroff, the plugin doesn't want to run any further so exit as well*/
    long result = rb->default_event_handler_ex(button, cleanup_wrapper, NULL);
    if (result == SYS_USB_CONNECTED)
        _exit(PLUGIN_USB_CONNECTED);
    else if (result == SYS_POWEROFF || result == SYS_REBOOT)
        /* note: nobody actually pays attention to this exit code */
        _exit(PLUGIN_POWEROFF);
}
