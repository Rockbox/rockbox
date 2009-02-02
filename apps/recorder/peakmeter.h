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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __PEAKMETER_H__
#define __PEAKMETER_H__

#define PEAK_METER_FPS 20

/*#define PM_DEBUG */
#ifdef PM_DEBUG 
extern bool peak_meter_histogram(void);
#endif

extern int pm_get_clipcount(void);
extern void pm_reset_clipcount(void);
extern void pm_activate_clipcount(bool active);

extern bool peak_meter_enabled;

extern void peak_meter_playback(bool playback);
extern int  peak_meter_draw_get_btn(int action_context, int x[], int y[],
                                    int height[], int nb_screens,
                                    struct viewport vps[]);
extern void peak_meter_set_clip_hold(int time);
extern void peak_meter_peek(void);
extern void peak_meter_init_range( bool dbfs, int range_min, int range_max);
extern void peak_meter_init_times(int release, int hold, int clip_hold);
#ifdef HAVE_AGC
extern void peak_meter_get_peakhold(int *peak_left, int *peak_right);
#endif
extern int  peak_meter_get_min(void);
extern int  peak_meter_get_max(void);
extern void peak_meter_set_use_dbfs(bool use);
extern bool peak_meter_get_use_dbfs(void);
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

#define TRIG_HEIGHT 8

extern void peak_meter_draw_trig(int x[], int y[], int trig_width[], 
                                      int nb_screens);

#define DB_SCALE_SRC_VALUES_SIZE 12
struct meter_scales{
    /* buffered peak values */
    int pm_peak_left;
    int pm_peak_right;
    /* if db_scale_valid is false the content of
       db_scale_lcd_coord needs recalculation */
    bool db_scale_valid;
    /* contains the lcd x coordinates of the magical
       scale values in db_scale_src_values */
    int db_scale_lcd_coord[DB_SCALE_SRC_VALUES_SIZE];
    int last_left;
    int last_right;
    /* peak hold timeouts */
    long pm_peak_timeout_l;
    long pm_peak_timeout_r;
};

extern void peak_meter_screen(struct screen *display, int x, int y, int height);
#endif /* __PEAKMETER_H__ */
