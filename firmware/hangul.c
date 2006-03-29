/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *
 * Copyright (C) 2006 by Frank Dischner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "hangul.h"

const char jamo_table[51][3] = {
    { 1,  0,  1},
    { 2,  0,  2},
    { 0,  0,  3},
    { 3,  0,  4},
    { 0,  0,  5},
    { 0,  0,  6},
    { 4,  0,  7},
    { 5,  0,  0},
    { 6,  0,  8},
    { 0,  0,  9},
    { 0,  0, 10},
    { 0,  0, 11},
    { 0,  0, 12},
    { 0,  0, 13},
    { 0,  0, 14},
    { 0,  0, 15},
    { 7,  0, 16},
    { 8,  0, 17},
    { 9,  0,  0},
    { 0,  0, 18},
    {10,  0, 19},
    {11,  0, 20},
    {12,  0, 21},
    {13,  0, 22},
    {14,  0,  0},
    {15,  0, 23},
    {16,  0, 24},
    {17,  0, 25},
    {18,  0, 26},
    {19,  0, 27},
    { 0,  1,  0},
    { 0,  2,  0},
    { 0,  3,  0},
    { 0,  4,  0},
    { 0,  5,  0},
    { 0,  6,  0},
    { 0,  7,  0},
    { 0,  8,  0},
    { 0,  9,  0},
    { 0, 10,  0},
    { 0, 11,  0},
    { 0, 12,  0},
    { 0, 13,  0},
    { 0, 14,  0},
    { 0, 15,  0},
    { 0, 16,  0},
    { 0, 17,  0},
    { 0, 18,  0},
    { 0, 19,  0},
    { 0, 20,  0},
    { 0, 21,  0},
};

/* takes three jamo chars and joins them into one hangul */
unsigned short hangul_join(unsigned short lead, unsigned short vowel,
                                unsigned short tail)
{
    unsigned short ch = 0xfffd;

    if (lead < 0x3131 || lead > 0x3163)
        return ch;
    lead = jamo_table[lead-0x3131][0];

    if (vowel < 0x3131 || vowel > 0x3163)
        return ch;
    vowel = jamo_table[vowel-0x3131][1];

    if (tail) {
        if (tail < 0x3131 || tail > 0x3163)
            return ch;
        tail = jamo_table[tail-0x3131][2];
        if (!tail)
            return ch;
    }

    if (lead && vowel)
        ch = tail + (vowel - 1)*28 + (lead - 1)*588 + 44032;

    return ch;
}
