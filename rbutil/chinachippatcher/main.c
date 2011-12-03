/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
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
#include <stdarg.h>
#include "chinachip.h"

#define VERSION         "0.1"
#define PRINT(fmt, ...) fprintf(stderr, fmt"\n", ##__VA_ARGS__)

static void info(void* userdata, char* fmt, ...)
{
    (void)userdata;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

static void err(void* userdata, char* fmt, ...)
{
    (void)userdata;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void usage(char* name)
{
    PRINT("Usage:");
    PRINT(" %s <firmware> <bootloader> <firmware_output> [backup]", name);
    PRINT("\nExample:");
    PRINT(" %s VX747.HXF bootloader.bin output.HXF ccpmp.bak", name);
    PRINT(" This will copy ccpmp.bin in VX747.HXF as ccpmp.old and replace it"
          " with bootloader.bin, the output will get written to output.HXF."
          " The old ccpmp.bin will get written to ccpmp.bak.");
}

int main(int argc, char* argv[])
{
    PRINT("ChinaChipPatcher v" VERSION " - (C) Maurus Cuelenaere 2009");
    PRINT("This is free software; see the source for copying conditions. There is NO");
    PRINT("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");

    if(argc < 4)
    {
        usage(argv[0]);
        return 1;
    }

    return chinachip_patch(argv[1], argv[2], argv[3], argc > 4 ? argv[4] : NULL,
                           &info, &err, NULL);
}

