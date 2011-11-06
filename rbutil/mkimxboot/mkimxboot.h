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

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "sb.h"

enum imx_error_t
{
    IMX_SUCCESS = 0,
    IMX_ERROR = -1,
    IMX_OPEN_ERROR = -2,
    IMX_READ_ERROR = -3,
    IMX_NO_MATCH = -4,
    IMX_BOOT_INVALID = -5,
    IMX_BOOT_MISMATCH = -6,
    IMX_BOOT_CHECKSUM_ERROR = -7,
    IMX_DONT_KNOW_HOW_TO_PATCH = -8,
    IMX_FIRST_SB_ERROR = -9,
};

enum imx_output_type_t
{
    IMX_DUALBOOT = 0,
    IMX_RECOVERY = 1,
    IMX_SINGLEBOOT = 2,
};

struct imx_option_t
{
    bool debug;
    enum imx_output_type_t output;
};

enum imx_error_t mkimxboot(const char *infile, const char *bootfile,
    const char *outfile, struct imx_option_t opt);
