/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _MPEG_H_
#define _MPEG_H_

void mpeg_init(void);
void mpeg_play(char* trackname);
void mpeg_stop(void);
void mpeg_pause(void);
void mpeg_resume(void);
void mpeg_next(void);
void mpeg_prev(void);
void mpeg_volume(int percent);
void mpeg_bass(int percent);
void mpeg_treble(int percent);
struct mp3entry* mpeg_current_track(void);

#endif
