/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"

#if (CONFIG_HWCODEC == MASNONE)
/* software codec platforms */

#include <codecs/libmad/mad.h>

#include "lib/xxx2wav.h" /* Helper functions common to test decoders */

static struct plugin_api* rb;

struct mad_stream  Stream __attribute__ ((section(".idata")));
struct mad_frame  Frame __attribute__ ((section(".idata")));
struct mad_synth  Synth __attribute__ ((section(".idata")));
mad_timer_t      Timer;
struct dither d0, d1;

/* The following function is used inside libmad - let's hope it's never
   called. 
*/

void abort(void) {
}

/* The "dither" code to convert the 24-bit samples produced by libmad was
   taken from the coolplayer project - coolplayer.sourceforge.net */

struct dither {
	mad_fixed_t error[3];
	mad_fixed_t random;
};

# define SAMPLE_DEPTH	16
# define scale(x, y)	dither((x), (y))

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

#define SHRT_MAX 32767

#define INPUT_BUFFER_SIZE	(10*8192)
#define OUTPUT_BUFFER_SIZE	65536 /* Must be an integer multiple of 4. */

unsigned char InputBuffer[INPUT_BUFFER_SIZE+MAD_BUFFER_GUARD];
unsigned char OutputBuffer[OUTPUT_BUFFER_SIZE];
unsigned char *OutputPtr=OutputBuffer;
unsigned char *GuardPtr=NULL;
const unsigned char *OutputBufferEnd=OutputBuffer+OUTPUT_BUFFER_SIZE;

#ifndef SIMULATOR
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* file)
{
  file_info_struct file_info;
  int Status=0;
  unsigned short Sample;
  int i;
  size_t ReadSize, Remaining;
  unsigned char  *ReadStart;

  /* Generic plugin inititialisation */

  TEST_PLUGIN_API(api);
  rb = api;

#ifndef SIMULATOR
  rb->memcpy(iramstart, iramcopy, iramend-iramstart);
#endif

  /* This function sets up the buffers and reads the file into RAM */

  if (local_init(file,"/libmadtest.wav",&file_info,api)) {
    return PLUGIN_ERROR;
  }

  /* Create a decoder instance */

  mad_stream_init(&Stream);
  mad_frame_init(&Frame);
  mad_synth_init(&Synth);
  mad_timer_reset(&Timer);

 //if error:   return PLUGIN_ERROR;

  file_info.curpos=0;
  file_info.start_tick=*(rb->current_tick);

  rb->button_clear_queue();

  /* This is the decoding loop. */
  while (file_info.curpos < file_info.filesize) {
    if(Stream.buffer==NULL || Stream.error==MAD_ERROR_BUFLEN) {
      if(Stream.next_frame!=NULL) {
        Remaining=Stream.bufend-Stream.next_frame;
        memmove(InputBuffer,Stream.next_frame,Remaining);
        ReadStart=InputBuffer+Remaining;
        ReadSize=INPUT_BUFFER_SIZE-Remaining;
      } else {
        ReadSize=INPUT_BUFFER_SIZE;
        ReadStart=InputBuffer;
        Remaining=0;
      }

      /* Fill-in the buffer. If an error occurs print a message
       * and leave the decoding loop. If the end of stream is
       * reached we also leave the loop but the return status is
       * left untouched.
       */

      if ((file_info.filesize-file_info.curpos) < (int) ReadSize) {
         ReadSize=file_info.filesize-file_info.curpos;
      }
      memcpy(ReadStart,&filebuf[file_info.curpos],ReadSize);
      file_info.curpos+=ReadSize;

      if (file_info.curpos >= file_info.filesize) 
      {
        GuardPtr=ReadStart+ReadSize;
        memset(GuardPtr,0,MAD_BUFFER_GUARD);
        ReadSize+=MAD_BUFFER_GUARD;
      }

      /* Pipe the new buffer content to libmad's stream decoder facility */
      mad_stream_buffer(&Stream,InputBuffer,ReadSize+Remaining);
      Stream.error=0;
    }

    if(mad_frame_decode(&Frame,&Stream))
    {
      if(MAD_RECOVERABLE(Stream.error))
      {
        if(Stream.error!=MAD_ERROR_LOSTSYNC || Stream.this_frame!=GuardPtr)
        {
          rb->splash(HZ*1, true, "Recoverable...!");
        }
        continue;
      }
      else
        if(Stream.error==MAD_ERROR_BUFLEN)
          continue;
        else
        {
          rb->splash(HZ*1, true, "Recoverable...!");
          //fprintf(stderr,"%s: unrecoverable frame level error.\n",ProgName);
          Status=1;
          break;
        }
    }

    /* We assume all frames have same samplerate as the first */
    if(file_info.frames_decoded==0) {
      file_info.samplerate=Frame.header.samplerate;
    }

    file_info.frames_decoded++;

    /* ?? Do we need the timer module? */
    mad_timer_add(&Timer,Frame.header.duration);

/* DAVE: This can be used to attenuate the audio */
//    if(DoFilter)
//      ApplyFilter(&Frame);   

    mad_synth_frame(&Synth,&Frame);

    /* Convert MAD's numbers to an array of 16-bit LE signed integers */
    for(i=0;i<Synth.pcm.length;i++)
    {
      /* Left channel */
      Sample=scale(Synth.pcm.samples[0][i],&d0);
      *(OutputPtr++)=Sample&0xff;
      *(OutputPtr++)=Sample>>8;

      /* Right channel. If the decoded stream is monophonic then
       * the right output channel is the same as the left one.
       */
      if(MAD_NCHANNELS(&Frame.header)==2)
        Sample=scale(Synth.pcm.samples[1][i],&d1);
      *(OutputPtr++)=Sample&0xff;
      *(OutputPtr++)=Sample>>8;

      /* Flush the buffer if it is full. */
      if(OutputPtr==OutputBufferEnd)
      {
        rb->write(file_info.outfile,OutputBuffer,OUTPUT_BUFFER_SIZE);
        OutputPtr=OutputBuffer;
      }
    }

    file_info.current_sample+=Synth.pcm.length;

    display_status(&file_info);

    if (rb->button_get(false)!=BUTTON_NONE) { 
      close_wav(&file_info);
      return PLUGIN_OK;
    }
  }

  close_wav(&file_info);
  rb->splash(HZ*2, true, "FINISHED!");

  return PLUGIN_OK;
}
#endif /* CONFIG_HWCODEC == MASNONE */
