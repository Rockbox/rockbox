/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "ata.h"
#include "disk.h"
#include "fat.h"
#include "lcd.h"
#include "rtc.h"
#include "debug.h"
#include "led.h"
#include "kernel.h"
#include "button.h"
#include "tree.h"
#include "panic.h"
#include "menu.h"
#include "system.h"
#ifndef SIMULATOR
#include "dmalloc.h"
#include "bmalloc.h"
#endif
#include "mpeg.h"
#include "main_menu.h"
#include "thread.h"
#include "settings.h"
#include "backlight.h"

#include "version.h"

char appsversion[]=APPSVERSION;

void init(void);

void app_main(void)
{
    init();
    browse_root();
}

#ifdef SIMULATOR

void init(void)
{
    init_threads();
    lcd_init();
    show_logo();
    sleep(HZ/2);
}

#else

/* defined in linker script */
extern int poolstart[];
extern int poolend[];

void init(void)
{
    int rc;
    struct partinfo* pinfo;

    system_init();
    kernel_init();

    reset_settings(&global_settings);
    
    dmalloc_initialize();
    bmalloc_add_pool(poolstart, poolend-poolstart);
    lcd_init();

    show_logo();

#ifdef DEBUG
    debug_init();
#endif
    set_irq_level(0);

    rc = ata_init();
    if(rc)
        panicf("ata: %d",rc);

    pinfo = disk_init();
    if (!pinfo)
        panicf("disk: NULL");

    rc = fat_mount(pinfo[0].start);
    if(rc)
        panicf("mount: %d",rc);

    backlight_init();

    button_init();
    mpeg_init( DEFAULT_VOLUME_SETTING,
               DEFAULT_BASS_SETTING,
               DEFAULT_TREBLE_SETTING );
}

int main(void)
{
    app_main();

    while(1) {
        led(true); sleep(HZ/10);
        led(false); sleep(HZ/10);
    }
    return 0;
}
#endif
