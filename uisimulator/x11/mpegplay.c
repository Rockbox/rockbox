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

#ifdef MPEG_PLAY

#include <string.h>
#include <stdlib.h>
#include <file.h>
#include <types.h>
#include <lcd.h>
#include <button.h>
#include "id3.h"

#include <stdio.h>
#include <mad.h>

#include "oss_sound.h"

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

/*
 * NAME:    pack_pcm()
 * DESCRIPTION:  scale and dither MAD output
 */
static
void pack_pcm(unsigned char **pcm, unsigned int nsamples,
        mad_fixed_t const *ch1, mad_fixed_t const *ch2)
{
  register signed int s0, s1;
  static struct dither d0, d1;

  if (ch2) {  /* stereo */
    while (nsamples--) {
      s0 = scale(*ch1++, &d0);
      s1 = scale(*ch2++, &d1);
# if SAMPLE_DEPTH == 16
      (*pcm)[0 + 0] = s0 >> 0;
      (*pcm)[0 + 1] = s0 >> 8;
      (*pcm)[2 + 0] = s1 >> 0;
      (*pcm)[2 + 1] = s1 >> 8;

      *pcm += 2 * 2;
# elif SAMPLE_DEPTH == 8
      (*pcm)[0] = s0 ^ 0x80;
      (*pcm)[1] = s1 ^ 0x80;

      *pcm += 2;
# else
#  error "bad SAMPLE_DEPTH"
# endif
    }
  }
  else {  /* mono */
    while (nsamples--) {
      s0 = scale(*ch1++, &d0);

# if SAMPLE_DEPTH == 16
      (*pcm)[0] = s0 >> 0;
      (*pcm)[1] = s0 >> 8;

      *pcm += 2;
# elif SAMPLE_DEPTH == 8
      *(*pcm)++ = s0 ^ 0x80;
# endif
    }
  }
}

#define INPUT_BUFFER_SIZE  (5*8192)
#define OUTPUT_BUFFER_SIZE  8192 /* Must be an integer multiple of 4. */
int mpeg_play(char* fname)
{
  unsigned char    InputBuffer[INPUT_BUFFER_SIZE],
            OutputBuffer[OUTPUT_BUFFER_SIZE],
            *OutputPtr=OutputBuffer;
  const unsigned char  *OutputBufferEnd=OutputBuffer+OUTPUT_BUFFER_SIZE;
  int          Status=0,
            i;
  unsigned long    FrameCount=0;
  sound_t sound;
  int fd;
  mp3entry mp3;
  register signed int s0, s1;
  static struct dither d0, d1;
  
  mp3info(&mp3, fname);

  init_sound(&sound);

  /* Configure sound device for this file - always select Stereo because
     some sound cards don't support mono */
  config_sound(&sound,mp3.frequency,2);

  fd=x11_open(fname,O_RDONLY);
  if (fd < 0) {
    fprintf(stderr,"could not open %s\n",fname);
    return 0;
  }

  /* First the structures used by libmad must be initialized. */
  mad_stream_init(&Stream);
  mad_frame_init(&Frame);
  mad_synth_init(&Synth);
  mad_timer_reset(&Timer);

  do
  {
    if (button_get()) break; /* Return if a key is pressed */

    if(Stream.buffer==NULL || Stream.error==MAD_ERROR_BUFLEN)
    {
      size_t ReadSize,Remaining;
      unsigned char *ReadStart;

      if(Stream.next_frame!=NULL)
      {
        Remaining=Stream.bufend-Stream.next_frame;
        memmove(InputBuffer,Stream.next_frame,Remaining);
        ReadStart=InputBuffer+Remaining;
        ReadSize=INPUT_BUFFER_SIZE-Remaining;
      }
      else
        ReadSize=INPUT_BUFFER_SIZE,
          ReadStart=InputBuffer,
          Remaining=0;
      
      ReadSize=read(fd,ReadStart,ReadSize);
      if(ReadSize<=0)
      {
        fprintf(stderr,"end of input stream\n");
        break;
      }

      mad_stream_buffer(&Stream,InputBuffer,ReadSize+Remaining);
      Stream.error=0;
    }

    if(mad_frame_decode(&Frame,&Stream)) {
      if(MAD_RECOVERABLE(Stream.error))
      {
        fprintf(stderr,"recoverable frame level error\n");
        fflush(stderr);
        continue;
      }
      else
        if(Stream.error==MAD_ERROR_BUFLEN) {
          continue;
        } else {
          fprintf(stderr,"unrecoverable frame level error\n");
          Status=1;
          break;
        }
    }

    FrameCount++;
    mad_timer_add(&Timer,Frame.header.duration);
        
    mad_synth_frame(&Synth,&Frame);

    for(i=0;i<Synth.pcm.length;i++)
    {
      unsigned short  Sample;

      /* Left channel */
      Sample=scale(Synth.pcm.samples[0][i],&d0);
      *(OutputPtr++)=Sample&0xff;
      *(OutputPtr++)=Sample>>8;

      /* Right channel. If the decoded stream is monophonic then
       * the right output channel is the same as the left one.
       */
      if(MAD_NCHANNELS(&Frame.header)==2)
        Sample=scale(Synth.pcm.samples[1][i],&d0);
      *(OutputPtr++)=Sample&0xff;
      *(OutputPtr++)=Sample>>8;

      /* Flush the buffer if it is full. */
      if(OutputPtr==OutputBufferEnd)
      {
        if(output_sound(&sound,OutputBuffer,OUTPUT_BUFFER_SIZE)!=OUTPUT_BUFFER_SIZE)
        {
          fprintf(stderr,"PCM write error.\n");
          Status=2;
          break;
        }
        OutputPtr=OutputBuffer;
      }
    }
  }while(1);

  /* Mad is no longer used, the structures that were initialized must
     * now be cleared.
   */
  mad_synth_finish(&Synth);
  mad_frame_finish(&Frame);
  mad_stream_finish(&Stream);

  /* If the output buffer is not empty and no error occured during
     * the last write, then flush it.
   */
  if(OutputPtr!=OutputBuffer && Status!=2)
  {
    size_t  BufferSize=OutputPtr-OutputBuffer;

    if(write(sound,OutputBuffer,1,BufferSize)!=BufferSize)
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
  return(Status);
}


#endif
