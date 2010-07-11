/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2002     Manuel Novoa III
 * Copyright (C) 2000-2005 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, code originally in uclibc
 *
 ****************************************************************************/
#include <string.h>

/* NOTE: This is the simple-minded O(len(s1) * len(s2)) worst-case approach. */

char *strstr(const char *s1, const char *s2)
{
    register const char *s = s1;
    register const char *p = s2;

    do {
        if (!*p) {
            return (char *) s1;
        }
        if (*p == *s) {
            ++p;
            ++s;
        } else {
            p = s2;
            if (!*s) {
                return NULL;
            }
            s = ++s1;
        }
    } while (1);
}
