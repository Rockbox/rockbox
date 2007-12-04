/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: metadata.c 15859 2007-11-30 18:48:07Z lostlogic $
 *
 * Copyright (C) 2005 Magnus Holmgren
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "id3.h"
#include "logf.h"

static const unsigned short a52_bitrates[] =
{
     32,  40,  48,  56,  64,  80,  96, 112, 128, 160, 
    192, 224, 256, 320, 384, 448, 512, 576, 640
};

/* Only store frame sizes for 44.1KHz - others are simply multiples 
   of the bitrate */
static const unsigned short a52_441framesizes[] =
{
      69 * 2,   70 * 2,   87 * 2,   88 * 2,  104 * 2,  105 * 2,  121 * 2, 
     122 * 2,  139 * 2,  140 * 2,  174 * 2,  175 * 2,  208 * 2,  209 * 2,
     243 * 2,  244 * 2,  278 * 2,  279 * 2,  348 * 2,  349 * 2,  417 * 2,
     418 * 2,  487 * 2,  488 * 2,  557 * 2,  558 * 2,  696 * 2,  697 * 2,  
     835 * 2,  836 * 2,  975 * 2,  976 * 2, 1114 * 2, 1115 * 2, 1253 * 2, 
    1254 * 2, 1393 * 2, 1394 * 2
};

bool get_a52_metadata(int fd, struct mp3entry *id3)
{        
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    unsigned long totalsamples;
    int i;

    if ((lseek(fd, 0, SEEK_SET) < 0) || (read(fd, buf, 5) < 5))
    {
        return false;
    }

    if ((buf[0] != 0x0b) || (buf[1] != 0x77)) 
    { 
        logf("%s is not an A52/AC3 file\n",trackname);
        return false;
    }

    i = buf[4] & 0x3e;
  
    if (i > 36) 
    {
        logf("A52: Invalid frmsizecod: %d\n",i);
        return false;
    }
  
    id3->bitrate = a52_bitrates[i >> 1];
    id3->vbr = false;
    id3->filesize = filesize(fd);

    switch (buf[4] & 0xc0) 
    {
    case 0x00: 
        id3->frequency = 48000;
        id3->bytesperframe=id3->bitrate * 2 * 2;
        break;
        
    case 0x40: 
        id3->frequency = 44100;
        id3->bytesperframe = a52_441framesizes[i];
        break;
    
    case 0x80: 
        id3->frequency = 32000;
        id3->bytesperframe = id3->bitrate * 3 * 2;
        break;
    
    default: 
        logf("A52: Invalid samplerate code: 0x%02x\n", buf[4] & 0xc0);
        return false;
        break;
    }

    /* One A52 frame contains 6 blocks, each containing 256 samples */
    totalsamples = id3->filesize / id3->bytesperframe * 6 * 256;
    id3->length = totalsamples / id3->frequency * 1000;
    return true;
}
