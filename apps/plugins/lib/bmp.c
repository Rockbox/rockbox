/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Antoine Cellerier <dionoea -at- videolan -dot- org>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "bmp.h"

#include "lcd.h"
#include "file.h"
#include "lcd.h"
#include "system.h"

#define LE16(x) (htole16(x))&0xff, ((htole16(x))>>8)&0xff
#define LE32(x) (htole32(x))&0xff, ((htole32(x))>>8)&0xff, ((htole32(x))>>16)&0xff, ((htole32(x))>>24)&0xff
/**
 * Save to 24 bit bitmap.
 */
int save_bmp_file( char* filename, struct bitmap *bm, struct plugin_api* rb )
{
    /* I'm not really sure about this one :) */
    int line_width = bm->width*3+((bm->width*3)%4?4-((bm->width*3)%4):0);

    const unsigned char header[] =
    {
        0x42, 0x4d, /* signature - 'BM' */
        LE32( bm->height*line_width + 54 + 4*0 ), /* file size in bytes */
        0x00, 0x00, 0x00, 0x00, /* 0, 0 */
        LE32( 54 + 4*0 ), /* offset to bitmap */
        0x28, 0x00, 0x00, 0x00, /* size of this struct (40) */
        LE32( bm->width ), /* bmap width in pixels */
        LE32( bm->height ), /* bmap height in pixels */
        0x01, 0x00, /* num planes - always 1 */
        LE16( 24 ), /* bits per pixel */
        LE32( 0 ), /* compression flag */
        LE32( bm->height*line_width ), /* image size in bytes */
        0xc4, 0x0e, 0x00, 0x00, /* horz resolution */
        0xc4, 0x0e, 0x00, 0x00, /* vert resolution */
        LE32( 0 ), /* 0 -> color table size */
        LE32( 0 ) /* important color count */
    };

    int fh;
    int x,y;
    if( bm->format != FORMAT_NATIVE ) return -1;
    fh = rb->creat( filename );
    if( fh < 0 ) return -1;

    rb->write( fh, header, sizeof( header ) );
    for( y = bm->height-1; y >= 0; y-- )
    {
        for( x = 0; x <  bm->width; x++ )
        {
            fb_data *d = (fb_data*)( bm->data ) + (x+y*bm->width);
            unsigned char c[] =
            {
                RGB_UNPACK_BLUE( *d ),
                RGB_UNPACK_GREEN( *d ),
                RGB_UNPACK_RED( *d )
            };
            rb->write( fh, c, 3 );
        }
        for( x = 0; x < 3*bm->width - line_width; x++ )
        {
            unsigned char c = 0;
            rb->write( fh, &c, 1 );
        }
    }
    rb->close( fh );
    return 1;
}
