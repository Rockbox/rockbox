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

/* Welcome to the PDBox plugin */
PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

/* Quit flag. */
bool quit = false;

/* Thread IDs. */
unsigned int core_thread_id;
unsigned int gui_thread_id;

/* Stacks for threads. */
#define STACK_SIZE 16384
uint32_t core_stack[STACK_SIZE / sizeof(uint32_t)];
uint32_t gui_stack[STACK_SIZE / sizeof(uint32_t)];


/* Core thread */
void core_thread(void)
{
    struct datagram ping;

    /* Main loop */
    while(!quit)
    {
        /* Wait for request. */
        while(!RECEIVE_TO_CORE(&ping))
            rb->yield();

        if(memcmp("Ping!", ping.data, ping.size) == 0)
        {
            SEND_FROM_CORE("Pong!");
            break;
        }

        rb->yield();
    }

   rb->thread_exit();
}

/* GUI thread */
void gui_thread(void)
{
    struct datagram pong;

    /* GUI loop */
    while(!quit)
    {
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

        rb->yield();
    }

    rb->thread_exit();
}


/* Plug-in entry point */
enum plugin_status plugin_start(const void* parameter)
{
    PLUGIN_IRAM_INIT(rb)

    size_t mem_size;
    void* mem_pool;

    /* Get the file name. */
    const char* filename = (const char*) parameter;

#if 0
    /* Allocate memory; check it's size; add to the pool. */
    mem_pool = rb->plugin_get_audio_buffer(&mem_size);
    if(mem_size < MIN_MEM_SIZE)
    {
        rb->splash(HZ, "Not enough memory!");
        return PLUGIN_ERROR;
    }
    add_pool(mem_pool, mem_size);
#endif

    /* Initialze net. */
    net_init();

    /* Start threads. */
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
        rb->yield();

    /* Wait for threads to complete. */
    rb->thread_wait(gui_thread_id);
    rb->thread_wait(core_thread_id);

    /* Destroy net. */
    net_destroy();

    return PLUGIN_OK;
}
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

/* Welcome to the PDBox plugin */
PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

/* Quit flag. */
bool quit = false;

/* Thread IDs. */
unsigned int core_thread_id;
unsigned int gui_thread_id;

/* Stacks for threads. */
#define STACK_SIZE 16384
uint32_t core_stack[STACK_SIZE / sizeof(uint32_t)];
uint32_t gui_stack[STACK_SIZE / sizeof(uint32_t)];


/* Core thread */
void core_thread(void)
{
    struct datagram ping;

    /* Main loop */
    while(!quit)
    {
        /* Wait for request. */
        while(!RECEIVE_TO_CORE(&ping))
            rb->yield();

        if(memcmp("Ping!", ping.data, ping.size) == 0)
        {
            SEND_FROM_CORE("Pong!");
            break;
        }

        rb->yield();
    }

   rb->thread_exit();
}

/* GUI thread */
void gui_thread(void)
{
    struct datagram pong;

    /* GUI loop */
    while(!quit)
    {
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

        rb->yield();
    }

    rb->thread_exit();
}


/* Plug-in entry point */
enum plugin_status plugin_start(const void* parameter)
{
    PLUGIN_IRAM_INIT(rb)

    size_t mem_size;
    void* mem_pool;

    /* Get the file name. */
    const char* filename = (const char*) parameter;

#if 0
    /* Allocate memory; check it's size; add to the pool. */
    mem_pool = rb->plugin_get_audio_buffer(&mem_size);
    if(mem_size < MIN_MEM_SIZE)
    {
        rb->splash(HZ, "Not enough memory!");
        return PLUGIN_ERROR;
    }
    add_pool(mem_pool, mem_size);
#endif

    /* Initialze net. */
    net_init();

    /* Start threads. */
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
        rb->yield();

    /* Wait for threads to complete. */
    rb->thread_wait(gui_thread_id);
    rb->thread_wait(core_thread_id);

    /* Destroy net. */
    net_destroy();

    return PLUGIN_OK;
}
