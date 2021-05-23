/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "cw2015.h"
#include "i2c-async.h"
#include <string.h>
#include "system.h"

/* Headers for the debug menu */
#ifndef BOOTLOADER
# include "action.h"
# include "list.h"
# include <stdio.h>
#endif

/* Battery profile info is an opaque blob. According to this,
 * https://lore.kernel.org/linux-pm/20200503154855.duwj2djgqfiyleq5@earth.universe/T/#u
 * the blob only comes from Cellwise testing a physical battery and cannot be
 * obtained any other way. It's specific to a given battery so each target has
 * its own profile.
 *
 * Profile data seems to be retained on the chip so it's not a hard requirement
 * to define this. Provided you don't lose power in the meantime, it should be
 * enough to just boot the OF, then boot Rockbox and read out the battery info
 * from the CW2015 debug screen.
 */
#if defined(SHANLING_Q1)
static const uint8_t device_batinfo[CW2015_SIZE_BATINFO] = {
    0x15, 0x7E, 0x61, 0x59, 0x57, 0x55, 0x56, 0x4C,
    0x4E, 0x4D, 0x50, 0x4C, 0x45, 0x3A, 0x2D, 0x27,
    0x22, 0x1E, 0x19, 0x1E, 0x2A, 0x3C, 0x48, 0x45,
    0x1D, 0x94, 0x08, 0xF6, 0x15, 0x29, 0x48, 0x51,
    0x5D, 0x60, 0x63, 0x66, 0x45, 0x1D, 0x83, 0x38,
    0x09, 0x43, 0x16, 0x42, 0x76, 0x98, 0xA5, 0x1B,
    0x41, 0x76, 0x99, 0xBF, 0x80, 0xC0, 0xEF, 0xCB,
    0x2F, 0x00, 0x64, 0xA5, 0xB5, 0x0E, 0x30, 0x29,
};
#else
# define NO_BATINFO
#endif

static uint8_t chip_batinfo[CW2015_SIZE_BATINFO];

/* TODO: Finish implementing this
 *
 * Although this chip might give a better battery estimate than voltage does,
 * the mainline linux driver has a lot of weird hacks due to oddities like the
 * SoC getting stuck during charging, and from limited testing it seems this
 * may occur for the Q1 too.
 */

static int cw2015_read_bat_info(uint8_t* data)
{
    for(int i = 0; i < CW2015_SIZE_BATINFO; ++i) {
        int r = i2c_reg_read1(CW2015_BUS, CW2015_ADDR, CW2015_REG_BATINFO + i);
        if(r < 0)
            return r;

        data[i] = r & 0xff;
    }

    return 0;
}

void cw2015_init(void)
{
    /* mdelay(100); */
    int rc = cw2015_read_bat_info(&chip_batinfo[0]);
    if(rc < 0)
        memset(chip_batinfo, 0, sizeof(chip_batinfo));
}

int cw2015_get_vcell(void)
{
    int vcell_msb = i2c_reg_read1(CW2015_BUS, CW2015_ADDR, CW2015_REG_VCELL);
    int vcell_lsb = i2c_reg_read1(CW2015_BUS, CW2015_ADDR, CW2015_REG_VCELL+1);

    if(vcell_msb < 0 || vcell_lsb < 0)
        return -1;

    /* 14 bits, resolution 305 uV */
    int v_raw = ((vcell_msb & 0x3f) << 8) | vcell_lsb;
    return v_raw * 61 / 200;
}

int cw2015_get_soc(void)
{
    int soc_msb = i2c_reg_read1(CW2015_BUS, CW2015_ADDR, CW2015_REG_SOC);

    if(soc_msb < 0)
        return -1;

    /* MSB is the state of charge in percentage.
     * the LSB contains fractional information not useful to Rockbox. */
    return soc_msb & 0xff;
}

int cw2015_get_rrt(void)
{
    int rrt_msb = i2c_reg_read1(CW2015_BUS, CW2015_ADDR, CW2015_REG_RRT_ALERT);
    int rrt_lsb = i2c_reg_read1(CW2015_BUS, CW2015_ADDR, CW2015_REG_RRT_ALERT+1);

    if(rrt_msb < 0 || rrt_lsb < 0)
        return -1;

    /* 13 bits, resolution 1 minute */
    return ((rrt_msb & 0x1f) << 8) | rrt_lsb;
}

const uint8_t* cw2015_get_bat_info(void)
{
    return &chip_batinfo[0];
}

#ifndef BOOTLOADER
enum {
    CW2015_DEBUG_VCELL = 0,
    CW2015_DEBUG_SOC,
    CW2015_DEBUG_RRT,
    CW2015_DEBUG_BATINFO,
    CW2015_DEBUG_BATINFO_LAST = CW2015_DEBUG_BATINFO + 7,
    CW2015_DEBUG_NUM_ENTRIES,
};

static int cw2015_debug_menu_cb(int action, struct gui_synclist* lists)
{
    (void)lists;

    if(action == ACTION_NONE)
        action = ACTION_REDRAW;

    return action;
}

static const char* cw2015_debug_menu_get_name(int item, void* data,
                                              char* buf, size_t buflen)
{
    (void)data;

    /* hexdump of battery info */
    if(item >= CW2015_DEBUG_BATINFO && item <= CW2015_DEBUG_BATINFO_LAST) {
        int i = item - CW2015_DEBUG_BATINFO;
        const uint8_t* batinfo = cw2015_get_bat_info();
        snprintf(buf, buflen, "BatInfo%d: %02x %02x %02x %02x %02x %02x %02x %02x", i,
                 batinfo[8*i + 0], batinfo[8*i + 1], batinfo[8*i + 2], batinfo[8*i + 3],
                 batinfo[8*i + 4], batinfo[8*i + 5], batinfo[8*i + 6], batinfo[8*i + 7]);
        return buf;
    }

    switch(item) {
    case CW2015_DEBUG_VCELL:
        snprintf(buf, buflen, "VCell: %d mV", cw2015_get_vcell());
        return buf;
    case CW2015_DEBUG_SOC:
        snprintf(buf, buflen, "SOC: %d%%", cw2015_get_soc());
        return buf;
    case CW2015_DEBUG_RRT:
        snprintf(buf, buflen, "Runtime: %d min", cw2015_get_rrt());
        return buf;
    default:
        return "---";
    }
}

bool cw2015_debug_menu(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, "CW2015 debug", CW2015_DEBUG_NUM_ENTRIES, NULL);
    info.action_callback = cw2015_debug_menu_cb;
    info.get_name = cw2015_debug_menu_get_name;
    return simplelist_show_list(&info);
}
#endif
