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

/* Sound output buffer. */
#define AUDIO_OUTPUT_BLOCKS 3
static struct pdbox_audio_block audio_output[AUDIO_OUTPUT_BLOCKS];
static unsigned int output_head;
static unsigned int output_tail;
static unsigned int output_fill;

/* Open audio. */
void rockbox_open_audio(int rate)
{
    unsigned int i;

    /* Initialize output buffer. */
    for(i = 0; i < AUDIO_OUTPUT_BLOCKS; i++)
        audio_output[i].fill = 0;

    output_head = 0;
    output_tail = 0;
    output_fill = 0;

#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif

    /* Set sample rate of the audio buffer. */
    rb->pcm_set_frequency(rate);
}

/* Close audio. */
void rockbox_close_audio(void)
{
    /* Stop playback. */
    rb->pcm_play_stop();

    /* Restore default sampling rate. */
    rb->pcm_set_frequency(HW_SAMPR_DEFAULT);
}

/* Rockbox audio callback. */
void pdbox_get_more(unsigned char** start, size_t* size)
{
    if(output_fill > 0)
    {
        /* Store output data address and size. */
        *start = (unsigned char*) audio_output[output_tail].data;
        *size = audio_output[output_tail].fill;

        /* Advance tail index. */
        audio_output[output_tail].fill = 0;
        output_fill--;
        if(output_tail == AUDIO_OUTPUT_BLOCKS-1)
            output_tail = 0;
        else
            output_tail++;
    }
    else
    {
        /* Nothing to play. */
        *start = NULL;
        *size = 0;
        return;
    }
}

/* Audio I/O. */
int rockbox_send_dacs(void)
{


    /* Start playback if needed and possible. */
    if(output_fill > 1 &&
       audio_output[output_tail].fill == PD_AUDIO_BLOCK_SIZE)
    {
        /* Start playback. */
        rb->pcm_play_data(pdbox_get_more,
                          (unsigned char*) audio_output[output_tail].data,
                          PD_AUDIO_BLOCK_SIZE);

        /* Advance tail index. */
        audio_output[output_tail].fill = PD_AUDIO_BLOCK_SIZE;
        output_fill--;
        if(output_tail == AUDIO_OUTPUT_BLOCKS-1)
            output_tail = 0;
        else
            output_tail++;
    }




#if 0
    if (sys_getrealtime() > timebefore + 0.002)
    {
        /* post("slept"); */
    	return (SENDDACS_SLEPT);
    }
    else 
#endif
    return (SENDDACS_YES);
}

/* Placeholder. */
void rockbox_listdevs(void)
{
}

#if 0
/* Scanning for devices */
void rockbox_getdevs(char *indevlist, int *nindevs,
    char *outdevlist, int *noutdevs, int *canmulti, 
    	int maxndev, int devdescsize)
{
}
#endif

