/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2011 by Bertrik Sikken
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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <strlcpy.h>
#include <system.h>
#include <kernel.h>
#include "rds.h"
#include "time.h"

// timeout before segment obsolescence
#define PS_SEGMENT_TIMEOUT  (HZ / 2)
#define RT_SEGMENT_TIMEOUT  (10 * HZ)

/* programme identification */
static uint16_t pi_code;
static uint16_t pi_last;
/* program service name */
static char ps_data[9];
static char ps_copy[9];
static long ps_segment_timeout[4];
static int ps_segment;// bitmap of received segments
/* radio text */
static char rt_data[65];
static char rt_copy[65];
static long rt_segment_timeout[16];
static int rt_segment;// bitmap of received segments
static int rt_abflag;
/* date/time */
static time_t ct_data;

#ifdef RDS_ISR_PROCESSING
/* Functions are called in ISR context */
#define rds_disable_irq_save() disable_irq_save()
#define rds_restore_irq(old) restore_irq(old)
/* Need triple buffer so string isn't clobbered while caller is using it */
static inline char * get_ps(void)
{
    static char ps_out[9];
    int oldlevel = rds_disable_irq_save();
    strcpy(ps_out, ps_copy);
    rds_restore_irq(oldlevel);
    return ps_out;
}
static inline char * get_rt(void)
{
    static char rt_out[65];
    int oldlevel = rds_disable_irq_save();
    strcpy(rt_out, rt_copy);
    rds_restore_irq(oldlevel);
    return rt_out;
}
#else /* ndef RDS_ISR_PROCESSING */
#define rds_disable_irq_save() 0
#define rds_restore_irq(old) ((void)(old))
static inline char * get_ps(void) { return ps_copy; }
static inline char * get_rt(void) { return rt_copy; }
#endif /* RDS_ISR_PROCESSING */

/* resets the rds parser */
void rds_reset(void)
{
    int oldlevel = rds_disable_irq_save();

    pi_code = 0;
    pi_last = 0;
    ps_copy[0] = '\0';
    ps_segment = 0;
    rt_copy[0] = '\0';
    rt_segment = 0;
    ct_data = 0;

    rds_restore_irq(oldlevel);
}

/* initialises the rds parser */
void rds_init(void)
{
    rds_reset();
}

/* handles a group 0 packet, returns true if a new message was received */
static bool handle_group0(uint16_t data[4])
{
    int segment, pos;

    /* remove obsolete segments */
    for(int i = 0; i < 4; i++)
        if(TIME_AFTER(current_tick, ps_segment_timeout[i]))
            ps_segment &= ~(1 << i);

    segment = data[1] & 3;

    /* store data */
    pos = segment * 2;
    ps_data[pos++] = (data[3] >> 8) & 0xFF;
    ps_data[pos++] = (data[3] >> 0) & 0xFF;
    ps_segment |= 1 << segment;
    ps_segment_timeout[segment] = current_tick + PS_SEGMENT_TIMEOUT;
    if (ps_segment == 0xf) {
        ps_data[8] = '\0';
        if (strcmp(ps_copy, ps_data) != 0) {
            /* we got an updated message */
            strcpy(ps_copy, ps_data);
            return true;
        }
    }
    return false;
}

/* handles a radio text characters, returns true if end-of-line found */
static bool handle_rt(int pos, char c)
{
    switch (c) {
    case 0x0A:
        /* line break hint */
        rt_data[pos] = ' ';
        return false;
    case 0x0D:
        /* end of line */
        rt_data[pos] = '\0';
        return true;
    default:
        rt_data[pos] = c;
        return false;
    }
}

