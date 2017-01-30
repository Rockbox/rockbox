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
#include <system.h>
#include <kernel.h>
#include "rds.h"
#include "time.h"
#include "string-extra.h"

#define TIMED_OUT(tick) \
    TIME_AFTER(current_tick, (tick))
#define SET_TIMEOUT(tick, duration) \
    ({ (tick) = current_tick + (duration); })

/* Driver keeps strings in native character format, translating on demand */
static char     ps_copy[9];     /* copy of final message */
static long     ps_copy_tmo;    /* timeout to discard programme service name */
static char     rt_copy[65];    /* copy of final message */
static long     rt_copy_tmo;    /* time to discard radio text */
static uint16_t pi_code;        /* current programme identifier code */
static time_t   ct_data;        /* date/time (not robust; not essential) */

/* timeout before text times out */
#define TEXT_TIMEOUT    (30 * HZ)

/* timeout before RDS is considered idle and is reset */
#define RDS_TIMEOUT     (10 * HZ)
static long rds_timeout;        /* timeout until rds is thought idle */
static bool rds_active;         /* if active, timeouts are monitored */

#if (CONFIG_RDS & RDS_CFG_PROCESS)
/* timeout before group segment obsolescence */
#define GROUP0_TIMEOUT  (2 * HZ)
#define GROUP2_TIMEOUT  (10 * HZ)

/* programme identification (not robust; not really used anyway) */
static uint16_t pi_last;        /* previously read code */

/* programme service name */
static char ps_data[2][9];      /* round-robin driver work queue */
static int  ps_segment;         /* next expected segment */
static long ps_timeout;         /* timeout to receive full group */
static int  ps_data_idx;        /* ps_data[0 or 1] */
#define PS_DATA_INC(x) ps_data[ps_data_idx ^= (x)]

/* radio text */
static char rt_data[2][65];     /* round-robin driver work queue */
static int  rt_segment;         /* next expected segment */
static long rt_timeout;         /* timeout to receive full group */
static int  rt_abflag;          /* message change flag */
static int  rt_data_idx;        /* rt_data[0 or 1] */
#define RT_DATA_INC(x) rt_data[rt_data_idx ^= (x)]
#endif /* (CONFIG_RDS & RDS_CFG_PROCESS) */

#if (CONFIG_RDS & RDS_CFG_ISR)
/* Functions are called in ISR context */
#define rds_disable_irq_save() disable_irq_save()
#define rds_restore_irq(old) restore_irq(old)
#else /* !(CONFIG_RDS & RDS_CFG_ISR) */
#define rds_disable_irq_save() 0
#define rds_restore_irq(old) ((void)(old))
#endif /* (CONFIG_RDS & RDS_CFG_ISR) */

