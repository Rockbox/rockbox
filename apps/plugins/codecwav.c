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
#include "playback.h"
#include "lib/codeclib.h"

#define BYTESWAP(x) (((x>>8) & 0xff) | ((x<<8) & 0xff00))

/* Number of bytes to process in one iteration */
#define WAV_CHUNK_SIZE 16384

#ifndef SIMULATOR
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parm)
{
  struct plugin_api* rb = (struct plugin_api*)api;
  struct codec_api* ci = (struct codec_api*)parm;
  unsigned long samplerate,numbytes,totalsamples,samplesdone,nsamples;
  int channels,bytespersample,bitspersample;
  unsigned int i;
  size_t n;
  int endofstream;
  unsigned char* header;
  unsigned short* wavbuf;

  /* Generic plugin initialisation */
  TEST_PLUGIN_API(api);

  /* if you are using a global api pointer, don't forget to copy it!
     otherwise you will get lovely "I04: IllInstr" errors... :-) */
  rb = api;

#ifndef SIMULATOR
  rb->memcpy(iramstart, iramcopy, iramend-iramstart);
#endif

  ci->configure(CODEC_SET_FILEBUF_LIMIT, (int *)(1024*1024*10));
  ci->configure(CODEC_SET_FILEBUF_WATERMARK, (int *)(1024*512));

  next_track:

  if (codec_init(api, ci)) {
    return PLUGIN_ERROR;
  }

  /* FIX: Correctly parse WAV header - we assume canonical 44-byte header */

  header=ci->request_buffer(&n,44);
  if (n!=44) {
    return PLUGIN_ERROR;
  }
  if ((memcmp(header,"RIFF",4)!=0) || (memcmp(&header[8],"WAVEfmt",7)!=0)) {
    return PLUGIN_ERROR;
  }

  samplerate=header[24]|(header[25]<<8)|(header[26]<<16)|(header[27]<<24);
  bitspersample=header[34];
  channels=header[22];
  bytespersample=((bitspersample/8)*channels);
  numbytes=(header[40]|(header[41]<<8)|(header[42]<<16)|(header[43]<<24));
  totalsamples=numbytes/bytespersample;

  if ((bitspersample!=16) || (channels != 2)) {
    return PLUGIN_ERROR;
  }

  ci->advance_buffer(44);

  /* The main decoder loop */

  samplesdone=0;
  ci->set_elapsed(0);
  endofstream=0;
  while (!endofstream) {
    if (ci->stop_codec || ci->reload_codec) {
      break;
    }

    wavbuf=ci->request_buffer(&n,WAV_CHUNK_SIZE);

    if (n==0) break; /* End of stream */

    nsamples=(n/bytespersample);

    /* WAV files can contain extra data at the end - so we can't just
       process until the end of the file */

    if (samplesdone+nsamples > totalsamples) {
      nsamples=(totalsamples-samplesdone);
      n=nsamples*bytespersample;
      endofstream=1;
    }

    /* Byte-swap data */
    for (i=0;i<n/2;i++) {
      wavbuf[i]=BYTESWAP(wavbuf[i]);
    }

    samplesdone+=nsamples;
    ci->set_elapsed(samplesdone/(ci->id3->frequency/1000));
   
    while (!ci->audiobuffer_insert((unsigned char*)wavbuf, n))
      rb->yield();

    ci->advance_buffer(n);
  }

  if (ci->request_next_track())
    goto next_track;

  return PLUGIN_OK;
}
