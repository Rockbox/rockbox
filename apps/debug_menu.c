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
#include "audio.h"
#include "mp3_playback.h"
#include "settings.h"
#include "ata.h"
#include "fat.h"
#include "dir.h"
#include "panic.h"
#include "screens.h"
#include "misc.h"
#include "splash.h"
#include "lcd-remote.h"

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
#include "logfdisp.h"
#if CONFIG_CODEC == SWCODEC
#include "pcmbuf.h"
#include "pcm_playback.h"
#endif

/*---------------------------------------------------*/
/*    SPECIAL DEBUG STUFF                            */
/*---------------------------------------------------*/
extern char ata_device;
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

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
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
#else /* !HAVE_LCD_BITMAP */
bool dbg_os(void)
{
    char buf[32];
    int button;
    int usage;
    int currval = 0;

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
#endif /* !HAVE_LCD_BITMAP */

#ifdef HAVE_LCD_BITMAP
#if CONFIG_CODEC != SWCODEC
bool dbg_audio_thread(void)
{
    char buf[32];
    int button;
    struct audio_debug d;

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
    
    while(1)
    {
        button = button_get_w_tmo(HZ/5);
        switch(button)
        {
            case SETTINGS_CANCEL:
                return false;
        }

        audio_get_debugdata(&d);
        
        lcd_clear_display();

        snprintf(buf, sizeof(buf), "read: %x", d.audiobuf_read);
        lcd_puts(0, 0, buf);
        snprintf(buf, sizeof(buf), "write: %x", d.audiobuf_write);
        lcd_puts(0, 1, buf);
        snprintf(buf, sizeof(buf), "swap: %x", d.audiobuf_swapwrite);
        lcd_puts(0, 2, buf);
        snprintf(buf, sizeof(buf), "playing: %d", d.playing);
        lcd_puts(0, 3, buf);
        snprintf(buf, sizeof(buf), "playable: %x", d.playable_space);
        lcd_puts(0, 4, buf);
        snprintf(buf, sizeof(buf), "unswapped: %x", d.unswapped_space);
        lcd_puts(0, 5, buf);

        /* Playable space left */
        scrollbar(0, 6*8, 112, 4, d.audiobuflen, 0, 
                  d.playable_space, HORIZONTAL);

        /* Show the watermark limit */
        scrollbar(0, 6*8+4, 112, 4, d.audiobuflen, 0, 
                  d.low_watermark_level, HORIZONTAL);

        snprintf(buf, sizeof(buf), "wm: %x - %x",
                 d.low_watermark_level, d.lowest_watermark_level);
        lcd_puts(0, 7, buf);
        
        lcd_update();
    }
    return false;
}
#else /* CONFIG_CODEC == SWCODEC */
extern size_t audiobuffer_free;
extern int filebuflen;
extern int filebufused;
extern int track_count;

static unsigned int ticks, boost_ticks;

void dbg_audio_task(void)
{
    if(FREQ > CPUFREQ_NORMAL)
        boost_ticks++;

    ticks++;
}

bool dbg_audio_thread(void)
{
    char buf[32];
    int button;
    int line;
    bool done = false;
    size_t bufsize = pcmbuf_get_bufsize();
    int pcmbufdescs = pcmbuf_descs();

    ticks = boost_ticks = 0;

    tick_add_task(dbg_audio_task);
    
    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
    
    while(!done)
    {
        button = button_get_w_tmo(HZ/5);
        switch(button)
        {
            case SETTINGS_NEXT:
                audio_next();
                break;
            case SETTINGS_PREV:
                audio_prev();
                break;
            case SETTINGS_CANCEL:
                done = true;
                break;
        }

        line = 0;
        
        lcd_clear_display();

        snprintf(buf, sizeof(buf), "pcm: %7ld/%7ld",
                 bufsize-audiobuffer_free, bufsize);
        lcd_puts(0, line++, buf);

        /* Playable space left */
        scrollbar(0, line*8, LCD_WIDTH, 6, bufsize, 0,
                  bufsize-audiobuffer_free, HORIZONTAL);
        line++;

        snprintf(buf, sizeof(buf), "codec: %8d/%8d", filebufused, filebuflen);
        lcd_puts(0, line++, buf);

        /* Playable space left */
        scrollbar(0, line*8, LCD_WIDTH, 6, filebuflen, 0,
                  filebufused, HORIZONTAL);
        line++;

        snprintf(buf, sizeof(buf), "track count: %2d", track_count);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "cpu freq: %3dMHz",
                 (int)((FREQ + 500000) / 1000000));
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "boost ratio: %3d%%",
                 boost_ticks * 100 / ticks);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "pcmbufdesc: %2d/%2d",
                pcmbuf_used_descs(), pcmbufdescs);
        lcd_puts(0, line++, buf);
        
        lcd_update();
    }

    tick_remove_task(dbg_audio_task);
    
    return false;
}
#endif /* CONFIG_CODEC */
#endif /* HAVE_LCD_BITMAP */

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


