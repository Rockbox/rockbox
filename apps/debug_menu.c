/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Heikki Hannikainen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#ifndef SIMULATOR
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "lcd.h"
#include "menu.h"
#include "debug_menu.h"
#include "kernel.h"
#include "sprintf.h"
#include "button.h"
#include "adc.h"
#include "mas.h"
#include "power.h"
#include "rtc.h"
#include "debug.h"
#include "thread.h"
#include "powermgmt.h"
#include "system.h"
#include "font.h"
#include "disk.h"
#include "mpeg.h"
#include "mp3_playback.h"
#include "settings.h"
#include "ata.h"
#include "fat.h"
#include "dir.h"
#include "panic.h"
#include "screens.h"
#include "misc.h"
#ifdef HAVE_LCD_BITMAP
#include "widgets.h"
#include "peakmeter.h"
#endif
#ifdef CONFIG_TUNER
#include "radio.h"
#endif
#ifdef HAVE_MMC
#include "ata_mmc.h"
#endif

/*---------------------------------------------------*/
/*    SPECIAL DEBUG STUFF                            */
/*---------------------------------------------------*/
extern int ata_device;
extern int ata_io_address;
extern int num_threads;
extern const char *thread_name[];

#ifdef HAVE_LCD_BITMAP
/* Test code!!! */
bool dbg_os(void)
{
    char buf[32];
    int button;
    int i;
    int usage;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();

    while(1)
    {
        lcd_puts(0, 0, "Stack usage:");
        for(i = 0; i < num_threads;i++)
        {
            usage = thread_stack_usage(i);
            snprintf(buf, 32, "%s: %d%%", thread_name[i], usage);
            lcd_puts(0, 1+i, buf);
        }

        lcd_update();

        button = button_get_w_tmo(HZ/10);

        switch(button)
        {
            case SETTINGS_CANCEL:
                return false;
        }
    }
    return false;
}
#else
bool dbg_os(void)
{
    char buf[32];
    int button;
    int usage;
    int currval = 0;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();

    while(1)
    {
        lcd_puts(0, 0, "Stack usage");

        usage = thread_stack_usage(currval);
        snprintf(buf, 32, "%d: %d%%  ", currval, usage);
        lcd_puts(0, 1, buf);
    
        button = button_get_w_tmo(HZ/10);

        switch(button)
        {
        case SETTINGS_CANCEL:
            return false;

        case SETTINGS_DEC:
            currval--;
            if(currval < 0)
                currval = num_threads-1;
            break;

        case SETTINGS_INC:
            currval++;
            if(currval > num_threads-1)
                currval = 0;
            break;
        }
    }
    return false;
}
#endif

#ifdef HAVE_LCD_BITMAP
bool dbg_mpeg_thread(void)
{
    char buf[32];
    int button;
    struct mpeg_debug d;

    lcd_setmargins(0, 0);
    
    while(1)
    {
        button = button_get_w_tmo(HZ/5);
        switch(button)
        {
            case SETTINGS_CANCEL:
                return false;
        }

        mpeg_get_debugdata(&d);
        
        lcd_clear_display();

        snprintf(buf, sizeof(buf), "read: %x", d.mp3buf_read);
        lcd_puts(0, 0, buf);
        snprintf(buf, sizeof(buf), "write: %x", d.mp3buf_write);
        lcd_puts(0, 1, buf);
        snprintf(buf, sizeof(buf), "swap: %x", d.mp3buf_swapwrite);
        lcd_puts(0, 2, buf);
        snprintf(buf, sizeof(buf), "playing: %d", d.playing);
        lcd_puts(0, 3, buf);
        snprintf(buf, sizeof(buf), "playable: %x", d.playable_space);
        lcd_puts(0, 4, buf);
        snprintf(buf, sizeof(buf), "unswapped: %x", d.unswapped_space);
        lcd_puts(0, 5, buf);

        /* Playable space left */
        scrollbar(0, 6*8, 112, 4, d.mp3buflen, 0, 
                  d.playable_space, HORIZONTAL);

        /* Show the watermark limit */
        scrollbar(0, 6*8+4, 112, 4, d.mp3buflen, 0, 
                  d.low_watermark_level, HORIZONTAL);

        snprintf(buf, sizeof(buf), "wm: %x - %x",
                 d.low_watermark_level, d.lowest_watermark_level);
        lcd_puts(0, 7, buf);
        
        lcd_update();
    }
    return false;
}
#endif


/* Tool function to calculate a CRC16 across some buffer */
unsigned short crc_16(const unsigned char* buf, unsigned len)
{
    /* CCITT standard polynomial 0x1021 */
    static const unsigned short crc16_lookup[16] = 
    {   /* lookup table for 4 bits at a time is affordable */
        0x0000, 0x1021, 0x2042, 0x3063, 
        0x4084, 0x50A5, 0x60C6, 0x70E7, 
        0x8108, 0x9129, 0xA14A, 0xB16B, 
        0xC18C, 0xD1AD, 0xE1CE, 0xF1EF
    };
    unsigned short crc16 = 0xFFFF; /* initialise to 0xFFFF (CCITT specification) */
    unsigned t;
    unsigned char byte;

    while (len--)
    {   
        byte = *buf++; /* get one byte of data */

        /* upper nibble of our data */
        t = crc16 >> 12; /* extract the 4 most significant bits */
        t ^= byte >> 4; /* XOR in 4 bits of the data into the extracted bits */
        crc16 <<= 4; /* shift the CRC Register left 4 bits */     
        crc16 ^= crc16_lookup[t]; /* do the table lookup and XOR the result */

        /* lower nibble of our data */
        t = crc16 >> 12; /* extract the 4 most significant bits */
        t ^= byte & 0x0F; /* XOR in 4 bits of the data into the extracted bits */
        crc16 <<= 4; /* shift the CRC Register left 4 bits */     
        crc16 ^= crc16_lookup[t]; /* do the table lookup and XOR the result */
    }

    return crc16;
}



/* Tool function to read the flash manufacturer and type, if available.
   Only chips which could be reprogrammed in system will return values.
   (The mode switch addresses vary between flash manufacturers, hence addr1/2) */
   /* In IRAM to avoid problems when running directly from Flash */
bool dbg_flash_id(unsigned* p_manufacturer, unsigned* p_device, 
                  unsigned addr1, unsigned addr2)
                  __attribute__ ((section (".icode")));
