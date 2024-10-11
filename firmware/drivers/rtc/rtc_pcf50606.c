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
#include "pcf50606.h"
#include "timefuncs.h"

/* Values which each disable one alarm time register */
static const char alarm_disable[] = {
    0x7f, 0x7f, 0x3f, 0x07, 0x3f, 0x1f, 0xff
};

void rtc_init(void)
{
    rtc_check_alarm_started(false);
}

int rtc_read_datetime(struct tm *tm)
{
    unsigned int i;
    int rc, oldlevel;
    unsigned char buf[7];

    oldlevel = disable_irq_save();

    rc = pcf50606_read_multiple(0x0a, buf, sizeof(buf));

    restore_irq(oldlevel);

    for (i = 0; i < sizeof(buf); i++)
        buf[i] = BCD2DEC(buf[i]);

    tm->tm_sec = buf[0];
    tm->tm_min = buf[1];
    tm->tm_hour = buf[2];
    tm->tm_mday = buf[4];
    tm->tm_mon = buf[5] - 1;

#ifdef IRIVER_H300_SERIES
    /* Special kludge to coexist with the iriver firmware. The iriver firmware
       stores the date as 1965+nn, and allows a range of 1980..2064. We use
       1964+nn here to make leap years work correctly, so the date will be one
       year off in the iriver firmware but at least won't be reset anymore. */
    tm->tm_year = buf[6] + 64;
#else /* Not IRIVER_H300_SERIES */
    tm->tm_year = buf[6] + 100;
#endif /* IRIVER_H300_SERIES */

    set_day_of_week(tm);
    set_day_of_year(tm);

    return rc;
}

int rtc_write_datetime(const struct tm *tm)
{
    unsigned int i;
    int rc, oldlevel;
    unsigned char buf[7];

    buf[0] = tm->tm_sec;
    buf[1] = tm->tm_min;
    buf[2] = tm->tm_hour;
    buf[3] = tm->tm_wday;
    buf[4] = tm->tm_mday;
    buf[5] = tm->tm_mon + 1;
#ifdef IRIVER_H300_SERIES
    /* Iriver firmware compatibility kludge, see rtc_read_datetime(). */
    buf[6] = tm->tm_year - 64;
#else /* Not IRIVER_H300_SERIES */
    buf[6] = tm->tm_year - 100;
#endif /* IRIVER_H300_SERIES */

    for (i = 0; i < sizeof(buf); i++)
         buf[i] = DEC2BCD(buf[i]);

    oldlevel = disable_irq_save();

    rc = pcf50606_write_multiple(0x0a, buf, sizeof(buf));

    restore_irq(oldlevel);

    return rc;
}

/**
 * Checks the PCF interrupt 1 register bit 7 to see if an alarm interrupt has
 * triggered since last we checked.
 */
bool rtc_check_alarm_flag(void)
{
    int oldlevel = disable_irq_save();
    int rc = pcf50606_read(0x02) & 0x80;
    restore_irq(oldlevel);
    return rc;
}

/**
 * Enables or disables the alarm.
 * The Ipod bootloader clears all PCF interrupt registers and always enables
 * the "wake on RTC" bit on OOCC1, so we have to rely on other means to find
 * out if we just woke from an alarm.
 * Return value is always false for us.
 */
void rtc_enable_alarm(bool enable)
{
    int oldlevel = disable_irq_save();
    if (enable) {
        /* Tell the PCF to ignore everything but second, minute and hour, so
         * that an alarm will trigger the next time the alarm time occurs.
         */
        pcf50606_write_multiple(0x14, alarm_disable + 3, 4);
        /* Unmask the alarm interrupt (might be unneeded) */
        pcf50606_write(0x5, pcf50606_read(0x5) & ~0x80);
        /* Make sure wake on RTC is set when shutting down */
        pcf50606_write(0x8, pcf50606_read(0x8) | 0x10);
    } else {
        /* We use this year to indicate a disabled alarm. If you happen to live
         * around this time and are annoyed by this, feel free to seek out my
         * grave and do something nasty to it.
         */
        pcf50606_write(0x17, 0x99);
        /* Make sure we don't wake on RTC after shutting down */
        pcf50606_write(0x8, pcf50606_read(0x8) & ~0x10);
    }
    restore_irq(oldlevel);
    return;
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
        alarm_state &= !release_alarm;
    } else {
        char rt[3], at[3];
        /* The Ipod bootloader seems to read (and thus clear) the PCF interrupt
         * registers, so we need to find some other way to detect if an alarm
         * just happened
         */
        int oldlevel = disable_irq_save();
        pcf50606_read_multiple(0x0a, rt, 3);
        pcf50606_read_multiple(0x11, at, 3);
        restore_irq(oldlevel);

        /* If alarm time and real time match within 10 seconds of each other, we
         * assume an alarm just triggered
         */
        rc = alarm_state = rt[1] == at[1] && rt[2] == at[2]
                           && (rt[0] - at[0]) <= 10;
        run_before = true;
    }
    return rc;
}


/* set alarm time registers to the given time (repeat once per day) */
void rtc_set_alarm(int h, int m)
{
    int oldlevel = disable_irq_save();
    /* Set us to wake at the first second of the specified time */
    pcf50606_write(0x11, 0);
    /* Convert to BCD */
    pcf50606_write(0x12, ((m/10) << 4) | m%10);
    pcf50606_write(0x13, ((h/10) << 4) | h%10);
    restore_irq(oldlevel);
}

/* read out the current alarm time */
void rtc_get_alarm(int *h, int *m)
{
    char buf[2];

    int oldlevel = disable_irq_save();
    pcf50606_read_multiple(0x12, buf, 2);
    restore_irq(oldlevel);
    /* Convert from BCD */
    *m = ((buf[0] >> 4) & 0x7)*10 + (buf[0] & 0x0f);
    *h = ((buf[1] >> 4) & 0x3)*10 + (buf[1] & 0x0f);
}
