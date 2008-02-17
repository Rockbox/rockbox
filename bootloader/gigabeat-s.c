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

#define TAR_CHUNK 512
#define TAR_HEADER_SIZE 157

char version[] = APPSVERSION;
char basedir[] = "/Content/0b00/00/"; /* Where files sent via MTP are stored */
int (*kernel_entry)(void);
extern void reference_system_c(void);

/* Dummy stub that creates C references for C functions only used by
   assembly - never called */
void reference_files(void)
{
    reference_system_c();
}

void untar(int tar_fd)
{
    char header[TAR_HEADER_SIZE];
    char copybuf[TAR_CHUNK];
    char path[102];
    int fd, i, size = 0, pos = 0;

    while (1)
    {
        read(tar_fd, header, TAR_HEADER_SIZE);

        if (*header == '\0')  /* Check for EOF */
            break;

        /* Parse the size field */
        size = 0;
        for (i = 124 ; i < 124 + 11 ; i++) {
            size = (8 * size) + header[i] - '0';
        }

        /* Skip rest of header */
        pos = lseek(tar_fd, TAR_CHUNK - TAR_HEADER_SIZE, SEEK_CUR);

        /* Make the path absolute */
        strcpy(path, "/");
        strcat(path, header);

        if (header[156] == '0')  /* file */
        {
            int rc, wc, total = 0;

            fd = creat(path);
            if (fd < 0)
            {
                printf("failed to create file (%d)", fd);
                /* Skip the file */
                lseek(tar_fd, (size + 511) & (~511), SEEK_CUR);
            }
            else
            {
                /* Copy the file over 512 bytes at a time */
                while (total < size)
                {
                    rc = read(tar_fd, copybuf, TAR_CHUNK);
                    pos += rc;

                    wc = write(fd, copybuf, MIN(rc, size - total));
                    if (wc < 0)
                    {
                        printf("write failed (%d)", wc);
                        break;
                    }
                    total += wc;
                }
                close(fd);
            }
        }
        else if (header[156] == '5')  /* directory */
        {
            int ret;

            /* Remove the trailing slash */
            if (path[strlen(path) - 1] == '/')
                path[strlen(path) - 1] = '\0';

            /* Create the dir */
            ret = mkdir(path);
            if (ret < 0 && ret != -4)
            {
                printf("failed to create dir (%d)", ret);
            }
        }
    }
}

void main(void)
{
    char buf[MAX_PATH];
    char tarstring[6];

    lcd_clear_display();
    printf("Hello world!");
    printf("Gigabeat S Rockbox Bootloader v.00000004");
    system_init();
    kernel_init();
    printf("kernel init done");
    int rc;

    set_interrupt_status(IRQ_FIQ_ENABLED, IRQ_FIQ_STATUS);

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

    /* Look for a tar file */
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
                lseek(fd, 257, SEEK_SET);
                rc = read(fd, tarstring, 5);
                if (rc == 5)
                {
                    tarstring[5] = 0;
                    if (strcmp(tarstring, "ustar") == 0)
                    {
                        printf("Found tar file. Unarchiving...");
                        lseek(fd, 0, SEEK_SET);
                        untar(fd);
                        close(fd);
                        printf("Removing tar file");
                        remove(buf);
                        break;
                    }
                }
                close(fd);
            }
        }
    }

    unsigned char *loadbuffer = (unsigned char *)0x0;
    int buffer_size = 31*1024*1024;

    rc = load_firmware(loadbuffer, "/.rockbox/rockbox.gigabeat", buffer_size);
    if(rc < 0)
        error((int)buf, rc);

    system_prepare_fw_start();

    if (rc == EOK)
    {
        kernel_entry = (void*) loadbuffer;
        invalidate_icache();
        rc = kernel_entry();
    }

    while (1);
}

