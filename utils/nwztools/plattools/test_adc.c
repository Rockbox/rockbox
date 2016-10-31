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
#include "nwz_plattools.h"

int NWZ_TOOL_MAIN(test_adc)(int argc, char **argv)
{
    /* clear screen and display welcome message */
    nwz_lcdmsg(true, 0, 0, "test_adc");
    nwz_lcdmsg(false, 0, 2, "BACK: quit");
    /* open input device */
    int input_fd = nwz_key_open();
    if(input_fd < 0)
    {
        nwz_lcdmsg(false, 3, 4, "Cannot open input device");
        sleep(2);
        return 1;
    }
    /* open adc device */
    int adc_fd = nwz_adc_open();
    if(adc_fd < 0)
    {
        nwz_key_close(input_fd);
        nwz_lcdmsg(false, 3, 4, "Cannot open adc device");
        sleep(2);
        return 1;
    }
    /* display input state in a loop */
    while(1)
    {
        /* print channels */
        for(int i = NWZ_ADC_MIN_CHAN; i <= NWZ_ADC_MAX_CHAN; i++)
            nwz_lcdmsgf(false, 1, 4 + i, "%s: %d    ", nwz_adc_get_name(i),
                nwz_adc_get_val(adc_fd, i));
        /* wait for event (10ms) */
        int ret = nwz_key_wait_event(input_fd, 10000);
        if(ret != 1)
            continue;
        struct input_event evt;
        if(nwz_key_read_event(input_fd, &evt) != 1)
            continue;
        if(nwz_key_event_get_keycode(&evt) == NWZ_KEY_BACK && !nwz_key_event_is_press(&evt))
            break;
    }
    /* finish nicely */
    nwz_key_close(input_fd);
    nwz_adc_close(adc_fd);
    return 0;
}
