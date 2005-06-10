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

#ifndef SIMULATOR
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif

/* This is probably a waste of IRAM, but why not? */
static unsigned char wavbuf[16384] IDATA_ATTR;

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parm)
{
  struct plugin_api* rb = (struct plugin_api*)api;
  struct codec_api* ci = (struct codec_api*)parm;
  unsigned long samplerate,numbytes,totalsamples,samplesdone,nsamples;
  int channels,bytespersample,bitspersample;
  unsigned int i,j,n;
  int endofstream;

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

  n=(unsigned)(ci->read_filebuf(wavbuf,44));
  if (n!=44) {
    return PLUGIN_ERROR;
  }
  if ((memcmp(wavbuf,"RIFF",4)!=0) || (memcmp(&wavbuf[8],"WAVEfmt",7)!=0)) {
    return PLUGIN_ERROR;
  }

  samplerate=wavbuf[24]|(wavbuf[25]<<8)|(wavbuf[26]<<16)|(wavbuf[27]<<24);
  bitspersample=wavbuf[34];
  channels=wavbuf[22];
  bytespersample=((bitspersample/8)*channels);
  numbytes=(wavbuf[40]|(wavbuf[41]<<8)|(wavbuf[42]<<16)|(wavbuf[43]<<24));
  totalsamples=numbytes/bytespersample;

  if ((bitspersample!=16) || (channels != 2)) {
    return PLUGIN_ERROR;
  }
    
  /* The main decoder loop */

  samplesdone=0;
  ci->set_elapsed(0);
  endofstream=0;
  while (!endofstream) {
    rb->yield();
    if (ci->stop_codec || ci->reload_codec) {
      break;
    }

    n=(unsigned)(ci->read_filebuf(wavbuf,sizeof(wavbuf)));

    rb->yield();

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
    for (i=0;i<n;i+=2) {
        j=wavbuf[i];
        wavbuf[i]=wavbuf[i+1];
        wavbuf[i+1]=j;
    }

    samplesdone+=nsamples;
    ci->set_elapsed(samplesdone/(ci->id3->frequency/1000));
   
    rb->yield();
    while (!ci->audiobuffer_insert(wavbuf, n))
      rb->yield();
  }

  if (ci->request_next_track())
    goto next_track;

  return PLUGIN_OK;
}
