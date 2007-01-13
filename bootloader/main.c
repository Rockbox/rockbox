/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include "lcd-remote.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "usb.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "uda1380.h"
#include "eeprom_settings.h"

#include "pcf50606.h"

#include <stdarg.h>

#define DRAM_START 0x31000000

#ifdef HAVE_EEPROM_SETTINGS
static bool recovery_mode = false;
#endif

int line = 0;
#ifdef HAVE_REMOTE_LCD
int remote_line = 0;
#endif

int usb_screen(void)
{
   return 0;
}

char version[] = APPSVERSION;

char printfbuf[256];

void reset_screen(void)
{
    lcd_clear_display();
    line = 0;
#ifdef HAVE_REMOTE_LCD
    lcd_remote_clear_display();
    remote_line = 0;
#endif
}

void printf(const char *format, ...)
{
    int len;
    unsigned char *ptr;
    va_list ap;
    va_start(ap, format);

    ptr = printfbuf;
    len = vsnprintf(ptr, sizeof(printfbuf), format, ap);
    va_end(ap);

    lcd_puts(0, line++, ptr);
    lcd_update();
    if(line >= 16)
        line = 0;
#ifdef HAVE_REMOTE_LCD
    lcd_remote_puts(0, remote_line++, ptr);
    lcd_remote_update();
    if(remote_line >= 8)
        remote_line = 0;
#endif
}

/* Reset the cookie for the crt0 crash check */
inline void __reset_cookie(void)
{
    asm(" move.l #0,%d0");
    asm(" move.l %d0,0x10017ffc");
}

void start_iriver_fw(void)
{
    asm(" move.w #0x2700,%sr");
    __reset_cookie();
    asm(" movec.l %d0,%vbr");
    asm(" move.l 0,%sp");
    asm(" lea.l 8,%a0");
    asm(" jmp (%a0)");
}

int load_firmware(void)
{
    int fd;
    int rc;
    int len;
    unsigned long chksum;
    char model[5];
    unsigned long sum;
    int i;
    unsigned char *buf = (unsigned char *)DRAM_START;

    fd = open("/.rockbox/" BOOTFILE, O_RDONLY);
    if(fd < 0)
    {
        fd = open("/" BOOTFILE, O_RDONLY);
        if(fd < 0)
            return -1;
    }

    len = filesize(fd) - 8;

    printf("Length: %x", len);

    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);

    rc = read(fd, &chksum, 4);
    if(rc < 4)
        return -2;

    printf("Checksum: %x", chksum);

    rc = read(fd, model, 4);
    if(rc < 4)
        return -3;

    model[4] = 0;

    printf("Model name: %s", model);
    lcd_update();

    lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);

    rc = read(fd, buf, len);
    if(rc < len)
        return -4;

    close(fd);

    sum = MODEL_NUMBER;

    for(i = 0;i < len;i++) {
        sum += buf[i];
    }

    printf("Sum: %x", sum);

    if(sum != chksum)
        return -5;

    return 0;
}

void start_firmware(void)
{
    asm(" move.w #0x2700,%sr");
    __reset_cookie();
    asm(" move.l %0,%%d0" :: "i"(DRAM_START));
    asm(" movec.l %d0,%vbr");
    asm(" move.l %0,%%sp" :: "m"(*(int *)DRAM_START));
    asm(" move.l %0,%%a0" :: "m"(*(int *)(DRAM_START+4)));
    asm(" jmp (%a0)");
}

#ifdef IRIVER_H100_SERIES
void start_flashed_romimage(void)
{
    uint8_t *src = (uint8_t *)FLASH_ROMIMAGE_ENTRY;
    int *reset_vector;
    
    if (!detect_flashed_romimage())
        return ;
        
    reset_vector = (int *)(&src[sizeof(struct flash_header)+4]);

    asm(" move.w #0x2700,%sr");
    __reset_cookie();
    
    asm(" move.l %0,%%d0" :: "i"(DRAM_START));
    asm(" movec.l %d0,%vbr");
    asm(" move.l %0,%%sp" :: "m"(reset_vector[0]));
    asm(" move.l %0,%%a0" :: "m"(reset_vector[1]));
    asm(" jmp (%a0)");
    
    /* Failure */
    power_off();
}

