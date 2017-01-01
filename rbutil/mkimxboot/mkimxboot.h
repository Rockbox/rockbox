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

#ifndef MKIMXBOOT_H
#define MKIMXBOOT_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

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
    IMX_VARIANT_MISMATCH = -9,
    IMX_WRITE_ERROR = -10,
    IMX_FIRST_SB_ERROR = -11,
    IMX_MODEL_MISMATCH = -12,
};

enum imx_output_type_t
{
    IMX_DUALBOOT = 0,
    IMX_RECOVERY,
    IMX_SINGLEBOOT,
    IMX_CHARGE,
    IMX_ORIG_FW,
};

/* Supported models */
enum imx_model_t
{
    MODEL_UNKNOWN = 0,
    MODEL_FUZEPLUS,
    MODEL_ZENXFI2,
    MODEL_ZENXFI3,
    MODEL_ZENXFISTYLE,
    MODEL_ZENSTYLE, /* Style 100 and Style 300 */
    MODEL_NWZE370,
    MODEL_NWZE360,
    /* Last */
    MODEL_COUNT
};

/* Supported firmware variants */
enum imx_firmware_variant_t
{
    VARIANT_DEFAULT = 0,
    /* For the Creative ZEN X-Fi2 */
    VARIANT_ZENXFI2_NAND,
    VARIANT_ZENXFI2_SD,
    VARIANT_ZENXFI2_RECOVERY,
    /* For the Creative X-Fi Style */
    VARIANT_ZENXFISTYLE_RECOVERY,
    /* For the Creative Zen Style 100/300 */
    VARIANT_ZENSTYLE_RECOVERY,
    /* Last */
    VARIANT_COUNT
};

struct imx_option_t
{
    bool debug;
    enum imx_model_t model;
    enum imx_output_type_t output;
    enum imx_firmware_variant_t fw_variant;
    const char *force_version; // set to NULL to ignore
};

/* Print internal information to stdout about device database */
void dump_imx_dev_info(const char *prefix);
/* Build a SB image from an input firmware and a bootloader, input firmware
 * can either be a firmware update or another SB file produced by this tool */
enum imx_error_t mkimxboot(const char *infile, const char *bootfile,
    const char *outfile, struct imx_option_t opt);
/* Compute MD5 sum of an entire file */
enum imx_error_t compute_md5sum(const char *file, uint8_t file_md5sum[16]);
/* Compute "soft" MD5 sum of a SB file */
enum imx_error_t compute_soft_md5sum(const char *file, uint8_t soft_md5sum[16]);

#ifdef __cplusplus
}
#endif
#endif

