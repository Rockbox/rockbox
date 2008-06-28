/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *
 * Copyright (C) 2005 by Frank Dischner
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
#include <stdlib.h>
#include <string.h>
#include "codepage_tables.h"

#define MAX_TABLE_SIZE     32768

static const int mini_index[6] = {
    0, 1, 3, 6, 7, 8
};

static unsigned short iso_table[MAX_TABLE_SIZE];

unsigned short iso_decode(unsigned char *latin1, int cp, int count)
{
    unsigned short ucs = 0;

    /* cp tells us which codepage to convert from */
    switch (cp) {
        case 0x01: /* Greek (ISO-8859-7) */
            while (count--) {
                /* first convert to unicode */
                if (*latin1 < 0xA1)
                    ucs = *latin1++;
                else if (*latin1 > 0xB7)
                    ucs = *latin1++ + 0x02D0;
                else 
                    ucs = iso8859_7_to_uni[*latin1++ - 0xA1];
            }
            break;

        case 0x02: /* Hebrew (ISO-8859-8) */
            while (count--) {
                /* first convert to unicode */
                if (*latin1 == 0xAA) {
                    ucs = 0xD7;
                    latin1++;
                } else if (*latin1 == 0xBA) {
                    ucs = 0xF7;
                    latin1++;
                } else if (*latin1 == 0xDF) {
                    ucs = 0x2017;
                    latin1++;
                } else if (*latin1 < 0xC0)
                    ucs = *latin1++;
                else
                    ucs = *latin1++ + 0x04F0;
            }
            break;

        case 0x03: /* Cyrillic (CP1251) */
            while (count--) {
                /* first convert to unicode */
                if (*latin1 < 0x80)
                    ucs = *latin1++;
                else if (*latin1 > 0xBF)
                    ucs = *latin1++ + 0x0350;
                else
                    ucs = cp1251_to_uni[*latin1++ - 0x80];
            }
            break;

        case 0x04: /* Thai (ISO-8859-11) */
            while (count--) {
                /* first convert to unicode */
                if (*latin1 < 0xA1)
                    ucs = *latin1++;
                else
                    ucs = *latin1++ + 0x0D60;
            }
            break;

        case 0x05: /* Arabic (CP1256) */
            while (count--) {
                /* first convert to unicode */
                if (*latin1 < 0x80)
                    ucs = *latin1++;
                else
                    ucs = cp1256_to_uni[*latin1++ - 0x80];
            }
            break;

        case 0x06: /* Turkish (ISO-8859-9) */
            while (count--) {
                /* first convert to unicode */
                switch (*latin1) {
                    case 0xD0:
                        ucs = 0x011E;
                        break;
                    case 0xDD:
                        ucs = 0x0130;
                        break;
                    case 0xDE:
                        ucs = 0x015E;
                        break;
                    case 0xF0:
                        ucs = 0x011F;
                        break;
                    case 0xFD:
                        ucs = 0x0131;
                        break;
                    case 0xFE:
                        ucs = 0x015F;
                        break;
                    default:
                        ucs = *latin1;
                        break;
                }

                latin1++;
            }
            break;

        case 0x07: /* Latin Extended (ISO-8859-2) */
            while (count--) {
                /* first convert to unicode */
                if (*latin1 < 0xA1)
                    ucs = *latin1++;
                else
                    ucs = iso8859_2_to_uni[*latin1++ - 0xA1];
            }
            break;
        
        case 0x08: /* Central European (CP1250) */
            while (count--) {
                /* first convert to unicode */
                if (*latin1 < 0x80)
                    ucs = *latin1++;
                else
                    ucs = cp1250_to_uni[*latin1++ - 0x80];
            }
            break;
            
        default:
            break;
    }
    return ucs;
}

int writeshort(FILE *f, unsigned short s)
{
    putc(s, f);
    return putc(s>>8, f) != EOF;
}

void print_usage(void)
{
    printf("Usage: codepages [-m]\n"
           "\t-m       Create isomini.cp only\n");
    printf("build date: " __DATE__ "\n\n");
}

int main(int argc, char **argv)
{
    int mini = 0;
    int i, j;
    unsigned char k;
    unsigned short uni;
    FILE *of;

    for (i = 1;i < argc;i++)
    {
        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
              case 'm':   /* create isomini.cp only */
                mini = 1;
                break;

              case 'h':   /* help */
              case '?':
                print_usage();
                exit(1);
                break;

              default:
                print_usage();
                exit(1);
                break;
            }
        }
    }

    for (i=0; i < MAX_TABLE_SIZE; i++)
        iso_table[i] = 0;

    if (mini) {
        of = fopen("isomini.cp", "wb");
        if (!of) return 1;

        for (i=1; i<6; i++) {

            for (j=0; j<128; j++) {
                k = (unsigned char)j + 128;
                uni = iso_decode(&k, mini_index[i], 1);
                writeshort(of, uni);
            }
        }
        fclose(of);
    }
    else {
        of = fopen("iso.cp", "wb");
        if (!of) return 1;

        for (i=1; i<9; i++) {

            for (j=0; j<128; j++) {
                k = (unsigned char)j + 128;
                uni = iso_decode(&k, i, 1);
                writeshort(of, uni);
            }
        }
        fclose(of);

        of = fopen("932.cp", "wb");
        if (!of) return 1;
        for (i=0; i < MAX_TABLE_SIZE; i++)
            writeshort(of, cp932_table[i]);
        fclose(of);

        of = fopen("936.cp", "wb");
        if (!of) return 1;
        for (i=0; i < MAX_TABLE_SIZE; i++)
            writeshort(of, cp936_table[i]);
        fclose(of);

        of = fopen("949.cp", "wb");
        if (!of) return 1;
        for (i=0; i < MAX_TABLE_SIZE; i++)
            writeshort(of, cp949_table[i]);
        fclose(of);

        of = fopen("950.cp", "wb");
        if (!of) return 1;
        for (i=0; i < MAX_TABLE_SIZE; i++)
            writeshort(of, cp950_table[i]);
        fclose(of);
    }

    return 0;
}