#if CONFIG_CPU == TCC730
static unsigned flash_word_temp __attribute__ ((section (".idata")));

static void flash_write_word(unsigned addr, unsigned value) __attribute__ ((section(".icode")));
static void flash_write_word(unsigned addr, unsigned value) {
    flash_word_temp = value;

    long extAddr = (long)addr << 1;
    ddma_transfer(1, 1, &flash_word_temp, extAddr, 2);  
}

static unsigned flash_read_word(unsigned addr) __attribute__ ((section(".icode")));
static unsigned flash_read_word(unsigned addr) {
    long extAddr = (long)addr << 1;
    ddma_transfer(1, 1, &flash_word_temp, extAddr, 2);
    return flash_word_temp;
}

#endif

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
#if (CONFIG_CPU == PP5002) || (CONFIG_CPU == PP5020)
    /* TODO: Implement for iPod */
    (void)p_manufacturer;
    (void)p_device;
    (void)addr1;
    (void)addr2;
#elif CONFIG_CPU == PNX0101
    /* TODO: Implement for iFP7xx */
    (void)p_manufacturer;
    (void)p_device;
    (void)addr1;
    (void)addr2;
#else
    unsigned not_manu, not_id; /* read values before switching to ID mode */
    unsigned manu, id; /* read values when in ID mode */
#if CONFIG_CPU == TCC730
#define FLASH(addr) (flash_read_word(addr))
#define SET_FLASH(addr, val) (flash_write_word((addr), (val)))

#else /* memory mapped */
#if CONFIG_CPU == SH7034
    volatile unsigned char* flash = (unsigned char*)0x2000000; /* flash mapping */
#elif defined(CPU_COLDFIRE)
    volatile unsigned short* flash = (unsigned short*)0; /* flash mapping */
#endif
#define FLASH(addr) (flash[addr])
#define SET_FLASH(addr, val) (flash[addr] = val)
#endif
    int old_level; /* saved interrupt level */

    not_manu = FLASH(0); /* read the normal content */
    not_id   = FLASH(1); /* should be 'A' (0x41) and 'R' (0x52) from the "ARCH" marker */

    /* disable interrupts, prevent any stray flash access */
    old_level = set_irq_level(HIGHEST_IRQ_LEVEL);

    SET_FLASH(addr1, 0xAA); /* enter command mode */
    SET_FLASH(addr2, 0x55);
    SET_FLASH(addr1, 0x90); /* ID command */
    /* Atmel wants 20ms pause here */
    /* sleep(HZ/50); no sleeping possible while interrupts are disabled */

    manu = FLASH(0); /* read the IDs */
    id   = FLASH(1);

    SET_FLASH(0, 0xF0); /* reset flash (back to normal read mode) */
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
#endif
    return false; /* fail */
}

