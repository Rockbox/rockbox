/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Grayscale framework
* gray_screendump() function
*
* This is a generic framework to use grayscale display within Rockbox
* plugins. It obviously does not work for the player.
*
* Copyright (C) 2004 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef SIMULATOR /* not for simulator by now */
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP /* and also not for the Player */
#include "gray.h"

static const unsigned char bmpheader[] =
{
    0x42, 0x4d, 0xba, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xba, 0x00,
    0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x40, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c,
    0x00, 0x00, 0xc4, 0x0e, 0x00, 0x00, 0xc4, 0x0e, 0x00, 0x00, 0x21, 0x00,
    0x00, 0x00, 0x21, 0x00, 0x00, 0x00
};

static unsigned char linebuf[LCD_WIDTH];

/*---------------------------------------------------------------------------
 Save the current display content (b&w and grayscale overlay) to an 8-bit
 BMP file in the root directory
 ----------------------------------------------------------------------------
 *
 * This one is rather slow if used with larger bit depths, but it's intended
 * primary use is for documenting the grayscale plugins. A much faster version
 * would be possible, but would take more than twice the RAM
 */
void gray_screendump(void)
{
    int fh, i, bright;
    int x, y, by, mask;
    int gx, gby;

    char filename[MAX_PATH];
    struct tm *tm = _gray_rb->get_time();

    unsigned char *lcdptr, *grayptr, *grayptr2;

    _gray_rb->snprintf(filename, MAX_PATH,
                       "/graydump %04d-%02d-%02d %02d-%02d-%02d.bmp",
                       tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                       tm->tm_hour, tm->tm_min, tm->tm_sec);
    fh = _gray_rb->creat(filename, O_WRONLY);

    if (fh < 0)
        return;
        
    _gray_rb->write(fh, bmpheader, sizeof(bmpheader));  /* write header */

    /* build clut, always 33 entries */

    linebuf[3] = 0;

    for (i = 0; i < 33; i++)   
    {
        bright = MIN(i, _graybuf->depth);
        linebuf[0] = linebuf[2] = MULU16(0x90, bright) / _graybuf->depth;
        linebuf[1] = MULU16(0xee, bright) / _graybuf->depth;
        _gray_rb->write(fh, linebuf, 4);
    }

    /* 8-bit BMP image goes bottom -> top */

    for (y = LCD_HEIGHT - 1; y >= 0; y--)
    {
        _gray_rb->memset(linebuf, 32, sizeof(linebuf)); /* max. brightness */
        
        mask = 1 << (y & 7);
        by = y / 8;
        lcdptr = _gray_rb->lcd_framebuffer + MULU16(LCD_WIDTH, by);
        gby = by - _graybuf->by;

        if ((_graybuf->flags & _GRAY_RUNNING)
            && (unsigned) gby < (unsigned) _graybuf->bheight)
        {   
            /* line contains grayscale (and maybe b&w) graphics */
            grayptr = _graybuf->data + MULU16(_graybuf->width, gby);

            for (x = 0; x < LCD_WIDTH; x++)
            {
                if (*lcdptr++ & mask)
                    linebuf[x] = 0;
            
                gx = x - _graybuf->x;
                
                if ((unsigned) gx < (unsigned) _graybuf->width)
                {
                    bright = 0;
                    grayptr2 = grayptr + gx;

                    for (i = 0; i < _graybuf->depth; i++)
                    {
                        if (!(*grayptr2 & mask))
                            bright++;
                        grayptr2 += _graybuf->plane_size;
                    }
                    linebuf[x] = bright;
                }
            }
        }
        else  
        {   
            /* line contains only b&w graphics */
            for (x = 0; x < LCD_WIDTH; x++)
                if (*lcdptr++ & mask)
                    linebuf[x] = 0;
        }

        _gray_rb->write(fh, linebuf, sizeof(linebuf));
    }

    _gray_rb->close(fh);
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR

