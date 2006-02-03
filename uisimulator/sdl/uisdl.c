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
#include "autoconf.h"
#include "uisdl.h"
#include "button.h"
#include "thread.h"
#include "thread-sdl.h"
#include "kernel.h"
#include "sound.h"

// extern functions
extern void                 app_main (void *); // mod entry point
extern void                 new_key(int key);
extern void                 sim_tick_tasks(void);

void button_event(int key, bool pressed);

SDL_Surface *gui_surface;

SDL_Thread *gui_thread;
SDL_TimerID tick_timer_id;
#ifdef ROCKBOX_HAS_SIMSOUND
SDL_Thread *sound_thread;
#endif

bool lcd_display_redraw=true; // Used for player simulator
char having_new_lcd=true; // Used for player simulator

long start_tick;

Uint32 tick_timer(Uint32 interval, void *param)
{
    long new_tick;

    (void) interval;
    (void) param;
    
    new_tick = (SDL_GetTicks() - start_tick) * HZ / 1000;
        
    if (new_tick != current_tick)
    {
        long i;
        for (i = new_tick - current_tick; i > 0; i--)
            sim_tick_tasks();
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
                button_event(event.key.keysym.sym, true);
                break;
            case SDL_KEYUP:
                button_event(event.key.keysym.sym, false);
                break;
            case SDL_QUIT:
                done = true;
                break;
            default:
                //printf("Unhandled event\n");
                break;
        }
    }
}

bool gui_startup()
{
    SDL_Surface *picture_surface;

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER)) {
        fprintf(stderr, "fatal: %s", SDL_GetError());
        return false;
    }

    atexit(SDL_Quit);

    if ((gui_surface = SDL_SetVideoMode(UI_WIDTH, UI_HEIGHT, 24, SDL_HWSURFACE|SDL_DOUBLEBUF)) == NULL) {
        fprintf(stderr, "fatal: %s", SDL_GetError());
        return false;
    }

    SDL_WM_SetCaption(UI_TITLE, NULL);

    simlcdinit();
		
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		
    picture_surface = SDL_LoadBMP("UI256.bmp");
    if (picture_surface == NULL) {
        fprintf(stderr, "warn: %s", SDL_GetError());
		                } else {
        SDL_BlitSurface(picture_surface, NULL, gui_surface, NULL);
        SDL_UpdateRect(gui_surface, 0, 0, 0, 0);
		                }
		
    start_tick = SDL_GetTicks();

    return true;
}

bool gui_shutdown()
{
    int i;

    SDL_KillThread(gui_thread);
    SDL_RemoveTimer(tick_timer_id);
#ifdef ROCKBOX_HAS_SIMSOUND
    SDL_KillThread(sound_thread);
#endif

    for (i = 0; i < threadCount; i++)
    {
        SDL_KillThread(threads[i]);
    }

    return true;
}

/**
 * Thin wrapper around normal app_main() to stop gcc complaining about types.
 */
int sim_app_main(void *param)
{
    app_main(param);

    return 0;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (!gui_startup())
        return -1;

    gui_thread = SDL_CreateThread(sim_app_main, NULL);
    if (gui_thread == NULL) {
        printf("Error creating GUI thread!\n");
        return -1;
    }

    tick_timer_id = SDL_AddTimer(10, tick_timer, NULL);

#ifdef ROCKBOX_HAS_SIMSOUND
    sound_thread = SDL_CreateThread(sound_playback_thread, NULL);
    if (sound_thread == NULL) {
        printf("Error creating sound thread!\n");
        return -1;
    }
#endif

    gui_message_loop();

    return gui_shutdown();
}

