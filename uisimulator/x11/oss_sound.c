/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2002 Dave Chapman
 *
 * oss_sound - a sound driver for Linux (and others?) OSS audio
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <fcntl.h>

#include <sys/soundcard.h>
#include "../common/sound.h"

/* We want to use the "real" open in this file */
#undef open

int init_sound(sound_t* sound) {
  sound->fd=open("/dev/dsp", O_WRONLY);
  sound->freq=-1;
  sound->channels=-1;

  if (sound->fd <= 0) {
    fprintf(stderr,"Can not open /dev/dsp - simulating sound output\n");
    sound->fd=0;
  }
}

int config_sound(sound_t* sound, int sound_freq, int channels) {
  int format=AFMT_S16_NE;
  int setting=0x000C000D;  // 12 fragments size 8kb ? WHAT IS THIS?

  sound->freq=sound_freq;
  sound->channels=channels;

  if (sound->fd) {
    if (ioctl(sound->fd,SNDCTL_DSP_SETFRAGMENT,&setting)==-1) {
      perror("SNDCTL_DSP_SETFRAGMENT");
    }

    if (ioctl(sound->fd,SNDCTL_DSP_CHANNELS,&channels)==-1) {
      perror("SNDCTL_DSP_STEREO");
    }
    if (channels==0) { fprintf(stderr,"Warning, only mono supported\n"); }

    if (ioctl(sound->fd,SNDCTL_DSP_SETFMT,&format)==-1) {
      perror("SNDCTL_DSP_SETFMT");
    }

    if (ioctl(sound->fd,SNDCTL_DSP_SPEED,&sound_freq)==-1) {
      perror("SNDCTL_DSP_SPEED");
    }
  }
}

int output_sound(sound_t* sound,const void* buf, int count) {
  unsigned long long t;

  if (sound->fd) {
    return(write(sound->fd,buf,count));
  } else {
    t=(unsigned int)(((unsigned int)(1000000/sound->channels)*count)/sound->freq);
//    fprintf(stderr,"writing %d bytes at %d frequency - sleeping for %u microseconds\n",count,sound->freq,t);
    usleep(t);
    return(count);
  }
}

void close_sound(sound_t* sound) {
  if (sound->fd) close(sound->fd);
  sound->fd=-1;
}
