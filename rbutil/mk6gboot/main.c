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
#include <string.h>
#include "mkdfu.h"

static void usage(void)
{
    printf("Usage: mk6gboot: <bootloader.ipod> <output file> [-u]\n");
    exit(1);
}

int main(int argc, char* argv[])
{
    char *infile, *outfile;
    int uninstall;

    if (argc != 3 && (argc != 4 || strcmp(argv[3], "-u"))) {
        usage();
    }

    infile = argv[1];
    outfile = argv[2];
    uninstall = (argc == 4);

    return mkdfu(infile, outfile, uninstall);
}