#ifdef HAVE_LCD_BITMAP
bool dbg_hw_info(void)
{
#if CONFIG_CPU == SH7034
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
    if(PADRL & 0x20)
#else
    if(PADRH & 0x04)
#endif
        usb_polarity = 0; /* Negative */
    else
        usb_polarity = 1; /* Positive */

    if(PADRH & 0x08)
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
#elif CONFIG_CPU == MCF5249
    char buf[32];
    int button;
    unsigned manu, id; /* flash IDs */
    bool got_id; /* flag if we managed to get the flash IDs */
    int oldmode;  /* saved memory guard mode */

    oldmode = system_memory_guard(MEMGUARD_NONE);  /* disable memory guard */

    /* get flash ROM type */
    got_id = dbg_flash_id(&manu, &id, 0x5555, 0x2AAA); /* try SST, Atmel, NexFlash */
    if (!got_id)
        got_id = dbg_flash_id(&manu, &id, 0x555, 0x2AA); /* try AMD, Macronix */
    
    system_memory_guard(oldmode);  /* re-enable memory guard */

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    lcd_puts(0, 0, "[Hardware info]");

    if (got_id)
        snprintf(buf, 32, "Flash: M=%04x D=%04x", manu, id);
    else
        snprintf(buf, 32, "Flash: M=???? D=????"); /* unknown, sorry */
    lcd_puts(0, 1, buf);

    lcd_update();

    while(1)
    {
        button = button_get(true);
        if(button == SETTINGS_CANCEL)
            return false;
    }
#endif /* CONFIG_CPU */
    return false;
}
#else /* !HAVE_LCD_BITMAP */
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

    if(PADRH & 0x04)
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
#endif /* !HAVE_LCD_BITMAP */

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
        snprintf(buf, sizeof buf, "P%d: S:%lx", partition, p->start);
        lcd_puts(0, 0, buf);
        snprintf(buf, sizeof buf, "T:%x %ld MB", p->type, p->size / 2048);
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

#if defined(CPU_COLDFIRE) && defined(HAVE_SPDIF)
bool dbg_spdif(void)
{
    char buf[128];
    int button;
    int line;
    unsigned int control;
    int x;
    char *s;
    int category;
    int generation;
    unsigned int interruptstat;
    bool valnogood, symbolerr, parityerr;

    lcd_setmargins(0, 0);
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        line = 0;

        control = EBU1RCVCCHANNEL1;
        interruptstat = INTERRUPTSTAT;
        INTERRUPTCLEAR = 0x03c00000;

        valnogood = (interruptstat & 0x01000000)?true:false;
        symbolerr = (interruptstat & 0x00800000)?true:false;
        parityerr = (interruptstat & 0x00400000)?true:false;

        snprintf(buf, sizeof(buf), "Val: %s Sym: %s Par: %s",
                 valnogood?"--":"OK",
                 symbolerr?"--":"OK",
                 parityerr?"--":"OK");
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "Status word: %08x", (int)control);
        lcd_puts(0, line++, buf);

        line++;
        
        x = control >> 31;
        snprintf(buf, sizeof(buf), "PRO: %d (%s)",
                 x, x?"Professional":"Consumer");
        lcd_puts(0, line++, buf);
        
        x = (control >> 30) & 1;
        snprintf(buf, sizeof(buf), "Audio: %d (%s)",
                 x, x?"Non-PCM":"PCM");
        lcd_puts(0, line++, buf);
        
        x = (control >> 29) & 1;
        snprintf(buf, sizeof(buf), "Copy: %d (%s)",
                 x, x?"Permitted":"Inhibited");
        lcd_puts(0, line++, buf);
        
        x = (control >> 27) & 7;
        switch(x)
        {
        case 0:
            s = "None";
            break;
        case 1:
            s = "50/15us";
            break;
        default:
            s = "Reserved";
            break;
        }
        snprintf(buf, sizeof(buf), "Preemphasis: %d (%s)", x, s);
        lcd_puts(0, line++, buf);

        x = (control >> 24) & 3;
        snprintf(buf, sizeof(buf), "Mode: %d", x);
        lcd_puts(0, line++, buf);
        
        category = (control >> 17) & 127;
        switch(category)
        {
        case 0x00:
            s = "General";
            break;
        case 0x40:
            s = "Audio CD";
            break;
        default:
            s = "Unknown";
        }
        snprintf(buf, sizeof(buf), "Category: 0x%02x (%s)", category, s);
        lcd_puts(0, line++, buf);
        
        x = (control >> 16) & 1;
        generation = x;
        if(((category & 0x70) == 0x10) ||
           ((category & 0x70) == 0x40) ||
           ((category & 0x78) == 0x38))
        {
            generation = !generation;
        }
        snprintf(buf, sizeof(buf), "Generation: %d (%s)",
                 x, generation?"Original":"No ind.");
        lcd_puts(0, line++, buf);
        
        x = (control >> 12) & 15;
        snprintf(buf, sizeof(buf), "Source: %d", x);
        lcd_puts(0, line++, buf);
        
        x = (control >> 8) & 15;
        switch(x)
        {
        case 0:
            s = "Unspecified";
            break;
        case 8:
            s = "A (Left)";
            break;
        case 4:
            s = "B (Right)";
            break;
        default:
            s = "";
            break;
        }
        snprintf(buf, sizeof(buf), "Channel: %d (%s)", x, s);
        lcd_puts(0, line++, buf);
        
        x = (control >> 4) & 15;
        switch(x)
        {
        case 0:
            s = "44.1kHz";
            break;
        case 0x4:
            s = "48kHz";
            break;
        case 0xc:
            s = "32kHz";
            break;
        }
        snprintf(buf, sizeof(buf), "Frequency: %d (%s)", x, s);
        lcd_puts(0, line++, buf);
        
        x = (control >> 2) & 3;
        snprintf(buf, sizeof(buf), "Clock accuracy: %d", x);
        lcd_puts(0, line++, buf);
        
        lcd_update();
        button = button_get_w_tmo(HZ/10);

        switch(button)
        {
        case SETTINGS_CANCEL:
        case SETTINGS_OK2:
                return false;
        }
    }

    return false;
}
#endif /* CPU_COLDFIRE */

