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

#ifndef PDBOX_H
#define PDBOX_H

#if 0
/* Use dbestfit. */
#include "bmalloc.h"
#include "dmalloc.h"
#endif

/* Minimal memory size. */
#define MIN_MEM_SIZE (4 * 1024 * 1024)

/* Maximal size of the datagram. */
#define MAX_DATAGRAM_SIZE 255

/* This structure replaces a UDP datagram. */
struct datagram
{
    bool used;
    uint8_t size;
    char data[MAX_DATAGRAM_SIZE];
};

/* Network functions prototypes. */
void net_init(void);
void net_destroy(void);
bool send_datagram(struct event_queue* route, int port,
                   char* data, size_t size);
bool receive_datagram(struct event_queue* route, int port,
                      struct datagram* buffer);

/* Network message queues. */
extern struct event_queue gui_to_core;
extern struct event_queue core_to_gui;

/* UDP ports of the original software. */
#define PD_CORE_PORT 3333
#define PD_GUI_PORT 3334

/* Convinience macros. */
#define SEND_TO_CORE(data) \
    send_datagram(&gui_to_core, PD_CORE_PORT, data, rb->strlen(data))
#define RECEIVE_TO_CORE(buffer) \
    receive_datagram(&gui_to_core, PD_CORE_PORT, buffer)
#define SEND_FROM_CORE(data) \
    send_datagram(&core_to_gui, PD_GUI_PORT, data, rb->strlen(data))
#define RECEIVE_FROM_CORE(buffer) \
    receive_datagram(&core_to_gui, PD_GUI_PORT, buffer)

#endif