/* RDS code table G0 to UTF-8 translation */
static const uint16_t rds_tbl_g0[0x100-0x20] =
{
    /* codes 0x00 .. 0x1F are omitted because they are identities and not
     * actually spec'ed as part of the character maps anyway */
    /* 0       1       2       3       4       5       6       7          */
    0x0020, 0x0021, 0x0022, 0x0023, 0x00A4, 0x0025, 0x0026, 0x0027, /* 20 */
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, /* 28 */
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, /* 30 */
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, /* 38 */
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, /* 40 */
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, /* 48 */
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, /* 50 */
    0x0058, 0x0059, 0x005A, 0x005B, 0x005B, 0x005D, 0x2015, 0x005F, /* 58 */
    0x2016, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, /* 60 */
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, /* 68 */
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, /* 70 */
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x203E, 0x0020, /* 78 */
    0x00E1, 0x00E0, 0x00E9, 0x00E8, 0x00ED, 0x00EC, 0x00F3, 0x00F2, /* 80 */
    0x00FA, 0x00F9, 0x00D1, 0x00C7, 0x015E, 0x00DF, 0x00A1, 0x0132, /* 88 */
    0x00E2, 0x00E4, 0x00EA, 0x00EB, 0x00EE, 0x00EF, 0x00F4, 0x00F6, /* 90 */
    0x00FB, 0x00FC, 0x00F1, 0x00E7, 0x015F, 0x01E7, 0x0131, 0x0133, /* 98 */
    0x00AA, 0x03B1, 0x00A9, 0x2030, 0x01E6, 0x011B, 0x0148, 0x0151, /* A0 */
    0x03C0, 0x20A0, 0x00A3, 0x0024, 0x2190, 0x2191, 0x2192, 0x2193, /* A8 */
    0x00BA, 0x00B9, 0x00B2, 0x00B3, 0x00B1, 0x0130, 0x0144, 0x0171, /* B0 */
    0x00B5, 0x00BF, 0x00F7, 0x00B0, 0x00BC, 0x00BD, 0x00BE, 0x00A7, /* B8 */
    0x00C1, 0x00C0, 0x00C9, 0x00C8, 0x00CD, 0x00CC, 0x00D3, 0x00D2, /* C0 */
    0x00DA, 0x00D9, 0x0158, 0x010C, 0x0160, 0x017D, 0x0110, 0x013F, /* C8 */
    0x00C2, 0x00C4, 0x00CA, 0x00CB, 0x00CE, 0x00CF, 0x00D4, 0x00D6, /* D0 */
    0x00DB, 0x00DC, 0x0159, 0x010D, 0x0161, 0x017E, 0x0111, 0x0140, /* D8 */
    0x00C3, 0x00C5, 0x00C6, 0x0152, 0x0177, 0x00DD, 0x00D5, 0x00D8, /* E0 */
    0x00DE, 0x014A, 0x0158, 0x0106, 0x015A, 0x0179, 0x0166, 0x00F0, /* E8 */
    0x00E3, 0x00E5, 0x00E6, 0x0153, 0x0175, 0x00FD, 0x00F5, 0x00F8, /* F0 */
    0x00FE, 0x014B, 0x0159, 0x0107, 0x015B, 0x017A, 0x0167, 0x0020, /* F8 */
};

/* could add tables G1 and G2 without much trouble */

/* write one UTF-8 character; returns original 'dst' if insufficient space */
static char * convert_rds_char(char *dst, unsigned int c, size_t dstsize)
{
    unsigned int u = c >= 0x20 ? (rds_tbl_g0 - 0x20)[c] : c;

    if (LIKELY(u <= 0x7F)) {
        /* U+0000 .. U+007F -> 0xxx xxxx */
        if (dstsize > 1) {
            *dst++ = u;
        }
    }
    else if (u <= 0x7FF) {
        /* U+0080 .. U+07FF -> 110x xxxx 10 xx xxxx */
        if (dstsize > 2) {
            *dst++ = 0xC0 | (u >> 6);
            *dst++ = 0x80 | (u & 0x3F);
        }
    }
    else /* if (u <= 0xFFFF) */ {
        /* U+0800 .. U+FFFF -> 1110 xxxx 10xx xxxx 10xx xxxx */
        if (dstsize > 3) {
            *dst++ = 0xE0 | (u >> 12);
            *dst++ = 0x80 | ((u >> 6) & 0x3F);
            *dst++ = 0x80 | (u & 0x3F);
        }
    }
#if 0 /* No four-byte characters are used right now */
    else {
        /* U+10000 .. U+10FFFF -> 11110xxx 10xx xxxx 10xx xxxx 10xx xxxx */
        if (dstsize > 4) {
            *dst++ = 0xF0 | (c >> 18);
            *dst++ = 0x80 | ((c >> 12) & 0x3F);
            *dst++ = 0x80 | ((c >> 6) & 0x3F);
            *dst++ = 0x80 | (c & 0x3F);
        }
    }
#endif /* 0 */
    return dst;
}

/* Copy RDS character string with conversion to UTF-8
 * Acts like strlcpy but won't split multibyte characters */
static size_t copy_rds_string(char *dst, const char *src, size_t dstsize)
{
    char *p = dst;
    unsigned int c;

    while ((c = (unsigned char)*src++)) {
        char *q = p;

        p = convert_rds_char(q, c, dstsize);
        if (p == q) {
            dst -= dstsize;
            break;
        }

        dstsize -= p - q;
    }

    if (dstsize > 0) {
        *p = '\0';
    }

    return p - dst;
}