#ifdef HAVE_LCD_BITMAP
/* Test code!!! */
bool dbg_ports(void)
{
#if CONFIG_CPU == SH7034
    unsigned short porta;
    unsigned short portb;
    unsigned char portc;
    char buf[32];
    int button;
    int battery_voltage;
    int batt_int, batt_frac;

    lcd_setfont(FONT_SYSFIXED);
    lcd_setmargins(0, 0);
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
#elif defined(CPU_COLDFIRE)
    unsigned int gpio_out;
    unsigned int gpio1_out;
    unsigned int gpio_read;
    unsigned int gpio1_read;
    unsigned int gpio_function;
    unsigned int gpio1_function;
    unsigned int gpio_enable;
    unsigned int gpio1_enable;
    int adc_buttons, adc_remote, adc_battery;
#ifdef IRIVER
    int adc_remotedetect;
#endif
    char buf[128];
    int button;
    int line;
    int battery_voltage;
    int batt_int, batt_frac;

    lcd_setmargins(0, 0);
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        line = 0;
        gpio_read = GPIO_READ;
        gpio1_read = GPIO1_READ;
        gpio_out = GPIO_OUT;
        gpio1_out = GPIO1_OUT;
        gpio_function = GPIO_FUNCTION;
        gpio1_function = GPIO1_FUNCTION;
        gpio_enable = GPIO_ENABLE;
        gpio1_enable = GPIO1_ENABLE;
        
        snprintf(buf, sizeof(buf), "GPIO_READ:     %08x", gpio_read);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_OUT:      %08x", gpio_out);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_FUNCTION: %08x", gpio_function);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_ENABLE:   %08x", gpio_enable);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "GPIO1_READ:     %08x", gpio1_read);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO1_OUT:      %08x", gpio1_out);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO1_FUNCTION: %08x", gpio1_function);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO1_ENABLE:   %08x", gpio1_enable);
        lcd_puts(0, line++, buf);

        adc_buttons = adc_read(ADC_BUTTONS);
        adc_remote = adc_read(ADC_REMOTE);
        adc_battery = adc_read(ADC_BATTERY);
#ifdef IRIVER
        adc_remotedetect = adc_read(ADC_REMOTEDETECT);
#endif
        
        snprintf(buf, sizeof(buf), "ADC_BUTTONS: %02x", adc_buttons);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_REMOTE:  %02x", adc_remote);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_BATTERY: %02x", adc_battery);
        lcd_puts(0, line++, buf);
#ifdef IRIVER
        snprintf(buf, sizeof(buf), "ADC_REMOTEDETECT: %02x", adc_remotedetect);
        lcd_puts(0, line++, buf);
#endif

        battery_voltage = (adc_battery * BATTERY_SCALE_FACTOR) / 10000;
        batt_int = battery_voltage / 100;
        batt_frac = battery_voltage % 100;
    
        snprintf(buf, 32, "Batt: %d.%02dV %d%%  ", batt_int, batt_frac,
                 battery_level());
        lcd_puts(0, line++, buf);
        
#ifdef IRIVER
        snprintf(buf, sizeof(buf), "remotetype:: %d", remote_type());
        lcd_puts(0, line++, buf);
#endif
        
        lcd_update();
        button = button_get_w_tmo(HZ/10);

        switch(button)
        {   /* quit on release to allow for reading the cancel button input */
            case (SETTINGS_CANCEL|BUTTON_REL):  
                return false;
        }
    }