bool dbg_flash_id(unsigned* p_manufacturer, unsigned* p_device,
                  unsigned addr1, unsigned addr2)
          
{
    unsigned not_manu, not_id; /* read values before switching to ID mode */
    unsigned manu, id; /* read values when in ID mode */
    volatile unsigned char* flash = (unsigned char*)0x2000000; /* flash mapping */
    int old_level; /* saved interrupt level */

    not_manu = flash[0]; /* read the normal content */
    not_id   = flash[1]; /* should be 'A' (0x41) and 'R' (0x52) from the "ARCH" marker */

    /* disable interrupts, prevent any stray flash access */
    old_level = set_irq_level(HIGHEST_IRQ_LEVEL);

    flash[addr1] = 0xAA; /* enter command mode */
    flash[addr2] = 0x55;
    flash[addr1] = 0x90; /* ID command */
    /* Atmel wants 20ms pause here */
    /* sleep(HZ/50); no sleeping possible while interrupts are disabled */

    manu = flash[0]; /* read the IDs */
    id   = flash[1];

    flash[0] = 0xF0; /* reset flash (back to normal read mode) */
    /* Atmel wants 20ms pause here */
    /* sleep(HZ/50); no sleeping possible while interrupts are disabled */

    set_irq_level(old_level); /* enable interrupts again */

    /* I assume success if the obtained values are different from
        the normal flash content. This is not perfectly bulletproof, they 
        could theoretically be the same by chance, causing us to fail. */
    if (not_manu != manu || not_id != id) /* a value has changed */
    {
        *p_manufacturer = manu; /* return the results */
        *p_device = id;
        return true; /* success */
    }
    return false; /* fail */
}


#ifdef HAVE_LCD_BITMAP
bool dbg_hw_info(void)
{
    char buf[32];
    int button;
    int usb_polarity;
    int pr_polarity;
    int bitmask = *(unsigned short*)0x20000fc;
    int rom_version = *(unsigned short*)0x20000fe;
    unsigned manu, id; /* flash IDs */
    bool got_id; /* flag if we managed to get the flash IDs */
    unsigned rom_crc = 0xFFFF; /* CRC16 of the boot ROM */
    bool has_bootrom; /* flag for boot ROM present */
    int oldmode;  /* saved memory guard mode */

#ifdef USB_ENABLE_ONDIOSTYLE
    if(PADR & 0x20)
#else
    if(PADR & 0x400)
#endif
        usb_polarity = 0; /* Negative */
    else
        usb_polarity = 1; /* Positive */

    if(PADR & 0x800)
        pr_polarity = 0; /* Negative */
    else
        pr_polarity = 1; /* Positive */

    oldmode = system_memory_guard(MEMGUARD_NONE);  /* disable memory guard */

    /* get flash ROM type */
    got_id = dbg_flash_id(&manu, &id, 0x5555, 0x2AAA); /* try SST, Atmel, NexFlash */
    if (!got_id)
        got_id = dbg_flash_id(&manu, &id, 0x555, 0x2AA); /* try AMD, Macronix */
    
    /* check if the boot ROM area is a flash mirror */
    has_bootrom = (memcmp((char*)0, (char*)0x02000000, 64*1024) != 0);
    if (has_bootrom)  /* if ROM and Flash different */
    {
        /* calculate CRC16 checksum of boot ROM */
        rom_crc = crc_16((unsigned char*)0x0000, 64*1024);
    }

    system_memory_guard(oldmode);  /* re-enable memory guard */

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    lcd_puts(0, 0, "[Hardware info]");

    snprintf(buf, 32, "ROM: %d.%02d", rom_version/100, rom_version%100);
    lcd_puts(0, 1, buf);
    
    snprintf(buf, 32, "Mask: 0x%04x", bitmask);
    lcd_puts(0, 2, buf);
    
    snprintf(buf, 32, "USB: %s", usb_polarity?"positive":"negative");
    lcd_puts(0, 3, buf);
    
    snprintf(buf, 32, "PR: %s", pr_polarity?"positive":"negative");
    lcd_puts(0, 4, buf);
    
    if (got_id)
        snprintf(buf, 32, "Flash: M=%02x D=%02x", manu, id);
    else
        snprintf(buf, 32, "Flash: M=?? D=??"); /* unknown, sorry */
    lcd_puts(0, 5, buf);

    if (has_bootrom)
    {
        snprintf(buf, 32-3, "ROM CRC: 0x%04x", rom_crc);
        if (rom_crc == 0x222F) /* known Version 1 */
            strcat(buf, " V1");
    }
    else
    {
        snprintf(buf, 32, "Boot ROM: none");
    }
    lcd_puts(0, 6, buf);

#ifndef HAVE_MMC /* have ATA */
    snprintf(buf, 32, "ATA: 0x%x,%s", ata_io_address,
             ata_device ? "slave":"master");
    lcd_puts(0, 7, buf);
#endif    
    lcd_update();

    while(1)
    {
        button = button_get(true);
        if(button == SETTINGS_CANCEL)
            return false;
    }

    return false;
}
#else
bool dbg_hw_info(void)
{
    char buf[32];
    int button;
    int currval = 0;
    int usb_polarity;
    int bitmask = *(unsigned short*)0x20000fc;
    int rom_version = *(unsigned short*)0x20000fe;
    unsigned manu, id; /* flash IDs */
    bool got_id; /* flag if we managed to get the flash IDs */
    unsigned rom_crc = 0xFFFF; /* CRC16 of the boot ROM */
    bool has_bootrom; /* flag for boot ROM present */
    int oldmode;  /* saved memory guard mode */

    if(PADR & 0x400)
        usb_polarity = 0; /* Negative */
    else
        usb_polarity = 1; /* Positive */

    oldmode = system_memory_guard(MEMGUARD_NONE);  /* disable memory guard */

    /* get flash ROM type */
    got_id = dbg_flash_id(&manu, &id, 0x5555, 0x2AAA); /* try SST, Atmel, NexFlash */
    if (!got_id)
        got_id = dbg_flash_id(&manu, &id, 0x555, 0x2AA); /* try AMD, Macronix */

    /* check if the boot ROM area is a flash mirror */
    has_bootrom = (memcmp((char*)0, (char*)0x02000000, 64*1024) != 0);
    if (has_bootrom)  /* if ROM and Flash different */
    {
        /* calculate CRC16 checksum of boot ROM */
        rom_crc = crc_16((unsigned char*)0x0000, 64*1024);
    }
    
    system_memory_guard(oldmode);  /* re-enable memory guard */

    lcd_clear_display();

    lcd_puts(0, 0, "[HW Info]");
    while(1)
    {
        switch(currval)
        {
            case 0:
                snprintf(buf, 32, "ROM: %d.%02d",
                         rom_version/100, rom_version%100);
                break;
            case 1:
                snprintf(buf, 32, "USB: %s",
                         usb_polarity?"pos":"neg");
                break;
            case 2:
                snprintf(buf, 32, "ATA: 0x%x%s",
                         ata_io_address, ata_device ? "s":"m");
                break;
            case 3:
                snprintf(buf, 32, "Mask: %04x", bitmask);
                break;
            case 4:
                if (got_id)
                    snprintf(buf, 32, "Flash:%02x,%02x", manu, id);
                else
                    snprintf(buf, 32, "Flash:??,??"); /* unknown, sorry */
                break;
            case 5:
                if (has_bootrom)
                    snprintf(buf, 32, "RomCRC:%04x", rom_crc);
                else
                    snprintf(buf, 32, "BootROM: no");
        }
            
        lcd_puts(0, 1, buf);
        lcd_update();
        
        button = button_get(true);

        switch(button)
        {
            case SETTINGS_CANCEL:
                return false;

            case SETTINGS_DEC:
                currval--;
                if(currval < 0)
                    currval = 5;
                break;
                
            case SETTINGS_INC:
                currval++;
                if(currval > 5)
                    currval = 0;
                break;
        }
    }
    return false;
}
#endif

