/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2002 Dave Chapman
 *
 * This file contains significant code from two other projects:
 *
 * 1) madldd - a sample application to use libmad
 * 2) CoolPlayer - a win32 audio player that also uses libmad
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifdef HAVE_MPEG_PLAY
#ifdef HAVE_LIBMAD

#include <string.h>
#include <stdlib.h>
#include <file.h>
#include <lcd.h>
#include <button.h>
#include "id3.h"

#include <stdio.h>
#include <mad.h>

#include "sound.h"

/* The "dither" code to convert the 24-bit samples produced by libmad was
   taken from the coolplayer project - coolplayer.sourceforge.net */

struct dither {
	mad_fixed_t error[3];
	mad_fixed_t random;
};
# define SAMPLE_DEPTH	16
# define scale(x, y)	dither((x), (y))

struct mad_stream  Stream;
struct mad_frame  Frame;
struct mad_synth  Synth;
mad_timer_t      Timer;

/*
 * NAME:		prng()
 * DESCRIPTION:	32-bit pseudo-random number generator
 */
static __inline
unsigned long prng(unsigned long state)
{
  return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}

/*
 * NAME:        dither()
 * DESCRIPTION:	dither and scale sample
 */
static __inline
signed int dither(mad_fixed_t sample, struct dither *dither)
{
  unsigned int scalebits;
  mad_fixed_t output, mask, random;

  enum {
    MIN = -MAD_F_ONE,
    MAX =  MAD_F_ONE - 1
  };

  /* noise shape */
  sample += dither->error[0] - dither->error[1] + dither->error[2];

  dither->error[2] = dither->error[1];
  dither->error[1] = dither->error[0] / 2;

  /* bias */
  output = sample + (1L << (MAD_F_FRACBITS + 1 - SAMPLE_DEPTH - 1));

  scalebits = MAD_F_FRACBITS + 1 - SAMPLE_DEPTH;
  mask = (1L << scalebits) - 1;

  /* dither */
  random  = prng(dither->random);
  output += (random & mask) - (dither->random & mask);

  dither->random = random;

  /* clip */
  if (output > MAX) {
    output = MAX;

    if (sample > MAX)
      sample = MAX;
  }
  else if (output < MIN) {
    output = MIN;

    if (sample < MIN)
      sample = MIN;
  }

  /* quantize */
  output &= ~mask;

  /* error feedback */
  dither->error[0] = sample - output;

  /* scale */
  return output >> scalebits;
}

#define INPUT_BUFFER_SIZE  (5*8192)
#define OUTPUT_BUFFER_SIZE  8192 /* Must be an integer multiple of 4. */
void real_mpeg_play(char* fname)
{
    unsigned char InputBuffer[INPUT_BUFFER_SIZE],
        OutputBuffer[OUTPUT_BUFFER_SIZE],
        *OutputPtr=OutputBuffer;
    const unsigned char  *OutputBufferEnd=OutputBuffer+OUTPUT_BUFFER_SIZE;
    int Status=0, i, fd;
    unsigned long FrameCount=0;
    sound_t sound;
    struct mp3entry mp3;
    static struct dither d0, d1;
    int key=0;
  
    mp3info(&mp3, fname, false); /* FIXME: honor the v1first setting */

    init_sound(&sound);

    /* Configure sound device for this file - always select Stereo because
       some sound cards don't support mono */
    config_sound(&sound,mp3.frequency,2);

    if ((fd=open(fname,O_RDONLY)) < 0) {
        fprintf(stderr,"could not open %s\n",fname);
        return;
    }

    /* First the structures used by libmad must be initialized. */
    mad_stream_init(&Stream);
    mad_frame_init(&Frame);
    mad_synth_init(&Synth);
    mad_timer_reset(&Timer);

    do {
        if (Stream.buffer==NULL || Stream.error==MAD_ERROR_BUFLEN) {
            size_t ReadSize,Remaining;
            unsigned char *ReadStart;

            if(Stream.next_frame!=NULL) {
                Remaining=Stream.bufend-Stream.next_frame;
                memmove(InputBuffer,Stream.next_frame,Remaining);
                ReadStart=InputBuffer+Remaining;
                ReadSize=INPUT_BUFFER_SIZE-Remaining;
            } else {
                ReadSize=INPUT_BUFFER_SIZE,
                    ReadStart=InputBuffer,
                    Remaining=0;
            }

            if ((int)(ReadSize=read(fd,ReadStart,ReadSize)) < 0) {
                fprintf(stderr,"end of input stream\n");
                break;
            }

            mad_stream_buffer(&Stream,InputBuffer,ReadSize+Remaining);
            Stream.error=0;
        }

        if(mad_frame_decode(&Frame,&Stream)) {
            if(MAD_RECOVERABLE(Stream.error)) {
                fprintf(stderr,"recoverable frame level error\n");
                fflush(stderr);
                continue;
            } else {
                if(Stream.error==MAD_ERROR_BUFLEN) {
                    continue;
                } else {
                    fprintf(stderr,"unrecoverable frame level error\n");
                    Status=1;
                    break;
                }
            }
        }

        FrameCount++;
        mad_timer_add(&Timer,Frame.header.duration);
        
        mad_synth_frame(&Synth,&Frame);

        for(i=0;i<Synth.pcm.length;i++) {
            unsigned short  Sample;
            
            /* Left channel */
            Sample=scale(Synth.pcm.samples[0][i],&d0);
            *(OutputPtr++)=Sample&0xff;
            *(OutputPtr++)=Sample>>8;
            
            /* Right channel. If the decoded stream is monophonic then
             * the right output channel is the same as the left one.
             */
            if(MAD_NCHANNELS(&Frame.header)==2) {
                Sample=scale(Synth.pcm.samples[1][i],&d1);
            }

            *(OutputPtr++)=Sample&0xff;
            *(OutputPtr++)=Sample>>8;
            
            /* Flush the buffer if it is full. */
            if (OutputPtr==OutputBufferEnd) {
                if (output_sound(&sound, OutputBuffer,
                                 OUTPUT_BUFFER_SIZE)!=OUTPUT_BUFFER_SIZE) {
                    fprintf(stderr,"PCM write error.\n");
                    Status=2;
                    break;
                }
                OutputPtr=OutputBuffer;
            }
            }

        if ((key=button_get(0))==BUTTON_STOP)
	{
            break; 
	}

    }while(1);
    
    /* Mad is no longer used, the structures that were initialized must
     * now be cleared.
     */
    mad_synth_finish(&Synth);
    mad_frame_finish(&Frame);
    mad_stream_finish(&Stream);

    /* If the output buffer is not empty and no error occured during
     * the last write, then flush it. */
    if(OutputPtr!=OutputBuffer && Status!=2)
        {
            size_t  BufferSize=OutputPtr-OutputBuffer;

            if (output_sound(&sound, OutputPtr, BufferSize)!=(int)BufferSize)
                {
                    fprintf(stderr,"PCM write error\n");
                    Status=2;
                }
        }

    /* Accounting report if no error occured. */
    if(!Status)
        {
            char  Buffer[80];

            mad_timer_string(Timer,Buffer,"%lu:%02lu.%03u",
                             MAD_UNITS_MINUTES,MAD_UNITS_MILLISECONDS,0);
            fprintf(stderr,"%lu frames decoded (%s).\n",FrameCount,Buffer);
        }

    close_sound(&sound);
    /* That's the end of the world (in the H. G. Wells way). */
    return;
}


#endif /* HAVE_LIBMAD */
#endif /* HAVE_MPEG_PLAY */
