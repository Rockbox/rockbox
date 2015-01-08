/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: main-e200r-installer.c 15599 2007-11-12 18:49:53Z amiconn $
 *
 * Copyright (C) 2011 by Frank Gevaerts
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "cpu.h"
#include "file.h"
#include "system.h"
#include "../kernel-internal.h"
#include "lcd.h"
#include "font.h"
#include "storage.h"
#include "button.h"
#include "disk.h"
#include <string.h>
#include "i2c.h"
#include "backlight-target.h"
#include "power.h"

unsigned char zero[1024*64];
unsigned char nonzero[1024*64];
unsigned char scratch[1024*64];
struct storage_info info;

int format_partition(int start, int size);

#define SPT 63
#define HPC 255

int c(int lba)
{
    return lba/(SPT * HPC);
}
int h(int lba)
{
    return (lba/SPT)%HPC;
}
int s(int lba)
{
    return (lba%SPT) + 1;
}

int write_mbr(int datastart,int datasize, int firmwarestart, int firmwaresize)
{
    unsigned char mbr[512];
    memset(mbr,0,512);
    mbr[446]=0x80; /* flags */
    mbr[447]=h(datastart); /* chs1[0] (h) */
    mbr[448]=s(datastart); /* chs1[1] (s) */
    mbr[449]=c(datastart); /* chs1[2] (c) */
    mbr[450]=0x0b; /* type */
    mbr[451]=h(datastart+datasize-1); /* chs2[0] (h) */
    mbr[452]=s(datastart+datasize-1); /* chs2[1] (s) */
    mbr[453]=c(datastart+datasize-1); /* chs2[2] (c) */
    mbr[454]=(datastart&0x000000ff); /* lba[0] */
    mbr[455]=(datastart&0x0000ff00) >>8; /* lba[1] */
    mbr[456]=(datastart&0x00ff0000) >>16; /* lba[2] */
    mbr[457]=(datastart&0xff000000) >>24; /* lba[3] */
    mbr[458]=(datasize&0x000000ff); /* size[0] */
    mbr[459]=(datasize&0x0000ff00) >>8; /* size[1] */
    mbr[460]=(datasize&0x00ff0000) >>16; /* size[2] */
    mbr[461]=(datasize&0xff000000) >>24; /* size[3] */

    mbr[462]=0; /* flags */
    mbr[463]=h(firmwarestart); /* chs1[0] (h) */
    mbr[464]=s(firmwarestart); /* chs1[1] (s) */
    mbr[465]=c(firmwarestart); /* chs1[2] (c) */
    mbr[466]=0x84; /* type */
    mbr[467]=h(firmwarestart+firmwaresize-1); /* chs2[0] (h) */
    mbr[468]=s(firmwarestart+firmwaresize-1); /* chs2[1] (s) */
    mbr[469]=c(firmwarestart+firmwaresize-1); /* chs2[2] (c) */
    mbr[470]=(firmwarestart&0x000000ffu); /* lba[0] */
    mbr[471]=(firmwarestart&0x0000ff00u) >>8; /* lba[1] */
    mbr[472]=(firmwarestart&0x00ff0000u) >>16; /* lba[2] */
    mbr[473]=(firmwarestart&0xff000000u) >>24; /* lba[3] */
    mbr[474]=(firmwaresize&0x000000ffu); /* size[0] */
    mbr[475]=(firmwaresize&0x0000ff00u) >>8; /* size[1] */
    mbr[476]=(firmwaresize&0x00ff0000u) >>16; /* size[2] */
    mbr[477]=(firmwaresize&0xff000000u) >>24; /* size[3] */

    mbr[510]=0x55;
    mbr[511]=0xaa;

    int res = storage_write_sectors(0,1,mbr);
    if(res != 0)
    {
        return -1;
    }
    return 0;
}

/* Hack. We "steal" line from common.c to reset the line number
 * so we can overwrite the previous line for nicer progress info
 */
extern int line;
int wipe(int size, int verify)
{
    int i;
    int res;
    int sectors = sizeof(nonzero)/512;
    for(i=0;i<size;i+=sectors)
    {
        if(verify)
        {
            res = storage_write_sectors(i,sectors,nonzero);
            if(res != 0)
            {
                printf("write error (1) on sector %d (of %d)!",i,size);
                return -1;
            }
            res = storage_read_sectors(i,sectors,scratch);
            if(res != 0)
            {
                printf("read error (1) on sector %d (of %d)!",i,size);
                return -1;
            }
            res = memcmp(nonzero, scratch, sizeof(nonzero));
            if(res != 0)
            {
                printf("compare error (1) on sector %d (of %d)!",i,size);
                return -1;
            }
        }
        res = storage_write_sectors(i,sectors,zero);
        if(res != 0)
        {
            printf("write error (2) on sector %d (of %d)!",i,size);
            return -1;
        }
        if(verify)
        {
            res = storage_read_sectors(i,sectors,scratch);
            if(res != 0)
            {
                printf("read error (2) on sector %d (of %d)!",i,size);
                return -1;
            }
            res = memcmp(zero, scratch, sizeof(nonzero));
            if(res != 0)
            {
                printf("compare error (2) on sector %d (of %d)!",i,size);
                return -1;
            }
        }

        if(i%2048 == 0)
        {
            printf("%d of %d MB done",i/2048, size/2048);
            /* Hack to overwrite the previous line */
            line--;
        }
    }
    return 0;
}

void* main(void)
{
    int i;
    int btn;

    system_init();
    kernel_init();
    lcd_init();
    font_init();
    button_init();
    i2c_init();
    backlight_hw_on();

    lcd_set_foreground(LCD_WHITE);
    lcd_set_background(LCD_BLACK);
    lcd_clear_display();

    btn = button_read_device();
    verbose = true;

    lcd_setfont(FONT_SYSFIXED);

    printf("Sansa initialiser");
    printf("");


    i=storage_init();
    disk_init(IF_MD(0));

    storage_get_info(0,&info);
    int size = info.num_sectors;
    memset(zero,0,sizeof(zero));
    memset(nonzero,0xff,sizeof(nonzero));
    printf("Zeroing flash");
    int res;

    res = wipe(size, 0);
    if(res != 0)
    {
        printf("error wiping flash");
    }

    int firmwaresize = 0xa000;
    int firmwarestart = size - firmwaresize;
    int datastart = 600;
    int datasize = firmwarestart - datastart;

    res = write_mbr(datastart,datasize,firmwarestart,firmwaresize);
    if(res != 0)
    {
        printf("error writing mbr");
    }
    res = format_partition(datastart, datasize);
    if(res != 0)
    {
        printf("error formatting");
    }

    printf("Wipe done.");
    if (button_hold())
        printf("Release Hold and");
    printf("press any key to");
    printf("shutdown.");

    printf("Remember to use");
    printf("manufacturing");
    printf("mode to recover");
    printf("further");

    while(button_read_device() == BUTTON_NONE);

    power_off();

    return NULL;
}
