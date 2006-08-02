/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
 *
 * Based on ipod.c by Dave Chapman
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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
#include "backlight.h"
#include "panic.h"
#include "power.h"
#include "file.h"

#define XSC(X) #X
#define SC(X) XSC(X)

#define DRAM_START              0x10000000

#define BUTTON_LEFT  1
#define BUTTON_MENU  2
#define BUTTON_RIGHT 3
#define BUTTON_PLAY  4
#define BUTTON_HOLD  5

/* Size of the buffer to store the loaded Rockbox/Linux image */
#define MAX_LOADSIZE (4*1024*1024)

char version[] = APPSVERSION;

typedef struct _image {
    unsigned type;              /* '' */
    unsigned id;                /* */
    unsigned pad1;              /* 0000 0000 */
    unsigned devOffset;         /* byte offset of start of image code */
    unsigned len;               /* length in bytes of image */
    void    *addr;              /* load address */
    unsigned entryOffset;       /* execution start within image */
    unsigned chksum;            /* checksum for image */
    unsigned vers;              /* image version */
    unsigned loadAddr;          /* load address for image */
} image_t;

extern image_t boot_table[];

int line=0;

void main(void)
{
    char buf[256];
    int imageno=0;
    int i;
    int rc;
    int padding = 0x4400;
    image_t *tblp = boot_table;
    void* entry;
    struct partinfo* pinfo;
    unsigned short* identify_info;
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */

void reset_poweroff_timer(void)
{
}

int dbg_ports(void)
{
   return 0;
}

void mpeg_stop(void)
{
}

void usb_acknowledge(void)
{
}

void usb_wait_for_disconnect(void)
{
}

void sys_poweroff(void)
{
}

void system_reboot(void)
{

}
