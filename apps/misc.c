/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdlib.h>
#include <ctype.h>
#include "lang.h"
#include "string.h"
#include "config.h"
#include "file.h"
#include "lcd.h"
#include "sprintf.h"
#include "errno.h"
#include "system.h"
#include "timefuncs.h"
#include "screens.h"
#include "mpeg.h"
#include "mp3_playback.h"
#include "settings.h"
#include "ata.h"
#include "kernel.h"
#include "power.h"
#include "backlight.h"
#ifdef HAVE_MMC
#include "ata_mmc.h"
#endif

#define ONE_KILOBYTE 1024
#define ONE_MEGABYTE (1024*1024)

/* The point of this function would be to return a string of the input data,
   but never longer than 5 columns. Add suffix k and M when suitable...
   Make sure to have space for 6 bytes in the buffer. 5 letters plus the
   terminating zero byte. */
char *num2max5(unsigned int bytes, char *max5)
{
    if(bytes < 100000) {
        snprintf(max5, 6, "%5d", bytes);
        return max5;
    }
    if(bytes < (9999*ONE_KILOBYTE)) {
        snprintf(max5, 6, "%4dk", bytes/ONE_KILOBYTE);
        return max5;
    }
    if(bytes < (100*ONE_MEGABYTE)) {
        /* 'XX.XM' is good as long as we're less than 100 megs */
        snprintf(max5, 6, "%2d.%0dM",
                 bytes/ONE_MEGABYTE,
                 (bytes%ONE_MEGABYTE)/(ONE_MEGABYTE/10) );
        return max5;
    }
    snprintf(max5, 6, "%4dM", bytes/ONE_MEGABYTE);
    return max5;
}

/* Read (up to) a line of text from fd into buffer and return number of bytes
 * read (which may be larger than the number of bytes stored in buffer). If 
 * an error occurs, -1 is returned (and buffer contains whatever could be 
 * read). A line is terminated by a LF char. Neither LF nor CR chars are 
 * stored in buffer.
 */
int read_line(int fd, char* buffer, int buffer_size)
{
    int count = 0;
    int num_read = 0;
    
    errno = 0;

    while (count < buffer_size)
    {
        unsigned char c;

        if (1 != read(fd, &c, 1))
            break;
        
        num_read++;
            
        if ( c == '\n' )
            break;

        if ( c == '\r' )
            continue;

        buffer[count++] = c;
    }

    buffer[MIN(count, buffer_size - 1)] = 0;

    return errno ? -1 : num_read;
}

#ifdef TEST_MAX5
int main(int argc, char **argv)
{
    char buffer[32];
    if(argc>1) {
        printf("%d => %s\n",
               atoi(argv[1]),
               num2max5(atoi(argv[1]), buffer));
    }
    return 0;
}

#endif

#ifdef HAVE_LCD_BITMAP
extern unsigned char lcd_framebuffer[LCD_HEIGHT/8][LCD_WIDTH];
static const unsigned char bmpheader[] =
{
    0x42, 0x4d, 0x3e, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00,
    0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x40, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x00, 0x00, 0xc4, 0x0e, 0x00, 0x00, 0xc4, 0x0e, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0xee, 0x90, 0x00, 0x00, 0x00,
    0x00, 0x00
};

void screen_dump(void)
{
    int fh;
    int bx, by, ix, iy;
    int src_byte, src_mask, dst_mask;
    char filename[MAX_PATH];
    static unsigned char line_block[8][16];
    struct tm *tm = get_time();
    
    snprintf(filename, MAX_PATH, "/dump %04d-%02d-%02d %02d-%02d-%02d.bmp",
             tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec);

    fh = creat(filename, O_WRONLY);
    if (fh < 0)
        return;

    write(fh, bmpheader, sizeof(bmpheader));

    /* BMP image goes bottom up */
    for (by = LCD_HEIGHT/8 - 1; by >= 0; by--)
    {
        memset(&line_block[0][0], 0, 8*16);

        for (bx = 0; bx < LCD_WIDTH/8; bx++)
        {
            dst_mask = 0x01;
            for (ix = 7; ix >= 0; ix--)
            {
                src_byte = lcd_framebuffer[by][8*bx+ix];
                src_mask = 0x01;
                for (iy = 7; iy >= 0; iy--)
                {
                    if (src_byte & src_mask)
                        line_block[iy][bx] |= dst_mask;
                    src_mask <<= 1;
                }
                dst_mask <<= 1;
            }
        }
        
        write(fh, &line_block[0][0], 8*16);
    }
    close(fh);
}
#endif

/* parse a line from a configuration file. the line format is:

   name: value

   Any whitespace before setting name or value (after ':') is ignored.
   A # as first non-whitespace character discards the whole line.
   Function sets pointers to null-terminated setting name and value.
   Returns false if no valid config entry was found.
*/

bool settings_parseline(char* line, char** name, char** value)
{
    char* ptr;

    while ( isspace(*line) )
        line++;

    if ( *line == '#' )
        return false;

    ptr = strchr(line, ':');
    if ( !ptr )
        return false;

    *name = line;
    *ptr = 0;
    ptr++;
    while (isspace(*ptr))
        ptr++;
    *value = ptr;
    return true;
}

bool clean_shutdown(void)
{
#ifdef SIMULATOR
    exit(0);
#else
    if(!charger_inserted())
    {
        lcd_clear_display();
        splash(0, true, str(LANG_SHUTTINGDOWN));
        mpeg_stop();
        ata_flush();
        ata_spindown(1);
        while(ata_disk_is_active())
            sleep(HZ/10);

        mp3_shutdown();
#if CONFIG_KEYPAD == ONDIO_PAD
        backlight_off();
        sleep(1);
        lcd_set_contrast(0);
#endif
        power_off();
    }
#endif
    return false;
}

int default_event_handler_ex(int event, void (*callback)(void *), void *parameter)
{
    switch(event)
    {
        case SYS_USB_CONNECTED:
            if (callback != NULL)
                callback(parameter);
#ifdef HAVE_MMC
            if (!mmc_detect() || (mmc_remove_request() == SYS_MMC_EXTRACTED))
#endif
                usb_screen();
            return SYS_USB_CONNECTED;
        case SYS_POWEROFF:
            if (callback != NULL)
                callback(parameter);
            if (!clean_shutdown())
                return SYS_POWEROFF;
            break;
    }
    return 0;
}

int default_event_handler(int event)
{
    return default_event_handler_ex(event, NULL, NULL);
}

