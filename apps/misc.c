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
static unsigned char bmpheader[] =
{
    0x42, 0x4d, 0x3e, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00,
    0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x40, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x00, 0x00, 0xc4, 0x0e, 0x00, 0x00, 0xc4, 0x0e, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0xee, 0x90, 0x00, 0x00, 0x00,
    0x00, 0x00
};

static unsigned char buf[112*8];
static unsigned char buf2[112*8];
static char dummy[2] = {0, 0};

void screen_dump(void)
{
    int f;
    int i, shift;
    int x, y;
    char filename[MAX_PATH];
    struct tm *tm = get_time();

    i = 0;
    for(y = 0;y < LCD_HEIGHT/8;y++)
    {
        for(x = 0;x < LCD_WIDTH;x++)
        {
            buf[i++] = lcd_framebuffer[y][x];
        }
    }

    memset(buf2, 0, sizeof(buf2));
    
    for(y = 0;y < 64;y++)
    {
        shift = y & 7;
        
        for(x = 0;x < 112/8;x++)
        {
            for(i = 0;i < 8;i++)
            {
                buf2[y*112/8+x] |= ((buf[y/8*112+x*8+i] >> shift)
                                    & 0x01) << (7-i);
            }
        }
    }

    snprintf(filename, MAX_PATH, "/dump %04d-%02d-%02d %02d-%02d-%02d.bmp",
             tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec);
    f = creat(filename, O_WRONLY);
    if(f >= 0)
    {
        write(f, bmpheader, sizeof(bmpheader));
        
        for(i = 63;i >= 0;i--)
        {
            write(f, &buf2[i*14], 14);
            write(f, dummy, 2);
        }
        
        close(f);
    }
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
#ifndef SIMULATOR
    if(!charger_inserted())
    {
        lcd_clear_display();
        splash(0, true, str(LANG_SHUTTINGDOWN));
        mpeg_stop();
        settings_save();
        ata_flush();
        ata_spindown(1);
        while(ata_disk_is_active())
            sleep(HZ/10);
        mp3_shutdown();
        power_off();
    }
#endif
    return false;
}
