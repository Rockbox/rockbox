/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 William Wilgus
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
#ifndef _LIB_ARG_HELPER_H_
#define _LIB_ARG_HELPER_H_

#include "plugin.h"

#define ARGPARSE_MAX_FRAC_DIGITS 9 /* Uses 30 bits max (0.999999999) */

#define ARGP_EXP(a, b) (a ##E## b)
#define ARGP_FRAC_DEC_MULTIPLIER(n) ARGP_EXP(1,n) /*1x10^n*/
#define ARGPARSE_FRAC_DEC_MULTIPLIER \
        (long)ARGP_FRAC_DEC_MULTIPLIER(ARGPARSE_MAX_FRAC_DIGITS)

/* fills buf with a string upto buf_sz, null terminates the buffer
 * strings break on WS by default but can be enclosed in single or double quotes
 * opening and closing quotes will not be included in the buffer but will be counted
 * use alternating quotes if you really want them included '"text"' or "'text'"
 * failure to close the string will result in eating all remaining args till \0
 * If buffer full remaining chars are discarded till stopchar or \0 is reached */
int string_parse(const char **parameter, char* buf, size_t buf_sz);
/* passes *character a single character eats remaining non-WS characters */
int char_parse(const char **parameter, char* character);
/* determine true false using the first character the rest are skipped/ignored */
int bool_parse(const char **parameter, bool *choice);
/* passes number and or decimal portion of number base 10 only.. */
int longnum_parse(const char **parameter, long *number, long *decimal);
int num_parse(const char **parameter, int *number, int *decimal);

/*
*argparse(const char *parameter, int parameter_len,
*         int (*arg_callback)(char argchar, const char **parameter))
* parameter     : constant char string of arguments
* parameter_len : may be set to -1 if your parameter string is NULL (\0) terminated
* arg_callback  : function gets called for each SWCHAR found in the parameter string
* Note: WS at beginning is stripped, **parameter starts at the first NON WS char
* return 0 for arg_callback to quit parsing immediately
*/
void argparse(const char *parameter, int parameter_len,
              int (*arg_callback)(char argchar, const char **parameter));

#endif /* _LIB_ARG_HELPER_H_ */
