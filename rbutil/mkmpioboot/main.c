/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 by Marcin Bukat
 *
 * code taken mostly from mkboot.c
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include "mkmpioboot.h"

static void usage(void)
{
    printf("usage: mkmpioboot <firmware file> <boot file> <output file>\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    char *infile, *bootfile, *outfile;
    int origin = 0xe0000;   /* MPIO HD200 bootloader address */

    if(argc < 3) {
        usage();
    }

        infile = argv[1];
        bootfile = argv[2];
        outfile = argv[3];

    return mkmpioboot(infile, bootfile, outfile, origin);
}
