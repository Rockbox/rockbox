/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef RADIO_H
#define RADIO_H
#define FMPRESET_PATH ROCKBOX_DIR "/fmpresets"

#define FMRADIO_OFF     0
#define FMRADIO_PLAYING 1
#define FMRADIO_PLAYING_OUT  2
#define FMRADIO_PAUSED  3
#define FMRADIO_PAUSED_OUT  4

#ifdef CONFIG_TUNER
void radio_load_presets(char *filename);
void radio_init(void);
bool radio_screen(void);
void radio_stop(void);
bool radio_hardware_present(void);
int  get_radio_status(void);

struct fmstation
{
    int frequency; /* In Hz */
    char name[28];
};

#endif

#endif
