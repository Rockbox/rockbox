/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Nicolas Pitre <nico@cam.org>
 * Copyright (C) 2006-2007 by Stéphane Doyon <s.doyon@videotron.ca>
 * Copyright (C) 2012 Michael Sevakis
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

#include "platform.h"
#include "dsp_core.h"
#include "core_alloc.h"
#include "tdspeed.h"

#ifdef HAVE_PITCHCONTROL
static int handles[4] = { 0, 0, 0, 0 };

static int move_callback(int handle, void *current, void *new)
{
#if 0
    /* Should not currently need to block this since DSP loop completes an
       iteration before yielding and begins again at its input buffer */
    if (dsp_is_busy(tdspeed_state.dsp))
        return BUFLIB_CB_CANNOT_MOVE; /* DSP processing in progress */
#endif

    for (unsigned int i = 0; i < ARRAYLEN(handles); i++)
    {
        if (handle != handles[i])
            continue;

        tdspeed_move(i, current, new);
        break;
    }

    return BUFLIB_CB_OK;
}

static struct buflib_callbacks ops =
{
    .move_callback = move_callback,
    .shrink_callback = NULL,
};

/* Allocate timestretch buffers */
bool tdspeed_alloc_buffers(int32_t **buffers, const int *buf_s, int nbuf)
{
    static const char *buffer_names[4] = {
        "tdspeed ovl L",
        "tdspeed ovl R",
        "tdspeed out L",
        "tdspeed out R"
    };

    for (int i = 0; i < nbuf; i++)
    {
        if (handles[i] <= 0)
        {
            handles[i] = core_alloc_ex(buffer_names[i], buf_s[i], &ops);

            if (handles[i] <= 0)
                return false;
        }

        if (buffers[i] == NULL)
        {
            buffers[i] = core_get_data(handles[i]);

            if (buffers[i] == NULL)
                return false;
        }
    }

    return true;
}

/* Free timestretch buffers */
void tdspeed_free_buffers(int32_t **buffers, int nbuf)
{
    for (int i = 0; i < nbuf; i++)
    {
        if (handles[i] > 0)
            core_free(handles[i]);

        handles[i] = 0;
        buffers[i] = NULL;
    }
}
#endif /* HAVE_PITCHCONTROL */

