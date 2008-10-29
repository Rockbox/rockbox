/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rafaël Carré
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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

#include <stdio.h>
#include <system.h>
#include <inttypes.h>
#include "config.h"
#include "lcd.h"
#include "backlight-target.h"
#include "as3525-codec.h"
#include "common.h"
#include "ata.h"

int show_logo(void);
void main(void)
{
    int i;
    unsigned char buf[8];

    system_init();

    lcd_init();

    show_logo();

    as3525_codec_init();  /* Required for backlight on e200v2 */

    _backlight_on();

    /* show player id to demonstrate communication with codec part */
    for (i = 0; i < 8; i++) {
        buf[i] = as3525_codec_read(0x38 + i);
    }
    printf("ID: %02X%02X%02X%02X%02X%02X%02X%02X", buf[7], buf[6], buf[5], buf[4], buf[3], buf[2], buf[1], buf[0]);

    ata_init();

#ifdef SANSA_CLIP
    /* Use hardware scrolling */

    lcd_write_command(0x26); /* scroll setup */
    lcd_write_command(0x01); /* columns scrolled per step */
    lcd_write_command(0x00); /* start page */
    lcd_write_command(0x00); /* steps freqency */
    lcd_write_command(0x07); /* end page (including) */

    lcd_write_command(0x2F); /* start horizontal scrolling */
#endif

    /* never returns */
    while(1) ;
}