/* handles a group 2 packet, returns true if a new message was received */
static bool handle_group2(uint16_t data[4])
{
    int abflag, segment, version, pos;
    bool done;

    /* remove obsolete segments */
    for(int i = 0; i < 16; i++)
        if(TIME_AFTER(current_tick, rt_segment_timeout[i]))
            rt_segment &= ~(1 << i);
    
    /* reset parsing if the message type changed */
    abflag = (data[1] >> 4) & 1;
    segment = data[1] & 0xF;
    if (abflag != rt_abflag) {
        rt_abflag = abflag;
        rt_segment = 0;
    }

    rt_segment |= 1 << segment;
    rt_segment_timeout[segment] = current_tick + RT_SEGMENT_TIMEOUT;

    /* store data */
    version = (data[1] >> 11) & 1;
    if (version == 0) {
        pos = segment * 4;
        done = done || handle_rt(pos++, (data[2] >> 8) & 0xFF);
        done = done || handle_rt(pos++, (data[2] >> 0) & 0xFF);
        done = done || handle_rt(pos++, (data[3] >> 8) & 0xFF);
        done = done || handle_rt(pos++, (data[3] >> 0) & 0xFF);
    } else {
        pos = segment * 2;
        done = done || handle_rt(pos++, (data[3] >> 8) & 0xFF);
        done = done || handle_rt(pos++, (data[3] >> 0) & 0xFF);
    }
    /* there are two cases for completion:
     * - we got all 16 segments
     * - we found a end of line AND we got all segments before it */
    if (rt_segment == 0xffff || (done && rt_segment == (1 << segment) - 1)) {
        rt_data[pos] = '\0';
        if (strcmp(rt_copy, rt_data) != 0) {
            /* we got an updated message */
            strcpy(rt_copy, rt_data);
            return true;
        }
    }

    return false;
}

/* handles a group 4a packet (clock-time) */
static bool handle_group4a(uint16_t data[4])
{
    int daycode = ((data[1] << 15) & 0x18000) |
                  ((data[2] >> 1) & 0x07FFF);
    int hour = ((data[2] << 4) & 0x10) |
               ((data[3] >> 12) & 0x0F);
    int minute = ((data[3] >> 6) & 0x3F);
    int offset_sig = (data[3] >> 5) & 1;
    int offset_abs = data[3] & 0x1F;

    if (daycode < 55927) {
        /* invalid date, before 2012-01-01 */
        return false;
    }
    if ((hour >= 24) || (minute >= 60)) {
        /* invalid time */
        return false;
    }
    if (offset_abs > 24) {
        /* invalid local time offset */
        return false;
    }

    /* convert modified julian day + time to UTC */
    time_t seconds = (daycode - 40587) * 86400;
    seconds += hour * 3600;
    seconds += minute * 60;
    seconds += ((offset_sig == 0) ? offset_abs : -offset_abs) * 1800;
    ct_data = seconds;

    return true;
}

/* processes one rds packet, returns true if a new message was received */
bool rds_process(uint16_t data[4])
{
    int group;

    /* process programme identification (PI) code */
    uint16_t pi = data[0];
    if (pi == pi_last) {
        pi_code = pi;
    }
    pi_last = pi;
    
    /* handle rds data based on group */
    group = (data[1] >> 11) & 0x1F;
    switch (group) {
    
    case 0: /* group 0A: basic info */
    case 1: /* group 0B: basic info */
        return handle_group0(data);
    
    case 4: /* group 2A: radio text */
    case 5: /* group 2B: radio text */
        return handle_group2(data);

    case 8: /* group 4A: clock-time */
        return handle_group4a(data);
    
    default:
        break;
    }

    return false;
}

/* TODO: The caller really should provide the buffer in order to regulate
   access */

/* returns the programme identification code */
uint16_t rds_get_pi(void)
{
    return pi_code;
}

/* returns the most recent valid programme service name */
char* rds_get_ps(void)
{
    return get_ps();
}

/* returns the most recent valid RadioText message */
char* rds_get_rt(void)
{
    return get_rt();
}

/* returns the most recent valid clock-time value (or 0 if invalid) */
time_t rds_get_ct(void)
{
    return ct_data;
}

