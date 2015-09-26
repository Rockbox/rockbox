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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "config.h"

#include "inttypes.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "../kernel-internal.h"
#include "file_internal.h"
#include "storage.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"
#include "rb-loader.h"
#include "loader_strerror.h"
#include "version.h"
#include "powermgmt.h"
#include "usb.h"

#include "s5l8702.h"
#include "clocking-s5l8702.h"
#include "spi-s5l8702.h"
#include "i2c-s5l8702.h"
#include "pmu-target.h"
#include "crypto-s5l8702.h"
#include "cwheel-s5l8702.h"


#undef DEBUG_ALIVE
#ifdef DEBUG_ALIVE
#include "piezo.h"
/* tone sequences: period (uS), duration (ms), silence (ms) */
static uint16_t alive[] = { 500,100,0, 0 };
#endif


#define FW_ROCKBOX  0
#define FW_APPLE    1

/* Safety measure - maximum allowed firmware image size.
   The largest known current (October 2009) firmware is about 6.2MB so
   we set this to 8MB.
*/
#define MAX_LOADSIZE (8*1024*1024)

#define LCD_RBYELLOW    LCD_RGBPACK(255,192,0)
#define LCD_REDORANGE   LCD_RGBPACK(255,70,0)

extern void bss_init(void);
extern uint32_t _movestart;
extern uint32_t start_loc;

extern int line;

void fatal_error(int fw)
{
    verbose = true;

    if (ide_powered())
        ata_sleepnow(); /* Immediately spindown the disk. */

    /* System font is 6 pixels wide */
    line++;
    printf("Hold MENU+SELECT to reboot");
    if (fw == FW_ROCKBOX)
        printf("then SELECT+PLAY for disk mode");
    else
        printf("into Rockbox firmware");
    line++;
    lcd_set_foreground(LCD_REDORANGE);
    while (1) {
        lcd_puts(0, line, button_hold() ? "Hold switch on!"
                                        : "               ");
        lcd_update();
    }
}

static unsigned int get_power_input_status(void)
{
    unsigned int power_input = POWER_INPUT_NONE;

    if (!(PDAT(12) & 8))
        power_input |= POWER_INPUT_USB_CHARGER;

    if (pmu_read(PCF5063X_REG_MBCS1) & 4)
        power_input |= POWER_INPUT_MAIN_CHARGER;

    return power_input;
}

static void battery_trap(void)
{
    int vbat, old_verb;
    int th = 50;

    old_verb = verbose;
    verbose = true;

    usb_charging_maxcurrent_change(100);

    while (1)
    {
        vbat = pmu_read_battery_voltage();

        /*  two reasons to use this threshold (may require adjustments):
         *  - when USB (or wall adaptor) is plugged/unplugged, Vbat readings
         *    differ as much as more than 200 mV when charge current is at
         *    maximum (~340 mA).
         *  - RB uses some sort of average/compensation for battery voltage
         *    measurements, battery icon blinks at battery_level_dangerous,
         *    when the HDD is used heavily (large database) the level drops
         *    to battery_level_shutoff quickly.
         */
        if (vbat >= battery_level_dangerous[0] + th)
            break;
        th = 200;

        if (get_power_input_status() != POWER_INPUT_NONE) {
            lcd_set_foreground(LCD_RBYELLOW);
            printf("Low battery: %d mV, charging...     ", vbat);
            sleep(HZ*3);
        }
        else {
            /* wait for the user to insert a charger */
            int tmo = 10;
            lcd_set_foreground(LCD_REDORANGE);
            while (1) {
                vbat = pmu_read_battery_voltage();
                printf("Low battery: %d mV, power off in %d ", vbat, tmo);
                if (!tmo--) {
                    /* raise Vsysok (hyst=0.02*Vsysok) to avoid PMU
                       standby<->active looping */
                    if (vbat < 3200)
                        pmu_write(PCF5063X_REG_SVMCTL, 0xA /*3200mV*/);
                    power_off();
                }
                sleep(HZ*1);
                if (get_power_input_status() != POWER_INPUT_NONE)
                    break;
                line--;
            }
        }
        line--;
    }

    verbose = old_verb;
    lcd_set_foreground(LCD_WHITE);
    printf("Battery status ok: %d mV            ", vbat);
}