void start_flashed_ramimage(void)
{
    struct flash_header hdr;
    unsigned char *buf = (unsigned char *)DRAM_START;
    uint8_t *src = (uint8_t *)FLASH_RAMIMAGE_ENTRY;

    if (!detect_flashed_ramimage())
         return;
    
    /* Load firmware from flash */
    cpu_boost(true);
    memcpy(&hdr, src, sizeof(struct flash_header));
    src += sizeof(struct flash_header);
    memcpy(buf, src, hdr.length);
    cpu_boost(false);
    
    start_firmware();

    /* Failure */
    power_off();
}
#endif /* IRIVER_H100_SERIES */

void shutdown(void)
{
    printf("Shutting down...");
#ifdef HAVE_EEPROM_SETTINGS
    /* Reset the rockbox crash check. */
    firmware_settings.bl_version = 0;
    eeprom_settings_store();
#endif
    
    /* We need to gracefully spin down the disk to prevent clicks. */
    if (ide_powered())
    {
        /* Make sure ATA has been initialized. */
        ata_init();
        
        /* And put the disk into sleep immediately. */
        ata_sleepnow();
    }

    sleep(HZ*2);
    
    /* Backlight OFF */
    __backlight_off();
#ifdef HAVE_REMOTE_LCD
    __remote_backlight_off();
#endif
    
    __reset_cookie();
    power_off();
}

/* Print the battery voltage (and a warning message). */
void check_battery(void)
{
    int adc_battery, battery_voltage, batt_int, batt_frac;
    
    adc_battery = adc_read(ADC_BATTERY);

    battery_voltage = (adc_battery * BATTERY_SCALE_FACTOR) / 10000;
    batt_int = battery_voltage / 100;
    batt_frac = battery_voltage % 100;

    printf("Batt: %d.%02dV", batt_int, batt_frac);

    if (battery_voltage <= 310) 
    {
        printf("WARNING! BATTERY LOW!!");
        sleep(HZ*2);
    }
}

#ifdef HAVE_EEPROM_SETTINGS
void initialize_eeprom(void)
{
    if (detect_original_firmware())
        return ;
    
    if (!eeprom_settings_init())
    {
        recovery_mode = true;
        return ;
    }
    
    /* If bootloader version has not been reset, disk might
     * not be intact. */
    if (firmware_settings.bl_version || !firmware_settings.disk_clean)
    {
        firmware_settings.disk_clean = false;
        recovery_mode = true;
    }
    
    firmware_settings.bl_version = EEPROM_SETTINGS_BL_MINVER;
    eeprom_settings_store();
}

void try_flashboot(void)
{
    if (!firmware_settings.initialized)
        return ;
    
   switch (firmware_settings.bootmethod)
    {
        case BOOT_DISK:
            return;
        
        case BOOT_ROM:
            start_flashed_romimage();
            recovery_mode = true;
            break;
        
        case BOOT_RAM:
            start_flashed_ramimage();
            recovery_mode = true;
            break;
        
        default:
            recovery_mode = true;
            return;
    }
}

static const char *options[] = { 
    "Boot from disk",
    "Boot RAM image",
    "Boot ROM image",
    "Shutdown" 
};