#elif CONFIG_CPU == PP5020

    unsigned int gpio_a, gpio_b, gpio_c, gpio_d;
    unsigned int gpio_e, gpio_f, gpio_g, gpio_h;
    unsigned int gpio_i, gpio_j, gpio_k, gpio_l;

    char buf[128];
    int button;
    int line;

    lcd_setmargins(0, 0);
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        gpio_a = GPIOA_INPUT_VAL;
        gpio_b = GPIOB_INPUT_VAL;
        gpio_c = GPIOC_INPUT_VAL;
        gpio_d = GPIOD_INPUT_VAL;

        line = 0;
        snprintf(buf, sizeof(buf), "IPOD version: 0x%08x", ipod_hw_rev);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_A:       %02x", gpio_a);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_B:       %02x", gpio_b);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_C:       %02x", gpio_c);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_D:       %02x", gpio_d);
        lcd_puts(0, line++, buf);
        line++;

        gpio_e = GPIOE_INPUT_VAL;
        gpio_f = GPIOF_INPUT_VAL;
        gpio_g = GPIOG_INPUT_VAL;
        gpio_h = GPIOH_INPUT_VAL;

        snprintf(buf, sizeof(buf), "GPIO_E:       %02x", gpio_e);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_F:       %02x", gpio_f);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_G:       %02x", gpio_g);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_H:       %02x", gpio_h);
        lcd_puts(0, line++, buf);
        line++;

        gpio_i = GPIOI_INPUT_VAL;
        gpio_j = GPIOJ_INPUT_VAL;
        gpio_k = GPIOK_INPUT_VAL;
        gpio_l = GPIOL_INPUT_VAL;

        snprintf(buf, sizeof(buf), "GPIO_I:       %02x", gpio_i);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_J:       %02x", gpio_j);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_K:       %02x", gpio_k);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_L:       %02x", gpio_l);
        lcd_puts(0, line++, buf);
      
        lcd_update();
        button = button_get_w_tmo(HZ/10);

        switch(button)
        {
            case SETTINGS_CANCEL:
                return false;
        }
    }

#endif /* CPU */
    return false;
}
#else /* !HAVE_LCD_BITMAP */
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
#endif /* !HAVE_LCD_BITMAP */

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
extern int boost_counter;
bool dbg_cpufreq(void)
{
    char buf[128];
    int line;
    int button;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
#endif
    lcd_clear_display();

    while(1)
    {
        line = 0;
        
        snprintf(buf, sizeof(buf), "Frequency: %ld", FREQ);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "boost_counter: %d", boost_counter);
        lcd_puts(0, line++, buf);

        lcd_update();
        button = button_get_w_tmo(HZ/10);

        switch(button)
        {
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD)
        case BUTTON_MENU:
#else
        case BUTTON_UP:
#endif
            cpu_boost(true);
            break;
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD)
        case BUTTON_PLAY:
#else 
        case BUTTON_DOWN:
#endif
            cpu_boost(false);
            break;

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD) || \
    (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD)
        case BUTTON_SELECT:
#else
        case BUTTON_PLAY:
#endif
            set_cpu_frequency(CPUFREQ_DEFAULT);
            boost_counter = 0;
            break;

#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD)
        case BUTTON_LEFT:
#else
        case SETTINGS_CANCEL:
        case SETTINGS_OK2:
#endif
            return false;
        }
    }

    return false;
}
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */
               
#ifdef HAVE_LCD_BITMAP
/*
 * view_battery() shows a automatically scaled graph of the battery voltage
 * over time. Usable for estimating battery life / charging rate.
 * The power_history array is updated in power_thread of powermgmt.c.
 */

#define BAT_LAST_VAL  MIN(LCD_WIDTH, POWER_HISTORY_LEN)
#define BAT_YSPACE    (LCD_HEIGHT - 20)

