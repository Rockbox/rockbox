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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codepages.h"

#define MAX_TABLE_SIZE     32768

static unsigned short iso_table[MAX_TABLE_SIZE];

static const unsigned short iso8859_7_to_uni[] = {
          0x2018, 0x2019, 0x00A3, 0x20AC, 0x20AF, 0x00A6, 0x00A7, /* A1-A7 */
  0x00A8, 0x00A9, 0x037A, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x2015, /* A8-AF */
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x0384, 0x0385, 0x0386, 0x00B7  /* B0-B7 */
};

static const unsigned short cp1251_to_uni[] = {
  0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, /* 80-87 */
  0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F, /* 88-8F */
  0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, /* 90-97 */
  0x0098, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F, /* 98-9F */
  0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7, /* A0-A7 */
  0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407, /* A8-AF */
  0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7, /* B0-B7 */
  0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457  /* B8-BF */
};

static const unsigned short iso8859_2_to_uni[] = {
          0x0104, 0x02D8, 0x0141, 0x00A4, 0x013D, 0x015A, 0x00A7, /* A1-A7 */
  0x00A8, 0x0160, 0x015E, 0x0164, 0x0179, 0x00AD, 0x017D, 0x017B, /* A8-AF */
  0x00B0, 0x0105, 0x02DB, 0x0142, 0x00B4, 0x013E, 0x015B, 0x02C7, /* B0-B7 */
  0x00B8, 0x0161, 0x015F, 0x0165, 0x017A, 0x02DD, 0x017E, 0x017C, /* B8-BF */
  0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7, /* C0-C7 */
  0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E, /* C8-CF */
  0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7, /* D0-D7 */
  0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF, /* D8-DF */
  0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7, /* E0-E7 */
  0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F, /* E8-EF */
  0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7, /* F0-F7 */
  0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9  /* F8-FF */
};

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

        case 0x03: /* Russian (CP1251) */
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

        case 0x05: /* Arabic (ISO-8859-6) */
            while (count--) {
                /* first convert to unicode */
                if (*latin1 < 0xAC || *latin1 == 0xAD)
                    ucs = *latin1++;
                else                    
                    ucs = *latin1++ + 0x0560;
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

int main(void)
{

    int i, j;
    unsigned char k;
    unsigned short uni;
    FILE *of;

    for (i=0; i < MAX_TABLE_SIZE; i++)
        iso_table[i] = 0;

    of = fopen("iso.cp", "wb");
    if (!of) return 1;

    for (i=1; i<8; i++) {

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

    return 0;
}