#define FAILSAFE_OPTIONS 4
void failsafe_menu(void)
{
    int timeout = 15;
    int option = 3;
    int button;
    int defopt = -1;
    char buf[32];
    int i;
    
    reset_screen();
    printf("Bootloader %s", version);
    check_battery();
    printf("=========================");
    line += FAILSAFE_OPTIONS;
    printf("");
    printf("  [NAVI] to confirm.");
    printf("  [REC] to set as default.");
    printf("");

    if (firmware_settings.initialized)
    {
        defopt = firmware_settings.bootmethod;
        if (defopt < 0 || defopt >= FAILSAFE_OPTIONS)
            defopt = option;
    }
    
    while (timeout > 0)
    {
        /* Draw the menu. */
        line = 3;
        for (i = 0; i < FAILSAFE_OPTIONS; i++)
        {
            char *def = "[DEF]";
            char *arrow = "->";
            
            if (i != defopt)
                def = "";
            if (i != option)
                arrow = "  ";
            
            printf("%s %s %s", arrow, options[i], def);
        }
        
        snprintf(buf, sizeof(buf), "Time left: %ds", timeout);
        lcd_puts(0, 10, buf);
        lcd_update();
        button = button_get_w_tmo(HZ);

        if (button == BUTTON_NONE)
        {
            timeout--;
            continue ;
        }

        timeout = 15;
        /* Ignore the ON/PLAY -button because it can cause trouble 
           with the RTC alarm mod. */
        switch (button & ~(BUTTON_ON))
        {
            case BUTTON_UP:
            case BUTTON_RC_REW:
                if (option > 0)
                    option--;
                break ;
                
            case BUTTON_DOWN:
            case BUTTON_RC_FF:
                if (option < FAILSAFE_OPTIONS-1)
                    option++;
                break ;

            case BUTTON_SELECT:
            case BUTTON_RC_ON:
                timeout = 0;
                break ;
            
            case BUTTON_REC:
            case BUTTON_RC_REC:
                if (firmware_settings.initialized)
                {
                    firmware_settings.bootmethod = option;
                    eeprom_settings_store();
                    defopt = option;
                }
                break ;
        }
    }
    
    lcd_puts(0, 10, "Executing command...");
    lcd_update();
    sleep(HZ);
    reset_screen();

    switch (option)
    {
        case BOOT_DISK:
            return ;
        
        case BOOT_RAM:
            start_flashed_ramimage();
            printf("Image not found");
            break;
        
        case BOOT_ROM:
            start_flashed_romimage();
            printf("Image not found");
            break;
    }
    
    shutdown();
}
#endif

