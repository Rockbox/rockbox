/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "time.h"
#include "system.h"
#include "rtc.h"
#include "timefuncs.h"
#include "rtc-imx233.h"

/** Persistent registers usage by the OF based on the Firmware SDK
 * (this includes the Fuze+, ZEN X-Fi3, NWZ-E360/E370/E380)
 *
 * The following are used:
 * - PERSISTENT0: mostly standard stuff described in the datasheet
 * - PERSISTENT1: mostly proprietary stuff + some bits described in the datasheet
 * - PERSISTENT2: used to keep track of time (see below)
 * - PERSISTENT3: proprietary stuff
 *
 * In particular, the following bits are involved in the firmware upgrade process
 * and thus worth mentioning (Px means PERSISTENTx). Some of this information
 * might not be entirely accurate:
 * - P1[18]: when 0, indicates to the freescale boot stub to start the updater
 *           rather than the main firmware (play) or the usb firmware (host)
 * - P1[22]: when 0, indicates that the OF database/store should be rebuilt
 * - P3[10]: when 0, indicates that the firmware has been upgraded
 * - P3[11]: when 1, indicates to the freescale boot stub to boot without
 *           requiring the user to hold the power button for a small delay
 * - P3[12]: when 1, indicates that the internal drive or micro-sd card was
 *           modified in USB mode
 * - P3[16]: when 1, indicates that a firmware upgrade was attempted but aborted
 *           due to a too low battery
 *
 * To understand how all this works out together, recall that the boot sequence
 * usually looks as follows (fslx = freescale boot stub stage x, in section 0
 * of the firmware; rb = rockbox dualboot stub), where arrows indicate boot flow
 * (since every stage can choose to continue in the same section or jump to another):
 *
 *                                 +---> host (usb)
 *                                 |
 * fsl0 -> fsl1 -> fsl2 -> rb -> fsl3 -> fsl4 (updater)
 *                         |       |
 *                         |       +---> play (firmware)
 *                         |
 *                         +-----------> rock (bootloader) (-> loads rockbox)
 *
 * The fsl3 decides which stage to boot based on the following logic (order is
 * important):
 * - if P1[18] is 0, it goes to fsl4, to perform a firmware upgrade
 * - if usb is plugged, it goes to host, the OF USB mode
 * - if P1[22] is 1, it requires the user to hold the power button for small
 *   delay and aborts boot if this is not the case
 * - it goes to play, the OF normal firmware
 *
 * The fsl4 (updater) performs the following action:
 * - it clears P1[18] so that next boot will be a normal boot (ie NOT updater)
 * - if firmware.sb does not exist or is invalid, it reboots
 * - if the battery is too low for an upgrade, it sets P3[16]
 *   otherwise, it performs a firmware upgrade and clear P1[22]
 * - it shutdowns
 *
 * The play (firmware) performs the following actions:
 * - if P1[22] is 0 or P3[12] is 1, it rebuilds the store (the 'loading' screen)
 *   and set P1[22] to 1 and P3[12] to 0
 * - if P3[16] is 1, it displays a 'battery was too low to upgrade' message
 *   and clears P3[16]
 * - if P3[10] is 0, it displays a 'firmware was successfully upgraded' message
 *   and sets P3[10] to 1
 * - it performs its usual (crappy) functions
 *
 * The host (USB) performs the following actions:
 * - it clears P1[18] so that the next boot will run the updater
 * - it sets P3[11] to 1 so that the device will reboot without user intervention
 *   at the end
 * - if the host modifies the internal drive or micro-SD card, it sets P3[12]
 *   to 1 and clears P1[22]
 * - after USB is unplugged, it reboots
 *
 * Thus a typical firmware upgrade sequence will look like this:
 * - initially, the main firmware is running and flags are in the following state:
 *     P1[18] = 1 (normal boot)
 *     P1[22] = 1 (store is clean)
 *     P3[10] = 1 (firmware has not been upgraded)
 *     P3[11] = 0 (user needs to hold power button to boot)
 *     P3[12] = 0 (drive is clean)
 * - the user plugs the USB cable, play reboots, fsl3 boots to host because
 *   P1[18] = 1, the users put firmware.sb on the drive, thus modifying its
 *   content and then unplugs the drive; the device reboots with the following
 *   flags:
 *     P1[18] = 0 (updater boot)
 *     P1[22] = 0 (store is dirty)
 *     P3[10] = 1 (firmware has not been upgraded)
 *     P3[11] = 1 (user does not needs to hold power button to boot)
 *     P3[12] = 1 (drive is dirty)
 * - fsl3 boots to the updater because P1[18] = 0, the updater sees firmware.sb
 *   and performs a firmware upgrade; the device then shutdowns with the following
 *   flags:
 *     P1[18] = 1 (normal boot)
 *     P1[22] = 0 (store is dirty)
 *     P3[10] = 0 (firmware has been upgraded)
 *     P3[11] = 1 (user does not needs to hold power button to boot)
 *     P3[12] = 1 (drive is dirty)
 * - the user presses the power button, fsl3 boots to play (firmware) because
 *   P1[18] = 1, it rebuilds the store because P1[22] is clear, it then display
 *   a message to the user saying that the firmware has been upgraded because
 *   P3[10] is 0, and it resets the flags to same state as initially
 *
 * Note that the OF is lazy: it reboots to updater after USB mode in all cases
 * (even if firmware.sb was not present). In this case, the updater simply clears
 * the update flags and reboot immediately, thus it looks like a normal boot.
 */

