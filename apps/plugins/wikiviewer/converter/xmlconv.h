/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

/* Mode-defines: */
#define TAG_OUTSIDE 1
#define TAG_ERR -1
#define TAG_PAGE 2
#define TAG_REVISION 3
#define TAG_TEXT 4
#define TAG_CONTRIBUTOR 5
#define S_ROWBUF (32*1024)      /* 32kByte Rowbuffer */
#define S_ARTBUF (8096*1024)        /* 8MB Article Buffer */
#pragma pack(push, 1)
typedef struct
{
    unsigned int id;
    unsigned int article_length;
    unsigned short title_length;
} article_header;

typedef struct
{
    unsigned int filenumber;
    unsigned int fileposition;
    unsigned int title_length;
} title_entry;
#pragma pack(pop)

#define MARKUP_STARTULINE   1
#define MARKUP_ENDULINE     2
#define MARKUP_STARTBOLD    3
#define MARKUP_ENDBOLD      4
#define MARKUP_STARTITALIC  5
#define MARKUP_ENDITALIC    6
#define MARKUP_STARTLINK    7
#define MARKUP_ENDLINK      8
#define MARKUP_LINEFEED     10
#define MARKUP_MODE     15
