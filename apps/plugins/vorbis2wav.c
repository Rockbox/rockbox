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

#if (CONFIG_HWCODEC == MASNONE)
/* software codec platforms */

#include <codecs/Tremor/ivorbisfile.h>

#include "lib/xxx2wav.h" /* Helper functions common to test decoders */

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
    size_t len;
    file_info_struct *p = (file_info_struct *) datasource;

    if (p->curpos >= p->filesize) {
      return 0; /* EOF */
    }

    len=nmemb*size;
    if ((long)(p->curpos+len) > (long)p->filesize) { len=p->filesize-p->curpos; }

    rb->memcpy(ptr,&filebuf[p->curpos],len);
    p->curpos+=len;

    return(len);  
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
  file_info_struct *p = (file_info_struct *) datasource;
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
enum plugin_status plugin_start(struct plugin_api* api, void* file)
{
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

  file_info_struct file_info;

  TEST_PLUGIN_API(api);

  /* if you are using a global api pointer, don't forget to copy it!
     otherwise you will get lovely "I04: IllInstr" errors... :-) */
  rb = api;

  #ifdef USE_IRAM
  rb->memcpy(iramstart, iramcopy, iramend-iramstart);
  #endif
    
  /* This function sets up the buffers and reads the file into RAM */

  if (local_init(file,"/vorbistest.wav",&file_info,api)) {
    return PLUGIN_ERROR;
  }


  /* Create a decoder instance */

  callbacks.read_func=read_handler;
  callbacks.seek_func=seek_handler;
  callbacks.tell_func=tell_handler;
  callbacks.close_func=close_handler;

  file_info.frames_decoded=0;
  file_info.start_tick=*(rb->current_tick);
  rb->button_clear_queue();

  error=ov_open_callbacks(&file_info,&vf,NULL,0,callbacks);

  vi=ov_info(&vf,-1);

  if (vi==NULL) {
    rb->splash(HZ*2, true, "Error");
  }
  file_info.samplerate=vi->rate;
  
  eof=0;
  while (!eof) {
    /* Read host-endian signed 16 bit PCM samples */
    n=ov_read(&vf,pcmbuf,sizeof(pcmbuf),&current_section);

    if (n==0) {
      eof=1;
    } else if (n < 0) {
      DEBUGF("Error decoding frame\n");
    } else {
      file_info.frames_decoded++;
#if BYTE_ORDER == BIG_ENDIAN
      for (i=0;i<n;i+=2) { 
        x=pcmbuf[i]; pcmbuf[i]=pcmbuf[i+1]; pcmbuf[i+1]=x;
      }
#endif
      rb->write(file_info.outfile,pcmbuf,n);
      file_info.current_sample+=(n/4);
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
