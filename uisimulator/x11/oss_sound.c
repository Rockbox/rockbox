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

#include <linux/soundcard.h>
#include "oss_sound.h"

/* We want to use the "real" open in some cases */
#undef open

int init_sound(sound_t* sound) {
  *sound=open("/dev/dsp", O_WRONLY);

  if (sound < 0) {
    fprintf(stderr,"Can not open /dev/dsp - Aborting - sound=%d\n",sound);
    exit(-1);
  }
}

int config_sound(sound_t* sound, int sound_freq, int channels) {
  int format=AFMT_U16_LE;
  int setting=0x000C000D;  // 12 fragments size 8kb ? WHAT IS THIS?

  if (ioctl(*sound,SNDCTL_DSP_SETFRAGMENT,&setting)==-1) {
    perror("SNDCTL_DSP_SETFRAGMENT");
  }

  if (ioctl(*sound,SNDCTL_DSP_CHANNELS,&channels)==-1) {
    perror("SNDCTL_DSP_STEREO");
  }
  if (channels==0) { fprintf(stderr,"Warning, only mono supported\n"); }

  if (ioctl(*sound,SNDCTL_DSP_SETFMT,&format)==-1) {
    perror("SNDCTL_DSP_SETFMT");
  }

  if (ioctl(*sound,SNDCTL_DSP_SPEED,&sound_freq)==-1) {
    perror("SNDCTL_DSP_SPEED");
  }
}

int output_sound(sound_t* sound,const void* buf, int count) {
  return(write(*sound,buf,count));
}

void close_sound(sound_t* sound) {
  if (*sound) close(*sound);
}
