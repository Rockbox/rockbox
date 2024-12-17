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
#ifdef HAVE_SERIAL
#include "serial.h"
#endif

#include "s5l87xx.h"
#include "clocking-s5l8702.h"
#include "spi-s5l8702.h"
#include "i2c-s5l8702.h"
#include "gpio-s5l8702.h"
#include "pmu-target.h"
#if defined(IPOD_6G) || defined(IPOD_NANO3G)
#include "norboot-target.h"
#endif


#define ERR_RB      0
#define ERR_OF      1
#define ERR_STORAGE 2

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

#ifndef S5L87XX_DEVELOPMENT_BOOTLOADER
#ifdef HAVE_BOOTLOADER_USB_MODE
static void usb_mode(void)
{
    int button;

    verbose = true;

    printf("Entering USB mode...");

    powermgmt_init();

    /* The code will ask for the maximum possible value */
    usb_charging_enable(USB_CHARGING_ENABLE);

    usb_init();
    usb_start_monitoring();

    /* Wait until USB is plugged */
    while (usb_detect() != USB_INSERTED)
    {
        printf("Plug USB cable");
        line--;
        sleep(HZ/10);
    }

    while(1)
    {
        button = button_get_w_tmo(HZ/10);

        if (button == SYS_USB_CONNECTED)
            break; /* Hit */

        if (usb_detect() == USB_EXTRACTED)
            break; /* Cable pulled */

        /* Wait for threads to connect or cable is pulled */
        printf("USB: Connecting...");
        line--;
    }

    if (button == SYS_USB_CONNECTED)
    {
        /* Got the message - wait for disconnect */
        printf("Bootloader USB mode");

        /* Ack the SYS_USB_CONNECTED polled from the button queue */
        usb_acknowledge(SYS_USB_CONNECTED_ACK);

        while(1)
        {
            button = button_get_w_tmo(HZ/2);
            if (button == SYS_USB_DISCONNECTED)
                break;
        }
    }

    /* We don't want the HDD to spin up if the USB is attached again */
    usb_close();
    printf("USB mode exit     ");
}
#endif /* HAVE_BOOTLOADER_USB_MODE */

void fatal_error(int err)
{
    verbose = true;

    /* System font is 6 pixels wide */
    line++;
    switch (err)
    {
        case ERR_RB:
#ifdef HAVE_BOOTLOADER_USB_MODE
            usb_mode();
            printf("Hold MENU+SELECT to reboot");
            break;
#endif
        case ERR_STORAGE:
            printf("Hold MENU+SELECT to reboot");
            printf("then SELECT+PLAY for disk mode");
            break;
        case ERR_OF:
            printf("Hold MENU+SELECT to reboot");
            printf("and enter Rockbox firmware");
            break;
    }

#if (CONFIG_STORAGE & STORAGE_ATA)
    if (ide_powered())
        ata_sleepnow(); /* Immediately spindown the disk. */
#endif

    line++;
    lcd_set_foreground(LCD_REDORANGE);
    while (1) {
        lcd_puts(0, line, button_hold() ? "Hold switch on!"
                                        : "               ");
        lcd_update();
    }
}

