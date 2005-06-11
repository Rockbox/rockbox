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

#include <inttypes.h>  /* Needed by a52.h */
#include <codecs/liba52/config-a52.h>
#include <codecs/liba52/a52.h>

#include "playback.h"
#include "lib/codeclib.h"

#define BUFFER_SIZE 4096

struct plugin_api* rb;
struct codec_api* ci;

static float gain = 1;
static a52_state_t * state;
unsigned long samplesdone;
unsigned long frequency;

/* A post-processing buffer used outside liba52 */
static uint8_t buf[3840] IDATA_ATTR;

static inline int16_t convert (int32_t i)
{
    i >>= 15;
    return (i > 32767) ? 32767 : ((i < -32768) ? -32768 : i);
}

void output_audio(sample_t* samples,int flags) {
  int i;
  static int16_t int16_samples[256*2];

  flags &= A52_CHANNEL_MASK | A52_LFE;

  /* We may need to check the output format in flags - I'm not sure... */
  for (i = 0; i < 256; i++) {
    int16_samples[2*i] = convert (samples[i]);
    int16_samples[2*i+1] = convert (samples[i+256]);
  }

  rb->yield();
  while(!ci->audiobuffer_insert((unsigned char*)int16_samples,256*2*2))
    rb->yield();
}


void a52_decode_data (uint8_t * start, uint8_t * end)
{
  static uint8_t * bufptr = buf;
  static uint8_t * bufpos = buf + 7;

  /*
   * sample_rate and flags are static because this routine could
   * exit between the a52_syncinfo() and the ao_setup(), and we want
   * to have the same values when we get back !
   */

  static int sample_rate;
  static int flags;
  int bit_rate;
  int len;

  while (1) {
    len = end - start;
    if (!len)
      break;
    if (len > bufpos - bufptr)
      len = bufpos - bufptr;
    memcpy (bufptr, start, len);
    bufptr += len;
    start += len;
    if (bufptr == bufpos) {
      if (bufpos == buf + 7) {
        int length;

        length = a52_syncinfo (buf, &flags, &sample_rate, &bit_rate);
        if (!length) {
          DEBUGF("skip\n");
          for (bufptr = buf; bufptr < buf + 6; bufptr++)
            bufptr[0] = bufptr[1];
          continue;
        }
        bufpos = buf + length;
      } else {
        // The following two defaults are taken from audio_out_oss.c:
        level_t level;
        sample_t bias;
        int i;

        /* This is the configuration for the downmixing: */
        flags=A52_STEREO|A52_ADJUST_LEVEL|A52_LFE;
        level=(1 << 26);
        bias=0;

        level = (level_t) (level * gain);

        if (a52_frame (state, buf, &flags, &level, bias)) {
          goto error;
        }

//        file_info->frames_decoded++;

//        /* We assume this never changes */
//        file_info->samplerate=sample_rate;
          frequency=sample_rate;

        // An A52 frame consists of 6 blocks of 256 samples
        // So we decode and output them one block at a time
        for (i = 0; i < 6; i++) {
          if (a52_block (state)) {
            goto error;
          }

          output_audio(a52_samples (state),flags);
          samplesdone+=256;
        }
        ci->set_elapsed(samplesdone/(frequency/1000));
        bufptr = buf;
        bufpos = buf + 7;
        continue;

        error:

        //logf("Error decoding A52 stream\n");
        bufptr = buf;
        bufpos = buf + 7;
      }
    }
  }
}

#ifndef SIMULATOR
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parm)
{
  size_t n;
  unsigned char* filebuf;

  /* Generic plugin initialisation */
  TEST_PLUGIN_API(api);

  rb = api;
  ci = (struct codec_api*)parm;

#ifndef SIMULATOR
  rb->memcpy(iramstart, iramcopy, iramend-iramstart);
#endif

  ci->configure(CODEC_SET_FILEBUF_LIMIT, (int *)(1024*1024*2));
  ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (int *)(1024*16));

  next_track:

  if (codec_init(api, ci)) {
  return PLUGIN_ERROR;
  }

  /* Intialise the A52 decoder and check for success */
  state = a52_init (0); // Parameter is "accel"

  /* The main decoding loop */

  samplesdone=0;
  while (1) {
    if (ci->stop_codec || ci->reload_codec) {
      break;
    }

    filebuf=ci->request_buffer(&n,BUFFER_SIZE);

    if (n==0) {      /* End of Stream */
      break;
    }
  
    a52_decode_data(filebuf,filebuf+n);

    ci->advance_buffer(n);
  }

  if (ci->request_next_track())
   goto next_track;

//NOT NEEDED??:  a52_free (state);

  return PLUGIN_OK;
}
