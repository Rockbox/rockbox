/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009, 2010 Wincent Balin
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
#include "../../pdbox.h"

#include "m_pd.h"
#include "s_stuff.h"

/* Declare functions that go to IRAM. */
void pdbox_get_more(const void** start, size_t* size) ICODE_ATTR;
int rockbox_send_dacs(void) ICODE_ATTR;

/* Extern variables. */
extern float sys_dacsr;
extern t_sample *sys_soundout;
extern t_sample *sys_soundin;

/* Output buffer. */
#define OUTBUFSIZE 3
static struct audio_buffer outbuf[OUTBUFSIZE];
static unsigned int outbuf_head;
static unsigned int outbuf_tail;
static unsigned int outbuf_fill;

/* Playing status. */
static bool playing;


/* Open audio. */
void rockbox_open_audio(int rate)
{
    unsigned int i;

    /* No sound yet. */
    playing = false;

    /* Stop playing to reconfigure audio settings. */
    rb->pcm_play_stop();

#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif

    /* Set sample rate of the audio buffer. */
    rb->pcm_set_frequency(rate);
    rb->pcm_apply_settings();

    /* Initialize output buffer. */
    for(i = 0; i < OUTBUFSIZE; i++)
        outbuf[i].fill = 0;

    outbuf_head = 0;
    outbuf_tail = 0;
    outbuf_fill = 0;
}

/* Close audio. */
void rockbox_close_audio(void)
{
    /* Stop playback. */
    rb->pcm_play_stop();

    /* Reset playing status. */
    playing = false;

    /* Restore default sampling rate. */
    rb->pcm_set_frequency(HW_SAMPR_DEFAULT);
    rb->pcm_apply_settings();
}

/* Rockbox audio callback. */
void pdbox_get_more(const void** start, size_t* size)
{
    if(outbuf_fill > 0)
    {
        /* Store output data address and size. */
        *start = outbuf[outbuf_tail].data;
        *size = sizeof(outbuf[outbuf_tail].data);

        /* Free this part of output buffer. */
        outbuf[outbuf_tail].fill = 0;

        /* Advance to the next part of output buffer. */
        if(outbuf_tail == OUTBUFSIZE-1)
            outbuf_tail = 0;
        else
            outbuf_tail++;

        /* Decrease output buffer fill. */
        outbuf_fill--;
    }
    else
    {
        /* Reset playing status. */
        playing = false;

        /* Nothing to play. */
    }
}

/* Audio I/O. */
int rockbox_send_dacs(void)
{
    /* Copy sys_output buffer. */
    t_sample* left = sys_soundout + DEFDACBLKSIZE*0;
    t_sample* right = sys_soundout + DEFDACBLKSIZE*1;
    unsigned int samples_out = 0;
    int16_t* out;
    register int sample;

    /* Cancel if whole buffer filled. */
    if(outbuf_fill >= OUTBUFSIZE-1)
        return SENDDACS_NO;

    do
    {
      /* Write the block of sound. */
      for(out = outbuf[outbuf_head].data +
                outbuf[outbuf_head].fill * PD_OUT_CHANNELS;
          outbuf[outbuf_head].fill < (AUDIOBUFSIZE / PD_OUT_CHANNELS) &&
              samples_out < DEFDACBLKSIZE;
          left++, right++, samples_out++, outbuf[outbuf_head].fill++)
      {
          /* Copy samples from both channels. */
          sample = SCALE16(*left);
          if(sample > 32767)
              sample = 32767;
          else if(sample < -32767)
              sample = -32767;
          *out++ = sample;
          sample = SCALE16(*right);
          if(sample > 32767)
              sample = 32767;
          else if(sample < -32767)
              sample = -32767;
          *out++ = sample;
      }

      /* If part of output buffer filled... */
      if(outbuf[outbuf_head].fill >= (AUDIOBUFSIZE / PD_OUT_CHANNELS))
      {
          /* Advance one part of output buffer. */
          if(outbuf_head == OUTBUFSIZE-1)
              outbuf_head = 0;
          else
              outbuf_head++;

          /* Increase fill counter. */
          outbuf_fill++;
      }

    /* If needed, fill the next frame. */
    }
    while(samples_out < DEFDACBLKSIZE);

    /* Clear Pure Data output buffer. */
    memset(sys_soundout,
           0,
           sizeof(t_sample) * DEFDACBLKSIZE * PD_OUT_CHANNELS);

    /* If still not playing... */
    if(!playing && outbuf_fill > 0)
    {
        /* Start playing. */
        rb->pcm_play_data(pdbox_get_more, NULL, NULL, 0);

        /* Set status flag. */
        playing = true;
    }

    return SENDDACS_YES;
}

