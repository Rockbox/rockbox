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

#ifndef GENERAL_H
#define GENERAL_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"

/* round a signed/unsigned 32bit value to the closest of a list of values */
/* returns the index of the closest value */
int round_value_to_list32(unsigned long value,
                          const unsigned long list[],
                          int count,
                          bool signd);

int make_list_from_caps32(unsigned long src_mask,
                          const unsigned long *src_list,
                          unsigned long caps_mask,
                          unsigned long *caps_list);

/* Create a filename with a number part in a way that the number is 1
 * higher than the highest numbered file matching the same pattern.
 * It is allowed that buffer and path point to the same memory location,
 * saving a strcpy(). Path must always be given without trailing slash.
 *
 * "num" can point to an int specifying the number to use or NULL or a value
 * less than zero to number automatically. The final number used will also
 * be returned in *num. If *num is >= 0 then *num will be incremented by
 * one. */
#if defined(HAVE_RECORDING) && (CONFIG_RTC == 0)
/* this feature is needed by recording without a RTC to prevent disk access
   when changing files */
#define IF_CNFN_NUM_(...) __VA_ARGS__
#define IF_CNFN_NUM
#else
#define IF_CNFN_NUM_(...)
#endif
char *create_numbered_filename(char *buffer, const char *path,
                               const char *prefix, const char *suffix,
                               int numberlen IF_CNFN_NUM_(, int *num));

#if CONFIG_RTC
/* Create a filename with a date+time part.
   It is allowed that buffer and path point to the same memory location,
   saving a strcpy(). Path must always be given without trailing slash.
   unique_time as true makes the function wait until the current time has
   changed. */
char *create_datetime_filename(char *buffer, const char *path,
                               const char *prefix, const char *suffix,
                               bool unique_time);
#endif /* CONFIG_RTC */

/***
 ** Compacted pointer lists
 **
 ** N-length list requires N+1 elements to ensure NULL-termination.
 **/

/* Find a pointer in a pointer array. Returns the addess of the element if
   found or the address of the terminating NULL otherwise. This can be used
   to bounds check and add items. */
void ** find_array_ptr(void **arr, void *ptr);

/* Remove a pointer from a pointer array if it exists. Compacts it so that
   no gaps exist. Returns 0 on success and -1 if the element wasn't found. */
int remove_array_ptr(void **arr, void *ptr);

#endif /* GENERAL_H */
