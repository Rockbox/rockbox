/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Wincent Balin
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
#include "pdbox.h"

#include "m_pd.h"
#include "s_stuff.h"

/* Welcome to the PDBox plugin */
PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

/* Name of the file to open. */
char* filename;

/* Running time. */
uint64_t runningtime = 0;

/* Variables for Pure Data. */
int sys_verbose;
int sys_noloadbang;
t_symbol *sys_libdir;
t_namelist *sys_openlist;
int sys_nsoundin = 0;
int sys_soundindevlist[MAXAUDIOINDEV];
int sys_nchin = 0;
int sys_chinlist[MAXAUDIOINDEV];
int sys_nsoundout = 1;
int sys_soundoutdevlist[MAXAUDIOOUTDEV];
int sys_nchout = 2;
int sys_choutlist[MAXAUDIOOUTDEV];
static int sys_main_srate = PD_SAMPLERATE;
static int sys_main_advance = PD_SAMPLES_PER_HZ;

/* References for scheduler variables and functions. */
extern t_time sys_time;
extern t_time sys_time_per_dsp_tick;
extern void sched_tick(t_time next_sys_time);

#define SAMPLES_SIZE 1000
t_sample samples[SAMPLES_SIZE];

/* Quit flag. */
bool quit = false;

/* Thread IDs. */
unsigned int core_thread_id;
unsigned int gui_thread_id;
unsigned int time_thread_id;

/* Stacks for threads. */
#define STACK_SIZE 16384
uint32_t core_stack[STACK_SIZE / sizeof(uint32_t)];
uint32_t gui_stack[STACK_SIZE / sizeof(uint32_t)];
uint32_t time_stack[256 / sizeof(uint32_t)];


/* Core thread, scheduler. */
void core_thread(void)
{
/*    struct datagram ping; */

    /* LATER consider making this variable.  It's now the LCM of all sample
    rates we expect to see: 32000, 44100, 48000, 88200, 96000. */
    #define TIMEUNITPERSEC (32.*441000.)

    sys_time = 0;
    sys_time_per_dsp_tick = (TIMEUNITPERSEC) *
    	((double)sys_schedblocksize) / sys_dacsr;


    /* Main loop */
    while(!quit)
    {
#if 0
        /* Wait for request. */
        while(!RECEIVE_TO_CORE(&ping))
            rb->yield();

        if(memcmp("Ping!", ping.data, ping.size) == 0)
        {
            SEND_FROM_CORE("Pong!");
            break;
        }
#endif

        /* Use sys_send_dacs() function as timer. */
        if(sys_send_dacs() != SENDDACS_NO)
            sched_tick(sys_time + sys_time_per_dsp_tick);

        rb->sleep(1);
    }

   rb->thread_exit();
}

/* GUI thread */
void gui_thread(void)
{
/*    struct datagram pong; */

    /* GUI loop */
    while(!quit)
    {
#if 0
        /* Send ping to the core. */
        SEND_TO_CORE("Ping!");
        rb->splash(HZ, "Sent ping.");

        /* Wait for answer. */
        while(!RECEIVE_FROM_CORE(&pong))
            rb->yield();

        /* If got a pong -- everything allright. */
        if(memcmp("Pong!", pong.data, pong.size) == 0)
        {
            rb->splash(HZ, "Got pong!");
            quit = true;
            break;
        }
#endif

        rb->sleep(1);
    }

    rb->thread_exit();
}

/* Running time thread. */
void time_thread(void)
{
    while(!quit)
    {
        /* Add time slice in milliseconds. */
        runningtime += (1000 / HZ);
        rb->sleep(1);
    }

    rb->thread_exit();
}


/* Plug-in entry point */
enum plugin_status plugin_start(const void* parameter)
{
    PLUGIN_IRAM_INIT(rb)

    /* Memory pool variables. */
    size_t mem_size;
    void* mem_pool;

    /* Get the file name. */
    filename = (char*) parameter;

    /* Allocate memory; check it's size; add to the pool. */
    mem_pool = rb->plugin_get_audio_buffer(&mem_size);
    if(mem_size < MIN_MEM_SIZE)
    {
        rb->splash(HZ, "Not enough memory!");
        return PLUGIN_ERROR;
    }
    add_pool(mem_pool, mem_size);

    /* Initialize net. */
    net_init();

    /* Initialize Pure Data, as does sys_main in s_main.c */
    pd_init();

    /* Add the directory the called .pd resides in to lib directories. */
    sys_findlibdir(filename);

    /* Open the parameter file. */
    sys_openlist = namelist_append(sys_openlist, filename);

    /* Set audio API. */
    sys_set_audio_api(API_ROCKBOX);

    /* Fake a GUI start. */
    sys_startgui(NULL);

    /* Initialize audio subsystem. */
    sys_open_audio(sys_nsoundin, sys_soundindevlist, sys_nchin, sys_chinlist,
        sys_nsoundout, sys_soundoutdevlist, sys_nchout, sys_choutlist,
        sys_main_srate, sys_main_advance, 1);

    /* Start threads. */
    time_thread_id =
        rb->create_thread(&time_thread,
                          time_stack,
                          sizeof(time_stack),
                          0, /* FIXME Which flags? */
                          "PD running time"
                          IF_PRIO(, PRIORITY_REALTIME)
                          IF_COP(, COP));
    core_thread_id =
        rb->create_thread(&core_thread,
                          core_stack,
                          sizeof(core_stack),
                          0, /* FIXME Which flags? */
                          "PD core"
                          IF_PRIO(, MAX(PRIORITY_USER_INTERFACE / 2,
                                       PRIORITY_REALTIME + 1))
                          IF_COP(, COP));
    gui_thread_id =
        rb->create_thread(&gui_thread,
                          gui_stack,
                          sizeof(gui_stack),
                          0, /* FIXME Which flags? */
                          "PD GUI"
                          IF_PRIO(, PRIORITY_USER_INTERFACE)
                          IF_COP(, CPU));

    /* If having an error creating threads, bail out. */
    if(core_thread_id == 0 || gui_thread_id == 0)
        return PLUGIN_ERROR;

    /* Wait for quit flag. */
    while(!quit)
        yield();

    /* Wait for threads to complete. */
    rb->thread_wait(gui_thread_id);
    rb->thread_wait(core_thread_id);
    rb->thread_wait(time_thread_id);

    /* Destroy net. */
    net_destroy();

    return PLUGIN_OK;
}