#define YEAR1980    315532800   /* 1980/1/1 00:00:00 in UTC */

#if defined(SANSA_FUZEPLUS) || defined(CREATIVE_ZENXFI3) || defined(SONY_NWZE360) || \
    defined(SONY_NWZE370)
#define USE_PERSISTENT
#endif

void rtc_init(void)
{
    /* rtc-imx233 is initialized by the system */
}

static void seconds_to_datetime(uint32_t seconds, struct tm *tm)
{
#ifdef USE_PERSISTENT
    /* The OF uses PERSISTENT2 register to keep the adjustment and only changes
     * SECONDS if necessary. */
    seconds += imx233_rtc_read_persistent(2);
#else
    /* The Freescale recommended way of keeping time is the number of seconds
     * since 00:00 1/1/1980 */
    seconds += YEAR1980;
#endif

    gmtime_r(&seconds, tm);
}

int rtc_read_datetime(struct tm *tm)
{
    seconds_to_datetime(imx233_rtc_read_seconds(), tm);
    return 0;
}

int rtc_write_datetime(const struct tm *tm)
{
    uint32_t seconds;

    seconds = mktime((struct tm *)tm);

#ifdef USE_PERSISTENT
    /* The OF uses PERSISTENT2 register to keep the adjustment and only changes
     * SECONDS if necessary.
     * NOTE: the OF uses this mechanism to prevent roll back in time. Although
     *       Rockbox will handle a negative PERSISTENT2 value, the OF will detect
     *       it and won't return in time before SECONDS */
    imx233_rtc_write_persistent(2, seconds - imx233_rtc_read_seconds());
#else
    /* The Freescale recommended way of keeping time is the number of seconds
     * since 00:00 1/1/1980 */
    imx233_rtc_write_seconds(seconds - YEAR1980);
#endif

    return 0;
}

void rtc_set_alarm(int h, int m)
{
    /* transform alarm time to absolute time */
    struct tm tm;
    seconds_to_datetime(imx233_rtc_read_seconds(), &tm);
    /* if same date and hour/min is in the past, advance one day */
    if(h < tm.tm_hour || (h == tm.tm_hour && m <= tm.tm_min))
        seconds_to_datetime(imx233_rtc_read_seconds() + 3600 * 60, &tm);
    tm.tm_hour = h;
    tm.tm_min = m;
    tm.tm_sec = 0;

    uint32_t seconds = mktime(&tm);
#ifdef USE_PERSISTENT
    imx233_rtc_write_alarm(seconds - imx233_rtc_read_persistent(2));
#else
    imx233_rtc_write_alarm(seconds - YEAR1980);
#endif
}

void rtc_get_alarm(int *h, int *m)
{
    struct tm tm;
    seconds_to_datetime(imx233_rtc_read_alarm(), &tm);
    *m = tm.tm_min;
    *h = tm.tm_hour;
}

void rtc_enable_alarm(bool enable)
{
    BF_CLR(RTC_CTRL, ALARM_IRQ_EN);
    BF_CLR(RTC_CTRL, ALARM_IRQ);
    uint32_t val = imx233_rtc_read_persistent(0);
    BF_WRX(val, RTC_PERSISTENT0, ALARM_EN(enable));
    BF_WRX(val, RTC_PERSISTENT0, ALARM_WAKE_EN(enable));
    BF_WRX(val, RTC_PERSISTENT0, ALARM_WAKE(0));
    imx233_rtc_write_persistent(0, val);
}

/**
 * Check if alarm caused unit to start.
 */
bool rtc_check_alarm_started(bool release_alarm)
{
    bool res = BF_RDX(imx233_rtc_read_persistent(0), RTC_PERSISTENT0, ALARM_WAKE);
    if(release_alarm)
        rtc_enable_alarm(false);
    return res;
}

/**
 * Checks if an alarm interrupt has triggered since last we checked.
 */
bool rtc_check_alarm_flag(void)
{
    return BF_RD(RTC_CTRL, ALARM_IRQ);
}
