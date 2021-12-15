/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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

#ifndef MKZENBOOT_H
#define MKZENBOOT_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum zen_error_t
{
    ZEN_SUCCESS = 0,
    ZEN_ERROR = -1,
    ZEN_OPEN_ERROR = -2,
    ZEN_READ_ERROR = -3,
    ZEN_NO_MATCH = -4,
    ZEN_BOOT_INVALID = -5,
    ZEN_BOOT_MISMATCH = -6,
    ZEN_BOOT_CHECKSUM_ERROR = -7,
    ZEN_DONT_KNOW_HOW_TO_PATCH = -8,
    ZEN_WRITE_ERROR = -9,
    ZEN_UNSUPPORTED = 10,
    ZEN_FW_INVALID = -11,
    ZEN_FW_MISMATCH = -12,
    ZEN_FIRST_ZENTOOLS_ERROR = -12,
};

enum zen_output_type_t
{
    ZEN_DUALBOOT = 0, /* keep all OF data and pack OF+RB into firmware for dualboot */
    ZEN_MIXEDBOOT, /* rename OF, keep data, put RB as firmware, use RB bootloader to dualboot */
    ZEN_RECOVERY, /* only put rockbox with recovery mode */
    ZEN_SINGLEBOOT, /* only put rockbox with recovery mode */
};

/* Supported models */
enum zen_model_t
{
    MODEL_UNKNOWN = 0,
    MODEL_ZENMOZAIC,
    MODEL_ZENV,
    MODEL_ZENXFI,
    MODEL_ZEN,
    /* new models go here */

    NUM_MODELS
};

struct zen_option_t
{
    bool debug;
    enum zen_output_type_t output;
    enum zen_model_t model; /* pass MODEL_UNKNOWN to use MD5 knowledge base */
};

enum zen_error_t mkzenboot(const char *infile, const char *bootfile,
    const char *outfile, struct zen_option_t opt);

#ifdef __cplusplus
}
#endif
#endif