bool view_battery(void)
{
    int view = 0;
    int i, x, y;
    unsigned short maxv, minv;
    char buf[32];
    
    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        switch (view) {
            case 0: /* voltage history graph */
                /* Find maximum and minimum voltage for scaling */
                maxv = 0;
                minv = 65535;
                for (i = 0; i < BAT_LAST_VAL; i++) {
                    if (power_history[i] > maxv)
                        maxv = power_history[i];
                    if (power_history[i] && (power_history[i] < minv))
                    {
                        minv = power_history[i];
                    }
                }
                
                if ((minv < 1) || (minv >= 65535))
                    minv = 1;
                if (maxv < 2)
                    maxv = 2;
                    
                lcd_clear_display();
                snprintf(buf, 30, "Battery %d.%02d", power_history[0] / 100,
                         power_history[0] % 100);
                lcd_puts(0, 0, buf);
                snprintf(buf, 30, "scale %d.%02d-%d.%02d V", 
                         minv / 100, minv % 100, maxv / 100, maxv % 100);
                lcd_puts(0, 1, buf);
                
                x = 0;
                for (i = BAT_LAST_VAL - 1; i >= 0; i--) {
                    y = (power_history[i] - minv) * BAT_YSPACE / (maxv - minv);
                    lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                    lcd_vline(x, LCD_HEIGHT-1, 20);
                    lcd_set_drawmode(DRMODE_SOLID);
                    lcd_vline(x, LCD_HEIGHT-1,
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
#ifdef HAVE_CHARGE_CTRL
                snprintf(buf, 30, "Chgr: %s %s", 
                         charger_inserted() ? "present" : "absent",
                         charger_enabled ? "on" : "off");
                lcd_puts(0, 3, buf);
                snprintf(buf, 30, "short delta: %d", short_delta);
                lcd_puts(0, 5, buf);
                snprintf(buf, 30, "long delta: %d", long_delta);
                lcd_puts(0, 6, buf);
                lcd_puts(0, 7, power_message);
#else /* !HAVE_CHARGE_CTRL */
#if defined APPLE_IPODNANO || defined APPLE_IPODVIDEO 
                int usb_pwr  = (GPIOL_INPUT_VAL & 0x10)?true:false;
                int ext_pwr  = (GPIOL_INPUT_VAL & 0x08)?false:true;
                int dock     = (GPIOA_INPUT_VAL & 0x10)?true:false;
                int charging = (GPIOB_INPUT_VAL & 0x01)?false:true;
                int headphone= (GPIOA_INPUT_VAL & 0x80)?true:false;

                snprintf(buf, 30, "USB pwr:   %s",
                         usb_pwr ? "present" : "absent");
                lcd_puts(0, 3, buf);
                snprintf(buf, 30, "EXT pwr:   %s",
                         ext_pwr ? "present" : "absent");
                lcd_puts(0, 4, buf);
                snprintf(buf, 30, "Battery:   %s",
					charging ? "charging" : (usb_pwr||ext_pwr) ? "charged" : "discharging");
                lcd_puts(0, 5, buf);
                snprintf(buf, 30, "Dock mode: %s",
                         dock    ? "enabled" : "disabled");
                lcd_puts(0, 6, buf);
                snprintf(buf, 30, "Headphone: %s",
                         headphone ? "connected" : "disconnected");
                lcd_puts(0, 7, buf);
#else
                snprintf(buf, 30, "Charger: %s",
                         charger_inserted() ? "present" : "absent");
                lcd_puts(0, 3, buf);
#endif
#endif /* !HAVE_CHARGE_CTRL */
#endif /* HAVE_CHARGING */
                break;
                
            case 2: /* voltage deltas: */
                lcd_clear_display();
                lcd_puts(0, 0, "Voltage deltas:");
                
                for (i = 0; i <= 6; i++) {
                    y = power_history[i] - power_history[i+i];
                    snprintf(buf, 30, "-%d min: %s%d.%02d V", i,
                             (y < 0) ? "-" : "", ((y < 0) ? y * -1 : y) / 100, 
                             ((y < 0) ? y * -1 : y ) % 100);
                    lcd_puts(0, i+1, buf);
                }
                break;

            case 3: /* remaining time estimation: */
                lcd_clear_display();

#ifdef HAVE_CHARGE_CTRL
                snprintf(buf, 30, "charge_state: %d", charge_state);
                lcd_puts(0, 0, buf);

                snprintf(buf, 30, "Cycle time: %d m", powermgmt_last_cycle_startstop_min);
                lcd_puts(0, 1, buf);

                snprintf(buf, 30, "Lvl@cyc st: %d%%", powermgmt_last_cycle_level);
                lcd_puts(0, 2, buf);

                snprintf(buf, 30, "P=%2d I=%2d", pid_p, pid_i);
                lcd_puts(0, 3, buf);

                snprintf(buf, 30, "Trickle sec: %d/60", trickle_sec);
                lcd_puts(0, 4, buf);
#endif /* HAVE_CHARGE_CTRL */

                snprintf(buf, 30, "Last PwrHist: %d.%02d V",
                    power_history[0] / 100,
                    power_history[0] % 100);
                lcd_puts(0, 5, buf);

                snprintf(buf, 30, "battery level: %d%%", battery_level());
                lcd_puts(0, 6, buf);

                snprintf(buf, 30, "Est. remain: %d m", battery_time());
                lcd_puts(0, 7, buf);
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

#endif /* HAVE_LCD_BITMAP */
                              
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
#ifdef HAVE_CHARGING
            if (charger_inserted())
            {
                global_settings.runtime = 0;
            }
            else
#endif
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
bool dbg_mmc_info(void)
{
    bool done = false;
    int currval = 0;
    tCardInfo *card;
    unsigned char pbuf[32], pbuf2[32];
    unsigned char card_name[7];

    static const unsigned char i_vmin[] = { 0, 1, 5, 10, 25, 35, 60, 100 };
    static const unsigned char i_vmax[] = { 1, 5, 10, 25, 35, 45, 80, 200 };
    static const unsigned char *kbit_units[] = { "kBit/s", "MBit/s", "GBit/s" };
    static const unsigned char *nsec_units[] = { "ns", "µs", "ms" };
    
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
                         (int) mmc_extract_bits(card->cid, 72, 4),
                         (int) mmc_extract_bits(card->cid, 76, 4));
                lcd_puts(0, 1, pbuf);
                snprintf(pbuf, sizeof(pbuf), "Prod: %d/%d",
                         (int) mmc_extract_bits(card->cid, 112, 4),
                         (int) mmc_extract_bits(card->cid, 116, 4) + 1997);
                lcd_puts(0, 2, pbuf);
                snprintf(pbuf, sizeof(pbuf), "Ser#: 0x%08lx",
                         mmc_extract_bits(card->cid, 80, 32));
                lcd_puts(0, 3, pbuf);
                snprintf(pbuf, sizeof(pbuf), "M=%02x, O=%04x",
                         (int) mmc_extract_bits(card->cid, 0, 8),
                         (int) mmc_extract_bits(card->cid, 8, 16));
                lcd_puts(0, 4, pbuf);
                snprintf(pbuf, sizeof(pbuf), "Blocks: 0x%06lx", card->numblocks);
                lcd_puts(0, 5, pbuf);
                snprintf(pbuf, sizeof(pbuf), "Blksz.: %d", card->blocksize);
                lcd_puts(0, 6, pbuf);
            }
            else                  /* Technical details */
            {
                output_dyn_value(pbuf2, sizeof pbuf2, card->speed / 1000,
                                 kbit_units, false);
                snprintf(pbuf, sizeof pbuf, "Speed: %s", pbuf2);
                lcd_puts(0, 1, pbuf);

                output_dyn_value(pbuf2, sizeof pbuf2, card->tsac,
                                 nsec_units, false);
                snprintf(pbuf, sizeof pbuf, "Tsac: %s", pbuf2);
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
#else /* !HAVE_MMC */
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
                    ((unsigned short*)buf)[i]=htobe16(identify_info[i+27]);
                buf[40]=0;
                /* kill trailing space */
                for (i=39; i && buf[i]==' '; i--)
                    buf[i] = 0;
                lcd_puts(0, y++, "Model");
                lcd_puts_scroll(0, y++, buf);
                break;

            case 1:
                for (i=0; i < 4; i++)
                    ((unsigned short*)buf)[i]=htobe16(identify_info[i+23]);
                buf[8]=0;
                lcd_puts(0, y++, "Firmware");
                lcd_puts(0, y++, buf);
                break;

            case 2:
                snprintf(buf, sizeof buf, "%ld MB",
                         ((unsigned long)identify_info[61] << 16 | 
                          (unsigned long)identify_info[60]) / 2048 );
                lcd_puts(0, y++, "Size");
                lcd_puts(0, y++, buf);
                break;

            case 3: {
                unsigned long free;
                fat_size( IF_MV2(0,) NULL, &free );
                snprintf(buf, sizeof buf, "%ld MB",  free / 1024 );
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
                    audio_stop(); /* stop playback, to avoid disk access */
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
#endif /* !HAVE_MMC */

#ifdef HAVE_DIRCACHE
static bool dbg_dircache_info(void)
{
    bool done = false;
    int line;
    char buf[32];

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);

    while (!done)
    {
        line = 0;
        
        lcd_clear_display();
        snprintf(buf, sizeof(buf), "Cache initialized: %s", 
                 dircache_is_enabled() ? "Yes" : "No");
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "Cache size: %d B",
                 dircache_get_cache_size());
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "Last size: %d B",
                 global_settings.dircache_size);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "Limit: %d B", DIRCACHE_LIMIT);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "Reserve: %d/%d B",
                 dircache_get_reserve_used(), DIRCACHE_RESERVE);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "Scanning took: %d s",
                 dircache_get_build_ticks() / HZ);
        lcd_puts(0, line++, buf);

        lcd_update();

        switch (button_get_w_tmo(HZ/2))
        {
            case SETTINGS_OK:
            case SETTINGS_CANCEL:
                done = true;
                break;
        }
    }

    return false;
}

