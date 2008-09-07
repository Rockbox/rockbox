/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2008 by Maurus Cuelenaere
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "ata-target.h"
#include "disk.h"
#include "font.h"
#include "backlight.h"
#include "button.h"
#include "common.h"


static void load_fw(unsigned char* ptr, unsigned int len)
{
    (void)ptr;
    (void)len;
    asm volatile("ldr pc, =0x1EE0000");
}

void main(void)
{
    unsigned char* loadbuffer;
    int buffer_size;
    int(*kernel_entry)(void);
    int ret;
    
    /* Make sure interrupts are disabled */
    set_irq_level(IRQ_DISABLED);
    set_fiq_status(FIQ_DISABLED);
    system_init();
    kernel_init();
    
    /* Now enable interrupts */
    set_irq_level(IRQ_ENABLED);
    set_fiq_status(FIQ_ENABLED);
    backlight_init();
    lcd_init();
    font_init();
    button_init();

    lcd_enable(true);
    lcd_setfont(FONT_SYSFIXED);
    reset_screen();
    printf("Rockbox boot loader");
    printf("Version %s", APPSVERSION);
    
    ret = ata_init();
    if(ret)
        printf("ATA error: %d", ret);
    
    /* If no button is held, start the OF */
    if(button_read_device() == 0)
    {
        printf("Loading Creative firmware...");
        
        loadbuffer = (unsigned char*)0x00A00000;
        ret = load_minifs_file("creativeos.jrm", loadbuffer);
        if(ret != -1)
        {
            set_irq_level(IRQ_DISABLED);
            set_fiq_status(FIQ_DISABLED);
            /* Doesn't return! */
            load_fw(loadbuffer, ret);
        }
        else
            printf("FAILED!");
    }
    else
    {
        disk_init();

        ret = disk_mount_all();
        if (ret <= 0)
            error(EDISK, ret);
        
        printf("Loading Rockbox firmware...");

        loadbuffer = (unsigned char*)0x00900000;
        buffer_size = (unsigned char*)0x01900000 - loadbuffer;

        ret = load_firmware(loadbuffer, BOOTFILE, buffer_size);
        if(ret < 0)
            error(EBOOTFILE, ret);
        
        else if(ret == EOK)
        {
            kernel_entry = (void*) loadbuffer;
            ret = kernel_entry();
            printf("FAILED!");
        }
    }
    
    while(1);
}
