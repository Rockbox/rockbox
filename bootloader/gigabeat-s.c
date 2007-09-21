/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Greg White
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
#include "inttypes.h"
#include "string.h"
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
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"
#include "rbunicode.h"
#include "usb.h"
#include "mmu-imx31.h"
#include "lcd-target.h"
#include "avic-imx31.h"
#include <stdarg.h>

char version[] = APPSVERSION;

void main(void)
{
    lcd_clear_display();
    printf("Hello world!");
    printf("Gigabeat S Rockbox Bootloader v.00000001");          
    kernel_init();
    int rc;

    rc = ata_init();
    if(rc)
    {
        reset_screen();
        error(EATA, rc);
    }

    disk_init();

    rc = disk_mount_all();
    if (rc<=0)
    {
        error(EDISK,rc);
    }

    printf("Congratulations!");
    while(1);

#if 0
    printf("Loading firmware");

    loadbuffer = (unsigned char*) 0x100;
    buffer_size = (unsigned char*)0x400000 - loadbuffer;

    rc = load_firmware(loadbuffer, BOOTFILE, buffer_size);
    if(rc < 0)
        error(EBOOTFILE, rc);

    if (rc == EOK)
    {
        kernel_entry = (void*) loadbuffer;
        rc = kernel_entry();
    }
#endif
}