bool dbg_partitions(void)
{
    int partition=0;

    lcd_clear_display();
    lcd_puts(0, 0, "Partition");
    lcd_puts(0, 1, "list");
    lcd_update();
    sleep(HZ/2);

    while(1)
    {
        char buf[32];
        int button;
        struct partinfo* p = disk_partinfo(partition);

        lcd_clear_display();
        snprintf(buf, sizeof buf, "P%d: S:%x", partition, p->start);
        lcd_puts(0, 0, buf);
        snprintf(buf, sizeof buf, "T:%x %d MB", p->type, p->size / 2048);
        lcd_puts(0, 1, buf);
        lcd_update();
        
        button = button_get(true);

        switch(button)
        {
            case SETTINGS_OK:
            case SETTINGS_CANCEL:
                return false;

            case SETTINGS_DEC:
                partition--;
                if (partition < 0)
                    partition = 3;
                break;
                
            case SETTINGS_INC:
                partition++;
                if (partition > 3)
                    partition = 0;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }
    return false;
}

#ifdef HAVE_LCD_BITMAP
/* Test code!!! */
bool dbg_ports(void)
{
    unsigned short porta;
    unsigned short portb;
    unsigned char portc;
    char buf[32];
    int button;
    int battery_voltage;
    int batt_int, batt_frac;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();

    while(1)
    {
        porta = PADR;
        portb = PBDR;
        portc = PCDR;

        snprintf(buf, 32, "PADR: %04x", porta);
        lcd_puts(0, 0, buf);
        snprintf(buf, 32, "PBDR: %04x", portb);
        lcd_puts(0, 1, buf);

        snprintf(buf, 32, "AN0: %03x AN4: %03x", adc_read(0), adc_read(4));
        lcd_puts(0, 2, buf);
        snprintf(buf, 32, "AN1: %03x AN5: %03x", adc_read(1), adc_read(5));
        lcd_puts(0, 3, buf);
        snprintf(buf, 32, "AN2: %03x AN6: %03x", adc_read(2), adc_read(6));
        lcd_puts(0, 4, buf);
        snprintf(buf, 32, "AN3: %03x AN7: %03x", adc_read(3), adc_read(7));
        lcd_puts(0, 5, buf);

        battery_voltage = (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) / 10000;
        batt_int = battery_voltage / 100;
        batt_frac = battery_voltage % 100;
    
        snprintf(buf, 32, "Batt: %d.%02dV %d%%  ", batt_int, batt_frac,
                 battery_level());
        lcd_puts(0, 6, buf);
#ifndef HAVE_MMC /* have ATA */
        snprintf(buf, 32, "ATA: %s, 0x%x",
                 ata_device?"slave":"master", ata_io_address);
        lcd_puts(0, 7, buf);
#endif
        lcd_update();
        button = button_get_w_tmo(HZ/10);

        switch(button)
        {
            case SETTINGS_CANCEL:
                return false;
        }
    }
    return false;
}
#else
bool dbg_ports(void)
{
    unsigned short porta;
    unsigned short portb;
    unsigned char portc;
    char buf[32];
    int button;
    int battery_voltage;
    int batt_int, batt_frac;
    int currval = 0;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();

    while(1)
    {
        porta = PADR;
        portb = PBDR;
        portc = PCDR;

        switch(currval)
        {
        case 0:
            snprintf(buf, 32, "PADR: %04x  ", porta);
            break;
        case 1:
            snprintf(buf, 32, "PBDR: %04x  ", portb);
            break;
        case 2:
            snprintf(buf, 32, "AN0: %03x  ", adc_read(0));
            break;
        case 3:
            snprintf(buf, 32, "AN1: %03x  ", adc_read(1));
            break;
        case 4:
            snprintf(buf, 32, "AN2: %03x  ", adc_read(2));
            break;
        case 5:
            snprintf(buf, 32, "AN3: %03x  ", adc_read(3));
            break;
        case 6:
            snprintf(buf, 32, "AN4: %03x  ", adc_read(4));
            break;
        case 7:
            snprintf(buf, 32, "AN5: %03x  ", adc_read(5));
            break;
        case 8:
            snprintf(buf, 32, "AN6: %03x  ", adc_read(6));
            break;
        case 9:
            snprintf(buf, 32, "AN7: %03x  ", adc_read(7));
            break;
        case 10:
            snprintf(buf, 32, "%s, 0x%x ",
                     ata_device?"slv":"mst", ata_io_address);
            break;
        }
        lcd_puts(0, 0, buf);
        
        battery_voltage = (adc_read(ADC_UNREG_POWER) * 
                           BATTERY_SCALE_FACTOR) / 10000;
        batt_int = battery_voltage / 100;
        batt_frac = battery_voltage % 100;
    
        snprintf(buf, 32, "Batt: %d.%02dV", batt_int, batt_frac);
        lcd_puts(0, 1, buf);
        
        button = button_get_w_tmo(HZ/5);

        switch(button)
        {
        case SETTINGS_CANCEL:
            return false;

        case SETTINGS_DEC:
            currval--;
            if(currval < 0)
                currval = 10;
            break;

        case SETTINGS_INC:
            currval++;
            if(currval > 10)
                currval = 0;
            break;
        }
    }
    return false;
}
#endif

#ifdef HAVE_RTC
/* Read RTC RAM contents and display them */
bool dbg_rtc(void)
{
    char buf[32];
    unsigned char addr = 0, r, c;
    int i;
    int button;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();
    lcd_puts(0, 0, "RTC read:");

    while(1)
    {
        for (r = 0; r < 4; r++) {
            snprintf(buf, 10, "0x%02x: ", addr + r*4);
            for (c = 0; c <= 3; c++) {
                i = rtc_read(addr + r*4 + c);
                snprintf(buf + 6 + c*2, 3, "%02x", i);
            }
            lcd_puts(1, r+1, buf);
        }
        
        lcd_update();

        button = button_get_w_tmo(HZ/2);

        switch(button)
        {
            case SETTINGS_INC:
                if (addr < 63-16) { addr += 16; }
                break;

            case SETTINGS_DEC:
                if (addr) { addr -= 16; }
                break;

#ifdef BUTTON_F2
            case BUTTON_F2:
                /* clear the user RAM space */
                for (c = 0; c <= 43; c++)
                    rtc_write(0x14 + c, 0);
                break;
#endif

            case SETTINGS_OK:
            case SETTINGS_CANCEL:
                return false;
        }
    }
    return false;
}
#else
bool dbg_rtc(void)
{
    return false;
}
#endif

#ifdef HAVE_LCD_CHARCELLS
#define NUMROWS 1
#else
#define NUMROWS 4
#endif
/* Read MAS registers and display them */
bool dbg_mas(void)
{
    char buf[32];
    unsigned int addr = 0, r, i;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();
    lcd_puts(0, 0, "MAS register read:");

    while(1)
    {
        for (r = 0; r < NUMROWS; r++) {
            i = mas_readreg(addr + r);
            snprintf(buf, 30, "%02x %08x", addr + r, i);
            lcd_puts(0, r+1, buf);
        }

        lcd_update();

        switch(button_get_w_tmo(HZ/16))
        {
            case SETTINGS_INC:
                addr = (addr + NUMROWS) & 0xFF;  /* register addrs are 8 bit */
                break;

            case SETTINGS_DEC:
                addr = (addr - NUMROWS) & 0xFF;  /* register addrs are 8 bit */
                break;

            case SETTINGS_OK:
            case SETTINGS_CANCEL:
                return false;
        }
    }
    return false;
}

#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
bool dbg_mas_codec(void)
{
    char buf[32];
    unsigned int addr = 0, r, i;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();
    lcd_puts(0, 0, "MAS codec reg read:");

    while(1)
    {
        for (r = 0; r < 4; r++) {
            i = mas_codec_readreg(addr + r);
            snprintf(buf, 30, "0x%02x: %08x", addr + r, i);
            lcd_puts(1, r+1, buf);
        }
        
        lcd_update();

        switch(button_get_w_tmo(HZ/16))
        {
            case SETTINGS_INC:
                addr += 4;
                break;
            case SETTINGS_DEC:
                if (addr) { addr -= 4; }
                break;

            case SETTINGS_OK:
            case SETTINGS_CANCEL:
                return false;
        }
    }
    return false;
}
#endif

#ifdef HAVE_LCD_BITMAP
/*
 * view_battery() shows a automatically scaled graph of the battery voltage
 * over time. Usable for estimating battery life / charging rate.
 * The power_history array is updated in power_thread of powermgmt.c.
 */

#define BAT_FIRST_VAL  MAX(POWER_HISTORY_LEN - LCD_WIDTH - 1, 0)
#define BAT_YSPACE    (LCD_HEIGHT - 20)

bool view_battery(void)
{
    int view = 0;
    int i, x, y;
    int maxv, minv;
    char buf[32];
    
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    while(1)
    {
        switch (view) {
            case 0: /* voltage history graph */
                /* Find maximum and minimum voltage for scaling */
                maxv = minv = 0;
                for (i = BAT_FIRST_VAL; i < POWER_HISTORY_LEN; i++) {
                    if (power_history[i] > maxv)
                        maxv = power_history[i];
                    if ((minv == 0) || ((power_history[i]) && 
                                        (power_history[i] < minv)) )
                    {
                        minv = power_history[i];
                    }
                }
                
                if (minv < 1)
                    minv = 1;
                if (maxv < 2)
                    maxv = 2;
                    
                lcd_clear_display();
                lcd_puts(0, 0, "Battery voltage:");
                snprintf(buf, 30, "scale %d.%02d-%d.%02d V", 
                         minv / 100, minv % 100, maxv / 100, maxv % 100);
                lcd_puts(0, 1, buf);
                
                x = 0;
                for (i = BAT_FIRST_VAL+1; i < POWER_HISTORY_LEN; i++) {
                    y = (power_history[i] - minv) * BAT_YSPACE / (maxv - minv);
                    lcd_clearline(x, LCD_HEIGHT-1, x, 20);
                    lcd_drawline(x, LCD_HEIGHT-1, x, 
                                 MIN(MAX(LCD_HEIGHT-1 - y, 20), LCD_HEIGHT-1));
                    x++;
                }

                break;
                
            case 1: /* status: */
                lcd_clear_display();
                lcd_puts(0, 0, "Power status:");
                
                y = (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) / 10000;
                snprintf(buf, 30, "Battery: %d.%02d V", y / 100, y % 100);
                lcd_puts(0, 1, buf);
#ifdef ADC_EXT_POWER
                y = (adc_read(ADC_EXT_POWER) * EXT_SCALE_FACTOR) / 10000;
                snprintf(buf, 30, "External: %d.%02d V", y / 100, y % 100);
                lcd_puts(0, 2, buf);
#endif
#ifdef HAVE_CHARGING
                snprintf(buf, 30, "Charger: %s", 
                         charger_inserted() ? "present" : "absent");
                lcd_puts(0, 3, buf);
#ifdef HAVE_CHARGE_CTRL
                snprintf(buf, 30, "Charging: %s", 
                         charger_enabled ? "yes" : "no");
                lcd_puts(0, 4, buf);
#endif                
#endif
                y = ( power_history[POWER_HISTORY_LEN-1] * 100
                    + power_history[POWER_HISTORY_LEN-2] * 100
                    - power_history[POWER_HISTORY_LEN-1-CHARGE_END_NEGD+1] * 100
                    - power_history[POWER_HISTORY_LEN-1-CHARGE_END_NEGD] * 100 )
                    / CHARGE_END_NEGD / 2;
                
                snprintf(buf, 30, "short delta: %d", y);
                lcd_puts(0, 5, buf);
                
                y = ( power_history[POWER_HISTORY_LEN-1] * 100
                    + power_history[POWER_HISTORY_LEN-2] * 100
                    - power_history[POWER_HISTORY_LEN-1-CHARGE_END_ZEROD+1] * 100
                    - power_history[POWER_HISTORY_LEN-1-CHARGE_END_ZEROD] * 100 )
                    / CHARGE_END_ZEROD / 2;
                
                snprintf(buf, 30, "long delta: %d", y);
                lcd_puts(0, 6, buf);

#ifdef HAVE_CHARGE_CTRL
                lcd_puts(0, 7, power_message);
#endif
                break;
                
            case 2: /* voltage deltas: */
                lcd_clear_display();
                lcd_puts(0, 0, "Voltage deltas:");
                
                for (i = 0; i <= 6; i++) {
                    y = power_history[POWER_HISTORY_LEN-1-i] - 
                        power_history[POWER_HISTORY_LEN-1-i-1];
                    snprintf(buf, 30, "-%d min: %s%d.%02d V", i,
                             (y < 0) ? "-" : "", ((y < 0) ? y * -1 : y) / 100, 
                             ((y < 0) ? y * -1 : y ) % 100);
                    lcd_puts(0, i+1, buf);
                }
                break;

            case 3: /* remeining time estimation: */
                lcd_clear_display();

#ifdef HAVE_CHARGE_CTRL
                snprintf(buf, 30, "charge_state: %d", charge_state);
                lcd_puts(0, 0, buf);

                snprintf(buf, 30, "Cycle time: %d m", powermgmt_last_cycle_startstop_min);
                lcd_puts(0, 1, buf);

                snprintf(buf, 30, "Lev.at cycle start: %d%%", powermgmt_last_cycle_level);
                lcd_puts(0, 2, buf);
#endif

                snprintf(buf, 30, "Last PwrHist val: %d.%02d V",
                    power_history[POWER_HISTORY_LEN-1] / 100,
                    power_history[POWER_HISTORY_LEN-1] % 100);
                lcd_puts(0, 3, buf);

                snprintf(buf, 30, "battery level: %d%%", battery_level());
                lcd_puts(0, 5, buf);

                snprintf(buf, 30, "Est. remaining: %d m", battery_time());
                lcd_puts(0, 6, buf);

#ifdef HAVE_CHARGE_CTRL
                snprintf(buf, 30, "Trickle sec: %d/60", trickle_sec);
                lcd_puts(0, 7, buf);
#endif                
                break;
        }
        
        lcd_update();
        
        switch(button_get_w_tmo(HZ/2))
        {
            case SETTINGS_DEC:
                if (view)
                    view--;
                break;
                
            case SETTINGS_INC:
                if (view < 3)
                    view++;
                break;
                
            case SETTINGS_OK:
            case SETTINGS_CANCEL:
                return false;
        }
    }
    return false;
}

#endif

#if CONFIG_HWCODEC == MAS3507D
bool dbg_mas_info(void)
{
    int button;
    char buf[32];
    int currval = 0;
    unsigned long val;
    unsigned long pll48, pll44, config;
    int pll_toggle = 0;
    
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    while(1)
    {
        switch(currval)
        {
            case 0:
                mas_readmem(MAS_BANK_D1, 0xff7, &val, 1);
                lcd_puts(0, 0, "Design Code");
                snprintf(buf, 32, "%05x      ", val);
                break;
            case 1:
                lcd_puts(0, 0, "DC/DC mode ");
                snprintf(buf, 32, "8e: %05x  ", mas_readreg(0x8e) & 0xfffff);
                break;
            case 2:
                lcd_puts(0, 0, "Mute/Bypass");
                snprintf(buf, 32, "aa: %05x  ", mas_readreg(0xaa) & 0xfffff);
                break;
            case 3:
                lcd_puts(0, 0, "PIOData    ");
                snprintf(buf, 32, "ed: %05x  ", mas_readreg(0xed) & 0xfffff);
                break;
            case 4:
                lcd_puts(0, 0, "Startup Cfg");
                snprintf(buf, 32, "e6: %05x  ", mas_readreg(0xe6) & 0xfffff);
                break;
            case 5:
                lcd_puts(0, 0, "KPrescale  ");
                snprintf(buf, 32, "e7: %05x  ", mas_readreg(0xe7) & 0xfffff);
                break;
            case 6:
                lcd_puts(0, 0, "KBass      ");
                snprintf(buf, 32, "6b: %05x   ", mas_readreg(0x6b) & 0xfffff);
                break;
            case 7:
                lcd_puts(0, 0, "KTreble    ");
                snprintf(buf, 32, "6f: %05x   ", mas_readreg(0x6f) & 0xfffff);
                break;
            case 8:
                mas_readmem(MAS_BANK_D0, MAS_D0_MPEG_FRAME_COUNT, &val, 1);
                lcd_puts(0, 0, "Frame Count");
                snprintf(buf, 32, "0/300: %04x", val & 0xffff);
                break;
            case 9:
                mas_readmem(MAS_BANK_D0, MAS_D0_MPEG_STATUS_1, &val, 1);
                lcd_puts(0, 0, "Status1    ");
                snprintf(buf, 32, "0/301: %04x", val & 0xffff);
                break;
            case 10:
                mas_readmem(MAS_BANK_D0, MAS_D0_MPEG_STATUS_2, &val, 1);
                lcd_puts(0, 0, "Status2    ");
                snprintf(buf, 32, "0/302: %04x", val & 0xffff);
                break;
            case 11:
                mas_readmem(MAS_BANK_D0, MAS_D0_CRC_ERROR_COUNT, &val, 1);
                lcd_puts(0, 0, "CRC Count  ");
                snprintf(buf, 32, "0/303: %04x", val & 0xffff);
                break;
            case 12:
                mas_readmem(MAS_BANK_D0, 0x36d, &val, 1);
                lcd_puts(0, 0, "PLLOffset48");
                snprintf(buf, 32, "0/36d %05x", val & 0xfffff);
                break;
            case 13:
                mas_readmem(MAS_BANK_D0, 0x32d, &val, 1);
                lcd_puts(0, 0, "PLLOffset48");
                snprintf(buf, 32, "0/32d %05x", val & 0xfffff);
                break;
            case 14:
                mas_readmem(MAS_BANK_D0, 0x36e, &val, 1);
                lcd_puts(0, 0, "PLLOffset44");
                snprintf(buf, 32, "0/36e %05x", val & 0xfffff);
                break;
            case 15:
                mas_readmem(MAS_BANK_D0, 0x32e, &val, 1);
                lcd_puts(0, 0, "PLLOffset44");
                snprintf(buf, 32, "0/32e %05x", val & 0xfffff);
                break;
            case 16:
                mas_readmem(MAS_BANK_D0, 0x36f, &val, 1);
                lcd_puts(0, 0, "OutputConf ");
                snprintf(buf, 32, "0/36f %05x", val & 0xfffff);
                break;
            case 17:
                mas_readmem(MAS_BANK_D0, 0x32f, &val, 1);
                lcd_puts(0, 0, "OutputConf ");
                snprintf(buf, 32, "0/32f %05x", val & 0xfffff);
                break;
            case 18:
                mas_readmem(MAS_BANK_D1, 0x7f8, &val, 1);
                lcd_puts(0, 0, "LL Gain    ");
                snprintf(buf, 32, "1/7f8 %05x", val & 0xfffff);
                break;
            case 19:
                mas_readmem(MAS_BANK_D1, 0x7f9, &val, 1);
                lcd_puts(0, 0, "LR Gain    ");
                snprintf(buf, 32, "1/7f9 %05x", val & 0xfffff);
                break;
            case 20:
                mas_readmem(MAS_BANK_D1, 0x7fa, &val, 1);
                lcd_puts(0, 0, "RL Gain    ");
                snprintf(buf, 32, "1/7fa %05x", val & 0xfffff);
                break;
            case 21:
                mas_readmem(MAS_BANK_D1, 0x7fb, &val, 1);
                lcd_puts(0, 0, "RR Gain    ");
                snprintf(buf, 32, "1/7fb %05x", val & 0xfffff);
                break;
            case 22:
                lcd_puts(0, 0, "L Trailbits");
                snprintf(buf, 32, "c5: %05x   ", mas_readreg(0xc5) & 0xfffff);
                break;
            case 23:
                lcd_puts(0, 0, "R Trailbits");
                snprintf(buf, 32, "c6: %05x   ", mas_readreg(0xc6) & 0xfffff);
                break;
        }
        lcd_puts(0, 1, buf);
        
        button = button_get_w_tmo(HZ/5);
        switch(button)
        {
            case SETTINGS_CANCEL:
                return false;

            case SETTINGS_DEC:
                currval--;
                if(currval < 0)
                    currval = 23;
                break;

            case SETTINGS_INC:
                currval++;
                if(currval > 23)
                    currval = 0;
                break;

            case SETTINGS_OK:
                pll_toggle = !pll_toggle;
                if(pll_toggle)
                {
                    /* 14.31818 MHz crystal */
                    pll48 = 0x5d9d0;
                    pll44 = 0xfffceceb;
                    config = 0;
                }
                else
                {
                    /* 14.725 MHz crystal */
                    pll48 = 0x2d0de;
                    pll44 = 0xfffa2319;
                    config = 0;
                }
                mas_writemem(MAS_BANK_D0, 0x32d, &pll48, 1);
                mas_writemem(MAS_BANK_D0, 0x32e, &pll44, 1);
                mas_writemem(MAS_BANK_D0, 0x32f, &config, 1);
                mas_run(0x475);
                break;
        }
    }
    return false;
}
#endif

static bool view_runtime(void)
{
    char s[32];
    bool done = false;
    int state = 1;

    while(!done)
    {
        int y=0;
        int t;
        int key;
        lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
        lcd_puts(0, y++, "Running time:");
        y++;
#endif

        if (state & 1) {
            if (charger_inserted())
            {
                global_settings.runtime = 0;
            }
            else
            {
                global_settings.runtime += ((current_tick - lasttime) / HZ);
            }
            lasttime = current_tick;

            t = global_settings.runtime;
            lcd_puts(0, y++, "Current time");
        }
        else {
            t = global_settings.topruntime;
            lcd_puts(0, y++, "Top time");
        }
    
        snprintf(s, sizeof(s), "%dh %dm %ds",
                 t / 3600, (t % 3600) / 60, t % 60);
        lcd_puts(0, y++, s);
        lcd_update();

        /* Wait for a key to be pushed */
        key = button_get_w_tmo(HZ);
        switch(key) {
            case SETTINGS_CANCEL:
                done = true;
                break;

            case SETTINGS_INC:
            case SETTINGS_DEC:
                if (state == 1)
                    state = 2;
                else
                    state = 1;
                break;

            case SETTINGS_OK:
                lcd_clear_display();
                lcd_puts(0,0,"Clear time?");
                lcd_puts(0,1,"PLAY = Yes");
                lcd_update();
                while (1) {
                    key = button_get(true);
                    if ( key == SETTINGS_OK ) {
                        if ( state == 1 )
                            global_settings.runtime = 0;
                        else
                            global_settings.topruntime = 0;
                        break;
                    }
                    if (!(key & BUTTON_REL))  /* ignore button releases */
                        break;
                }
                break;
        }
    }

    return false;
}

#ifdef HAVE_MMC
/* value is 10 * real value */
static unsigned char prep_value_unit(unsigned int *value,
                                     const unsigned char *units)
{
    int unit_no = 0;

    while (*value >= 10000)
    {
        *value /= 1000;
        unit_no++;
    }
    return units[unit_no];
}

bool dbg_mmc_info(void)
{
    bool done = false;
    int currval = 0;
    unsigned int value;
    tCardInfo *card;
    unsigned char pbuf[32];
    unsigned char card_name[7];
    unsigned char unit;
    
    static const unsigned char i_vmin[] = { 0, 1, 5, 10, 25, 35, 60, 100 };
    static const unsigned char i_vmax[] = { 1, 5, 10, 25, 35, 45, 80, 200 };
    
    card_name[6] = '\0';

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);

    while (!done)
    {
        card = mmc_card_info(currval / 2);

        lcd_clear_display();
        snprintf(pbuf, sizeof(pbuf), "[MMC%d p%d]", currval / 2,
                 (currval % 2) + 1);
        lcd_puts(0, 0, pbuf);

        if (card->initialized)
        {
            if (!(currval % 2))   /* General info */
            {
                strncpy(card_name, ((unsigned char*)card->cid) + 3, 6);
                snprintf(pbuf, sizeof(pbuf), "%s Rev %d.%d", card_name,
                         mmc_extract_bits(card->cid, 72, 4),
                         mmc_extract_bits(card->cid, 76, 4));
                lcd_puts(0, 1, pbuf);
                snprintf(pbuf, sizeof(pbuf), "Prod: %d/%d",
                         mmc_extract_bits(card->cid, 112, 4),
                         mmc_extract_bits(card->cid, 116, 4) + 1997);
                lcd_puts(0, 2, pbuf);
                snprintf(pbuf, sizeof(pbuf), "Ser#: 0x%08x",
                         mmc_extract_bits(card->cid, 80, 32));
                lcd_puts(0, 3, pbuf);
                snprintf(pbuf, sizeof(pbuf), "M=%02x, O=%04x",
                         mmc_extract_bits(card->cid, 0, 8),
                         mmc_extract_bits(card->cid, 8, 16));
                lcd_puts(0, 4, pbuf);
                value = mmc_extract_bits(card->csd, 54, 12)
                      * (SECTOR_SIZE << (mmc_extract_bits(card->csd, 78, 3)+2));
                snprintf(pbuf, sizeof(pbuf), "Size: %d MB",
                         value / (1024*1024));
                lcd_puts(0, 5, pbuf);
            }
            else                  /* Technical details */
            {
                value = card->speed / 100;
                unit = prep_value_unit(&value, "kMG");
                if (value < 100)
                    snprintf(pbuf, sizeof(pbuf), "Speed: %d.%01d %cBit/s",
                             value / 10, value % 10, unit);
                else
                    snprintf(pbuf, sizeof(pbuf), "Speed: %d %cBit/s",
                             value / 10, unit);
                lcd_puts(0, 1, pbuf);

                value = card->tsac;
                unit = prep_value_unit(&value, "nµm");
                if (value < 100)
                    snprintf(pbuf, sizeof(pbuf), "Tsac: %d.%01d %cs",
                             value / 10, value % 10, unit);
                else
                    snprintf(pbuf, sizeof(pbuf), "Tsac: %d %cs",
                             value / 10, unit);
                lcd_puts(0, 2, pbuf);

                snprintf(pbuf, sizeof(pbuf), "Nsac: %d clk", card->nsac);
                lcd_puts(0, 3, pbuf);
                snprintf(pbuf, sizeof(pbuf), "R2W: *%d", card->r2w_factor);
                lcd_puts(0, 4, pbuf);
                snprintf(pbuf, sizeof(pbuf), "IRmax: %d..%d mA",
                         i_vmin[mmc_extract_bits(card->csd, 66, 3)],
                         i_vmax[mmc_extract_bits(card->csd, 69, 3)]);
                lcd_puts(0, 5, pbuf);
                snprintf(pbuf, sizeof(pbuf), "IWmax: %d..%d mA",
                         i_vmin[mmc_extract_bits(card->csd, 72, 3)],
                         i_vmax[mmc_extract_bits(card->csd, 75, 3)]);
                lcd_puts(0, 6, pbuf);
            }
        }
        else
            lcd_puts(0, 1, "Not found!");

        lcd_update();

        switch (button_get_w_tmo(HZ/2))
        {
            case SETTINGS_OK:
            case SETTINGS_CANCEL:
                done = true;
                break;
                
            case SETTINGS_DEC:
                currval--;
                if (currval < 0)
                    currval = 3;
                break;

            case SETTINGS_INC:
                currval++;
                if (currval > 3)
                    currval = 0;
                break;
        }
    }

    return false;
}
#else /* Disk-based jukebox */
static bool dbg_disk_info(void)
{
    char buf[128];
    bool done = false;
    int i;
    int page = 0;
    const int max_page = 11;
    unsigned short* identify_info = ata_get_identify();
    bool timing_info_present = false;
    char pio3[2], pio4[2];

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    
    while(!done)
    {
        int y=0;
        int key;
        lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
        lcd_puts(0, y++, "Disk info:");
        y++;
#endif

        switch (page) {
            case 0:
                for (i=0; i < 20; i++)
                    ((unsigned short*)buf)[i]=identify_info[i+27];
                buf[40]=0;
                /* kill trailing space */
                for (i=39; i && buf[i]==' '; i--)
                    buf[i] = 0;
                lcd_puts(0, y++, "Model");
                lcd_puts_scroll(0, y++, buf);
                break;

            case 1:
                for (i=0; i < 4; i++)
                    ((unsigned short*)buf)[i]=identify_info[i+23];
                buf[8]=0;
                lcd_puts(0, y++, "Firmware");
                lcd_puts(0, y++, buf);
                break;

            case 2:
                snprintf(buf, sizeof buf, "%d MB",
                         ((unsigned)identify_info[61] << 16 | 
                          (unsigned)identify_info[60]) / 2048 );
                lcd_puts(0, y++, "Size");
                lcd_puts(0, y++, buf);
                break;

            case 3: {
                unsigned int free;
                fat_size( IF_MV2(0,) NULL, &free );
                snprintf(buf, sizeof buf, "%d MB",  free / 1024 );
                lcd_puts(0, y++, "Free");
                lcd_puts(0, y++, buf);
                break;
            }

            case 4:
                snprintf(buf, sizeof buf, "%d ms", ata_spinup_time * (1000/HZ));
                lcd_puts(0, y++, "Spinup time");
                lcd_puts(0, y++, buf);
                break;

            case 5:
                i = identify_info[83] & (1<<3);
                lcd_puts(0, y++, "Power mgmt:");
                lcd_puts(0, y++, i ? "enabled" : "unsupported");
                break;

            case 6:
                i = identify_info[83] & (1<<9);
                lcd_puts(0, y++, "Noise mgmt:");
                lcd_puts(0, y++, i ? "enabled" : "unsupported");
                break;

            case 7:
                i = identify_info[82] & (1<<6);
                lcd_puts(0, y++, "Read-ahead:");
                lcd_puts(0, y++, i ? "enabled" : "unsupported");
                break;

            case 8:
                timing_info_present = identify_info[53] & (1<<1);
                if(timing_info_present) {
                    pio3[1] = 0;
                    pio4[1] = 0;
                    lcd_puts(0, y++, "PIO modes:");
                    pio3[0] = (identify_info[64] & (1<<0)) ? '3' : 0;
                    pio4[0] = (identify_info[64] & (1<<1)) ? '4' : 0;
                    snprintf(buf, 128, "0 1 2 %s %s", pio3, pio4);
                    lcd_puts(0, y++, buf);
                } else {
                    lcd_puts(0, y++, "No PIO mode info");
                }
                break;

            case 9:
                timing_info_present = identify_info[53] & (1<<1);
                if(timing_info_present) {
                    lcd_puts(0, y++, "Cycle times");
                    snprintf(buf, 128, "%dns/%dns",
                             identify_info[67],
                             identify_info[68]);
                    lcd_puts(0, y++, buf);
                } else {
                    lcd_puts(0, y++, "No timing info");
                }
                break;

            case 10:
                timing_info_present = identify_info[53] & (1<<1);
                if(timing_info_present) {
                    i = identify_info[49] & (1<<11);
                    snprintf(buf, 128, "IORDY support: %s", i ? "yes" : "no");
                    lcd_puts(0, y++, buf);
                    i = identify_info[49] & (1<<10);
                    snprintf(buf, 128, "IORDY disable: %s", i ? "yes" : "no");
                    lcd_puts(0, y++, buf);
                } else {
                    lcd_puts(0, y++, "No timing info");
                }
                break;

            case 11:
                lcd_puts(0, y++, "Cluster size");
                snprintf(buf, 128, "%d bytes", fat_get_cluster_size(IF_MV(0)));
                lcd_puts(0, y++, buf);
                break;
        }
        lcd_update();

        /* Wait for a key to be pushed */
        key = button_get_w_tmo(HZ*5);
        switch(key) {
            case SETTINGS_CANCEL:
                done = true;
                break;

            case SETTINGS_DEC:
                if (--page < 0)
                    page = max_page;
                break;
                
            case SETTINGS_INC:
                if (++page > max_page)
                    page = 0;
                break;

            case SETTINGS_OK:
                if (page == 3) {
                    mpeg_stop(); /* stop playback, to avoid disk access */
                    lcd_clear_display();
                    lcd_puts(0,0,"Scanning");
                    lcd_puts(0,1,"disk...");
                    lcd_update();
                    fat_recalc_free(IF_MV(0));
                }
                break;
        }
        lcd_stop_scroll();
    }

    return false;
}
#endif

