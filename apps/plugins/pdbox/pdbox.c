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

#include "PDa/src/m_pd.h"
#include "PDa/src/s_stuff.h"

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
int sys_soundindevlist[MAXAUDIOINDEV];
int sys_chinlist[MAXAUDIOINDEV];
int sys_soundoutdevlist[MAXAUDIOOUTDEV];
int sys_choutlist[MAXAUDIOOUTDEV];
t_binbuf* inbinbuf;

/* References for scheduler variables and functions. */
extern t_time sys_time;
extern t_time sys_time_per_dsp_tick;
extern void sched_tick(t_time next_sys_time);

/* LATER consider making this variable.  It's now the LCM of all sample
rates we expect to see: 32000, 44100, 48000, 88200, 96000. */
#define TIMEUNITPERSEC (32.*441000.)


/* Quit flag. */
bool quit = false;

/* Stack sizes for threads. */
#define CORESTACKSIZE (1 * 1024 * 1024)
#define GUISTACKSIZE (512 * 1024)

/* Thread stacks. */
void* core_stack;
void* gui_stack;

/* Thread IDs. */
unsigned int core_thread_id;
unsigned int gui_thread_id;


/* GUI thread */
void gui_thread(void)
{
    struct pd_widget widget[128];
    unsigned int widgets = 0;
    struct datagram dg;
    bool update;

    /* Initialize GUI. */
    pd_gui_init();

    /* Load PD patch. */
    widgets = pd_gui_load_patch(widget,
                                sizeof(widget) / sizeof(struct pd_widget));

    /* Draw initial state of UI. */
    pd_gui_draw(widget, widgets);

    /* GUI loop */
    while(!quit)
    {
        /* Reset update flag. */
        update = false;

        /* Apply timer to widgets. */
        update |=
        pd_gui_apply_timeouts(widget, widgets);

        /* Process buttons. */
        update |=
        pd_gui_parse_buttons(widgets);

        /* Receive and parse datagrams. */
        while(RECEIVE_FROM_CORE(&dg))
        {
            update = true;
            pd_gui_parse_message(&dg, widget, widgets);
        }

        /* If there is something to update in GUI, do so. */
        if(update)
            pd_gui_draw(widget, widgets);

        rb->sleep(1);
    }

    rb->thread_exit();
}

/* Core thread */
void core_thread(void)
{
    /* Add the directory the called .pd resides in to lib directories. */
    sys_findlibdir(filename);

    /* Open the PD design file. */
    sys_openlist = namelist_append(sys_openlist, filename);

    /* Fake a GUI start. */
    sys_startgui(NULL);

    /* Core scheduler loop */
    while(!quit)
    {
        /* Receive datagrams. */
        struct datagram dg;

        while(RECEIVE_TO_CORE(&dg))
            rockbox_receive_callback(&dg);

        /* Use sys_send_dacs() function as timer. */
        while(sys_send_dacs() != SENDDACS_NO)
            sched_tick(sys_time + sys_time_per_dsp_tick);

        yield();
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

    /* Get the file name; check whether parameter contains no file name. */
    filename = (char*) parameter;
    if(strlen(filename) == 0)
    {
        rb->splash(HZ, "Play a .pd file!");
        return PLUGIN_ERROR;
    }

    /* Initialize memory pool. */
    mem_pool = rb->plugin_get_audio_buffer(&mem_size);
    if(mem_size < MIN_MEM_SIZE)
    {
        rb->splash(HZ, "Not enough memory!");
        return PLUGIN_ERROR;
    }
#if 1
    init_memory_pool(mem_size, mem_pool);
#endif
#if 0
    set_memory_pool(mem_pool, mem_size);
#endif

    /* Initialize net. */
    net_init();

    /* Initialize Pure Data, as does sys_main in s_main.c */
    pd_init();

    /* Set audio API. */
    sys_set_audio_api(API_ROCKBOX);

    /* Initialize audio subsystem. */
    sys_open_audio(0, /* No sound input yet */
                   sys_soundindevlist,
                   0, /* No sound input yet */
                   sys_chinlist,
                   1, /* One sound output device */
                   sys_soundoutdevlist,
                   -1, /* Use the default amount (2) of channels */
                   sys_choutlist,
                   PD_SAMPLERATE, /* Sample rate */
                   DEFAULTADVANCE, /* Scheduler advance */
                   1 /* Enable */);

    /* Create stacks for threads. */
    core_stack = getbytes(CORESTACKSIZE);
    gui_stack = getbytes(GUISTACKSIZE);
    if(core_stack == NULL || gui_stack == NULL)
    {
        rb->splash(HZ, "Not enough memory!");
        return PLUGIN_ERROR;
    }

    /* Boost CPU. */
    cpu_boost(true);

    /* Start threads. */
    core_thread_id =
        rb->create_thread(&core_thread,
                          core_stack,
                          CORESTACKSIZE,
                          0, /* FIXME Which flags? */
                          "PD core"
                          IF_PRIO(, PRIORITY_REALTIME)
                          IF_COP(, COP));

    gui_thread_id =
        rb->create_thread(&gui_thread,
                          gui_stack,
                          GUISTACKSIZE,
                          0, /* FIXME Which flags? */
                          "PD GUI"
                          IF_PRIO(, PRIORITY_USER_INTERFACE)
                          IF_COP(, CPU));

    /* If having an error creating threads, bail out. */
    if(core_thread_id == 0 || gui_thread_id == 0)
        return PLUGIN_ERROR;

    /* Initialize scheduler time variables. */
    sys_time = 0;
    sys_time_per_dsp_tick = (TIMEUNITPERSEC) *
                            ((double) sys_schedblocksize) / sys_dacsr;


    /* Main loop. */
    while(!quit)
    {
        /* Add time slice in milliseconds. */
        runningtime += (1000 / HZ);

        /* Sleep to the next time slice. */
        rb->sleep(1);
    }

    /* Wait for threads to complete. */
    rb->thread_wait(gui_thread_id);
    rb->thread_wait(core_thread_id);

    /* Unboost CPU. */
    cpu_boost(false);

    /* Close audio subsystem. */
    sys_close_audio();

    /* Destroy net. */
    net_destroy();

    /* Clear memory pool. */
#if 1
    destroy_memory_pool(mem_pool);
#endif
#if 0
    clear_memory_pool();
#endif

    return PLUGIN_OK;
}
