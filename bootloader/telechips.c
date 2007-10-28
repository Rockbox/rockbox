/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Dave Chapman
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * 
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "adc-target.h"
#include "backlight-target.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"

char version[] = APPSVERSION;

extern int line;

void* main(void)
{
    unsigned short button;
    int gpioa;

    system_init();
    adc_init();
    lcd_init();
    font_init();

    __backlight_on();

    while(1) {
        line = 0;
        printf("Hello World!");

        gpioa = GPIOA;
        printf("GPIOA: 0x%08x",gpioa);

        button = adc_read(ADC_BUTTONS);
        printf("ADC[0]: 0x%04x",button);
    }

    return 0;
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */
void usb_acknowledge(void)
{
}

void usb_wait_for_disconnect(void)
{
}
