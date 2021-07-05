/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id: $
*
* Copyright (C) 2011-2021 by Tomasz Moń
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
#include "usb.h"

#define FLASH_BASE              0x00100000
#define PARAMETERS_FLASH_OFFSET 0x00010000
#define PARAMETERS_SIZE_BYTES   0x00010000
#define PARAMETERS_NUM          32

#define FLASH_WRITE(addr, val)  *(volatile uint16_t *)(FLASH_BASE + addr) = val
#define FLASH_READ(addr)        *(volatile uint16_t *)(FLASH_BASE + addr)

#define PARAMETER_TYPE_BINARY   0xF00FB00B
#define PARAMETER_TYPE_LONGBIN  0xBEADFEAD
#define PARAMETER_TYPE_STRING   0xBEEFFACE

typedef struct
{
    uint32_t magic;
    char name[60];
    char value[192];
} parameter_t;

/* Cache all parameters because parameters are stored on a single erase block */
static union
{
    parameter_t entry[PARAMETERS_NUM];
    /* raw consists of parameter_t array and bootloader graphics */
    uint16_t raw[PARAMETERS_SIZE_BYTES/sizeof(uint16_t)];
} parameters;

static void parameters_load_from_flash(void)
{
    uint32_t offset = PARAMETERS_FLASH_OFFSET;
    uint16_t *dst = parameters.raw;

    while (offset < (PARAMETERS_FLASH_OFFSET + PARAMETERS_SIZE_BYTES))
    {
        *dst++ = FLASH_READ(offset);
        offset += sizeof(uint16_t);
    }
}

void parameters_erase(void) __attribute__ ((section(".icode")));
void parameters_erase(void)
{
    uint32_t offset = PARAMETERS_FLASH_OFFSET;

    while (offset < (PARAMETERS_FLASH_OFFSET + PARAMETERS_SIZE_BYTES))
    {
        if (FLASH_READ(offset) != 0xFFFF)
        {
            /* Found programmed halfword */
            break;
        }
        offset += sizeof(uint16_t);
    }

    if (offset >= (PARAMETERS_FLASH_OFFSET + PARAMETERS_SIZE_BYTES))
    {
        /* Flash is already erased */
        return;
    }

    /* Execute Block Erase sequence */
    FLASH_WRITE(0xAAA, 0xAA);
    FLASH_WRITE(0x554, 0x55);
    FLASH_WRITE(0xAAA, 0x80);
    FLASH_WRITE(0xAAA, 0xAA);
    FLASH_WRITE(0x554, 0x55);
    FLASH_WRITE(PARAMETERS_FLASH_OFFSET, 0x30);

    /* Erase finishes once we read 0xFFFF on previously programmed halfword
     * Typical block erase time is 0.7 s, maximum 15 s. Do not check the
     * timeout here because we have to wait until the erase finishes as most
     * of Rockbox bootloader code executes from flash.
     */
    do
    {
        /* Discard caches to force reads from memory */
        commit_discard_idcache();
    }
    while (FLASH_READ(offset) != 0xFFFF);
}

void parameters_write_to_flash(void) __attribute__ ((section(".icode")));
void parameters_write_to_flash(void)
{
    uint16_t *src = parameters.raw;
    uint32_t offset = PARAMETERS_FLASH_OFFSET;

    while (offset < (PARAMETERS_FLASH_OFFSET + PARAMETERS_SIZE_BYTES))
    {
        if (FLASH_READ(offset) != *src)
        {
            /* Program halfword */
            FLASH_WRITE(0xAAA, 0xAA);
            FLASH_WRITE(0x554, 0x55);
            FLASH_WRITE(0xAAA, 0xA0);
            FLASH_WRITE(offset, *src);

            /* Word programming time typical is 14 us, maximum 330 us */
            do
            {
                /* Discard caches to force reads from memory */
                commit_discard_idcache();
            }
            while (FLASH_READ(offset) != *src);
        }
        offset += sizeof(uint16_t);
        src++;
    }
}

static void clear_recoverzap(void)
{
    int i;
    bool needs_reflash = false;

    parameters_load_from_flash();
    for (i = 0; i < PARAMETERS_NUM; i++)
    {
        if ((parameters.entry[i].magic == PARAMETER_TYPE_STRING) &&
            (!strcmp("recoverzap", parameters.entry[i].name)))
        {
            memset(&parameters.entry[i], 0xFF, sizeof(parameter_t));
            needs_reflash = true;
        }
    }

    if (needs_reflash)
    {
        int cpsr = disable_interrupt_save(IRQ_FIQ_STATUS);
        printf("Erasing OF parameters memory");
        parameters_erase();
        printf("Flashing OF parameters");
        parameters_write_to_flash();
        printf("Cleared recoverzap parameter");
        restore_interrupt(cpsr);
    }
}

static void handle_usb(int connect_timeout)
{
    long end_tick = 0;

    if (usb_detect() != USB_INSERTED)
    {
        return;
    }

    usb_init();
    usb_start_monitoring();

    printf("USB: Connecting");

    if (connect_timeout != TIMEOUT_BLOCK)
    {
        end_tick = current_tick + connect_timeout;
    }

    while (usb_detect() == USB_INSERTED)
    {
        if (button_get_w_tmo(HZ/2) == SYS_USB_CONNECTED)
        {
            printf("Bootloader USB mode");
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            while (button_get_w_tmo(HZ/2) != SYS_USB_DISCONNECTED)
            {
                storage_spin();
            }
            break;
        }

        if (connect_timeout != TIMEOUT_BLOCK &&
            TIME_AFTER(current_tick, end_tick))
        {
            printf("USB: Timed out");
            break;
        }
    }

    usb_close();
}

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

    lcd_enable(true);
    lcd_setfont(FONT_SYSFIXED);
    reset_screen();
    show_logo();

    printf("Rockbox boot loader");
    printf("Version %s", rbversion);

    clear_recoverzap();

    ret = storage_init();
    if(ret)
        printf("SD error: %d", ret);

    filesystem_init();

    handle_usb(2*HZ);

    ret = disk_mount_all();
    if (ret <= 0)
        error(EDISK, ret, true);

    btn = button_read_device();

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
            lcd_enable(false);
            system_prepare_fw_start();

            kernel_entry = (void*)0x01008000;
            ret = kernel_entry();
            lcd_enable(true);
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
        lcd_enable(false);
        system_prepare_fw_start();

        kernel_entry = (void*) loadbuffer;
        ret = kernel_entry();
        lcd_enable(true);
        printf("FAILED!");
    }

    storage_sleepnow();

    while(1);
}