void main(void)
{
    int i;
    int rc;
    bool rc_on_button = false;
    bool on_button = false;
    bool rec_button = false;
    bool hold_status = false;
    int data;

#ifdef IAUDIO_X5
    (void)rc_on_button;
    (void)on_button;
    (void)rec_button;
    (void)hold_status;
    (void)data;
    power_init();

    system_init();
    kernel_init();

    set_cpu_frequency(CPUFREQ_NORMAL);
    coldfire_set_pllcr_audio_bits(DEFAULT_PLLCR_AUDIO_BITS);

    set_irq_level(0);
    lcd_init();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_init();
#endif
    backlight_init();
    font_init();
    adc_init();
    button_init();

    printf("Rockbox boot loader");
    printf("Version %s", version);
    
    check_battery();

    rc = ata_init();
    if(rc)
    {
        printf("ATA error: %d", rc);
        sleep(HZ*5);
        power_off();
    }

    disk_init();

    rc = disk_mount_all();
    if (rc<=0)
    {
        printf("No partition found");
        sleep(HZ*5);
        power_off();
    }

    printf("Loading firmware");
    i = load_firmware();
    printf("Result: %d", i);

    if(i == 0)
        start_firmware();

    power_off();

#else
    /* We want to read the buttons as early as possible, before the user
       releases the ON button */

    /* Set GPIO33, GPIO37, GPIO38  and GPIO52 as general purpose inputs
       (The ON and Hold buttons on the main unit and the remote) */
    or_l(0x00100062, &GPIO1_FUNCTION);
    and_l(~0x00100062, &GPIO1_ENABLE);

    data = GPIO1_READ;
    if ((data & 0x20) == 0)
        on_button = true;

    if ((data & 0x40) == 0)
        rc_on_button = true;

    /* Set the default state of the hard drive power to OFF */
    ide_power_enable(false);

    power_init();

    /* Turn off if neither ON button is pressed */
    if(!(on_button || rc_on_button || usb_detect()))
    {
        __reset_cookie();
        power_off();
    }

    /* Start with the main backlight OFF. */
    __backlight_init();
    __backlight_off();
    
    /* Remote backlight ON */
#ifdef HAVE_REMOTE_LCD
    __remote_backlight_on();
#endif

    system_init();
    kernel_init();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    /* Set up waitstates for the peripherals */
    set_cpu_frequency(0); /* PLL off */
#ifdef CPU_COLDFIRE
    coldfire_set_pllcr_audio_bits(DEFAULT_PLLCR_AUDIO_BITS);
#endif
#endif
    set_irq_level(0);

#ifdef HAVE_EEPROM_SETTINGS
    initialize_eeprom();
#endif
    
    adc_init();
    button_init();
    
    if ((on_button && button_hold()) ||
         (rc_on_button && remote_button_hold()))
    {
        hold_status = true;
    }
    
    /* Power on the hard drive early, to speed up the loading.
       Some H300 don't like this, so we only do it for the H100 */
#ifndef IRIVER_H300_SERIES
    if (!hold_status && !recovery_mode)
    {
        ide_power_enable(true);
    }
    
    if (!hold_status && !usb_detect() && !recovery_mode)
        try_flashboot();
#endif

    backlight_init();
#ifdef HAVE_UDA1380
    audiohw_reset();
#endif

    lcd_init();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_init();
#endif
    font_init();

    lcd_setfont(FONT_SYSFIXED);

    printf("Rockbox boot loader");
    printf("Version %s", version);

    sleep(HZ/50); /* Allow the button driver to check the buttons */
    rec_button = ((button_status() & BUTTON_REC) == BUTTON_REC)
        || ((button_status() & BUTTON_RC_REC) == BUTTON_RC_REC);

    check_battery();

    /* Don't start if the Hold button is active on the device you
       are starting with */
    if (!usb_detect() && (hold_status || recovery_mode))
    {
        if (detect_original_firmware())
        {
            printf("Hold switch on");
            shutdown();
        }
        
#ifdef IRIVER_H100_SERIES
        failsafe_menu();
#endif
    }

    /* Holding REC while starting runs the original firmware */
    if (detect_original_firmware() && rec_button)
    {
        printf("Starting original firmware...");
        start_iriver_fw();
    }

    usb_init();

    rc = ata_init();
    if(rc)
    {
        reset_screen();
        printf("ATA error: %d", rc);
        printf("Insert USB cable and press");
        printf("a button");
        while(!(button_get(true) & BUTTON_REL));
    }

    /* A hack to enter USB mode without using the USB thread */
    if(usb_detect())
    {
        const char msg[] = "Bootloader USB mode";
        int w, h;
        font_getstringsize(msg, &w, &h, FONT_SYSFIXED);
        reset_screen();
        lcd_putsxy((LCD_WIDTH-w)/2, (LCD_HEIGHT-h)/2, msg);
        lcd_update();

#ifdef IRIVER_H300_SERIES
        sleep(HZ);
#endif

#ifdef HAVE_EEPROM_SETTINGS
        if (firmware_settings.initialized)
        {
            firmware_settings.disk_clean = false;
            eeprom_settings_store();
        }
#endif
        ata_spin();
        ata_enable(false);
        usb_enable(true);
        cpu_idle_mode(true);
        while (usb_detect())
        {
            /* Print the battery status. */
            line = 0;
            check_battery();
            
            ata_spin(); /* Prevent the drive from spinning down */
            sleep(HZ);

            /* Backlight OFF */
            __backlight_off();
        }

        cpu_idle_mode(false);
        usb_enable(false);
        ata_init(); /* Reinitialize ATA and continue booting */

        reset_screen();
        lcd_update();
    }

    disk_init();

    rc = disk_mount_all();
    if (rc<=0)
    {
        reset_screen();
        printf("No partition found");
        while(button_get(true) != SYS_USB_CONNECTED) {};
    }

    printf("Loading firmware");
    i = load_firmware();
    printf("Result: %d", i);

    if (i == 0)
        start_firmware();

    if (!detect_original_firmware())
    {
        printf("No firmware found on disk");
        shutdown();
    }
    else
        start_iriver_fw();
#endif /* IAUDIO_X5 */
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */

void reset_poweroff_timer(void)
{
}

void screen_dump(void)
{
}

int dbg_ports(void)
{
   return 0;
}

void mpeg_stop(void)
{
}

void sys_poweroff(void)
{
}
