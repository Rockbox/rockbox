/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <file.h>

#include "language.h"
#include "lang.h"
#include "debug.h"

static unsigned char language_buffer[MAX_LANGUAGE_SIZE];

int lang_load(char *filename)
{
    int filesize;
    int fd = open(filename, O_RDONLY);
    int retcode=0;
    if(fd == -1)
        return 1;
    filesize = read(fd, language_buffer, MAX_LANGUAGE_SIZE);
    if(filesize != MAX_LANGUAGE_SIZE) {
        if((language_buffer[0] == LANGUAGE_COOKIE) &&
           (language_buffer[1] == LANGUAGE_VERSION)) {
            unsigned char *ptr=&language_buffer[2];
            int id;
            filesize-=2;

            while(filesize>3) {
                id = (ptr[0]<<8) | ptr[1];  /* get two-byte id */
                ptr+=2;                     /* pass the id */
                if(id < LANG_LAST_INDEX_IN_ARRAY) {
#ifdef SIMULATOR
                    printf("%2x New: %30s ", id, ptr);
                    printf("Replaces: %s\n", language_strings[id]);
#endif
                    language_strings[id] = ptr; /* point to this string */
                }
                while(*ptr) {               /* pass the string */
                    filesize--;
                    ptr++;
                }
                filesize-=3; /* the id and the terminating zero */
                ptr++;       /* pass the terminating zero-byte */
            }
        }
        else {
            DEBUGF("Illegal language file\n");
            retcode = 2;
        }
    }
    else {
        DEBUGF("Language %s too large: %d\n", filename, filesize);
        retcode = 3;
    }
    close(fd);
    return retcode;
}

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../firmware/rockbox-mode.el")
 * end:
 * vim: et sw=4 ts=8 sts=4 tw=78
 */
