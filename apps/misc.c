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
#include "string.h"
#include "config.h"
#include "file.h"
#include "lcd.h"
#include "sprintf.h"
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

#ifdef SCREENDUMP
extern unsigned char lcd_framebuffer[LCD_WIDTH][LCD_HEIGHT/8];
static unsigned char bmpheader[] =
{
    0x42, 0x4d, 0x3e, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00,
    0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x40, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x00, 0x00, 0xc4, 0x0e, 0x00, 0x00, 0xc4, 0x0e, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xd0, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x00
};

static unsigned char buf[112*8];
static unsigned char buf2[112*8];
static char dummy[2] = {0, 0};
static int fileindex = 0;

void screen_dump(void)
{
    int f;
    int i, shift;
    int x, y;
    char filename[MAX_PATH];

    i = 0;
    for(y = 0;y < LCD_HEIGHT/8;y++)
    {
        for(x = 0;x < LCD_WIDTH;x++)
        {
            buf[i++] = lcd_framebuffer[x][y];
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

    snprintf(filename, MAX_PATH, "/dump%03d.bmp", fileindex++);
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
