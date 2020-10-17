/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2020 Solomon Peachy
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "lcd.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#include "audio.h"
#include "button.h"
#include "scroll_engine.h"
#include "storage.h"
#include "rolo.h"
#include "rbpaths.h"

//#define LOGF_ENABLE
#include "logf.h"

#if defined(HAVE_BOOTDATA) && !defined(SIMULATOR)
#include "bootdata.h"
#include "crc32.h"
extern  int write_bootdata(unsigned char* buf, int len, unsigned int boot_volume); /*rb-loader.c*/
#endif

static void rolo_error(const char *text, const char *text2)
{
    lcd_clear_display();
    lcd_puts(0, 0, "ROLO error:");
    lcd_puts_scroll(0, 1, text);
    if (text2)
      lcd_puts_scroll(0, 2, text2);
    lcd_update();
    button_get(true);
    button_get(true);
    button_get(true);
    lcd_scroll_stop();
}

int rolo_load(const char* filename)
{
    lcd_clear_display();
    lcd_puts(0, 0, "ROLO...");
    lcd_puts(0, 1, "Loading");
    lcd_update();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_clear_display();
    lcd_remote_puts(0, 0, "ROLO...");
    lcd_remote_puts(0, 1, "Loading");
    lcd_remote_update();
#endif

    audio_stop();

#if defined(HAVE_BOOTDATA) && !defined(SIMULATOR)
    /* write the bootdata as if rolo were the bootloader */
    unsigned int crc = 0;
    if (strcmp(filename, BOOTDIR "/" BOOTFILE) == 0)
        crc = crc_32(boot_data.payload, boot_data.length, 0xffffffff);

    if(crc > 0 && crc == boot_data.crc)
        write_bootdata(filebuf, filebuf_size, boot_data.boot_volume); /* rb-loader.c */
#endif

#ifdef HAVE_STORAGE_FLUSH
    lcd_puts(0, 2, "Flushing storage buffers");
    lcd_update();
    storage_flush();
#endif

    lcd_puts(0, 3, "Executing");
    lcd_update();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_puts(0, 1, "Executing");
    lcd_remote_update();
#endif

#ifdef PIVOT_ROOT
#define EXECDIR "/tmp"
#else
#define EXECDIR HOME_DIR
#endif

    char buf[256];
#ifdef PIVOT_ROOT
    snprintf(buf, sizeof(buf), "/bin/cp " PIVOT_ROOT "/%s " EXECDIR "/" BOOTFILE, filename);
    logf("system: %s", buf);
    system(buf);
    snprintf(buf, sizeof(buf), "/bin/chmod +x " EXECDIR "/" BOOTFILE);
    logf("system: %s", buf);
    system(buf);

    snprintf(buf, sizeof(buf), "%s/%s", EXECDIR, BOOTFILE);
#else
    snprintf(buf, sizeof(buf), "%s/%s", EXECDIR, filename);
#endif

    logf("execl: %s", buf);
    execl(buf, BOOTFILE, NULL);

    rolo_error("Failed to launch!", strerror(errno));

    /* never reached */
    return 0;
}
