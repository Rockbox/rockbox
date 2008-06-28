/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing, Uwe Freese, Laurent Baum
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
#include "config.h"
#include "i2c.h"
#include "rtc.h"
#include "kernel.h"
#include "system.h"
#include "pcf50605.h"
#include <stdbool.h>

/* Values which each disable one alarm time register */
static const char alarm_disable[] = {
    0x7f, 0x7f, 0x3f, 0x07, 0x3f, 0x1f, 0xff
};

void rtc_init(void)
{
    rtc_check_alarm_started(false);
}

int rtc_read_datetime(unsigned char* buf)
{
    return pcf50605_read_multiple(0x0a, buf, 7);
}

int rtc_write_datetime(unsigned char* buf)
{
    pcf50605_write_multiple(0x0a, buf, 7);
    return 1;
}

/**
 * Checks the PCF interrupt 1 register bit 7 to see if an alarm interrupt has
 * triggered since last we checked.
 */
bool rtc_check_alarm_flag(void) 
{
    return pcf50605_read(0x02) & 0x80;
}

/**
 * Enables or disables the alarm.
 * The Ipod bootloader clears all PCF interrupt registers and always enables
 * the "wake on RTC" bit on OOCC1, so we have to rely on other means to find
 * out if we just woke from an alarm.
 * Return value is always false for us. 
 */
bool rtc_enable_alarm(bool enable)
{
    if (enable) {
        /* Tell the PCF to ignore everything but second, minute and hour, so
         * that an alarm will trigger the next time the alarm time occurs.
         */
        pcf50605_write_multiple(0x14, alarm_disable + 3, 4);
        /* Unmask the alarm interrupt (might be unneeded) */
        pcf50605_write(0x5, pcf50605_read(0x5) & ~0x80);
        /* Make sure wake on RTC is set when shutting down */
        pcf50605_wakeup_flags |= 0x10;
    } else {
        /* We use this year to indicate a disabled alarm. If you happen to live
         * around this time and are annoyed by this, feel free to seek out my
         * grave and do something nasty to it.
         */
        pcf50605_write(0x17, 0x99);
        /* Make sure we don't wake on RTC after shutting down */
        pcf50605_wakeup_flags &= ~0x10;
    }
    return false;
}

/**
 * Check if alarm caused unit to start.
 */
bool rtc_check_alarm_started(bool release_alarm)
{
    static bool run_before = false, alarm_state;
    bool rc;

    if (run_before) { 
        rc = alarm_state;
        alarm_state &= ~release_alarm;
    } else {
        char rt[3], at[3];
        /* The Ipod bootloader seems to read (and thus clear) the PCF interrupt
         * registers, so we need to find some other way to detect if an alarm
         * just happened
         */
        pcf50605_read_multiple(0x0a, rt, 3);
        pcf50605_read_multiple(0x11, at, 3);

        /* If alarm time and real time match within 10 seconds of each other, we
         * assume an alarm just triggered
         */
        rc = alarm_state = rt[1] == at[1] && rt[2] == at[2]
                           && (rt[0] - at[0]) <= 10;
        run_before = true;
    }
    return rc;
}

void rtc_set_alarm(int h, int m)
{
    /* Set us to wake at the first second of the specified time */
    pcf50605_write(0x11, 0);
    /* Convert to BCD */
    pcf50605_write(0x12, ((m/10) << 4) | m%10);
    pcf50605_write(0x13, ((h/10) << 4) | h%10);
}

void rtc_get_alarm(int *h, int *m)
{
    char buf[2];

    pcf50605_read_multiple(0x12, buf, 2);
    /* Convert from BCD */
    *m = ((buf[0] >> 4) & 0x7)*10 + (buf[0] & 0x0f);
    *h = ((buf[1] >> 4) & 0x3)*10 + (buf[1] & 0x0f);
}