bool dbg_save_roms(void)
{
    int fd;
    int oldmode = system_memory_guard(MEMGUARD_NONE);

    fd = creat("/internal_rom_0000-FFFF.bin", O_WRONLY);
    if(fd >= 0)
    {
        write(fd, (void *)0, 0x10000);
        close(fd);
    }
    
    fd = creat("/internal_rom_2000000-203FFFF.bin", O_WRONLY);
    if(fd >= 0)
    {
        write(fd, (void *)0x2000000, 0x40000);
        close(fd);
    }

    system_memory_guard(oldmode);
    return false;
}

#ifdef CONFIG_TUNER
extern int debug_fm_detection;

bool dbg_fm_radio(void)
{
    char buf[32];
    int button;
    bool fm_detected;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif

    while(1)
    {
        lcd_clear_display();
        fm_detected = radio_hardware_present();
        
        snprintf(buf, sizeof buf, "HW detected: %s", fm_detected?"yes":"no");
        lcd_puts(0, 0, buf);
        snprintf(buf, sizeof buf, "Result: %08x", debug_fm_detection);
        lcd_puts(0, 1, buf);
        lcd_update();
        
        button = button_get(true);

        switch(button)
        {
            case SETTINGS_CANCEL:
                return false;
        }
    }
    return false;
}
#endif

