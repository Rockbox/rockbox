/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2006 by Daniel Everton <dan@iocaine.org>
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

#include <SDL.h>
#include <SDL_thread.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "system.h"
#include "thread-sdl.h"
#include "system-sdl.h"
#include "sim-ui-defines.h"
#include "lcd-sdl.h"
#ifdef HAVE_LCD_BITMAP
#include "lcd-bitmap.h"
#elif defined(HAVE_LCD_CHARCELLS)
#include "lcd-charcells.h"
#endif
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote-bitmap.h"
#endif
#include "panic.h"
#include "debug.h"

#if (CONFIG_PLATFORM & PLATFORM_MAEMO)
#include <glib.h>
#include <glib-object.h>
#include "maemo-thread.h"

#endif

SDL_Surface    *gui_surface;

bool            background = true;          /* use backgrounds by default */
#ifdef HAVE_REMOTE_LCD
bool            showremote = true;          /* include remote by default */
#endif
bool            mapping = false;
bool            debug_buttons = false;

bool            lcd_display_redraw = true;  /* Used for player simulator */
char            having_new_lcd = true;      /* Used for player simulator */
bool            sim_alarm_wakeup = false;
const char     *sim_root_dir = NULL;

static SDL_Thread *evt_thread = NULL;

#ifdef DEBUG
bool debug_audio = false;
#endif

bool debug_wps = false;
int wps_verbose_level = 3;

/*
 * This thread will read the buttons in an interrupt like fashion, and
 * also initializes SDL_INIT_VIDEO and the surfaces
 *
 * it must be done in the same thread (at least on windows) because events only
 * work in the thread which called SDL_Init(SubSystem) with SDL_INIT_VIDEO
 *
 * This is an SDL thread and relies on preemptive behavoir of the host
 **/
static int sdl_event_thread(void * param)
{
    SDL_InitSubSystem(SDL_INIT_VIDEO);

#if (CONFIG_PLATFORM & PLATFORM_MAEMO)
    SDL_sem *wait_for_maemo_startup;
#endif
    SDL_Surface *picture_surface = NULL;
    int width, height;
    int depth;
    Uint32 flags;

    /* Try and load the background image. If it fails go without */
    if (background) {
        picture_surface = SDL_LoadBMP("UI256.bmp");
        if (picture_surface == NULL) {
            background = false;
            DEBUGF("warn: %s\n", SDL_GetError());
        }
    }
    
    /* Set things up */
    if (background)
    {
        width = UI_WIDTH;
        height = UI_HEIGHT;
    } 
    else 
    {
#ifdef HAVE_REMOTE_LCD
        if (showremote)
        {
            width = SIM_LCD_WIDTH > SIM_REMOTE_WIDTH ? SIM_LCD_WIDTH : SIM_REMOTE_WIDTH;
            height = SIM_LCD_HEIGHT + SIM_REMOTE_HEIGHT;
        }
        else
#endif
        {
            width = SIM_LCD_WIDTH;
            height = SIM_LCD_HEIGHT;
        }
    }

    depth = LCD_DEPTH;
    if (depth < 8)
        depth = 16;

    flags = SDL_HWSURFACE|SDL_DOUBLEBUF;
#if (CONFIG_PLATFORM & (PLATFORM_MAEMO|PLATFORM_PANDORA))
    /* Fullscreen mode for maemo app */
    flags |= SDL_FULLSCREEN;
#endif

    SDL_WM_SetCaption(UI_TITLE, NULL);

    if ((gui_surface = SDL_SetVideoMode(width * display_zoom, height * display_zoom, depth, flags)) == NULL) {
        panicf("%s", SDL_GetError());
    }

#if (CONFIG_PLATFORM & (PLATFORM_MAEMO|PLATFORM_PANDORA))
    /* SDL touch screen fix: Work around a SDL assumption that returns
       relative mouse coordinates when you get to the screen edges
       using the touchscreen and a disabled mouse cursor.
     */
    uint8_t hiddenCursorData = 0;
    SDL_Cursor *hiddenCursor = SDL_CreateCursor(&hiddenCursorData, &hiddenCursorData, 8, 1, 0, 0);

    SDL_ShowCursor(SDL_ENABLE);
    SDL_SetCursor(hiddenCursor);
#endif

    if (background && picture_surface != NULL)
        SDL_BlitSurface(picture_surface, NULL, gui_surface, NULL);

    /* let system_init proceed */
    SDL_SemPost((SDL_sem *)param);

#if (CONFIG_PLATFORM & PLATFORM_MAEMO)
    /* Start maemo thread: Listen to display on/off events and battery monitoring */
    wait_for_maemo_startup = SDL_CreateSemaphore(0); /* 0-count so it blocks */
    SDL_Thread *maemo_thread = SDL_CreateThread(maemo_thread_func, wait_for_maemo_startup);
#endif

    /*
     * finally enter the button loop */
    gui_message_loop();

#if (CONFIG_PLATFORM & PLATFORM_MAEMO)
    /* Ensure maemo thread is up and running */
    SDL_SemWait(wait_for_maemo_startup);
    SDL_DestroySemaphore(wait_for_maemo_startup);

#if (CONFIG_PLATFORM & PLATFORM_MAEMO5)
    pcm_shutdown_gstreamer();
#endif

    g_main_loop_quit (maemo_main_loop);
    g_main_loop_unref(maemo_main_loop);
    SDL_WaitThread(maemo_thread, NULL);
#endif

#if (CONFIG_PLATFORM & (PLATFORM_MAEMO|PLATFORM_PANDORA))
    SDL_FreeCursor(hiddenCursor);
#endif

    if(picture_surface)
        SDL_FreeSurface(picture_surface);

    /* Order here is relevent to prevent deadlocks and use of destroyed
       sync primitives by kernel threads */
#ifdef HAVE_SDL_THREADS
    sim_thread_shutdown(); /* not needed for native threads */
#endif
    return 0;
}

