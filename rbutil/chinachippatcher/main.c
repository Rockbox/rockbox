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

#ifndef VERSION /* allow setting version from outside for svn builds */
#define VERSION         "0.1"
#endif
#define PRINT(fmt, ...) fprintf(stderr, fmt"\n", ##__VA_ARGS__)

void usage(char* name)
{
    PRINT("Usage:");
    PRINT(" %s <firmware> <bootloader> <firmware_output> [backup]", name);
    PRINT("\nExample:");
    PRINT(" %s VX747.HXF bootloader.bin output.HXF ccpmp.bak", name);
    PRINT(" This will copy ccpmp.bin in VX747.HXF as ccpmp.old and replace it\n"
          " with bootloader.bin, the output will get written to output.HXF.\n"
          " The old ccpmp.bin will get written to ccpmp.bak.\n");
}

int main(int argc, char* argv[])
{
    PRINT("ChinaChipPatcher v" VERSION " - (C) Maurus Cuelenaere 2009");
    PRINT("This is free software; see the source for copying conditions. There is NO");
    PRINT("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.");

    if(argc < 4)
    {
        usage(argv[0]);
        return 1;
    }

    return chinachip_patch(argv[1], argv[2], argv[3], argc > 4 ? argv[4] : NULL);
}

