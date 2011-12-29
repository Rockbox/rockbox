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
#include "rds.h"

/* programme identification */
static uint16_t pi;
/* program service name */
static char ps_data[9];
static char ps_copy[9];
static int ps_segment;
/* radio text */
static char rt_data[65];
static char rt_copy[65];
static int rt_segment;
static int rt_abflag;

#ifdef SI4700_RDS_ASYNC
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
#else /* ndef SI4700_RDS_ASYNC */
#define rds_disable_irq_save() 0
#define rds_restore_irq(old) ((void)(old))
static inline char * get_ps(void) { return ps_copy; }
static inline char * get_rt(void) { return rt_copy; }
#endif /* SI4700_RDS_ASYNC */

/* resets the rds parser */
void rds_reset(void)
{
    int oldlevel = rds_disable_irq_save();

    ps_copy[0] = '\0';
    ps_segment = 0;
    rt_copy[0] = '\0';
    rt_segment = 0;
    pi = 0;

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

    segment = data[1] & 3;

    /* reset parsing if not in expected order */
    if (segment != ps_segment) {
        ps_segment = 0;
        if (segment != 0) {
            return false;
        }
    }

    /* store data */
    pos = segment * 2;
    ps_data[pos++] = (data[3] >> 8) & 0xFF;
    ps_data[pos++] = (data[3] >> 0) & 0xFF;
    if (++ps_segment == 4) {
        ps_data[pos] = '\0';
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
    
    /* reset parsing if not in expected order */
    abflag = (data[1] >> 4) & 1;
    segment = data[1] & 0xF;
    if ((abflag != rt_abflag) || (segment != rt_segment)) {
        rt_abflag = abflag;
        rt_segment = 0;
        if (segment != 0) {
            return false;
        }
    }

    /* store data */
    version = (data[1] >> 11) & 1;
    done = false;
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
    if ((++rt_segment == 16) || done) {
        rt_data[pos] = '\0';
        if (strcmp(rt_copy, rt_data) != 0) {
            /* we got an updated message */
            strcpy(rt_copy, rt_data);
            return true;
        }
    }

    return false;
}

/* processes one rds packet, returns true if a new message was received */
bool rds_process(uint16_t data[4])
{
    int group;

    /* get programme identification */
    if (pi == 0) {
        pi = data[0];
    }
    
    /* handle rds data based on group */
    group = (data[1] >> 11) & 0x1F;
    switch (group) {
    
    case 0: /* group 0A: basic info */
    case 1: /* group 0B: basic info */
        return handle_group0(data);
    
    case 4: /* group 2A: radio text */
    case 5: /* group 2B: radio text */
        return handle_group2(data);
    
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
    return pi;
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

