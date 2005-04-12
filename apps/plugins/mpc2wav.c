/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Thom Johansen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* This is a lovely mishmash of sample.c from libmusepack and mpa2wav.c,
 * but happens to work, so no whining!
 */

#include "plugin.h"

#if (CONFIG_HWCODEC == MASNONE)
/* software codec platforms */

#include <codecs/libmusepack/musepack.h>

#include "lib/xxx2wav.h" /* Helper functions common to test decoders */

static struct plugin_api* rb;
mpc_decoder decoder;

/*
  Our implementations of the mpc_reader callback functions.
*/
mpc_int32_t
read_impl(void *data, void *ptr, mpc_int32_t size)
{
  file_info_struct *f = (file_info_struct *)data;
  mpc_int32_t num = f->filesize - f->curpos;
  if (num > size)
    num = size;
  rb->memcpy(ptr, filebuf + f->curpos, num); 
  f->curpos += num;
  return num;
}

bool
seek_impl(void *data, mpc_int32_t offset)
{  
  file_info_struct *f = (file_info_struct *)data;
  if (offset > f->filesize) {
    return 0;
  } else {
    f->curpos = offset;
    return 1;
  }
}

mpc_int32_t
tell_impl(void *data)
{
  file_info_struct *f = (file_info_struct *)data;
  return f->curpos;
}

mpc_int32_t
get_size_impl(void *data)
{
  file_info_struct *f = (file_info_struct *)data;
  return f->filesize;
}

bool
canseek_impl(void *data)
{
  file_info_struct *f = (file_info_struct *)data;
  return true;
}

static int
shift_signed(MPC_SAMPLE_FORMAT val, int shift)
{
  if (shift > 0)
    val <<= shift;
  else if (shift < 0)
    val >>= -shift;
  return (int)val;
}

#define OUTPUT_BUFFER_SIZE	65536 /* Must be an integer multiple of 4. */

unsigned char OutputBuffer[OUTPUT_BUFFER_SIZE];
MPC_SAMPLE_FORMAT sample_buffer[MPC_DECODER_BUFFER_LENGTH];
unsigned char *OutputPtr=OutputBuffer;
const unsigned char *OutputBufferEnd=OutputBuffer+OUTPUT_BUFFER_SIZE;

#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* file)
{
  file_info_struct file_info;
  unsigned short Sample;
  unsigned status = 1;
  int i;
  mpc_reader reader;

  /* Generic plugin inititialisation */

  TEST_PLUGIN_API(api);
  rb = api;

#ifdef USE_IRAM
  rb->memcpy(iramstart, iramcopy, iramend-iramstart);
#endif

  reader.read = read_impl;
  reader.seek = seek_impl;
  reader.tell = tell_impl;
  reader.get_size = get_size_impl;
  reader.canseek = canseek_impl;
  reader.data = &file_info;

  /* This function sets up the buffers and reads the file into RAM */

  if (local_init(file, "/libmusepacktest.wav", &file_info, api)) {
    return PLUGIN_ERROR;
  }
  
  /* read file's streaminfo data */
  mpc_streaminfo info;
  mpc_streaminfo_init(&info);
  if (mpc_streaminfo_read(&info, &reader) != ERROR_CODE_OK) {
    rb->splash(HZ, true, "Not an MPC file.");
    return PLUGIN_ERROR;  
  }
  file_info.samplerate=info.sample_freq;
  /* instantiate a decoder with our file reader */
  mpc_decoder_setup(&decoder, &reader);
  if (!mpc_decoder_initialize(&decoder, &info)) {
    rb->splash(HZ, true, "Error in init.");
    return PLUGIN_ERROR;
  }
  file_info.frames_decoded = 0;
  file_info.start_tick = *(rb->current_tick);

  rb->button_clear_queue();

  /* This is the decoding loop. */
  while (status != 0) {
    status = mpc_decoder_decode(&decoder, sample_buffer, 0, 0);
    if (status == (unsigned)(-1)) {
        //decode error
        rb->splash(HZ, true, "Error decoding file.");
        break;
    }
    else                    //status>0
    {
      file_info.current_sample += status;
      file_info.frames_decoded++;
      /* Convert musepack's numbers to an array of 16-bit LE signed integers */
#if 1 /* uncomment to time without byte swapping and disk writing */
      for(i = 0; i < status*info.channels; i += info.channels)
      {
        /* Left channel */
        Sample=shift_signed(sample_buffer[i], 16 - MPC_FIXED_POINT_SCALE_SHIFT);
        *(OutputPtr++)=Sample&0xff;
        *(OutputPtr++)=Sample>>8;

        /* Right channel. If the decoded stream is monophonic then
         * the right output channel is the same as the left one.
         */
        if(info.channels==2)
          Sample=shift_signed(sample_buffer[i + 1], 16 - MPC_FIXED_POINT_SCALE_SHIFT);
        *(OutputPtr++)=Sample&0xff;
        *(OutputPtr++)=Sample>>8;

        /* Flush the buffer if it is full. */
        if(OutputPtr==OutputBufferEnd)
        {
          rb->write(file_info.outfile,OutputBuffer,OUTPUT_BUFFER_SIZE);
          OutputPtr=OutputBuffer;
        }
      }
#endif

    }
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
