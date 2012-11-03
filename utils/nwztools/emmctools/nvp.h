/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
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
#ifndef __NVP_H__
#define __NVP_H__

#include <stdint.h>
#include <stdbool.h>
#include "misc.h"

#define NVP_AREA_LARGE_KIND     1
#define NVP_AREA_SMALL_KIND     2

#define NVP_SMALL_AREA_MAX_CLUSTER  128
#define NVP_LARGE_AREA_MAX_CLUSTER  256
#define NVP_AREA_TABLE_SIZE     512

#define NVP_SECTOR_SIZE         512
#define NVP_TABLE_SECTOR        16
#define NVP_DATA_SECTOR_MIN     64
#define NVP_DATA_SECTOR_MAX     32767
#define NVP_SECTOR_PER_CLUSTER  16
#define NVP_CLUSTER_SIZE        (NVP_SECTOR_SIZE * NVP_SECTOR_PER_CLUSTER)

#define NVP_LARGE_AREA_SIZE     NVP_CLUSTER_SIZE
#define NVP_SMALL_AREA_SIZE     NVP_SECTOR_SIZE

struct nvp_zone_info_entry_t
{
    int node;
    int start; // in 4 unit
    int count; // in 512 unit for kind 2 and in 8192 for kind 1
    int size;
    int res0, res1, res2, res3;
    const char *name;
};

struct nvp_area_info_entry_t
{
    int kind;
    struct nvp_zone_info_entry_t *zone_info;
    int nr_zones;
    int res0, res1, res2, res3;
    const char *name;
};

struct nvp_node_info_t
{
    int area;
    int zone;
};

#define NVP_NR_AREAS    16

extern struct nvp_area_info_entry_t nvp_area_info[NVP_NR_AREAS];

typedef int (*nvp_read_fn_t)(uint32_t offset, uint32_t size, void *buf);

int nvp_init(int nvp_size, nvp_read_fn_t read, bool debug);
bool nvp_is_valid_node(int node);
struct nvp_node_info_t nvp_get_node_info(int node);
int nvp_get_node_size(int node);
const char *nvp_get_node_name(int node);
const char *nvp_get_area_name(int node);
int nvp_read_node(int node, int offset, void *buffer, int size);

int nvp_info(void);

int nvp_get_cluster_status(int cluster);
int nvp_set_cluster_status(int cluster, int status);
int nvp_get_sector_status(int sector);
int nvp_set_sector_status(int sector, int status);
int nvp_get_cluster_number(int shadow, int area, int zone, int index);
int nvp_get_sector_number(int shadow, int area, int zone, int index);
/* returns amount of read data or -1 */
int nvp_read_data(int shadow, int area, int zone, int offset, void *buffer, int size);

#endif /* __NVP_H__ */
