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

#include <inttypes.h>  /* Needed by a52.h */

#include <codecs/liba52/config-a52.h>
#include <codecs/liba52/a52.h>

#include "lib/xxx2wav.h" /* Helper functions common to test decoders */

static struct plugin_api* rb;

/* FIX: We can remove this warning when the build system has a
   mechanism for auto-detecting the endianness of the target CPU -
   WORDS_BIGENDIAN is defined in liba52/config.h and is also used
   internally by liba52.
 */

#ifdef WORDS_BIGENDIAN
  #warning ************************************* BIG ENDIAN
  #define LE_S16(x) ( (uint16_t) ( ((uint16_t)(x) >> 8) | ((uint16_t)(x) << 8) ) )
#else
  #define LE_S16(x) (x)
#endif


static float gain = 1;
static a52_state_t * state;

static inline int16_t convert (int32_t i)
{
    i >>= 15;
    return (i > 32767) ? 32767 : ((i < -32768) ? -32768 : i);
}

void ao_play(file_info_struct* file_info,sample_t* samples,int flags) {
  int i;
  static int16_t int16_samples[256*2];

  flags &= A52_CHANNEL_MASK | A52_LFE;

  if (flags==A52_STEREO) {
    for (i = 0; i < 256; i++) {
        int16_samples[2*i] = LE_S16(convert (samples[i]));
        int16_samples[2*i+1] = LE_S16(convert (samples[i+256]));
    }
  } else {
    DEBUGF("ERROR: unsupported format: %d\n",flags);
  }

  /* FIX: Buffer the disk write to write larger amounts at one */
  i=rb->write(file_info->outfile,int16_samples,256*2*2);
}


void a52_decode_data (file_info_struct* file_info, uint8_t * start, uint8_t * end)
{
    static uint8_t buf[3840];
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

		if (a52_frame (state, buf, &flags, &level, bias))
		    goto error;
                file_info->frames_decoded++;

                /* We assume this never changes */
                file_info->samplerate=sample_rate;

                // An A52 frame consists of 6 blocks of 256 samples
                // So we decode and output them one block at a time
		for (i = 0; i < 6; i++) {
		    if (a52_block (state)) {
			goto error;
                    }
		    ao_play (file_info, a52_samples (state),flags);
                    file_info->current_sample+=256;
		}
		bufptr = buf;
		bufpos = buf + 7;
		continue;
	    error:
		DEBUGF("error\n");
		bufptr = buf;
		bufpos = buf + 7;
	    }
	}
    }
}


#define BUFFER_SIZE 4096

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* file)
{
  file_info_struct file_info;

  /* Generic plugin initialisation */

  TEST_PLUGIN_API(api);
  rb = api;


  /* This function sets up the buffers and reads the file into RAM */

  if (local_init(file,"/ac3test.wav",&file_info,api)) {
    return PLUGIN_ERROR;
  }

  /* Intialise the A52 decoder and check for success */
  state = a52_init (0); // Parameter is "accel"

  if (state == NULL) {
    rb->splash(HZ*2, true, "a52_init failed");
    return PLUGIN_ERROR;
  }

  /* The main decoding loop */

  file_info.start_tick=*(rb->current_tick);
  rb->button_clear_queue();

  while (file_info.curpos < file_info.filesize) {

    if ((file_info.curpos+BUFFER_SIZE) < file_info.filesize) {   
      a52_decode_data(&file_info,&filebuf[file_info.curpos],&filebuf[file_info.curpos+BUFFER_SIZE]);
      file_info.curpos+=BUFFER_SIZE;
    } else {
      a52_decode_data(&file_info,&filebuf[file_info.curpos],&filebuf[file_info.filesize-1]);
      file_info.curpos=file_info.filesize;
    }

    display_status(&file_info);

    if (rb->button_get(false)!=BUTTON_NONE) {
      close_wav(&file_info);
      return PLUGIN_OK;
    }
  }
  close_wav(&file_info);

  /* Cleanly close and exit */

//NOT NEEDED:  a52_free (state);

  rb->splash(HZ*2, true, "FINISHED!");
  return PLUGIN_OK;
}
#endif /* CONFIG_HWCODEC == MASNONE */
