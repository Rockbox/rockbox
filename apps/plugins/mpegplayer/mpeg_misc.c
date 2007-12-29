/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Miscellaneous helper API definitions
 *
 * Copyright (c) 2007 Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "mpegplayer.h"

/** Streams **/

/* Ensures direction is -1 or 1 and margin is properly initialized */
void stream_scan_normalize(struct stream_scan *sk)
{
    if (sk->dir >= 0)
    {
        sk->dir = SSCAN_FORWARD;
        sk->margin = sk->len;
    }
    else if (sk->dir < 0)
    {
        sk->dir = SSCAN_REVERSE;
        sk->margin = 0;
    }
}

/* Moves a scan cursor. If amount is positive, the increment is in the scan
 * direction, otherwise opposite the scan direction */
void stream_scan_offset(struct stream_scan *sk, off_t by)
{
    off_t bydir = by*sk->dir;
    sk->pos += bydir;
    sk->margin -= bydir;
    sk->len -= by;
}

/** Time helpers **/
void ts_to_hms(uint32_t pts, struct hms *hms)
{
    hms->frac = pts % TS_SECOND;
    hms->sec = pts / TS_SECOND;
    hms->min = hms->sec / 60;
    hms->hrs = hms->min / 60;
    hms->sec %= 60;
    hms->min %= 60;
}

void hms_format(char *buf, size_t bufsize, struct hms *hms)
{
    /* Only display hours if nonzero */
    if (hms->hrs != 0)
    {
        rb->snprintf(buf, bufsize, "%u:%02u:%02u",
                     hms->hrs, hms->min, hms->sec);
    }
    else
    {
        rb->snprintf(buf, bufsize, "%u:%02u",
                     hms->min, hms->sec);
    }
}

/** Maths **/
uint32_t muldiv_uint32(uint32_t multiplicand,
                       uint32_t multiplier,
                       uint32_t divisor)
{
    if (divisor != 0)
    {
        uint64_t prod = (uint64_t)multiplier*multiplicand + divisor/2;

        if ((uint32_t)(prod >> 32) < divisor)
            return (uint32_t)(prod / divisor);
    }
    else if (multiplicand == 0 || multiplier == 0)
    {
        return 0; /* 0/0 = 0 : yaya */
    }
    /* else (> 0) / 0 = UINT32_MAX */

    return UINT32_MAX; /* Saturate */
}
