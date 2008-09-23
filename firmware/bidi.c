/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Gadi Cohen
 *
 * Largely based on php_hebrev by Zeev Suraski <zeev@php.net>
 * Heavily modified by Gadi Cohen aka Kinslayer <dragon@wastelands.net>
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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "file.h"
#include "lcd.h"
#include "rbunicode.h"
#include "arabjoin.h"
#include "scroll_engine.h"
#include "bidi.h"

/* #define _HEB_BUFFER_LENGTH (MAX_PATH + LCD_WIDTH/2 + 3 + 2 + 2) * 2 */
#define _HEB_BLOCK_TYPE_ENG 1
#define _HEB_BLOCK_TYPE_HEB 0
#define _HEB_ORIENTATION_LTR 1
#define _HEB_ORIENTATION_RTL 0

#define ischar(c) ((c > 0x0589 && c < 0x0700) || \
                   (c >= 0xfb50 && c <= 0xfefc) ? 1 : 0)
#define _isblank(c) ((c==' ' || c=='\t') ? 1 : 0)
#define _isnewline(c) ((c=='\n' || c=='\r') ? 1 : 0)
#define XOR(a,b) ((a||b) && !(a&&b))

#ifndef BOOTLOADER
static const arab_t * arab_lookup(unsigned short uchar)
{
    if (uchar >= 0x621 && uchar <= 0x63a)
        return &(jointable[uchar - 0x621]);
    if (uchar >= 0x640 && uchar <= 0x64a)
        return &(jointable[uchar - 0x621 - 5]);
    if (uchar >= 0x671 && uchar <= 0x6d5)
        return &(jointable[uchar - 0x621 - 5 - 38]);
    if (uchar == 0x200D) /* Support for the zero-width joiner */
        return &zwj;
    return 0;
}

static void arabjoin(unsigned short * stringprt, int length)
{
    bool connected = false;
    unsigned short * writeprt = stringprt;

    const arab_t * prev = 0;
    const arab_t * cur;
    const arab_t * ligature = 0;
    short uchar;

    int i;
    for (i = 0; i <= length; i++) {
        cur = arab_lookup(uchar = *stringprt++);

        /* Skip non-arabic chars */
        if (cur == 0) {
            if (prev) {
                /* Finish the last char */
                if (connected) {
                    *writeprt++ = prev->final;
                    connected = false;
                } else
                    *writeprt++ = prev->isolated;
                prev = 0;
                *writeprt++ = uchar;
            } else {
                *writeprt++ = uchar;
            }
            continue;
        }

        /* nothing to do for arabic char if the previous was non-arabic */
        if (prev == 0) {
            prev = cur;
            continue;
        }

        /* if it's LAM, check for LAM+ALEPH ligatures */
        if (prev->isolated == 0xfedd) {
            switch (cur->isolated) {
                case 0xfe8d:
                    ligature = &(lamaleph[0]);
                    break;
                case 0xfe87:
                    ligature = &(lamaleph[1]);
                    break;
                case 0xfe83:
                    ligature = &(lamaleph[2]);
                    break;
                case 0xfe81:
                    ligature = &(lamaleph[3]);
            }
        }

        if (ligature) { /* replace the 2 glyphs by their ligature */
            prev = ligature;
            ligature = 0;
        } else {
            if (connected) { /* previous char has something connected to it */
                if (prev->medial && cur->final) /* Can we connect to it? */
                    *writeprt++ = prev->medial;
                else {
                    *writeprt++ = prev->final;
                    connected = false;
                }
            } else {
                if (prev->initial && cur->final) { /* Can we connect to it? */
                    *writeprt++ = prev->initial;
                    connected = true;
                } else
                    *writeprt++ = prev->isolated;
            }
            prev = cur;
        }
    }
}
#endif /* !BOOTLOADER */

