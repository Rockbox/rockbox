/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Amaury Pouly
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
#include "nwz_lib.h"
#include <string.h>
#include <stdlib.h>
#include "nwz_plattools.h"

static unsigned long read32(unsigned char *buf)
{
    return buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
}

static void write32(unsigned char *buf, unsigned long value)
{
    buf[0] = value & 0xff;
    buf[1] = (value >> 8) & 0xff;
    buf[2] = (value >> 16) & 0xff;
    buf[3] = (value >> 24) & 0xff;
}

static struct
{
    unsigned long dest;
    const char *name;
} g_dest_list[] =
{
    { 0, "J" },
    { 1, "U" },
    { 0x101, "U2" },
    { 0x201, "U3" },
    { 0x301, "CA" },
    { 2, "CEV" },
    { 0x102, "CE7" },
    { 3, "CEW" },
    { 0x103, "CEW2" },
    { 4, "CN" },
    { 5, "KR" },
    { 6, "E" },
    { 0x106, "MX" },
    { 0x206, "E2" },
    { 0x306, "MX3" },
    { 7, "TW" },
};

#define NR_DEST (sizeof(g_dest_list) / sizeof(g_dest_list[0]))

static int get_dest_index(unsigned long dest)
{
    for(size_t i = 0; i < NR_DEST; i++)
        if(g_dest_list[i].dest == dest)
            return i;
    return -1;
}

static const char *get_dest_name(unsigned long dest)
{
    int index = get_dest_index(dest);
    return index < 0 ? "NG" : g_dest_list[index].name;
}

int NWZ_TOOL_MAIN(dest_tool)(int argc, char **argv)
{
    /* clear screen and display welcome message */
    nwz_lcdmsg(true, 0, 0, "dest_tool");
    /* open input device */
    int input_fd = nwz_key_open();
    if(input_fd < 0)
    {
        nwz_lcdmsg(false, 3, 4, "Cannot open input device");
        sleep(2);
        return 1;
    }
    unsigned long model_id = nwz_get_model_id();
    if(model_id == 0)
    {
        nwz_key_close(input_fd);
        nwz_lcdmsg(false, 3, 4, "Cannot get model ID");
        sleep(2);
        return 1;
    }
    const char *model_name = nwz_get_model_name();
    if(model_name == NULL)
        model_name = "Unknown";
    const char *series_name = "Unknown";
    bool ok_model = false;
    if(nwz_get_series() != -1)
    {
        series_name = nwz_series[nwz_get_series()].name;
        ok_model = true;
    }
    nwz_lcdmsgf(false, 0, 2, "Model ID: %#x", model_id);
    nwz_lcdmsgf(false, 0, 3, "Model: %s", model_name);
    nwz_lcdmsgf(false, 0, 4, "Series: %s", series_name);
    nwz_lcdmsg(false, 0, 5, "BACK: quit");
    nwz_lcdmsg(false, 0, 6, "LEFT/RIGHT: change dest");
    nwz_lcdmsg(false, 0, 7, "PLAY/PAUSE: change sps");
    /* display input state in a loop */
    while(1)
    {
        unsigned char nvp_buf[32];
        bool ok_nvp = false;
        if(ok_model)
        {
            /* make sure node has the right size... */
            if(nwz_nvp_read(NWZ_NVP_SHP, NULL) == sizeof(nvp_buf))
            {
                if(nwz_nvp_read(NWZ_NVP_SHP, nvp_buf) == sizeof(nvp_buf))
                    ok_nvp = true;
                else
                    nwz_lcdmsg(false, 1, 9, "Cannot read NVP.\n");
            }
            else
                nwz_lcdmsg(false, 1, 9, "NVP node has the wrong size.\n");
        }
        else
        {
            nwz_lcdmsg(false, 1, 9, "Your model is not supported.\n");
            nwz_lcdmsg(false, 1, 10, "Please contact a developer.\n");
        }
        /* display information */
        if(ok_nvp)
        {
            unsigned long dest = read32(nvp_buf);
            unsigned long sps = read32(nvp_buf + 4);
            const char *dest_name = get_dest_name(dest);
            const char *sps_name = sps ? "ON" : "OFF";
            nwz_lcdmsgf(false, 1, 9, "DEST: %s (%#x)     ", dest_name, dest);
            nwz_lcdmsgf(false, 1, 10, "SPS: %s (%d)     ", sps_name, sps);
        }
        /* wait for event */
        int ret = nwz_key_wait_event(input_fd, -1);
        if(ret != 1)
            continue;
        struct input_event evt;
        if(nwz_key_read_event(input_fd, &evt) != 1)
            continue;
        /* only act on release */
        if(nwz_key_event_is_press(&evt))
            continue;
        int keycode = nwz_key_event_get_keycode(&evt);
        if(keycode == NWZ_KEY_BACK)
            break;
        bool write_nvp = false;
        if(keycode == NWZ_KEY_LEFT || keycode == NWZ_KEY_RIGHT)
        {
            int dest_idx = get_dest_index(read32(nvp_buf));
            /* if destination is unknown, replace by the first one */
            if(dest_idx == -1)
                dest_idx = 0;
            if(keycode == NWZ_KEY_LEFT)
                dest_idx--;
            else
                dest_idx++;
            dest_idx = (dest_idx + NR_DEST) % NR_DEST;
            write32(nvp_buf, g_dest_list[dest_idx].dest);
            write_nvp = true;
        }
        else if(keycode == NWZ_KEY_PLAY)
        {
            /* change 0 to 1 and anything nonzero to 0 */
            write32(nvp_buf + 4, read32(nvp_buf + 4) == 0 ? 1 : 0);
            write_nvp = true;
        }
        /* write nvp */
        if(ok_nvp && write_nvp)
        {
            if(nwz_nvp_write(NWZ_NVP_SHP, nvp_buf) != 0)
                nwz_lcdmsg(false, 1, 12, "Cannot write NVP.\n");
        }
    }
    /* finish nicely */
    nwz_key_close(input_fd);
    return 0;
}
