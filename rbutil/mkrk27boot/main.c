/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2012 by Andrew Ryabinin
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

#include<stdio.h>
#include "mkrk27boot.h"


int main(int argc, char **argv)
{
    char* img_file = NULL;
    char* rkw_file = NULL;
    char* out_img_file = NULL;
    char* out_rkw_file = NULL;

    if (argc == 4) {
        img_file = argv[1];
        rkw_file = argv[2];
        out_img_file = argv[3];
        out_rkw_file = "BASE.RKW";
    } else if (argc == 5) {
        img_file = argv[1];
        rkw_file = argv[2];
        out_img_file = argv[3];
        out_rkw_file = argv[4];
    } else {
        fprintf(stdout, "\tusage: mkrk27boot <img file> <rkw file> <output img file> [<output rkw file>]\n");
        fprintf(stdout, "\tIf output rkw file is not specified, the default is to put rkw file to BASE.RKW\n");
        return -1;
    }
    if (mkrk27_patch(img_file, rkw_file, out_img_file, out_rkw_file)) {
        fprintf(stdout, "%s\n", mkrk27_get_error());
        return -1;
    }
    fprintf(stdout, "Success!\n");
    return 0;
}

