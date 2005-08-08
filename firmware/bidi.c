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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

#define _HEB_BUFFER_LENGTH MAX_PATH + LCD_WIDTH/2 + 3 + 2 + 2
#define _HEB_BLOCK_TYPE_ENG 1
#define _HEB_BLOCK_TYPE_HEB 0
#define _HEB_ORIENTATION_LTR 1
#define _HEB_ORIENTATION_RTL 0

#define ischar(c) (((((unsigned char) c)>=193) && (((unsigned char) c)<=250)) ? 1 : 0)
#define _isblank(c) (((((unsigned char) c)==' ' || ((unsigned char) c)=='\t')) ? 1 : 0)
#define _isnewline(c) (((((unsigned char) c)=='\n' || ((unsigned char) c)=='\r')) ? 1 : 0)
#define XOR(a,b) ((a||b) && !(a&&b))

bool bidi_support_enabled = false;

unsigned char *bidi_l2v(const unsigned char *str, int orientation)
{
    static unsigned char buf_heb_str[_HEB_BUFFER_LENGTH];
    static unsigned char  buf_broken_str[_HEB_BUFFER_LENGTH];
    const unsigned char *tmp;
    unsigned char *heb_str, *target, *opposite_target, *broken_str;
    int block_start, block_end, block_type, block_length, i;
    int block_ended;
    long max_chars=0;
    int begin, end, char_count, orig_begin;

    if (!str || !*str)
        return "";

    tmp = str;
    block_start=block_end=0;
    block_ended=0;

    heb_str = buf_heb_str;
    if (orientation) {
        target = heb_str;
        opposite_target = heb_str + strlen(str);
    } else {
        target = heb_str + strlen(str);
        opposite_target = heb_str;
        *target = 0;
        target--;
    }
    
    block_length=0;
    if (ischar(*tmp))
        block_type = _HEB_BLOCK_TYPE_HEB;
    else
        block_type = _HEB_BLOCK_TYPE_ENG;
    
    do {
        while((XOR(ischar((int)*(tmp+1)),block_type)
               || _isblank((int)*(tmp+1)) || ispunct((int)*(tmp+1))
               || (int)*(tmp+1)=='\n')
              && block_end<(int)strlen(str)-1) {
            tmp++;
            block_end++;
            block_length++;
        }
        
        if (block_type != orientation) {
            while ((_isblank((int)*tmp) || ispunct((int)*tmp))
                   && *tmp!='/' && *tmp!='-' && block_end>block_start) {
                tmp--;
                block_end--;
            }
        }
        
        for (i=block_start; i<=block_end; i++) {
            *target = (block_type == orientation) ? *(str+i) : *(str+block_end-i+block_start);
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
    } while(block_end<(int)strlen(str)-1);
    
    broken_str = buf_broken_str;
    begin=end=strlen(str)-1;
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
        
        if (_isblank(heb_str[begin])) {
            heb_str[begin]='\n';
        }
        
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
}

void set_bidi_support(bool setting)
{
    bidi_support_enabled = setting;
}