#if (CONFIG_STORAGE & STORAGE_ATA)
static void battery_trap(void)
{
    int vbat, old_verb;
    int th = 50;

    old_verb = verbose;
    verbose = true;

    usb_charging_maxcurrent_change(100);

    while (1)
    {
        vbat = _battery_voltage();

        /*  Two reasons to use this threshold (may require adjustments):
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

        if (power_input_status() != POWER_INPUT_NONE) {
            lcd_set_foreground(LCD_RBYELLOW);
            printf("Low battery: %d mV, charging...     ", vbat);
            sleep(HZ*3);
        }
        else {
            /* Wait for the user to insert a charger */
            int tmo = 10;
            lcd_set_foreground(LCD_REDORANGE);
            while (1) {
                vbat = _battery_voltage();
                printf("Low battery: %d mV, power off in %d ", vbat, tmo);
                if (!tmo--) {
                    /* Raise Vsysok (hyst=0.02*Vsysok) to avoid PMU
                       standby<->active looping */
                    if (vbat < 3200)
                        pmu_write(PCF5063X_REG_SVMCTL, 0xA /*3200mV*/);
                    power_off();
                }
                sleep(HZ*1);
                if (power_input_status() != POWER_INPUT_NONE)
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
#endif /* CONFIG_STORAGE & STORAGE_ATA */
#endif /* S5L87XX_DEVELOPMENT_BOOTLOADER */

static int launch_onb(int clkdiv)
{
#if defined(IPOD_6G) || defined(IPOD_NANO3G)
    /* SPI clock = PClk/(clkdiv+1) */
    spi_clkdiv(SPI_PORT, clkdiv);

    /* Actually IRAM1_ORIG contains current RB bootloader IM3 header,
       it will be replaced by ONB IM3 header, so this function must
       be called once!!! */
    struct Im3Info *hinfo = (struct Im3Info*)IRAM1_ORIG;

    /* Loads ONB in IRAM0, exception vector table is destroyed !!! */
    int rc = im3_read(
            NORBOOT_OFF + im3_nor_sz(hinfo), hinfo, (void*)IRAM0_ORIG);

    if (rc != 0) {
        /* Restore exception vector table */
        memcpy((void*)IRAM0_ORIG, &_movestart, 4*(&start_loc-&_movestart));
        commit_discard_idcache();
        return rc;
    }

    /* Disable all external interrupts */
    eint_init();

    commit_discard_idcache();

    /* Branch to start of IRAM */
    asm volatile("mov pc, %0"::"r"(IRAM0_ORIG));
    while(1);
#elif defined(IPOD_NANO4G)
    (void) clkdiv;

    lcd_set_foreground(LCD_REDORANGE);
    printf("Not implemented");

    line++;
    lcd_set_foreground(LCD_RBYELLOW);
    printf("Press SELECT to continue");
    while (button_status() != BUTTON_SELECT)
        sleep(HZ/100);

    return 0;
#endif
}

/* Launch OF when kernel mode is running */
static int kernel_launch_onb(void)
{
    disable_irq();
    int rc = launch_onb(3); /* 54/4 = 13.5 MHz. */
    enable_irq();
    return rc;
}

/*  The boot sequence is executed on power-on or reset. After power-up
 *  the device could come from a state of hibernation, OF hibernates
 *  the iPod after an inactive period of ~30 minutes, on this state the
 *  SDRAM is in self-refresh mode.
 *
 *  t0 = 0
 *     S5L8702 BOOTROM loads an IM3 image located at NOR:
 *     - IM3 header (first 0x800 bytes) is loaded at IRAM1_ORIG
 *     - IM3 body (decrypted RB bootloader) is loaded at IRAM0_ORIG
 *     The time needed to load the RB bootloader (~100 Kb) is estimated
 *     on 200~250 ms. Once executed, RB booloader moves itself from
 *     IRAM0_ORIG to IRAM1_ORIG+0x800, preserving current IM3 header
 *     that contains the NOR offset where the ONB (original NOR boot),
 *     is located (see dualboot.c for details).
 *
 *  t1 = ~250 ms.
 *     If the PMU is hibernated, decrypted ONB (size 128Kb) is loaded
 *       and executed, it takes ~120 ms. Then the ONB restores the
 *       iPod to the state prior to hibernation.
 *     If not, initialize system and RB kernel, wait for t2.
 *
 *  t2 = ~650 ms.
 *     Check user button selection.
 *     If OF, diagmode, or diskmode is selected then launch ONB.
 *     If not, wait for LCD initialization.
 *
 *  t3 = ~700,~900 ms. (lcd_type_01,lcd_type_23)
 *     LCD is initialized, baclight ON.
 *     Wait for HDD spin-up.
 *
 *  t4 = ~2600,~2800 ms.
 *     HDD is ready.
 *     If hold switch is locked, then load and launch ONB.
 *     If not, load rockbox.ipod file from HDD.
 *
 *  t5 = ~2800,~3000 ms.
 *     rockbox.ipod is executed.
 */

#ifdef S5L87XX_DEVELOPMENT_BOOTLOADER
#include "piezo.h"
#include "lcd-s5l8702.h"
extern int lcd_type;

static uint16_t alive[] = { 500,100,0, 0 };
static uint16_t alivelcd[] = { 2000,200,0, 0 };

static void sleep_test(void)
{
#ifndef IPOD_6G
    int sleep_tmo = 5;
    int awake_tmo = 3;

    lcd_clear_display();
    lcd_set_foreground(LCD_WHITE);
    line = 0;

    printf("Entering LCD sleep mode in %d seconds,", sleep_tmo);
    printf("during sleep mode you will see a white");
    printf("screen for about %d seconds.", awake_tmo);
    while (sleep_tmo--) {
        printf("Sleep in %d...", sleep_tmo);
        sleep(HZ*1);
    }
    lcd_sleep();
    sleep(HZ*awake_tmo);
    lcd_awake();

    line++;
    printf("Awake!");

    line++;
    lcd_set_foreground(LCD_RBYELLOW);
    printf("Press SELECT to continue");
    while (button_status() != BUTTON_SELECT)
        sleep(HZ/100);
#endif
}

static void pmu_info(void)
{
    int loop = 0;

    lcd_clear_display();
    lcd_update();
    while (button_status() != BUTTON_NONE);

    while (1)
    {
        lcd_set_foreground(LCD_WHITE);
        lcd_clear_display();
        line = 0;
        printf("loop: %d", loop++);

        for (int i = 0; i < 128; i += 8)
        {
            unsigned char buf[8];

#if defined(IPOD_NANO3G)
            if (i == 0) {
                static int flip = 0;
                if (flip) {
                    pmu_write(6, 0xff);
                    pmu_write(7, 0xff);
                }
                else {
                    pmu_write(6, 0xe7);
                    pmu_write(7, 0xfe);
                }
                flip ^= 1;
            }
#elif defined(IPOD_NANO4G)
            if (i == 120)
                for (int j = 0; j < 8; j++)
                    pmu_write(i+j, j);
#endif
            for (int j = 0; j < 8; j++)
                buf[j] = pmu_read(i+j);

            printf(" %2x: %2x %2x %2x %2x %2x %2x %2x %2x", i,
                    buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
        }
        line++;
        printf("USB: %s    ", (usb_detect() == USB_INSERTED) ? "inserted" : "not inserted");
#if CONFIG_CHARGING
        printf("Firewire: %s    ", pmu_firewire_present() ? "inserted" : "not inserted");
#endif
#ifdef IPOD_ACCESSORY_PROTOCOL
        printf("Accessory: %s    ", pmu_accessory_present() ? "inserted" : "not inserted");
#endif
        printf("Hold Switch: %s  ", pmu_holdswitch_locked() ? "locked" : "unlocked");
        line++;
        lcd_set_foreground(LCD_RBYELLOW);
        printf("Press SELECT to continue");
        if (button_status() == BUTTON_SELECT)
            break;
        sleep(HZ/2);
    }
}

static void gpio_info(void)
{
    int loop = 0;

    lcd_clear_display();

    while (1)
    {
        lcd_set_foreground(LCD_WHITE);
        lcd_clear_display();
        line = 0;
        printf("loop: %d", loop++);
        for (int i = 0; i < GPIO_N_GROUPS; i ++)
        {
            printf(" %x: %8x %2x %4x %2x %2x", i,
                    PCON(i), PDAT(i), PUNA(i), PUNB(i), PUNC(i));
        }
        line++;
        lcd_set_foreground(LCD_RBYELLOW);
        printf("Press SELECT to continue");
        if (button_status() == BUTTON_SELECT)
            break;
        sleep(HZ/5);
    }
}

static void run_of(void)
{
    int tmo = 5;
    lcd_clear_display();
    lcd_set_foreground(LCD_WHITE);
    line = 0;
    while (tmo--) {
        printf("Booting OF in %d...", tmo);
        sleep(HZ*1);
    }
    kernel_launch_onb();
}

static void print_syscfg(void)
{
    lcd_clear_display();
    lcd_set_foreground(LCD_WHITE);
    line = 0;

    struct SysCfg syscfg;
    const ssize_t result = syscfg_read(&syscfg);

    if (result == -1) {
        printf("SCfg magic not found. NOR flash is corrupted.");
        goto end;
    }

    printf("Total size: %lu bytes, %lu entries", syscfg.header.size, syscfg.header.num_entries);

    if (result > 0) {
        printf("Wrong size: expected %ld, got %lu", result, syscfg.header.size);
    }

    if (syscfg.header.num_entries > SYSCFG_MAX_ENTRIES) {
        printf("Too many entries, showing only first %u", SYSCFG_MAX_ENTRIES);
    }

    const size_t syscfg_num_entries = MIN(syscfg.header.num_entries, SYSCFG_MAX_ENTRIES);

    for (size_t i = 0; i < syscfg_num_entries; i++) {
        const struct SysCfgEntry* entry = &syscfg.entries[i];
        const char* tag = (char *)&entry->tag;
        const uint32_t* data32 = (uint32_t *)entry->data;

        switch (entry->tag) {
        case SYSCFG_TAG_SRNM:
            printf("Serial number (SrNm): %s", entry->data);
            break;
        case SYSCFG_TAG_FWID:
            printf("Firmware ID (FwId): %07lX", data32[1] & 0x0FFFFFFF);
            break;
        case SYSCFG_TAG_HWID:
            printf("Hardware ID (HwId): %08lX", data32[0]);
            break;
        case SYSCFG_TAG_HWVR:
            printf("Hardware version (HwVr): %06lX", data32[1]);
            break;
        case SYSCFG_TAG_CODC:
            printf("Codec (Codc): %s", entry->data);
            break;
        case SYSCFG_TAG_SWVR:
            printf("Software version (SwVr): %s", entry->data);
            break;
        case SYSCFG_TAG_MLBN:
            printf("Logic board serial number (MLBN): %s", entry->data);
            break;
        case SYSCFG_TAG_MODN:
            printf("Model number (Mod#): %s", entry->data);
            break;
        case SYSCFG_TAG_REGN:
            printf("Sales region (Regn): %08lX %08lX", data32[0], data32[1]);
            break;
        default:
            printf("%c%c%c%c: %08lX %08lX %08lX %08lX",
                tag[3], tag[2], tag[1], tag[0],
                data32[0], data32[1], data32[2], data32[3]
            );
            break;
        }
    }

end:
    line++;
    lcd_set_foreground(LCD_RBYELLOW);
    printf("Press SELECT to continue");
    while (button_status() != BUTTON_SELECT)
        sleep(HZ/100);
}

static void devel_menu(void)
{
    while (1)
    {
        lcd_clear_display();
        lcd_set_foreground(LCD_RBYELLOW);
        line = 0;
        printf("Select action:");
        printf(" <MENU>    Show SysCfg");
        printf(" <LEFT>    LCD sleep/awake test");
        printf(" <SELECT>  PMU info");
        printf(" <RIGHT>   GPIO info");
        printf(" <PLAY>    Launch OF");

        while (button_status() != BUTTON_NONE);

        bool done = false;
        while (!done)
        {
            switch (button_status())
            {
                case BUTTON_MENU:
                    print_syscfg();
                    done = true;
                    break;
                case BUTTON_LEFT:
                    sleep_test();
                    done = true;
                    break;
                case BUTTON_SELECT:
                    pmu_info();
                    done = true;
                    break;
                case BUTTON_RIGHT:
                    gpio_info();
                    done = true;
                    break;
                case BUTTON_PLAY:
                {
                    run_of();
                    done = true;
                    break;
                }
                default:
                    sleep(HZ/100);
                    break;
            }
        }
    }
}
#endif /* S5L87XX_DEVELOPMENT_BOOTLOADER */

void main(void)
{
    int rc = 0;

#ifndef S5L87XX_DEVELOPMENT_BOOTLOADER
    unsigned char *loadbuffer;
    int (*kernel_entry)(void);
#endif

    usec_timer_init();

#ifdef S5L87XX_DEVELOPMENT_BOOTLOADER
    piezo_seq(alive);
#endif

    /* Configure I2C0 */
    i2c_preinit(0);

    if (pmu_is_hibernated()) {
        rc = launch_onb(1); /* 27/2 = 13.5 MHz. */
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

#ifdef HAVE_SERIAL
    serial_setup();
#endif

    button_init();
    if (rc == 0) {
        /* User button selection timeout */
        while (USEC_TIMER < 400000);
        int btn = button_read_device();
        /* This prevents HDD spin-up when the user enters DFU */
        if (btn == (BUTTON_SELECT|BUTTON_MENU)) {
            while (button_read_device() == (BUTTON_SELECT|BUTTON_MENU))
                sleep(HZ/10);
            sleep(HZ);
            btn = button_read_device();
        }
        /* Enter OF, diagmode and diskmode using ONB */
        if ((btn == BUTTON_MENU)
                || (btn == (BUTTON_SELECT|BUTTON_LEFT))
                || (btn == (BUTTON_SELECT|BUTTON_PLAY))) {
            rc = kernel_launch_onb();
        }
    }

    lcd_init();
    lcd_set_foreground(LCD_WHITE);
    lcd_set_background(LCD_BLACK);
    lcd_clear_display();
    font_init();
    lcd_setfont(FONT_SYSFIXED);

    // TODO: see if removing this causes the nano3g LCD to initialize properly
#ifdef S5L87XX_DEVELOPMENT_BOOTLOADER
    sleep(HZ);
    for (int i = 0; i < lcd_type+1; i++) {
        sleep(HZ/2);
        piezo_seq(alivelcd);
    }
#endif

    lcd_update();
    sleep(HZ/40);  /* wait for lcd update */

    verbose = true;

    printf("Rockbox boot loader");
    printf("Version: %s", rbversion);

    backlight_init(); /* Turns on the backlight */

#ifdef S5L87XX_DEVELOPMENT_BOOTLOADER
    line++;
    printf("lcd type: %d", lcd_type);
#ifdef S5L_LCD_WITH_READID
    extern unsigned char lcd_id[4];
    printf("lcd id: 0x%x", *((uint32_t*)&lcd_id[0]));
#endif
#ifdef IPOD_NANO4G
    printf("boot cfg: 0x%x", pmu_read(0x7f));
#endif
    line++;
    printf("Press SELECT to continue");
    while (button_status() != BUTTON_SELECT)
        sleep(HZ/100);

    devel_menu();
#endif /* S5L87XX_DEVELOPMENT_BOOTLOADER */

#ifndef S5L87XX_DEVELOPMENT_BOOTLOADER
    if (rc == 0) {
#if (CONFIG_STORAGE & STORAGE_ATA)
        /* Wait until there is enought power to spin-up HDD */
        battery_trap();
#endif

        rc = storage_init();
        if (rc != 0) {
            printf("Storage error: %d", rc);
            fatal_error(ERR_STORAGE);
        }

        filesystem_init();

        /* We wait until HDD spins up to check for hold button */
        if (button_hold()) {
            printf("Executing OF...");
#if (CONFIG_STORAGE & STORAGE_ATA)
            ata_sleepnow();
#endif
            rc = kernel_launch_onb();
        }
    }

    if (rc != 0) {
        printf("Load OF error: %d", rc);
        fatal_error(ERR_OF);
    }

#ifdef HAVE_BOOTLOADER_USB_MODE
    /* Enter USB mode if SELECT+RIGHT are pressed */
    if (button_read_device() == (BUTTON_SELECT|BUTTON_RIGHT)) {
#if defined(MAX_VIRT_SECTOR_SIZE) && defined(DEFAULT_VIRT_SECTOR_SIZE)
#ifdef HAVE_MULTIDRIVE
            for (int i = 0 ; i < NUM_DRIVES ; i++)
#endif
                disk_set_sector_multiplier(IF_MD(i,) DEFAULT_VIRT_SECTOR_SIZE/SECTOR_SIZE);
#endif
        usb_mode();
    }
#endif

    rc = disk_mount_all();
    if (rc <= 0) {
        printf("No partition found");
        fatal_error(ERR_RB);
    }

    printf("Loading Rockbox...");
    loadbuffer = (unsigned char *)DRAM_ORIG;
    rc = load_firmware(loadbuffer, BOOTFILE, MAX_LOADSIZE);

    if (rc <= EFILE_EMPTY) {
        printf("Error!");
        printf("Can't load " BOOTFILE ": ");
        printf(loader_strerror(rc));
        fatal_error(ERR_RB);
    }

    printf("Rockbox loaded.");

    /* If we get here, we have a new firmware image at 0x08000000, run it */
    disable_irq();

    kernel_entry = (void*) loadbuffer;
    commit_discard_idcache();
    rc = kernel_entry();

    /* End stop - should not get here */
    enable_irq();
    printf("ERR: Failed to boot");
    while(1);
#endif
}
