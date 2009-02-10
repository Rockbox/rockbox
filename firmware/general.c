/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Michael Sevakis
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

#include "config.h"
#include "general.h"

#include "dir.h"
#include "limits.h"
#include "sprintf.h"
#include "stdlib.h"
#include "string.h"
#include "system.h"
#include "time.h"
#include "timefuncs.h"

#if CONFIG_CODEC == SWCODEC
int round_value_to_list32(unsigned long value,
                          const unsigned long list[],
                          int count,
                          bool signd)
{
    unsigned long dmin = ULONG_MAX;
    int idmin = -1, i;

    for (i = 0; i < count; i++)
    {
        unsigned long diff;

        if (list[i] == value)
        {
            idmin = i;
            break;
        }

        if (signd ? ((long)list[i] < (long)value) : (list[i] < value))
            diff = value - list[i];
        else
            diff = list[i] - value;

        if (diff < dmin)
        {
            dmin = diff;
            idmin = i;
        }
    }

    return idmin;
} /* round_value_to_list32 */

/* Number of bits set in src_mask should equal src_list length */
int make_list_from_caps32(unsigned long src_mask,
                          const unsigned long *src_list,
                          unsigned long caps_mask,
                          unsigned long *caps_list)
{
    int i, count;
    unsigned long mask;

    for (mask = src_mask, count = 0, i = 0;
         mask != 0;
         src_mask = mask, i++)
    {
        unsigned long test_bit;
        mask &= mask - 1;           /* Zero lowest bit set */
        test_bit = mask ^ src_mask; /* Isolate the bit */
        if (test_bit & caps_mask)   /* Add item if caps has test bit set */
            caps_list[count++] = src_list ? src_list[i] : (unsigned long)i;
    }

    return count;
} /* make_list_from_caps32 */
#endif /* CONFIG_CODEC == SWCODEC */

/* Create a filename with a number part in a way that the number is 1
 * higher than the highest numbered file matching the same pattern.
 * It is allowed that buffer and path point to the same memory location,
 * saving a strcpy(). Path must always be given without trailing slash.
 * "num" can point to an int specifying the number to use or NULL or a value
 * less than zero to number automatically. The final number used will also
 * be returned in *num. If *num is >= 0 then *num will be incremented by
 * one. */
char *create_numbered_filename(char *buffer, const char *path,
                               const char *prefix, const char *suffix,
                               int numberlen IF_CNFN_NUM_(, int *num))
{
    DIR *dir;
    struct dirent *entry;
    int max_num;
    int pathlen;
    int prefixlen = strlen(prefix);
    char fmtstring[12];

    if (buffer != path)
        strncpy(buffer, path, MAX_PATH);

    pathlen = strlen(buffer);

#ifdef IF_CNFN_NUM
    if (num && *num >= 0)
    {
        /* number specified */
        max_num = *num;
    }
    else
#endif
    {
        /* automatic numbering */
        max_num = 0;

    dir = opendir(pathlen ? buffer : "/");
    if (!dir)
        return NULL;

    while ((entry = readdir(dir)))
    {
        int curr_num;

        if (strncasecmp((char *)entry->d_name, prefix, prefixlen)
            || strcasecmp((char *)entry->d_name + prefixlen + numberlen, suffix))
            continue;

        curr_num = atoi((char *)entry->d_name + prefixlen);
        if (curr_num > max_num)
            max_num = curr_num;
    }

    closedir(dir);
    }

    max_num++;

    snprintf(fmtstring, sizeof(fmtstring), "/%%s%%0%dd%%s", numberlen);
    snprintf(buffer + pathlen, MAX_PATH - pathlen, fmtstring, prefix,
             max_num, suffix);

#ifdef IF_CNFN_NUM
    if (num)
        *num = max_num;
#endif

    return buffer;
}


#if CONFIG_RTC
/* Create a filename with a date+time part.
   It is allowed that buffer and path point to the same memory location,
   saving a strcpy(). Path must always be given without trailing slash.
   unique_time as true makes the function wait until the current time has
   changed. */
char *create_datetime_filename(char *buffer, const char *path,
                               const char *prefix, const char *suffix,
                               bool unique_time)
{
    struct tm *tm = get_time();
    static struct tm last_tm;
    int pathlen;

    while (unique_time && !memcmp(get_time(), &last_tm, sizeof (struct tm)))
        sleep(HZ/10);

    last_tm = *tm;

    if (buffer != path)
        strncpy(buffer, path, MAX_PATH);

    pathlen = strlen(buffer);
    snprintf(buffer + pathlen, MAX_PATH - pathlen,
             "/%s%02d%02d%02d-%02d%02d%02d%s", prefix,
             tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec, suffix);

    return buffer;
}
#endif /* CONFIG_RTC */