unsigned short *bidi_l2v(const unsigned char *str, int orientation)
{
    static unsigned short  utf16_buf[SCROLL_LINE_SIZE];
    unsigned short *target, *tmp;
#ifndef BOOTLOADER
    static unsigned short  bidi_buf[SCROLL_LINE_SIZE];
    unsigned short *heb_str; /* *broken_str */
    int block_start, block_end, block_type, block_length, i;
    int length = utf8length(str);
#endif
    /*
    long max_chars=0;
    int begin, end, char_count, orig_begin;

    tmp = str;
    */
    target = tmp = utf16_buf;
    while (*str)
        str = utf8decode(str, target++);
    *target = 0;

#ifdef BOOTLOADER
    (void)orientation;
    return utf16_buf;
	
#else /* !BOOTLOADER */
    if (target == utf16_buf) /* empty string */
        return target;

    /* properly join any arabic chars */
    arabjoin(utf16_buf, length);

    block_start=block_end=block_length=0;

    heb_str = bidi_buf;
    if (orientation) {
        target = heb_str;
    } else {
        target = heb_str + length;
        *target = 0;
        target--;
    }

    if (ischar(*tmp))
        block_type = _HEB_BLOCK_TYPE_HEB;
    else
        block_type = _HEB_BLOCK_TYPE_ENG;

    do {
        while((XOR(ischar(*(tmp+1)),block_type)
               || _isblank(*(tmp+1)) || ispunct((int)*(tmp+1))
               || *(tmp+1)=='\n')
              && block_end < length-1) {
                tmp++;
                block_end++;
                block_length++;
        }

        if (block_type != orientation) {
            while ((_isblank(*tmp) || ispunct((int)*tmp))
                   && *tmp!='/' && *tmp!='-' && block_end>block_start) {
                tmp--;
                block_end--;
            }
        }

        for (i=block_start; i<=block_end; i++) {
            *target = (block_type == orientation) ?
                      *(utf16_buf+i) : *(utf16_buf+block_end-i+block_start);
            if (block_type!=orientation) {
                switch (*target) {
                case '(':
                    *target = ')';
                    break;
                case ')':
                    *target = '(';
                    break;
                default:
                    break;
                }
            }
            target += orientation ? 1 : -1;
        }
        block_type = !block_type;
        block_start=block_end+1;
    } while(block_end<length-1);

    *target = 0;

#if 0 /* Is this code really necessary? */
    broken_str = utf16_buf;
    begin=end=length-1;
    target = broken_str;

    while (1) {
        char_count=0;
        while ((!max_chars || char_count<max_chars) && begin>0) {
            char_count++;
            begin--;
            if (begin<=0 || _isnewline(heb_str[begin])) {
                while(begin>0 && _isnewline(heb_str[begin-1])) {
                    begin--;
                    char_count++;
                }
                break;
            }
        }
        if (char_count==max_chars) { /* try to avoid breaking words */
            int new_char_count = char_count;
            int new_begin = begin;
            
            while (new_char_count>0) {
                if (_isblank(heb_str[new_begin]) ||
                    _isnewline(heb_str[new_begin])) {
                    break;
                }
                new_begin++;
                new_char_count--;
            }
            if (new_char_count>0) {
                char_count=new_char_count;
                begin=new_begin;
            }
        }
        orig_begin=begin;
        
        /* if (_isblank(heb_str[begin])) {
            heb_str[begin]='\n';
        } */
        
        /* skip leading newlines */
        while (begin<=end && _isnewline(heb_str[begin])) {
            begin++;
        }

        /* copy content */
        for (i=begin; i<=end; i++) {
            *target = heb_str[i];
            target++;
        }

        for (i=orig_begin; i<=end && _isnewline(heb_str[i]); i++) {
            *target = heb_str[i];
            target++;
        }
        begin=orig_begin;
        
        if (begin<=0) {
            *target = 0;
            break;
        }
        begin--;
        end=begin;
    }
    return broken_str;
#endif
    return heb_str;
#endif /* !BOOTLOADER */
}

