/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by wavey@wavey.org
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __DISK_H__
#define __DISK_H__

int persist_volume_setting( void );
int persist_balance_setting( void );
int persist_bass_setting( void );
int persist_treble_setting( void );
int persist_loudness_setting( void );
int persist_bass_boost_setting( void );
int persist_contrast_setting( void );
int persist_poweroff_setting( void );
int persist_backlight_setting( void );
int persist_resume_setting( void );

int persist_playlist_filename( void );
int persist_playlist_indices( void );
int persist_playlist_index( void );
int persist_resume_track_time( void );

#endif /* _DISK_H__ */
