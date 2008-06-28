/**************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2007 Thom Johansen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "rbspeex.h"

#define USAGE_TEXT \
"Usage: rbspeexenc [options] infile outfile\n"\
"Options:\n"\
"  -q x   Quality, floating point number in the range [0-10], default 8.0\n"\
"  -c x   Complexity, increases quality for a given bitrate, but encodes\n"\
"         slower, range [0-10], default 3\n"\
"  -n     Enable narrowband mode, will resample input to 8 kHz\n"\
"  -v x   Volume, amplitude multiplier, default 1.0\n\n"\
"rbspeexenc expects a mono 16 bit WAV file as input. Files will be resampled\n"\
"to either 16 kHz by default, or 8 kHz if narrowband mode is enabled.\n"\
"WARNING: This tool will create files that are only usable by Rockbox!\n"


int main(int argc, char **argv)
{
    char *inname, *outname;
    FILE *fin, *fout;
    int i;
    int complexity = 3;
    float quality = 8.f, volume = 1.0f;
    bool narrowband = false;
    bool ret;
    char errstr[512];

    if (argc < 3) {
        printf(USAGE_TEXT);
        return 1;
    }

    i = 1;
    while (i < argc - 2) {
        if (strncmp(argv[i], "-q", 2) == 0)
            quality = atof(argv[++i]);
        else if (strncmp(argv[i], "-c", 2) == 0)
            complexity = atoi(argv[++i]);
        else if (strncmp(argv[i], "-v", 2) == 0)
            volume = atof(argv[++i]);
        else if (strncmp(argv[i], "-n", 2) == 0)
            narrowband = true;
        else {
            printf("Error: unrecognized option '%s'\n", argv[i]);
            return 1;
        }
        ++i;
    }
    inname = argv[argc - 2];
    outname = argv[argc - 1];

    if ((fin = fopen(inname, "rb")) == NULL) {
        printf("Error: could not open input file\n");
        return 1;
    }
    if ((fout = fopen(outname, "wb")) == NULL) {
        printf("Error: could not open output file\n");
        return 1;
    }

    ret = encode_file(fin, fout, quality, complexity, narrowband, volume,
                      errstr, sizeof(errstr));
    fclose(fin);
    fclose(fout);
    if (!ret) {
        /* Attempt to delete unfinished output */
        printf("Error: %s\n", errstr);
        remove(outname);
        return 1;
    }
    return 0;
}