#endif /* HAVE_DIRCACHE */

#if CONFIG_CPU == SH7034
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
#elif defined CPU_COLDFIRE
bool dbg_save_roms(void)
{
    int fd;
    int oldmode = system_memory_guard(MEMGUARD_NONE);

#if defined(IRIVER_H100_SERIES)
    fd = creat("/internal_rom_000000-1FFFFF.bin", O_WRONLY);
#elif defined(IRIVER_H300_SERIES)
    fd = creat("/internal_rom_000000-3FFFFF.bin", O_WRONLY);
#elif defined(IAUDIO_X5)
    fd = creat("/internal_rom_000000-3FFFFF.bin", O_WRONLY);
#endif
    if(fd >= 0)
    {
        write(fd, (void *)0, FLASH_SIZE);
        close(fd);
    }
    system_memory_guard(oldmode);
    return false;
}
#endif /* CPU */

#ifdef CONFIG_TUNER
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
#endif /* CONFIG_TUNER */

#ifdef HAVE_LCD_BITMAP
extern bool do_screendump_instead_of_usb;

bool dbg_screendump(void)
{
    do_screendump_instead_of_usb = !do_screendump_instead_of_usb;
    gui_syncsplash(HZ, true, "Screendump %s",
                 do_screendump_instead_of_usb?"enabled":"disabled");
    return false;
}
#endif /* HAVE_LCD_BITMAP */

