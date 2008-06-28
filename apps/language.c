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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <file.h>
#if defined(SIMULATOR) && defined(__MINGW32__)
extern int printf(const char *format, ...);
#endif

#include "language.h"
#include "lang.h"
#include "debug.h"
#include "string.h"

static unsigned char language_buffer[MAX_LANGUAGE_SIZE];

void lang_init(void)
{
    int i;
    unsigned char *ptr = (unsigned char *) language_builtin;
    
    for (i = 0; i < LANG_LAST_INDEX_IN_ARRAY; i++) {
        language_strings[i] = ptr;
        ptr += strlen((char *)ptr) + 1; /* advance pointer to next string */
    }
}

int lang_load(const char *filename)
{
    int fsize;
    int fd = open(filename, O_RDONLY);
    int retcode=0;
    unsigned char lang_header[3];
    if(fd < 0)
        return 1;
    fsize = filesize(fd) - 2;
    if(fsize <= MAX_LANGUAGE_SIZE) {
        read(fd, lang_header, 3);
        if((lang_header[0] == LANGUAGE_COOKIE) &&
           (lang_header[1] == LANGUAGE_VERSION) &&
           (lang_header[2] == TARGET_ID)) {
            read(fd, language_buffer, MAX_LANGUAGE_SIZE);
            unsigned char *ptr = language_buffer;
            int id;
            lang_init();                    /* initialize with builtin */

            while(fsize>3) {
                id = (ptr[0]<<8) | ptr[1];  /* get two-byte id */
                ptr+=2;                     /* pass the id */
                if(id < LANG_LAST_INDEX_IN_ARRAY) {
#if 0
                    printf("%2x New: %30s ", id, ptr);
                    printf("Replaces: %s\n", language_strings[id]);
#endif
                    language_strings[id] = ptr; /* point to this string */
                }
                while(*ptr) {               /* pass the string */
                    fsize--;
                    ptr++;
                }
                fsize-=3; /* the id and the terminating zero */
                ptr++;       /* pass the terminating zero-byte */
            }
        }
        else {
            DEBUGF("Illegal language file\n");
            retcode = 2;
        }
    }
    else {
        DEBUGF("Language %s too large: %d\n", filename, fsize);
        retcode = 3;
    }
    close(fd);
    return retcode;
}
