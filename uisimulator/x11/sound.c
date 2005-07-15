/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2005 by Daniel Stenberg <daniel@haxx.se>
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
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "sound.h"

int sim_sound_init(void)
{
  int fd;
  int pcmbits;
  int rc;
  int channels;
  int rate;

  fd = open("/dev/dsp", O_WRONLY);
  if(-1 == fd)
    return 1;

  pcmbits = 16;
  rc = ioctl(fd, SOUND_PCM_WRITE_BITS, &pcmbits);
  rc = ioctl(fd, SOUND_PCM_READ_BITS, &pcmbits);

  channels = 2; /* Number of channels, 1=mono */
  rc = ioctl(fd, SOUND_PCM_WRITE_CHANNELS, &channels);
  rc = ioctl(fd, SOUND_PCM_READ_CHANNELS, &channels);

  rate = 44100; /* Yeah. sampling rate */
  rc = ioctl(fd, SOUND_PCM_WRITE_RATE, &rate);
  rc = ioctl(fd, SOUND_PCM_READ_RATE, &rate);

  return fd;
}

void sim_sound_play(int soundfd, char *buffer, long len)
{
    write(soundfd, buffer, len);
}

void sound_playback_thread(void)
{
    int soundfd = sim_sound_init();
    unsigned char *buf;
    long size;

    while(-1 == soundfd)
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
            sim_sound_play(soundfd, buf, size);
            usleep(10000);
        } while(size);

    } while(1);
    
}

#endif /* ROCKBOX_HAS_SIMSOUND */
