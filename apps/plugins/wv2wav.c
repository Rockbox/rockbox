/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Christian Gmeiner
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

#include "lib/xxx2wav.h" /* Helper functions common to test decoders */
#include <codecs/libwavpack/wavpack.h>

#define BUFFER_SIZE 4096

static struct plugin_api* rb;
static file_info_struct file_info;
static long temp_buffer [BUFFER_SIZE];

/* Reformat samples from longs in processor's native endian mode to
 little-endian data with (possibly) less than 4 bytes / sample. */
uchar* format_samples (int bps, uchar *dst, long *src, ulong samcnt)
{
  long temp;

  switch (bps) 
  {
    case 1:
      while (samcnt--)
        *dst++ = *src++ + 128;

      break;

    case 2:
      while (samcnt--) 
      {
        *dst++ = (uchar)(temp = *src++);
        *dst++ = (uchar)(temp >> 8);
      }

      break;

    case 3:
      while (samcnt--) 
      {
        *dst++ = (uchar)(temp = *src++);
        *dst++ = (uchar)(temp >> 8);
        *dst++ = (uchar)(temp >> 16);
      }

      break;

    case 4:
      while (samcnt--) 
      {
        *dst++ = (uchar)(temp = *src++);
        *dst++ = (uchar)(temp >> 8);
        *dst++ = (uchar)(temp >> 16);
        *dst++ = (uchar)(temp >> 24);
      }

      break;
  }

  return dst;
}

/* this is our function to decode a memory block from a file */
void wvpack_decode_data(file_info_struct* file_info, int samples_to_decode, WavpackContext **wpc)
{
  int bps = WavpackGetBytesPerSample(*wpc);
    
  /* nothing to decode */
  if (!samples_to_decode)
  {
    return;
  }
 
  /* decode now */
  ulong samples_unpacked = WavpackUnpackSamples(*wpc, temp_buffer, samples_to_decode);  

  if (samples_unpacked)
  {
    /* update some infos */
    file_info->current_sample += samples_unpacked;    
 
    format_samples (bps, (uchar *) temp_buffer, temp_buffer, samples_unpacked *= file_info->channels);

    rb->write(file_info->outfile, temp_buffer, samples_unpacked * bps);
  }   
}

/* callback function for wavpack 
Maybe we do this at a lower level, but the
first thing is to get all working */
long Read(void* buffer, long size)
{
  long oldpos = file_info.curpos;
  
  if ((file_info.curpos + size) < file_info.filesize) 
  {   
    memcpy(buffer, &filebuf[file_info.curpos], size);
    file_info.curpos += size;
  } 
  else 
  {
    memcpy(buffer, &filebuf[file_info.curpos], (file_info.filesize-1-file_info.curpos));
    file_info.curpos = file_info.filesize;
  }
    
  return (file_info.curpos - oldpos);
}

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* file)
{
  WavpackContext *wpc;
  char error[80];
  
  /* generic plugin initialisation */
  TEST_PLUGIN_API(api);
  rb = api;
  
  /* this function sets up the buffers and reads the file into RAM */
  if (local_init(file,"/wvtest.wav",&file_info,api)) 
  {
    return PLUGIN_ERROR;
  }
  
  /* setup wavpack */
  wpc = WavpackOpenFileInput(Read, error);

  /* was there an error? */
  if (!wpc) 
  {
    rb->splash(HZ*2, true, error);
    return PLUGIN_ERROR;
  }  

  /* grap/set some infos */
  file_info.channels = WavpackGetReducedChannels(wpc);
  file_info.total_samples = WavpackGetNumSamples(wpc);
  file_info.bitspersample = WavpackGetBitsPerSample(wpc);
  file_info.samplerate = WavpackGetSampleRate(wpc);
  file_info.current_sample = 0;
  
  /* deciding loop */  
  file_info.start_tick=*(rb->current_tick);
  rb->button_clear_queue();

  while (file_info.current_sample < file_info.total_samples) 
  {
    wvpack_decode_data(&file_info, BUFFER_SIZE / file_info.channels, &wpc);

    display_status(&file_info);

    if (rb->button_get(false)!=BUTTON_NONE) 
    {
      close_wav(&file_info);
      return PLUGIN_OK;
    }
  }  
          
  /* do some last checks */
  if ((WavpackGetNumSamples (wpc) != (ulong) -1) && (file_info.total_samples != WavpackGetNumSamples (wpc))) 
  {
    rb->splash(HZ*2, true, "incorrect number of samples!");
    return PLUGIN_ERROR;
  }  
  
  if (WavpackGetNumErrors (wpc)) {
    rb->splash(HZ*2, true, "crc errors detected!");
    return PLUGIN_ERROR;
  }
  
  close_wav(&file_info);    
  rb->splash(HZ*2, true, "FINISHED!");

  return PLUGIN_OK;
}

#endif /* CONFIG_HWCODEC == MASNONE */