#ifdef HAVE_LCD_BITMAP
extern bool do_screendump_instead_of_usb;

bool dbg_screendump(void)
{
    do_screendump_instead_of_usb = !do_screendump_instead_of_usb;
    splash(HZ, true, "Screendump %s",
                 do_screendump_instead_of_usb?"enabled":"disabled");
    return false;
}
#endif

bool dbg_set_memory_guard(void)
{
    static const struct opt_items names[MAXMEMGUARD] = {
        { "None",             -1 },
        { "Flash ROM writes", -1 },
        { "Zero area (all)",  -1 }
    };
    int mode = system_memory_guard(MEMGUARD_KEEP);
    
    set_option( "Catch mem accesses", &mode, INT, names, MAXMEMGUARD, NULL);
    system_memory_guard(mode);

    return false;
}

bool debug_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { "Dump ROM contents", dbg_save_roms },
        { "View I/O ports", dbg_ports },
#ifdef HAVE_LCD_BITMAP
#ifdef HAVE_RTC
        { "View/clr RTC RAM", dbg_rtc },
#endif /* HAVE_RTC */
#endif /* HAVE_LCD_BITMAP */
        { "View OS stacks", dbg_os },
        { "Catch mem accesses", dbg_set_memory_guard },
#if CONFIG_HWCODEC == MAS3507D
        { "View MAS info", dbg_mas_info },
#endif
        { "View MAS regs", dbg_mas },
#if (CONFIG_HWCODEC == MAS3587F) || (CONFIG_HWCODEC == MAS3539F)
        { "View MAS codec", dbg_mas_codec },
#endif
#ifdef HAVE_LCD_BITMAP
        { "View battery", view_battery },
        { "Screendump", dbg_screendump },
#endif
        { "View HW info", dbg_hw_info },
        { "View partitions", dbg_partitions },
#ifdef HAVE_MMC
        { "View MMC info", dbg_mmc_info },
#else
        { "View disk info", dbg_disk_info },
#endif
#ifdef HAVE_LCD_BITMAP
        { "View mpeg thread", dbg_mpeg_thread },
#ifdef PM_DEBUG
        { "pm histogram", peak_meter_histogram},
#endif /* PM_DEBUG */
#endif /* HAVE_LCD_BITMAP */
        { "View runtime", view_runtime },
#ifdef CONFIG_TUNER
        { "FM Radio", dbg_fm_radio },
#endif
    };

    m=menu_init( items, sizeof items / sizeof(struct menu_item), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    
    return result;
}

#endif /* SIMULATOR */