/* indicate recent processing activity */
static void register_activity(void)
{
    SET_TIMEOUT(rds_timeout, RDS_TIMEOUT);
    rds_active  = true;
}

/* resets the rds parser */
void rds_reset(void)
{
    int oldlevel = rds_disable_irq_save();

    /* reset general info */
    pi_code    = 0;
    ct_data    = 0;
    ps_copy[0] = '\0';
    rt_copy[0] = '\0';
    rds_active = false;

#if (CONFIG_RDS & RDS_CFG_PROCESS)
    /* reset driver info */
    pi_last    = 0;
    ps_segment = 0;
    rt_segment = 0;
#endif /* (CONFIG_RDS & RDS_CFG_PROCESS) */

    rds_restore_irq(oldlevel);
}

/* initialises the rds parser */
void rds_init(void)
{
    rds_reset();
}

/* sync RDS state */
void rds_sync(void)
{
    int oldlevel = rds_disable_irq_save();

    if (rds_active) {
        if (TIMED_OUT(rds_timeout)) {
            rds_reset();
        }
        else {
            if (TIMED_OUT(ps_copy_tmo)) {
                ps_copy[0] = '\0';
            }
            if (TIMED_OUT(rt_copy_tmo)) {
                rt_copy[0] = '\0';
            }
        }
    }

    rds_restore_irq(oldlevel);
}

#if (CONFIG_RDS & RDS_CFG_PROCESS)
/* handles a group 0 packet, returns true if a new message was received */
static void handle_group0(const uint16_t data[4])
{
    int segment, pos;
    char *ps;

    segment = data[1] & 3;

    if (segment == 0) {
        ps_segment = 0;
    }
    else if (segment != ps_segment || TIMED_OUT(ps_timeout)) {
        ps_segment = 0;
        return;
    }

    /* store data */
    pos = segment * 2;
    ps = PS_DATA_INC(0);
    ps[pos + 0] = (data[3] >> 8) & 0xFF;
    ps[pos + 1] = (data[3] >> 0) & 0xFF;

    if (++ps_segment < 4) {
        /* don't have all segments yet */
        SET_TIMEOUT(ps_timeout, GROUP0_TIMEOUT);
        return;
    }

    ps[8] = '\0';

    /* two messages in a row must be the same */
    if (memcmp(ps, PS_DATA_INC(1), 8) == 0) {
        memcpy(ps_copy, ps, 9);
        SET_TIMEOUT(ps_copy_tmo, TEXT_TIMEOUT);
    }
}

/* handles a radio text characters, returns true if end-of-line found */
static bool handle_rt(int *pos_p, char c)
{
    char *rt = RT_DATA_INC(0);

    switch (c) {
    case 0x0D:          /* end of line */
        return true;
    case 0x0A:          /* optional line break */
    case 0x0B:          /* end of headline */
        c = ' ';
    default:            /* regular character */
        rt[(*pos_p)++] = c;
    case 0x00 ... 0x09: /* unprintable */
    case 0x0C:
    case 0x0E ... 0x1E:
    case 0x1F:          /* soft hyphen */
        return false;
    }
}

