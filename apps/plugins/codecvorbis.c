/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "kernel.h"
#include "plugin.h"

#include <codecs/Tremor/ivorbisfile.h>

#include "playback.h"
#include "lib/codeclib.h"

static struct plugin_api* rb;

/* Some standard functions and variables needed by Tremor */


int errno;

size_t strlen(const char *s) {
  return(rb->strlen(s));
}

char *strcpy(char *dest, const char *src) {
  return(rb->strcpy(dest,src));
}

char *strcat(char *dest, const char *src) {
  return(rb->strcat(dest,src));
}

size_t read_handler(void *ptr, size_t size, size_t nmemb, void *datasource) {
    struct codec_api *p = (struct codec_api *) datasource;

    return p->read_filebuf(ptr, nmemb*size);
}

int seek_handler(void *datasource, ogg_int64_t offset, int whence) {
  /* We are not seekable at the moment */
  (void)datasource;
  (void)offset;
  (void)whence;
  return -1;
}

int close_handler(void *datasource) {
  (void)datasource;
  return 0;
}

long tell_handler(void *datasource) {
    struct codec_api *p = (struct codec_api *) datasource;
  return p->curpos;
}

#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif


/* reserve the PCM buffer in the IRAM area */
static char pcmbuf[4096] IDATA_ATTR;

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parm)
{
  struct codec_api *ci = (struct codec_api *)parm;
  ov_callbacks callbacks;
  OggVorbis_File vf;
  vorbis_info* vi;

  int error;
  long n;
  int current_section;
  int eof;
#if BYTE_ORDER == BIG_ENDIAN
  int i;
  char x;
#endif

  TEST_PLUGIN_API(api);

  /* if you are using a global api pointer, don't forget to copy it!
     otherwise you will get lovely "I04: IllInstr" errors... :-) */
  rb = api;

  #ifdef USE_IRAM
  rb->memcpy(iramstart, iramcopy, iramend-iramstart);
  #endif
    
  ci->configure(CODEC_SET_FILEBUF_LIMIT, (int *)(1024*1024*2));
  ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (int *)(1024*32));
    
  /* We need to flush reserver memory every track load. */
  next_track:
  if (codec_init(api, ci)) {
    return PLUGIN_ERROR;
  }


  /* Create a decoder instance */

  callbacks.read_func=read_handler;
  callbacks.seek_func=seek_handler;
  callbacks.tell_func=tell_handler;
  callbacks.close_func=close_handler;

  error=ov_open_callbacks(ci,&vf,NULL,0,callbacks);
    
  vi=ov_info(&vf,-1);

  if (vi==NULL) {
    // rb->splash(HZ*2, true, "Vorbis Error");
    return PLUGIN_ERROR;
  }
  
  eof=0;
  while (!eof) {
    /* Read host-endian signed 16 bit PCM samples */
    n=ov_read(&vf,pcmbuf,sizeof(pcmbuf),&current_section);

    if (n==0) {
      eof=1;
    } else if (n < 0) {
      DEBUGF("Error decoding frame\n");
    } else {
      rb->yield();
      if (ci->stop_codec || ci->reload_codec)
        break ;
        
      rb->yield();
      while (!ci->audiobuffer_insert(pcmbuf, n))
        rb->yield();

      ci->set_elapsed(ov_time_tell(&vf));

#if BYTE_ORDER == BIG_ENDIAN
      for (i=0;i<n;i+=2) { 
        x=pcmbuf[i]; pcmbuf[i]=pcmbuf[i+1]; pcmbuf[i+1]=x;
      }
#endif
    }
  }
  
  if (ci->request_next_track())
    goto next_track;
    
  return PLUGIN_OK;
}

