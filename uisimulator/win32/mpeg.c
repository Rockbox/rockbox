/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* This file is for emulating some of the mpeg controlling functions of 
   the target */

#include "debug.h"
#include "mpeg.h"

static char *units[] =
{
    "%",    /* Volume */
    "%",    /* Bass */
    "%"     /* Treble */
};

static int numdecimals[] =
{
    0,    /* Volume */
    0,    /* Bass */
    0     /* Treble */
};

static int minval[] =
{
    0,    /* Volume */
    0,    /* Bass */
    0     /* Treble */
};

static int maxval[] =
{
    50,    /* Volume */
    50,    /* Bass */
    50     /* Treble */
};

static int defaultval[] =
{
    70/2,    /* Volume */
    50/2,    /* Bass */
    50/2     /* Treble */
};

char *mpeg_sound_unit(int setting)
{
    return units[setting];
}

int mpeg_sound_numdecimals(int setting)
{
    return numdecimals[setting];
}

int mpeg_sound_min(int setting)
{
    return minval[setting];
}

int mpeg_sound_max(int setting)
{
    return maxval[setting];
}

int mpeg_sound_default(int setting)
{
    return defaultval[setting];
}

void mpeg_volume(void)
{
}

void mpeg_bass(void)
{
}

void mpeg_treble(void)
{
}

void mpeg_stop(void)
{
}

void mpeg_resume(void)
{
}

void mpeg_pause(void)
{
}

void mpeg_next (void)
{
}

void mpeg_prev (void)
{
}

struct mp3entry* mpeg_current_track(void)
{
    return 0;
}

void mpeg_sound_set(int setting, int value)
{
}

#ifndef MPEGPLAY
void mpeg_play(char *tune)
{
  DEBUGF("We instruct the MPEG thread to play %s for us\n",
         tune);
}

int mpeg_val2phys(int setting, int value)
{
    int result = 0;
    
    switch(setting)
    {
        case SOUND_VOLUME:
            result = value * 2;
            break;
        
        case SOUND_BASS:
            result = value * 2;
            break;
        
        case SOUND_TREBLE:
            result = value * 2;
            break;
    }
    return result;
}

#endif
