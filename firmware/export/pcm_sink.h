/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 by Sho Tanimoto
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
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "config.h"

struct pcm_sink_caps {
    const unsigned long* samprs;
    uint16_t             num_samprs;
    uint16_t             default_freq;
};

struct pcm_sink_ops {
    void (*init)(void);
    void (*postinit)(void);
    void (*set_freq)(uint16_t freq);
    void (*lock)(void);
    void (*unlock)(void);
    void (*play)(const void* addr, size_t size);
    void (*stop)(void);
};

struct pcm_sink {
    /* characteristics */
    const struct pcm_sink_caps caps;

    /* operations */
    const struct pcm_sink_ops ops;

    /* runtime states */
    unsigned long pending_freq;
    unsigned long configured_freq;
};

enum pcm_sink_ids {
    PCM_SINK_BUILTIN = 0,
#ifdef USB_ENABLE_IAP
    PCM_SINK_IAP,
#endif
    PCM_SINK_NUM
};

/* defined in each platform pcm source */
extern struct pcm_sink builtin_pcm_sink;
