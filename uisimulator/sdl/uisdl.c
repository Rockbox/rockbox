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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "autoconf.h"
#include "button.h"
#include "system-sdl.h"
#include "thread.h"
#include "kernel.h"
#include "uisdl.h"
#include "lcd-sdl.h"
#ifdef HAVE_LCD_BITMAP
#include "lcd-bitmap.h"
#elif defined(HAVE_LCD_CHARCELLS)
#include "lcd-charcells.h"
#endif
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote-bitmap.h"
#endif
#include "thread-sdl.h"
#include "SDL_mutex.h"
#include "SDL_thread.h"

/* extern functions */
extern void                 app_main (void *); /* mod entry point */
extern void                 new_key(int key);
extern void                 sim_tick_tasks(void);
extern bool                 sim_io_init(void);
extern void                 sim_io_shutdown(void);

void button_event(int key, bool pressed);

SDL_Surface *gui_surface;
bool background = false;        /* Don't use backgrounds by default */

SDL_TimerID tick_timer_id;

bool lcd_display_redraw = true;         /* Used for player simulator */
char having_new_lcd = true;               /* Used for player simulator */
bool sim_alarm_wakeup = false;
const char *sim_root_dir = NULL;

bool debug_audio = false;

bool debug_wps = false;
int wps_verbose_level = 3;

long start_tick;

Uint32 tick_timer(Uint32 interval, void *param)
{
    long new_tick;

    (void) interval;
    (void) param;
    
    new_tick = (SDL_GetTicks() - start_tick) / (1000/HZ);
        
    if (new_tick != current_tick) {
        long i;
        for (i = new_tick - current_tick; i > 0; i--)
        {
            sim_enter_irq_handler();
            sim_tick_tasks();
            sim_exit_irq_handler();
        }
        current_tick = new_tick;
    }
    
    return 1;
}

void gui_message_loop(void)
{
    SDL_Event event;
    bool done = false;

    while(!done && SDL_WaitEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
                sim_enter_irq_handler();
                button_event(event.key.keysym.sym, true);
                sim_exit_irq_handler();
                break;
            case SDL_KEYUP:
                sim_enter_irq_handler();
                button_event(event.key.keysym.sym, false);
                sim_exit_irq_handler();
                break;
#ifndef HAVE_TOUCHPAD
            case SDL_MOUSEBUTTONDOWN:
                if (debug_wps && event.button.button == 1)
                {
                    printf("Mouse at: (%d, %d)\n", event.button.x, event.button.y);
                }
                break;
#endif
            case SDL_QUIT:
                done = true;
                break;
            default:
                /*printf("Unhandled event\n"); */
                break;
        }
    }
}

bool gui_startup(void)
{
    SDL_Surface *picture_surface;
    int width, height;

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER)) {
        fprintf(stderr, "fatal: %s\n", SDL_GetError());
        return false;
    }

    atexit(SDL_Quit);

    /* Try and load the background image. If it fails go without */
    if (background) {
        picture_surface = SDL_LoadBMP("UI256.bmp");
        if (picture_surface == NULL) {
            background = false;
            fprintf(stderr, "warn: %s\n", SDL_GetError());
        }
    }

    /* Set things up */

    if (background) {
        width = UI_WIDTH;
        height = UI_HEIGHT;
    } else {
#ifdef HAVE_REMOTE_LCD
        width = UI_LCD_WIDTH > UI_REMOTE_WIDTH ? UI_LCD_WIDTH : UI_REMOTE_WIDTH;
        height = UI_LCD_HEIGHT + UI_REMOTE_HEIGHT;
#else
        width = UI_LCD_WIDTH;
        height = UI_LCD_HEIGHT;
#endif
    }
   
    
    if ((gui_surface = SDL_SetVideoMode(width * display_zoom, height * display_zoom, 24, SDL_HWSURFACE|SDL_DOUBLEBUF)) == NULL) {
        fprintf(stderr, "fatal: %s\n", SDL_GetError());
        return false;
    }

    SDL_WM_SetCaption(UI_TITLE, NULL);

    sim_lcd_init();
#ifdef HAVE_REMOTE_LCD
    sim_lcd_remote_init();
#endif

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    if (background && picture_surface != NULL) {
        SDL_BlitSurface(picture_surface, NULL, gui_surface, NULL);
        SDL_UpdateRect(gui_surface, 0, 0, 0, 0);
    }
    
    start_tick = SDL_GetTicks();

    return true;
}

bool gui_shutdown(void)
{
    /* Order here is relevent to prevent deadlocks and use of destroyed
       sync primitives by kernel threads */
    thread_sdl_shutdown();
    SDL_RemoveTimer(tick_timer_id);
    sim_io_shutdown();
    sim_kernel_shutdown();
    return true;
}

#if defined(WIN32) && defined(main)
/* Don't use SDL_main on windows -> no more stdio redirection */
#undef main
#endif

int main(int argc, char *argv[])
{
    if (argc >= 1) 
    {
        int x;
        for (x = 1; x < argc; x++) 
        {
            if (!strcmp("--debugaudio", argv[x])) 
            {
                debug_audio = true;
                printf("Writing debug audio file.\n");
            } 
            else if (!strcmp("--debugwps", argv[x]))
            {
                debug_wps = true;
                printf("WPS debug mode enabled.\n");
            } 
            else if (!strcmp("--background", argv[x]))
            {
                background = true;
                printf("Using background image.\n");
            } 
            else if (!strcmp("--old_lcd", argv[x]))
            {
                having_new_lcd = false;
                printf("Using old LCD layout.\n");
            }
            else if (!strcmp("--zoom", argv[x]))
            {
                x++;
                if(x < argc)
                    display_zoom=atoi(argv[x]);
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
            else 
            {
                printf("rockboxui\n");
                printf("Arguments:\n");
                printf("  --debugaudio \t Write raw PCM data to audiodebug.raw\n");
                printf("  --debugwps \t Print advanced WPS debug info\n");
                printf("  --background \t Use background image of hardware\n");
                printf("  --old_lcd \t [Player] simulate old playermodel (ROM version<4.51)\n");
                printf("  --zoom [VAL]\t Window zoom (will disable backgrounds)\n");
                printf("  --alarm \t Simulate a wake-up on alarm\n");
                printf("  --root [DIR]\t Set root directory\n");
                exit(0);
            }
        }
    }

    if (display_zoom > 1) {
        background = false;
    }

    if (!sim_kernel_init()) {
        fprintf(stderr, "sim_kernel_init failed\n");
        return -1;
    }

    if (!sim_io_init()) {
        fprintf(stderr, "sim_io_init failed\n");
        return -1;
    }

    if (!gui_startup()) {
        fprintf(stderr, "gui_startup failed\n");
        return -1;
    }

    /* app_main will be called by the new main thread */
    if (!thread_sdl_init(NULL)) {
        fprintf(stderr, "thread_sdl_init failed\n");
        return -1;
    }

    tick_timer_id = SDL_AddTimer(10, tick_timer, NULL);

    gui_message_loop();

    return gui_shutdown();
}

