/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "jztool.h"
#include <string.h>

static const jz_device_info infotable[JZ_NUM_DEVICES] = {
    [JZ_DEVICE_FIIOM3K] = {
        .name = "fiiom3k",
        .file_ext = "m3k",
        .description = "FiiO M3K",
        .device_type = JZ_DEVICE_FIIOM3K,
        .cpu_type = JZ_CPU_X1000,
        .vendor_id = 0x2972,
        .product_id = 0x0003,
    },
    [JZ_DEVICE_SHANLINGQ1] = {
        .name = "shanlingq1",
        .file_ext = "q1",
        .description = "Shanling Q1",
        .device_type = JZ_DEVICE_SHANLINGQ1,
        .cpu_type = JZ_CPU_X1000,
        .vendor_id = 0x0525,
        .product_id = 0xa4a5,
    },
    [JZ_DEVICE_EROSQ] = {
        .name = "erosq",
        .file_ext = "erosq",
        .description = "AIGO Eros Q",
        .device_type = JZ_DEVICE_EROSQ,
        .cpu_type = JZ_CPU_X1000,
        .vendor_id = 0xc502,
        .product_id = 0x0023,
    },
    [JZ_DEVICE_SHANLINGM0PRO] = {
        .name = "shanlingm0pro",
        .file_ext = "m0pro",
        .description = "Shanling M0 Pro",
        .device_type = JZ_DEVICE_SHANLINGM0PRO,
        .cpu_type = JZ_CPU_X1000,
        .vendor_id = 0x0525,
        .product_id = 0xa4a5,
    },
    [JZ_DEVICE_HIBYR1] = {
        .name = "hibyr1",
        .file_ext = "r1",
        .description = "HiBy R1",
        .device_type = JZ_DEVICE_HIBYR1,
        .cpu_type = JZ_CPU_X1600,
        /* NOTE(x1600): these are the mass-storage-mode IDs and are UNVERIFIED.
         * The only IDs confirmed on the device are the BootROM's, which live in
         * the cputable entry below. Fill these in from `lsusb` with the player
         * booted into its normal firmware and mounted as UMS. */
        .vendor_id = 0x0000,
        .product_id = 0x0000,
    },
};

static const jz_cpu_info cputable[JZ_NUM_CPUS] = {
    [JZ_CPU_X1000] = {
        .info_str = "X1000_v1",
        .vendor_id = 0xa108,
        .product_id = 0x1000,
        .stage1_load_addr = 0xf4001000,
        .stage1_exec_addr = 0xf4001800,
        .stage2_load_addr = 0x80004000,
        .stage2_exec_addr = 0x80004000,
    },
    [JZ_CPU_X1600] = {
        /* A HiBy R1 in BootROM USB mode enumerates as
         * a108:eaef "Ingenic USB BOOT DEVICE", interface class 255, bulk
         * endpoints 0x01 OUT / 0x81 IN with 512-byte packets, and answers
         * GET_CPU_INFO (with bmRequestType 0xC0, see src/usb.c) with "X1600". */
        .info_str = "X1600",
        .vendor_id = 0xa108,
        .product_id = 0xeaef,

        /* Raw USB stage1, linked above the BootROM's cache area (PM 34.3).
         * Unlike the X1000 flash SPL, this image has no header to skip. */
        .stage1_load_addr = 0x8000a000,
        .stage1_exec_addr = 0x8000a000,

        /* Matches X1600_BOOT_LOAD_ADDR in firmware/export/x1600.h. */
        .stage2_load_addr = 0x80100000,
        .stage2_exec_addr = 0x80100000,
    },
};

/** \brief Lookup info for a device by type, returns NULL if not found. */
const jz_device_info* jz_get_device_info(jz_device_type type)
{
    return jz_get_device_info_indexed(type);
}

/** \brief Lookup info for a device by name, returns NULL if not found. */
const jz_device_info* jz_get_device_info_named(const char* name)
{
    for(int i = 0; i < JZ_NUM_DEVICES; ++i)
        if(!strcmp(infotable[i].name, name))
            return &infotable[i];

    return NULL;
}

/** \brief Get a device info entry by index, returns NULL if out of range. */
const jz_device_info* jz_get_device_info_indexed(int index)
{
    if(index < JZ_NUM_DEVICES)
        return &infotable[index];
    else
        return NULL;
}

/** \brief Lookup info for a CPU, returns NULL if not found. */
const jz_cpu_info* jz_get_cpu_info(jz_cpu_type type)
{
    if(type < JZ_NUM_CPUS)
        return &cputable[type];
    else
        return NULL;
}

/** \brief Lookup info for a CPU by info string, returns NULL if not found. */
const jz_cpu_info* jz_get_cpu_info_named(const char* info_str)
{
    for(int i = 0; i < JZ_NUM_CPUS; ++i)
        if(!strcmp(cputable[i].info_str, info_str))
            return &cputable[i];

    return NULL;
}