/* handles a group 2 packet, returns true if a new message was received */
static void handle_group2(const uint16_t data[4])
{
    int abflag, segment, version, pos;
    char *rt;
    bool done = false;

    /* reset parsing if the message type changed */
    abflag = (data[1] >> 4) & 1;
    segment = data[1] & 0xF;
    version = (data[1] >> 11) & 1;

    if (abflag != rt_abflag || segment == 0) {
        rt_abflag = abflag;
        rt_segment = 0;
    }
    else if (segment != rt_segment || TIMED_OUT(rt_timeout)) {
        rt_segment = 0;
        return;
    }

    /* store data */
    if (version == 0) {
        pos = segment * 4;
        done = done || handle_rt(&pos, (data[2] >> 8) & 0xFF);
        done = done || handle_rt(&pos, (data[2] >> 0) & 0xFF);
        done = done || handle_rt(&pos, (data[3] >> 8) & 0xFF);
        done = done || handle_rt(&pos, (data[3] >> 0) & 0xFF);
    } else {
        pos = segment * 2;
        done = done || handle_rt(&pos, (data[3] >> 8) & 0xFF);
        done = done || handle_rt(&pos, (data[3] >> 0) & 0xFF);
    }

    /* there are two cases for completion:
     * - we got all 16 segments
     * - we found an end of line */
    if (++rt_segment < 16 && !done) {
        SET_TIMEOUT(rt_timeout, GROUP2_TIMEOUT);
        return;
    }

    rt = RT_DATA_INC(0);
    rt[pos++] = '\0';

    /* two messages in a row must be the same */
    if (memcmp(rt, RT_DATA_INC(1), pos) == 0) {
        memcpy(rt_copy, rt, pos);
        SET_TIMEOUT(rt_copy_tmo, TEXT_TIMEOUT);
    }
}

/* handles a group 4a packet (clock-time) */
static void handle_group4a(const uint16_t data[4])
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
        return;
    }
    if ((hour >= 24) || (minute >= 60)) {
        /* invalid time */
        return;
    }
    if (offset_abs > 24) {
        /* invalid local time offset */
        return;
    }

    /* convert modified julian day + time to UTC */
    time_t seconds = daycode - 40587;
    if (seconds < 24854) {
        seconds *= 86400;
        seconds += hour * 3600;
        seconds += minute * 60;
        seconds += ((offset_sig == 0) ? offset_abs : -offset_abs) * 1800;
        ct_data = seconds;
    }
}

/* processes one rds packet */
void rds_process(const uint16_t data[4])
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
        handle_group0(data);
        break;

    case 4: /* group 2A: radio text */
    case 5: /* group 2B: radio text */
        handle_group2(data);
        break;

    case 8: /* group 4A: clock-time */
        handle_group4a(data);
        break;
    }

    register_activity();
}
#endif /* (CONFIG_RDS & RDS_CFG_PROCESS) */

#if (CONFIG_RDS & RDS_CFG_PUSH)
/* pushes preprocesed RDS information */
void rds_push_info(enum rds_info_id info_id, uintptr_t data, size_t size)
{
    switch (info_id) {
#if 0
    case RDS_INFO_CODETABLE:
        /* nothing doing for now */
        break;
#endif
    case RDS_INFO_PI:
        pi_code = (uint16_t)data;
        break;
    case RDS_INFO_PS:
        strmemcpy(ps_copy, (const char *)data, MIN(size, sizeof (ps_copy)-1));
        SET_TIMEOUT(ps_copy_tmo, TEXT_TIMEOUT);
        break;
    case RDS_INFO_RT:
        strmemcpy(rt_copy, (const char *)data, MIN(size, sizeof (rt_copy)-1));
        SET_TIMEOUT(rt_copy_tmo, TEXT_TIMEOUT);
        break;
    case RDS_INFO_CT:
        ct_data = (time_t)data;
        break;

    default:;
    }

    register_activity();
}
#endif /* (CONFIG_RDS & RDS_CFG_PUSH) */

/* read fully-processed RDS data */
size_t rds_pull_info(enum rds_info_id info_id, uintptr_t data, size_t size)
{
    int oldlevel = rds_disable_irq_save();

    rds_sync();

    switch (info_id) {
#if 0
    case RDS_INFO_CODETABLE:
        /* nothing doing for now */
        break;
#endif
    case RDS_INFO_PI:
        if (size >= sizeof (uint16_t)) {
            *(uint16_t *)data = pi_code;
        }
        size = sizeof (uint16_t);
        break;
    case RDS_INFO_PS:
        size = copy_rds_string((char *)data, ps_copy, size);
        break;
    case RDS_INFO_RT:
        size = copy_rds_string((char *)data, rt_copy, size);
        break;
    case RDS_INFO_CT:
        if (size >= sizeof (time_t)) {
            *(time_t *)data = ct_data;
        }
        size = sizeof (time_t);
        break;

    default:
        size = 0;
    }

    rds_restore_irq(oldlevel);
    return size;
}