void power_off(void)
{
    /* Shut down SDL event loop */
    SDL_Event event;
    memset(&event, 0, sizeof(SDL_Event));
    event.type = SDL_USEREVENT;
    SDL_PushEvent(&event);
#ifdef HAVE_SDL_THREADS
    /* since sim_thread_shutdown() grabs the mutex we need to let it free,
     * otherwise SDL_WaitThread will deadlock */
    struct thread_entry* t = sim_thread_unlock();
#endif
    /* wait for event thread to finish */
    SDL_WaitThread(evt_thread, NULL);

#ifdef HAVE_SDL_THREADS
    /* lock again before entering the scheduler */
    sim_thread_lock(t);
    /* sim_thread_shutdown() will cause sim_do_exit() to be called via longjmp,
     * but only if we let the sdl thread scheduler exit the other threads */
    while(1) yield();
#else
    sim_do_exit();
#endif
}

void sim_do_exit()
{
    sim_kernel_shutdown();

    SDL_Quit();
    exit(EXIT_SUCCESS);
}

uintptr_t *stackbegin;
uintptr_t *stackend;
void system_init(void)
{
    SDL_sem *s;
    /* fake stack, OS manages size (and growth) */
    stackbegin = stackend = (uintptr_t*)&s;

#if (CONFIG_PLATFORM & PLATFORM_MAEMO)
    /* Make glib thread safe */
    g_thread_init(NULL);
    g_type_init();
#endif

    if (SDL_Init(SDL_INIT_TIMER))
        panicf("%s", SDL_GetError());

    s = SDL_CreateSemaphore(0); /* 0-count so it blocks */

    evt_thread = SDL_CreateThread(sdl_event_thread, s);

    /* wait for sdl_event_thread to run so that it can initialize the surfaces
     * and video subsystem needed for SDL events */
    SDL_SemWait(s);
    /* cleanup */
    SDL_DestroySemaphore(s);
}


void system_reboot(void)
{
#ifdef HAVE_SDL_THREADS
    sim_thread_exception_wait();
#else
    sim_do_exit();
#endif
}

void system_exception_wait(void)
{
    system_reboot();
}

void sys_handle_argv(int argc, char *argv[])
{
    if (argc >= 1) 
    {
        int x;
        for (x = 1; x < argc; x++) 
        {
#ifdef DEBUG
            if (!strcmp("--debugaudio", argv[x])) 
            {
                debug_audio = true;
                printf("Writing debug audio file.\n");
            }
            else 
#endif
                if (!strcmp("--debugwps", argv[x]))
            {
                debug_wps = true;
                printf("WPS debug mode enabled.\n");
            } 
            else if (!strcmp("--nobackground", argv[x]))
            {
                background = false;
                printf("Disabling background image.\n");
            } 
#ifdef HAVE_REMOTE_LCD
            else if (!strcmp("--noremote", argv[x]))
            {
                showremote = false;
                background = false;
                printf("Disabling remote image.\n");
            }
#endif
            else if (!strcmp("--old_lcd", argv[x]))
            {
                having_new_lcd = false;
                printf("Using old LCD layout.\n");
            }
            else if (!strcmp("--zoom", argv[x]))
            {
                x++;
                if(x < argc)
                    display_zoom=atof(argv[x]);
                else
                    display_zoom = 2;
                printf("Window zoom is %d\n", display_zoom);
            }
            else if (!strcmp("--alarm", argv[x]))
            {
                sim_alarm_wakeup = true;
                printf("Simulating alarm wakeup.\n");
            }
            else if (!strcmp("--root", argv[x]))
            {
                x++;
                if (x < argc)
                {
                    sim_root_dir = argv[x];
                    printf("Root directory: %s\n", sim_root_dir);
                }
            }
            else if (!strcmp("--mapping", argv[x]))
            {
                    mapping = true;
                    printf("Printing click coords with drag radii.\n");
            }
            else if (!strcmp("--debugbuttons", argv[x]))
            {
                    debug_buttons = true;
                    printf("Printing background button clicks.\n");
            }
            else 
            {
                printf("rockboxui\n");
                printf("Arguments:\n");
#ifdef DEBUG
                printf("  --debugaudio \t Write raw PCM data to audiodebug.raw\n");
#endif
                printf("  --debugwps \t Print advanced WPS debug info\n");
                printf("  --nobackground \t Disable the background image\n");
#ifdef HAVE_REMOTE_LCD
                printf("  --noremote \t Disable the remote image (will disable backgrounds)\n");
#endif
                printf("  --old_lcd \t [Player] simulate old playermodel (ROM version<4.51)\n");
                printf("  --zoom [VAL]\t Window zoom (will disable backgrounds)\n");
                printf("  --alarm \t Simulate a wake-up on alarm\n");
                printf("  --root [DIR]\t Set root directory\n");
                printf("  --mapping \t Output coordinates and radius for mapping backgrounds\n");
                exit(0);
            }
        }
    }
    if (display_zoom != 1) {
        background = false;
    }
}