#if CONFIG_CPU == SH7034 || defined(CPU_COLDFIRE)
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
#endif /* CONFIG_CPU == SH7034 || defined(CPU_COLDFIRE) */

bool debug_menu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
#if CONFIG_CPU == SH7034 || defined(CPU_COLDFIRE)
        { "Dump ROM contents", dbg_save_roms },
#endif
#if CONFIG_CPU == SH7034 || defined(CPU_COLDFIRE) || (CONFIG_CPU == PP5020)
        { "View I/O ports", dbg_ports },
#endif
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        { "CPU frequency", dbg_cpufreq },
#endif
#if defined(IRIVER_H100_SERIES) && !defined(SIMULATOR)
        { "S/PDIF analyzer", dbg_spdif },
#endif
#if CONFIG_CPU == SH7034 || defined(CPU_COLDFIRE)
        { "Catch mem accesses", dbg_set_memory_guard },
#endif
        { "View OS stacks", dbg_os },
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
#ifdef HAVE_DIRCACHE
        { "View dircache info", dbg_dircache_info },
#endif
#ifdef HAVE_LCD_BITMAP
        { "View audio thread", dbg_audio_thread },
#ifdef PM_DEBUG
        { "pm histogram", peak_meter_histogram},
#endif /* PM_DEBUG */
#endif /* HAVE_LCD_BITMAP */
        { "View runtime", view_runtime },
#ifdef CONFIG_TUNER
        { "FM Radio", dbg_fm_radio },
#endif
#ifdef ROCKBOX_HAS_LOGF
        {"logf", logfdisplay },
        {"logfdump", logfdump },
#endif
    };

    m=menu_init( items, sizeof items / sizeof(struct menu_item), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    
    return result;
}

#endif /* SIMULATOR */
