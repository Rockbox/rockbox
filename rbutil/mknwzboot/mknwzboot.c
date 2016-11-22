/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "mknwzboot.h"
#include "upg.h"

struct nwz_model_desc_t
{
    /* Descriptive name of this model */
    const char *model_name;
    /* Model name used in the Rockbox header in ".sansa" files - these match the
       -add parameter to the "scramble" tool */
    const char *rb_model_name;
    /* Model number used to initialise the checksum in the Rockbox header in
       ".sansa" files - these are the same as MODEL_NUMBER in config-target.h */
    const int rb_model_num;
    /* Codename used in upgtool */
    const char *codename;
};

static const struct nwz_model_desc_t nwz_models[] =
{
    { "Sony NWZ-E450 Series", "e460", 100, "nwz-e450" },
    { "Sony NWZ-E460 Series", "e460", 101, "nwz-e460" },
    { "Sony NWZ-E580 Series", "e580", 102, "nwz-e580" },
};

#define NR_NWZ_MODELS     (sizeof(nwz_models) / sizeof(nwz_models[0]))

void dump_nwz_dev_info(const char *prefix)
{
    printf("%smknwzboot models:\n", prefix);
    for(int i = 0; i < NR_NWZ_MODELS; i++)
    {
        printf("%s  %s: rb_model=%s rb_num=%d codename=%s\n", prefix,
            nwz_models[i].model_name, nwz_models[i].rb_model_name,
            nwz_models[i].rb_model_num, nwz_models[i].codename);
    }
}

int mknwzboot(const char *bootfile, const char *outfile, bool debug)
{
    return 0;
}