static uint32_t get_uint32le(unsigned char *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static int launch_onb(void)
{
    /* actually IRAM1_ORIG contains current RB bootloader IM3 header,
       it will be replaced by ONB IM3 header. */
    struct Im3Info *hinfo = (struct Im3Info*)IRAM1_ORIG;
    uint32_t bl_nor_sz = ALIGN_UP(IM3HDR_SZ +
                            get_uint32le(hinfo->data_sz), 0x1000);

    /* loads ONB in IRAM0, exception vector table is destroyed !!! */
    int rc = read_im3(NORBOOT_OFF + bl_nor_sz, hinfo, (void*)IRAM0_ORIG);
    if (rc != 0) {
        /* restore exception vector table */
        memcpy((void*)IRAM0_ORIG, &_movestart, 4*(&start_loc-&_movestart));
        commit_discard_idcache();
        return rc;
    }

    commit_discard_idcache();
    /* Branch to start of IRAM */
    asm volatile("mov pc, %0"::"r"(IRAM0_ORIG));
    while(1);
}

static bool pmu_is_hibernated(void)
{
    PCON3 = (PCON3 & ~0x00000ff0) | 0x00000220;
    i2c_preinit(0);

    uint8_t oocshdwn = pmu_rd(PCF5063X_REG_OOCSHDWN);
    /* get (previously) configured output selection for GPIO3 */
    bool gpio3out = (pmu_rd(PCF5063X_REG_GPIO3CFG) & 7);
    /* restore OCCSHDWN status, ONB needs it */
    pmu_wr(PCF5063X_REG_OOCSHDWN, oocshdwn);

    return !(oocshdwn & PCF5063X_OOCSHDWN_COLDBOOT) && !gpio3out;
}

/*  The boot sequence is executed on power-on or reset. After power-up
 *  the device could come from a state of hibernation, OF hibernates
 *  the iPod after an inactive period of ~30 minutes (FW 1.1.2), on
 *  this state the SDRAM is in self-refresh mode.
 *
 *  t0 = 0
 *     S5L8702 BOOTROM loads an IM3 image located at NOR:
 *     - IM3 header (first 0x800 bytes) is loaded at IRAM1_ORIG
 *     - IM3 body (decrypted RB bootloader) is loaded at IRAM0_ORIG
 *     The time needed to load the RB bootloader (~90 Kb) is estimated
 *     on 200~250 ms. Once executed, RB booloader moves itself from
 *     IRAM0_ORIG to IRAM1_ORIG+0x800, preserving current IM3 header
 *     that contains the NOR offset where the ONB (original NOR boot),
 *     is located (see dualboot.c for details).
 *
 *  t1 = ~250 ms.
 *     If the PMU is hibernated, decrypted ONB (size 128Kb) is loaded
 *       and executed, it takes ~350 ms. Then the ONB restores the
 *       iPod to the state prior to hibernation.
 *     If not, wait 500 ms. for user button selection.
 *
 *  t2 = ~750 ms.
 *     Check user button selection.
 *     If OF, diagmode, or diskmode is selected then launch ONB.
 *     If not, initialize SDRAM, BSS, launch kernel, and init HW.
 *
 *  t3 = ~1100 ms.
 *     HW initialization ends.
 *     Wait for HDD spin-up.
 *
 *  t4 = ~3000 ms.
 *     HDD is ready.
 *     If hold switch is locked, then load and launch ONB.
 *     If not, load rockbox.ipod file from HDD.
 *
 *  t5 = ~3200 ms.
 *     rockbox.ipod is executed.
 */
void main(void)
{
    int fw = FW_ROCKBOX;
    int rc = 0;
    unsigned char *loadbuffer;
    int (*kernel_entry)(void);

    usec_timer_init();
#ifdef DEBUG_ALIVE
    piezo_seq(alive);
#endif
    /* initialize GPIO for external wheel controller ASAP */
    cwheel_set_gpio(CWMODE_MAN);

    if (pmu_is_hibernated())
        fw = FW_APPLE;

    if (fw == FW_ROCKBOX) {
        /* user button selection timeout */
        while (USEC_TIMER < 500000);
        int btn = read_all_buttons(3);
        /* this prevents HDD spin-up when the user enters DFU */
        if (btn == (BUTTON_SELECT|BUTTON_MENU)) {
            while (read_all_buttons(3) == (BUTTON_SELECT|BUTTON_MENU))
                udelay(100000);
            int32_t stop_time = USEC_TIMER + 1000000;
            while ((int32_t)USEC_TIMER - stop_time < 0);
            btn = read_all_buttons(3);
        }
        /* enter OF, diagmode and diskmode using ONB */
        if ((btn == BUTTON_MENU)
                || (btn == (BUTTON_SELECT|BUTTON_LEFT))
                || (btn == (BUTTON_SELECT|BUTTON_PLAY)))
            fw = FW_APPLE;
    }

    if (fw == FW_APPLE) {
        spi_clkdiv(SPI_PORT, 4); /* SPI clock = 27/5 MHz. */
        rc = launch_onb(); /* only returns on error */
    }

    system_preinit();
    memory_init();
    /*
     * XXX: BSS is initialized here, do not use .bss before this line
     */
    bss_init();

    system_init();
    kernel_init();
    i2c_init();
    power_init();

    enable_irq();

    lcd_init();
    lcd_set_foreground(LCD_WHITE);
    lcd_set_background(LCD_BLACK);
    lcd_clear_display();
    lcd_update();
    font_init();
    lcd_setfont(FONT_SYSFIXED);

    button_init();

    sleep(HZ/30); /* waits for lcd_update() to finish */
    backlight_init(); /* Turns on the backlight */

    verbose = true;

    printf("Rockbox boot loader");
    printf("Version: %s", rbversion);

    if (rc != 0) {
        printf("Load OF error: %d", rc);
        fatal_error(fw);
    }

    /* Wait until there is enought power to spin-up HDD */
    battery_trap();

    rc = storage_init();
    if (rc != 0) {
        printf("ATA error: %d", rc);
        fatal_error(fw);
    }

    if (button_hold()) {
        fw = FW_APPLE;
        ata_sleepnow();
        spi_clkdiv(SPI_PORT, 9); /* SPI clock = 54/10 MHz. */
        disable_irq();
        rc = launch_onb(); /* only returns on error */
        enable_irq();
        printf("Load OF error: %d", rc);
        fatal_error(fw);
    }

    filesystem_init();

    rc = disk_mount_all();
    if (rc <= 0) {
        printf("No partition found");
        fatal_error(fw);
    }

    printf("Loading Rockbox...");
    loadbuffer = (unsigned char *)DRAM_ORIG;
    rc = load_firmware(loadbuffer, BOOTFILE, MAX_LOADSIZE);

    if (rc <= EFILE_EMPTY) {
        printf("Error!");
        printf("Can't load " BOOTFILE ": ");
        printf(loader_strerror(rc));
        fatal_error(fw);
    }

    printf("Rockbox loaded.");

    /* If we get here, we have a new firmware image at 0x08000000, run it */
    printf("Executing...");

    disable_irq();

    kernel_entry = (void*) loadbuffer;
    commit_discard_idcache();
    rc = kernel_entry();

    /* end stop - should not get here */
    enable_irq();
    printf("ERR: Failed to boot");
    while(1);
}
