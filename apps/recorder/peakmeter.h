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
extern int  peak_meter_draw_get_btn(int x, int y, int width, int height);
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
extern int  peak_meter_db2sample(int db);
extern unsigned short peak_meter_scale_value(unsigned short val, int meterwidth);

/* valid values for trigger_status */
#define TRIG_OFF            0x00
#define TRIG_READY          0x01
#define TRIG_STEADY         0x02
#define TRIG_GO             0x03
#define TRIG_POSTREC        0x04
#define TRIG_RETRIG         0x05
#define TRIG_CONTINUE       0x06

extern void peak_meter_define_trigger(
    int start_threshold,
    long start_duration,
    long start_dropout,
    int stop_threshold,
    long stop_hold_time,
    long restart_gap
    );

extern void peak_meter_trigger(bool on);
extern int peak_meter_trigger_status(void);
extern void peak_meter_set_trigger_listener(void (*listener)(int status));

//#define TRIG_WIDTH 12
//#define TRIG_HEIGHT 14

#define TRIG_WIDTH 112
#define TRIG_HEIGHT 8
#define TRIGBAR_WIDTH (TRIG_WIDTH - (2 * (ICON_PLAY_STATE_WIDTH + 1)))

extern void peak_meter_draw_trig(int x, int y);

extern unsigned short peak_meter_range_min;
extern unsigned short peak_meter_range_max;

#endif /* __PEAKMETER_H__ */
