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
#include <linux/soundcard.h>

/* We want to use the "real" open in some cases */
#undef open

struct mad_stream  Stream;
struct mad_frame  Frame;
struct mad_synth  Synth;
mad_timer_t      Timer;
int sound;

void init_oss(int sound, int sound_freq, int channels) {
  int format=AFMT_U16_LE;
  int setting=0x000C000D;  // 12 fragments size 8kb ? WHAT IS THIS?

  if (ioctl(sound,SNDCTL_DSP_SETFRAGMENT,&setting)==-1) {
    perror("SNDCTL_DSP_SETFRAGMENT");
  }

  if (ioctl(sound,SNDCTL_DSP_STEREO,&channels)==-1) {
    perror("SNDCTL_DSP_STEREO");
  }
  if (channels==0) { fprintf(stderr,"Warning, only mono supported\n"); }

  if (ioctl(sound,SNDCTL_DSP_SETFMT,&format)==-1) {
    perror("SNDCTL_DSP_SETFMT");
  }

  fprintf(stderr,"SETTING /dev/dsp to %dHz\n",sound_freq);
  if (ioctl(sound,SNDCTL_DSP_SPEED,&sound_freq)==-1) {
    perror("SNDCTL_DSP_SPEED");
  }
}

unsigned short MadFixedToUshort(mad_fixed_t Fixed)
{
  Fixed=Fixed>>(MAD_F_FRACBITS-15);
  return((unsigned short)Fixed);
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
  int sound,fd;
  mp3entry mp3;
  
  mp3info(&mp3, fname);

  #undef open
  sound=open("/dev/dsp", O_WRONLY);

  if (sound < 0) {
    fprintf(stderr,"Can not open /dev/dsp - Aborting - sound=%d\n",sound);
    exit(-1);
  }

  init_oss(sound,mp3.frequency,2);

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
      Sample=MadFixedToUshort(Synth.pcm.samples[0][i]);
      *(OutputPtr++)=Sample&0xff;
      *(OutputPtr++)=Sample>>8;

      /* Right channel. If the decoded stream is monophonic then
       * the right output channel is the same as the left one.
       */
      if(MAD_NCHANNELS(&Frame.header)==2)
        Sample=MadFixedToUshort(Synth.pcm.samples[1][i]);
      *(OutputPtr++)=Sample&0xff;
      *(OutputPtr++)=Sample>>8;

      /* Flush the buffer if it is full. */
      if(OutputPtr==OutputBufferEnd)
      {
        if(write(sound,OutputBuffer,OUTPUT_BUFFER_SIZE)!=OUTPUT_BUFFER_SIZE)
        {
          fprintf(stderr,"PCM write error.\n");
          Status=2;
          break;
        }
        OutputPtr=OutputBuffer;
      }
    }
  }while(1);

  fprintf(stderr,"END OF MAD LOOP\n");
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

  close(sound);
  /* That's the end of the world (in the H. G. Wells way). */
  return(Status);
}


#endif
