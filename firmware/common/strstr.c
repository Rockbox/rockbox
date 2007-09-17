/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2007 by Christian Gmeiner
 *
 ****************************************************************************/

#include <string.h>

/**
 * Locate substring.
 * @param search c string to be scanned.
 * @param find c string containing the sequence of characters to match.
 * @return a pointer to the first occurrence in search of any of the 
 *         entire sequence of characters specified in find, or a
 *         null pointer if the sequence is not present in search.
 */
char *strstr(const char *search, const char *find)
{
    char *hend;
    char *a, *b;

    if (*find == 0) return (char*)search;
    hend = (char *)search + strlen(search) - strlen(find) + 1;
    while (search < hend) {
        if (*search == *find) {
            a = (char *)search;
            b = (char *)find;
            for (;;) {
                if (*b == 0) return (char*)search;
                if (*a++ != *b++) {
                    break;
                }
            }
        }
        search++;
    }
    return 0;
}
