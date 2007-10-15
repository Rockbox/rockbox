/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Stepan Moskovchenko
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "midiutil.h"

extern struct plugin_api * rb;

int chVol[16] IBSS_ATTR;       /* Channel volume                */
int chPan[16] IBSS_ATTR;       /* Channel panning               */
int chPat[16] IBSS_ATTR;                  /* Channel patch                 */
int chPW[16] IBSS_ATTR;                   /* Channel pitch wheel, MSB only */
int chPBDepth[16] IBSS_ATTR;                   /* Channel pitch wheel, MSB only */

struct GPatch * gusload(char *);
struct GPatch * patchSet[128];
struct GPatch * drumSet[128];

struct SynthObject voices[MAX_VOICES] IBSS_ATTR;

void *alloc(int size)
{
    static char *offset = NULL;
    static ssize_t totalSize = 0;
    char *ret;

    int remainder = size % 4;

    size = size + 4-remainder;

    if (offset == NULL)
    {
        offset = rb->plugin_get_audio_buffer((size_t *)&totalSize);
    }

    if (size + 4 > totalSize)
    {
        printf("MALLOC BARF");
        printf("MALLOC BARF");
        printf("MALLOC BARF");
        printf("MALLOC BARF");
        printf("MALLOC BARF");
        printf("MALLOC BARF");
        printf("MALLOC BARF");
        /* We've made our point. */

        return NULL;
    }

    ret = offset + 4;
    *((unsigned int *)offset) = size;

    offset += size + 4;
    totalSize -= size + 4;
    return ret;
}

/* Rick's code */
/*
void *alloc(int size)
{
    static char *offset = NULL;
    static ssize_t totalSize = 0;
    char *ret;


    if (offset == NULL)
    {
        offset = rb->plugin_get_audio_buffer((size_t *)&totalSize);
    }

    if (size + 4 > totalSize)
    {
        return NULL;
    }

    ret = offset + 4;
    *((unsigned int *)offset) = size;

    offset += size + 4;
    totalSize -= size + 4;
    return ret;
}
*/

#define malloc(n) my_malloc(n)
void * my_malloc(int size)
{
    return alloc(size);
}

unsigned char readChar(int file)
{
    char buf[2];
    rb->read(file, &buf, 1);
    return buf[0];
}

unsigned char * readData(int file, int len)
{
    unsigned char * dat = malloc(len);
    rb->read(file, dat, len);
    return dat;
}

int eof(int fd)
{
    int curPos = rb->lseek(fd, 0, SEEK_CUR);

    int size = rb->lseek(fd, 0, SEEK_END);

    rb->lseek(fd, curPos, SEEK_SET);
    return size+1 == rb->lseek(fd, 0, SEEK_CUR);
}

// Here is a hacked up printf command to get the output from the game.
int printf(const char *fmt, ...)
{
   static int p_xtpt = 0;
   char p_buf[50];
   bool ok;
   va_list ap;

   va_start(ap, fmt);
   ok = rb->vsnprintf(p_buf,sizeof(p_buf), fmt, ap);
   va_end(ap);

   rb->lcd_putsxy(1,p_xtpt, (unsigned char *)p_buf);
   rb->lcd_update();

   p_xtpt+=8;
   if(p_xtpt>LCD_HEIGHT-8)
   {
      p_xtpt=0;
      rb->lcd_clear_display();
   }
   return 1;
}

void exit(int code)
{
    code = code; /* Stub function, kill warning for now */
}

