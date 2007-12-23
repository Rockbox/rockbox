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
#include "dir.h"
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
char buf[MAX_PATH];
char basedir[] = "/Content/0b00/00/"; /* Where files sent via MTP are stored */
char model[5];
int (*kernel_entry)(void);

void main(void)
{
    lcd_clear_display();
    printf("Hello world!");
    printf("Gigabeat S Rockbox Bootloader v.00000001");
    kernel_init();
    printf("kernel init done");
    int rc;

    rc = ata_init();
    if(rc)
    {
        reset_screen();
        error(EATA, rc);
    }
    printf("ata init done");

    disk_init();
    printf("disk init done");

    rc = disk_mount_all();
    if (rc<=0)
    {
        error(EDISK,rc);
    }

    /* Look for the first valid firmware file */
    struct dirent_uncached* entry;
    DIR_UNCACHED* dir;
    int fd;
    dir = opendir_uncached(basedir);
    while ((entry = readdir_uncached(dir)))
    {
        if (*entry->d_name != '.')
        {
            snprintf(buf, sizeof(buf), "%s%s", basedir, entry->d_name);
            fd = open(buf, O_RDONLY);
            if (fd >= 0)
            {
                lseek(fd, 4, SEEK_SET);
                rc = read(fd, model, 4);
                close(fd);
                if (rc == 4)
                {
                    model[4] = 0;
                    if (strcmp(model, "gigs") == 0)
                        break;
                }
            }
        }
    }

    printf("Firmware file: %s", buf);
    printf("Loading firmware");

    unsigned char *loadbuffer = (unsigned char *)0x0;
    int buffer_size = 1024*1024;

    rc = load_firmware(loadbuffer, buf, buffer_size);
    if(rc < 0)
        error(buf, rc);

    if (rc == EOK)
    {
        kernel_entry = (void*) loadbuffer;
        rc = kernel_entry();
    }

    while (1);
}

