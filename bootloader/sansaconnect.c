/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id: $
*
* Copyright (C) 2011 by Tomasz Moń
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
#include "../kernel-internal.h"
#include "storage.h"
#include "file_internal.h"
#include "disk.h"
#include "font.h"
#include "backlight.h"
#include "button.h"
#include "common.h"
#include "rb-loader.h"
#include "version.h"
#include "uart-target.h"
#include "power.h"
#include "loader_strerror.h"

extern void show_logo(void);

void main(void)
{
    unsigned char* loadbuffer;
    int buffer_size;
    int(*kernel_entry)(void);
    int ret;
    int btn;

    /* Make sure interrupts are disabled */
    set_irq_level(IRQ_DISABLED);
    set_fiq_status(FIQ_DISABLED);
    system_init();
    kernel_init();

    /* Now enable interrupts */
    set_irq_level(IRQ_ENABLED);
    set_fiq_status(FIQ_ENABLED);
    lcd_init();
    backlight_init(); /* BUGFIX backlight_init MUST BE AFTER lcd_init */
    font_init();
    button_init();

#ifdef HAVE_LCD_ENABLE
    lcd_enable(true);
#endif
    lcd_setfont(FONT_SYSFIXED);
    reset_screen();
    show_logo();

    btn = button_read_device();

    printf("Rockbox boot loader");
    printf("Version %s", rbversion);

    ret = storage_init();
    if(ret)
        printf("SD error: %d", ret);

    filesystem_init();

    ret = disk_mount_all();
    if (ret <= 0)
        error(EDISK, ret, true);

    if (btn & BUTTON_PREV)
    {
        printf("Loading OF firmware...");
        printf("Loading vmlinux.bin...");
        loadbuffer = (unsigned char*)0x01008000;
        buffer_size = 0x200000;

        ret = load_raw_firmware(loadbuffer, "/vmlinux.bin", buffer_size);

        if (ret < 0)
        {
            printf("Unable to load vmlinux.bin");
        }
        else
        {
            printf("Loading initrd.bin...");
            loadbuffer = (unsigned char*)0x04400020;
            buffer_size = 0x200000;
            ret = load_raw_firmware(loadbuffer, "/initrd.bin", buffer_size);
        }

        if (ret > 0)
        {
            system_prepare_fw_start();

            kernel_entry = (void*)0x01008000;
            ret = kernel_entry();
            printf("FAILED to boot OF");
        }
    }

    printf("Loading Rockbox firmware...");

    loadbuffer = (unsigned char*)CONFIG_SDRAM_START;
    buffer_size = 0x1000000;

    ret = load_firmware(loadbuffer, BOOTFILE, buffer_size);

    if(ret <= EFILE_EMPTY)
    {
        error(EBOOTFILE, ret, true);
    }
    else
    {
        system_prepare_fw_start();

        kernel_entry = (void*) loadbuffer;
        ret = kernel_entry();
        printf("FAILED!");
    }
    
    storage_sleepnow();
    
    while(1);
}
