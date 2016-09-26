/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2011 by Amaury Pouly
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

/* IMPORTANT NOTE This file is used by both Rockbox (firmware and bootloader)
 * and the dualboot stub. The stub #include this file directly, so make sure
 * this file is independent and does not requires anything from the firmware */
#include "dualboot-imx233.h"

#ifdef HAVE_DUALBOOT_STUB
/** Persistent registers usage by the OF based on the Firmware SDK
 * and support for firmware upgrades in Rockbox
 * (this includes the Fuze+, ZEN X-Fi3, NWZ-E360/E370/E380)
 *
 * The following are used:
 * - PERSISTENT0: mostly standard stuff described in the datasheet
 * - PERSISTENT1: mostly proprietary stuff + some bits described in the datasheet
 * - PERSISTENT2: used to keep track of time (see below)
 * - PERSISTENT3: proprietary stuff
 * - PERSISTENT4: unused
 * - PERSISTENT5: used by Rockbox to tell the dualboot stub what to do
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
 * Note that the exact number of fsl stages is device-dependent, there 5 on the
 * fuze+, 3 on the NWZs for example.
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
 *
 *
 * To support firmware upgrades in Rockbox, we need to two things:
 * - a way to tell rb (rockbox dual stub) to continue to fsl3 instead of booting
 *   rock (our bootloader)
 * - a way to setup the persistent bits so that fsl3 will boot to fsl4 (updater)
 *   instead of booting host (usb) or play (firmware)
 *
 * The approach taken is to use PERSISTENT5 to tell the dualboot stub what we want
 * to do. Since previous dualboot stubs did not support this, and that other actions
 * may be added in the future, the registers stores both the capabilities of the
 * dualboot stub (so that Rockbox can read them) and the actions that the dualboot
 * stub must perform (so that Rockbox can write them). The register is encoded
 * so that older/random values will be detected as garbage by newer Rockbox and
 * dualboot stub, and that a value of 0 for a field always behaves as when it did
 * not exist. More precisely, the bottom 16-bit must be 'RB' and
 * the top 16-bit store the actual data. The following fields are currently defined:
 * - CAP_BOOT(1 bit): supports booting to OF and UPDATER using the BOOT field
 * - BOOT(2 bits): sets boot mode
 * 
 * At the moment, BOOT supports three values:
 * - IMX233_BOOT_NORMAL: the dualboot will do a normal boot (booting to Rockbox
 *   unless the user presses the magic button that boots to the OF)
 * - IMX233_BOOT_OF: the dualboot stub will continue booting with fsl3 instead
 *   of Rockbox, but it will not touch any of OF persistent bits (this is useful
 *   to simply reboot to the OF for example)
 * - IMX233_BOOT_UPDATER: the dualboot will setup OF persistents bits and
 *   continue so that fsl3 enters fsl4 (updater)
 * In this scheme, Rockbox does not have to care about how exactly those actions
 * are achieved, only the dualboot stub has to deal with the persistent bits.
 * When the dualboot stubs see either OF or UPDATER, it clears BOOT back
 * to NORMAL before continuing, so as to avoid any boot loop.
 *
 */

#include "regs/rtc.h"

/* the persistent register we use */
#define REG_DUALBOOT    HW_RTC_PERSISTENT5
/* the bottom 16-bit are a magic value to indicate that the content is valid */
#define MAGIC_MASK      0xffff
#define MAGIC_VALUE     ('R' | 'B' << 8)
/* CAP_BOOT: 1-bit (16) */
#define CAP_BOOT_POS    16
#define CAP_BOOT_MASK   (1 << 16)
/* BOOT field: 2-bits (18-17) */
#define BOOT_POS        17
#define BOOT_MASK       (3 << 17)

unsigned imx233_dualboot_get_field(enum imx233_dualboot_field_t field)
{
    unsigned val = HW_RTC_PERSISTENT5;
    /* if signature doesn't match, assume everything is 0 */
    if((val & MAGIC_MASK) != MAGIC_VALUE)
        return 0;
#define match(field) \
    case DUALBOOT_##field: return ((val & field##_MASK) >> field##_POS);
    switch(field)
    {
        match(CAP_BOOT)
        match(BOOT)
        default: return 0; /* unknown */
    }
#undef match
}

void imx233_dualboot_set_field(enum imx233_dualboot_field_t field, unsigned fval)
{
    unsigned val = HW_RTC_PERSISTENT5;
    /* if signature doesn't match, create an empty register */
    if((val & MAGIC_MASK) != MAGIC_VALUE)
        val = MAGIC_VALUE; /* all field are 0 */
#define match(field) \
    case DUALBOOT_##field: \
        val &= ~field##_MASK; \
        val |= (fval << field##_POS) & field##_MASK; \
        break;
    switch(field)
    {
        match(CAP_BOOT)
        match(BOOT)
        default: break;
    }
    HW_RTC_PERSISTENT5 = val;
#undef match
}

#endif /* HAVE_DUALBOOT_STUB */ 
