/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Philipp Pertermann
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __PEAKMETER_H__
#define __PEAKMETER_H__

/*#define PM_DEBUG */
#ifdef PM_DEBUG 
extern bool peak_meter_histogram(void);
#endif


extern bool peak_meter_enabled;
extern int  peak_meter_fps;

extern void peak_meter_playback(bool playback);
extern void peak_meter_draw(int x, int y, int width, int height);
extern void peak_meter_set_clip_hold(int time);
extern void peak_meter_peek(void);
extern void peak_meter_init_range( bool dbfs, int range_min, int range_max);
extern void peak_meter_init_times(int release, int hold, int clip_hold);

extern void peak_meter_set_min(int newmin);
extern int  peak_meter_get_min(void);
extern void peak_meter_set_max(int newmax);
extern int  peak_meter_get_max(void);
extern void peak_meter_set_use_dbfs(int use);
extern int  peak_meter_get_use_dbfs(void);
extern int  calc_db (int isample);
extern unsigned short peak_meter_scale_value(unsigned short val, int meterwidth);

extern unsigned short peak_meter_range_min;
extern unsigned short peak_meter_range_max;

#endif /* __PEAKMETER_H__ */
