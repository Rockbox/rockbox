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

#include "plugin.h"
#include "arg_helper.h"

#ifndef logf
#define logf(...) {}
#endif

#define SWCHAR '-'
#define DECSEPCHAR '.'
#ifdef PLUGIN
    #define strchr rb->strchr
#endif
int string_parse(const char **parameter, char* buf, size_t buf_sz)
{
/* fills buf with a string upto buf_sz, null terminates the buffer
 * strings break on WS by default but can be enclosed in single or double quotes
 * opening and closing quotes will not be included in the buffer but will be counted
 * use alternating quotes if you really want them included '"text"' or "'text'"
 * failure to close the string will result in eating all remaining args till \0
 * If buffer full remaining chars are discarded till stopchar or \0 is reached */

    char stopchar = ' ';
    char stopchars[] = "\'\"";
    int skipped = 0;
    int found = 0;
    const char* start = *parameter;

    if (strchr(stopchars, *start))
    {
        logf("stop char %c\n", *start);
        stopchar = *start;
        skipped++;
        start++;
    }
    while (*start && *start != stopchar)
    {
        if (buf_sz > 1)
        {
            *buf++ = *start;
            buf_sz--;
        }
        found++;
        start++;
    }
    if (*start == stopchar && skipped)
    {
        start++;
        skipped++;
    }

    *buf = '\0';

    if (found > 0)
        *parameter = start;
    else
        skipped = 0;

    return found + skipped;
}

int char_parse(const char **parameter, char* character)
{
/* passes *character a single character eats remaining non-WS characters */
    char buf[2];
    int ret =  string_parse(parameter, buf, sizeof(buf));
    if (ret && character)
        *character = buf[0];
    return ret;

}

int bool_parse(const char **parameter, bool *choice)
{
/* determine true false using the first character the rest are skipped/ignored */
    int found = 0;
    const char tf_val[]="fn0ty1";/* false chars on left f/t should be balanced fffttt */
    const char* start = *parameter;


    char c = tolower(*start);
    const char *tfval = strchr(tf_val, c);
    while(isalnum(*++start)) {;}

    if (tfval)
    {
        found = start - (*parameter);
        *parameter = start;
    }

    if (choice)
        *choice = (tfval - tf_val) > (signed int) (sizeof(tf_val) / 2) - 1;

    return found;
}

int longnum_parse(const char **parameter, long *number, long *decimal)
{
/* passes number and or decimal portion of number base 10 only..
   fractional portion is scaled by ARGPARSE_FRAC_DEC_MULTIPLIER
   Example (if ARGPARSE_FRAC_DEC_MULTIPLIER = 10 000)
   meaning .0009 returns 9   , 9    / 10000 = .0009
           .009  returns 90
           .099  returns 990
           .09   returns 900
           .9    returns 9000
           .9999 returns 9999
*/

    long num = 0;
    long dec = 0;
    int found = 0;
    int neg = 0;
    int digits = 0;
    //logf ("n: %s\n", *parameter);
    const char *start = *parameter;

    if (*start == '-')
    {
        neg = 1;
        start++;
    }
    while (isdigit(*start))
    {
        found++;
        num = num *10 + *start - '0';
        start++;
    }

    if (*start == DECSEPCHAR)
    {
        start++;
        while(*start == '0')
        {
            digits++;
            start++;
        }
        while (isdigit(*start))
        {
            dec = dec *10 + *start - '0';
            digits++;
            start++;
        }
        if (decimal && digits <= ARGPARSE_MAX_FRAC_DIGITS)
        {
            if(digits < ARGPARSE_MAX_FRAC_DIGITS)
            {
                digits = ARGPARSE_MAX_FRAC_DIGITS - digits;
                while (digits--)
                    dec *= 10;
            }
        }
        else
            dec = -1; /* error */
    }

    if (found > 0)
    {
        found = start - (*parameter);
        *parameter = start;
    }

    if(number)
        *number = neg ? -num : num;

    if (decimal)
        *decimal = dec;

    return found;
}

int num_parse(const char **parameter, int *number, int *decimal)
{
    long num, dec;
    int ret = longnum_parse(parameter, &num, &dec);
    if(number)
        *number = num;
    if (decimal)
        *decimal = dec;
    return ret;
}

/*
*argparse(const char *parameter, int parameter_len,
*         int (*arg_callback)(char argchar, const char **parameter))
* parameter     : constant char string of arguments
* parameter_len : may be set to -1 if your parameter string is NULL (\0) terminated
* arg_callback  : function gets called for each SWCHAR found in the parameter string
* Note: WS at beginning is stripped, **parameter starts at the first NON WS char
* return 0 for arg_callback to quit parsing immediately
*/
void argparse(const char *parameter, int parameter_len, int (*arg_callback)(char argchar, const char **parameter))
{
    bool lastchr;
    char argchar;
    const char *start = parameter;
    while (parameter_len < 0 || (parameter - start) < parameter_len)
    {
        switch (*parameter++)
        {
            case SWCHAR:
            {
                if ((*parameter) == '\0')
                    return;
                logf ("%s\n",parameter);
                argchar = *parameter;
                lastchr = (*(parameter + 1) == '\0');
                while (*++parameter || lastchr)
                {
                    lastchr = false;
                    if (isspace(*parameter))
                        continue; /* eat spaces at beginning */
                    if (!arg_callback(argchar, &parameter))
                        return;
                    break;
                }
                break;
            }
            case '\0':
            {
                if (parameter_len <= 0)
                    return;
            }
        }
    }
}

/* EXAMPLE USAGE
argparse("-n 42 -N 9.9 -n -78.9009 -f -P /rockbox/path/f -s 'Yestest' -B false -B 0 -B true -b n -by -b 1-c ops -c s -k", -1, &arg_callback);

int arg_callback(char argchar, const char **parameter)
{
    int ret;
    int num, dec;
    char c;
    char buf[32];
    bool bret;
    logf ("Arg: %c\n", argchar);
    switch (tolower(argchar))
    {
        case 'k' :
            logf("Option K!");
            break;
        case 'c' :
            ret = char_parse(parameter, &c);
            if (ret)
            {
                logf ("Val: %c\n", c);
                logf("ate %d chars\n", ret);
            }
            break;

        case 'n' :
            ret = num_parse(parameter, &num, &dec);
            if (ret)
            {
                logf ("Val: %d.%d\n", num, dec);
                logf("ate %d chars\n", ret);
            }
            break;
        case 's' :
            ret = string_parse(parameter, buf, sizeof(buf));
            if (ret)
            {
                logf ("Val: %s\n", buf);
                logf("ate %d chars\n", ret);
            }
            break;
        case 'p' :
            ret = string_parse(parameter, buf, sizeof(buf));
            if (ret)
            {
                logf ("Path: %s\n", buf);
                logf("ate %d chars\n", ret);
            }
            break;
        case 'b' :
            ret = bool_parse(parameter, &bret);
            if (ret)
            {
                logf ("Val: %s\n", bret ? "true" : "false");
                logf("ate %d chars\n", ret);
            }
            break;
        default :
            logf ("Unknown switch '%c'\n",argchar);
            //return 0;
    }
    return 1;
}
*/

