/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2005 by Nick Lanham
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "autoconf.h"

#ifdef ROCKBOX_HAS_SIMSOUND /* play sound in sim enabled */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <SDL.h>

#include "sound.h"

//static Uint8 *audio_chunk;
static int audio_len;
static char *audio_pos;
SDL_sem* sem;

/* The audio function callback takes the following parameters:
   stream:  A pointer to the audio buffer to be filled
   len:     The length (in bytes) of the audio buffer
*/
void mixaudio(void *udata, Uint8 *stream, int len)
{
  (void)udata;

  /* Only play if we have data left */
  if ( audio_len == 0 )
    return;

  len = (len > audio_len) ? audio_len : len;
  memcpy(stream, audio_pos, len);
  audio_pos += len;
  audio_len -= len;

  if(audio_len == 0) {
    if(SDL_SemPost(sem)) 
      fprintf(stderr,"Couldn't post: %s",SDL_GetError());
      
  }
}



int sim_sound_init(void)
{
  SDL_AudioSpec fmt;

  /* Set 16-bit stereo audio at 44Khz */
  fmt.freq = 44100;
  fmt.format = AUDIO_S16SYS;
  fmt.channels = 2;
  fmt.samples = 512;        /* A good value for games */
  fmt.callback = mixaudio;
  fmt.userdata = NULL;

  sem = SDL_CreateSemaphore(0);

  /* Open the audio device and start playing sound! */
  if(SDL_OpenAudio(&fmt, NULL) < 0) {
    fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
    return -1;
  }
  SDL_PauseAudio(0);
  return 0;

  //...
    
  //SDL_CloseAudio();
}

void sound_playback_thread(void)
{
  int sndret = sim_sound_init();
  unsigned char *buf;
  long size;

  while(sndret)
    sleep(100000); /* wait forever, can't play sound! */
    
  do {
    while(!sound_get_pcm)
      /* TODO: fix a fine thread-synch mechanism here */
      usleep(10000);
    do {
      sound_get_pcm(&buf, &size);
      if(!size) {
	sound_get_pcm = NULL;
	break;
      }
      audio_pos = buf; // TODO: is this safe?
      audio_len = size;
      //printf("len: %i\n",audio_len);
      if(SDL_SemWait(sem)) 
	fprintf(stderr,"Couldn't wait: %s",SDL_GetError());
    } while(size);
  } while(1);
}

#endif /* ROCKBOX_HAS_SIMSOUND */
