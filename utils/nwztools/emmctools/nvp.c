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
#include "nvp.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static struct nvp_zone_info_entry_t nvp_zone_ubt[] =
{
    {0x18, 0, 1, 4, 0, 0, 0, 0, "system information"},
    {0x17, 1, 1, 0x20, 0, 0, 0, 0, "u-boot password"},
    {9, 2, 1, 4, 0, 0, 0, 0, "firmware update flag"},
    {0xA, 3, 1, 4, 0, 0, 0, 0, "beep ok flag"},
    {0x22, 4, 1, 0x10, 0, 0, 0, 0, "rtc alarm"},
    {0x50, 5, 1, 4, 0, 0, 0, 0, "hold mode"}
};

static struct nvp_zone_info_entry_t nvp_zone_sys[] =
{
    {0x10, 1, 1, 0x40, 0, 0, 0, 0, "model id"},
    {4, 2, 1, 0x10, 0, 0, 0, 0, "serial number"},
    {0xB, 3, 1, 0x20, 0, 0, 0, 0, "ship information"},
    {0x44, 4, 1, 4, 0, 0, 0, 0, "color variation"},
    {0x1A, 5, 1, 5, 0, 0, 0, 0, "product code"},
    {0x1D, 6, 1, 8, 0, 0, 0, 0, "update file name"},
    {0x20, 7, 1, 0x40, 0, 0, 0, 0, "key and signature"},
    {0x11, 8, 1, 4, 0, 0, 0, 0, "test mode flag"},
    {0x12, 9, 1, 4, 0, 0, 0, 0, "getty mode flag"},
    {0x46, 0xA, 1, 4, 0, 0, 0, 0, "disable iptable flag"},
    {0x1E, 0xB, 1, 0x40, 0, 0, 0, 0, "sound driver parameter"},
    {0x1F, 0xC, 1, 0x40, 0, 0, 0, 0, "noise cancel driver parameter"},
    {0x4D, 0xD, 1, 6, 0, 0, 0, 0, "wifi mac address"},
    {0x4B, 0xE, 1, 4, 0, 0, 0, 0, "wifi protected setup"},
    {0x52, 0xF, 1, 0x10, 0, 0, 0, 0, "fm parameter"},
    {0x53, 0x10, 1, 4, 0, 0, 0, 0, "speaker ship info"},
    {0x54, 0x11, 1, 4, 0, 0, 0, 0, "mass storage class mode"},
    {0x19, 0x12, 1, 4, 0, 0, 0, 0, "exception monitor mode"},
    {0x1B, 0x13, 1, 4, 0, 0, 0, 0, "battery calibration"},
    {0x56, 0x14, 1, 0x200, 0, 0, 0, 0, "bluetooth pskey"}
};

static struct nvp_zone_info_entry_t nvp_zone_app[] =
{
    {5, 0, 8, 0x1000, 0, 0, 0, 0, "application parameter"},
    {7, 0x40, 1, 0x14, 0, 0, 0, 0, "secure clock"},
    {0xC, 0x41, 1, 0xA0, 0, 0, 0, 0, "aad icv"},
    {0xD, 0x42, 2, 0x208, 0, 0, 0, 0, "empr key"},
    {0x4C, 0x44, 1, 0x10, 0, 0, 0, 0, "slacker time"},
    {0x15, 0x45, 1, 4, 0, 0, 0, 0, "key mode (debug/release)"},
    {0x47, 0x46, 1, 0x40, 0, 0, 0, 0, "marlin time"},
    {0x48, 0x47, 0x20, 0x4000, 0, 0, 0, 0, "marlin crl"},
    {0x59, 0x76, 1, 0x200, 0, 0, 0, 0, "btmw factory pair info"},
    {0x58, 0x77, 1, 0x200, 0, 0, 0, 0, "btmw factory scdb"},
    {0x57, 0x78, 1, 4, 0, 0, 0, 0, "btmw log mode flag"},
    {0x55, 0x79, 1, 4, 0, 0, 0, 0, "europe vol regulation flag"},
    {8, 0x7A, 1, 8, 0, 0, 0, 0, "middleware parameter"},
    {0x16, 0x7B, 1, 4, 0, 0, 0, 0, "quick shutdown flag"},
    {0x45, 0x7C, 1, 4, 0, 0, 0, 0, "time out to sleep"},
    {0x4E, 0x7D, 1, 4, 0, 0, 0, 0, "application debug mode flag"},
    {0x4F, 0x7E, 1, 4, 0, 0, 0, 0, "browser log mode flag"}
};

static struct nvp_zone_info_entry_t nvp_zone_drm[] =
{
    {3, 1, 2, 0x2C0, 0, 0, 0, 0, "aad key"},
    {0x1C, 6, 1, 0x40, 0, 0, 0, 0, "wmt key"},
    {0x51, 9, 0x11, 0x2020, 0, 0, 0, 0, "slacker id file"},
    {0x49, 0x1A, 0x41, 0x8100, 0, 0, 0, 0, "marlin device key"},
    {0x21, 0x5B, 1, 0x40, 0, 0, 0, 0, "starfish id"},
    {0x23, 0x5C, 4, 0x800, 0, 0, 0, 0, "bluetooth address"}
};

static struct nvp_zone_info_entry_t nvp_zone_ekb[] =
{
    {0xE, 0, 0x20, 0x4000, 0, 0, 0, 0, "EKB 0"},
    {0xF, 0x20, 0x20, 0x4000, 0, 0, 0, 0, "EKB 1"},
    {0x4A, 0x40, 0x30, 0x6000, 0, 0, 0, 0, "marlin user key"}
};

static struct nvp_zone_info_entry_t nvp_zone_emp[] =
{
    {0x24, 0, 2, 0x400, 0, 0, 0, 0, "EMPR  0"},
    {0x25, 2, 2, 0x400, 0, 0, 0, 0, "EMPR  1"},
    {0x26, 4, 2, 0x400, 0, 0, 0, 0, "EMPR  2"},
    {0x27, 6, 2, 0x400, 0, 0, 0, 0, "EMPR  3"},
    {0x28, 8, 2, 0x400, 0, 0, 0, 0, "EMPR  4"},
    {0x29, 0xA, 2, 0x400, 0, 0, 0, 0, "EMPR  5"},
    {0x2A, 0xC, 2, 0x400, 0, 0, 0, 0, "EMPR  6"},
    {0x2B, 0xE, 2, 0x400, 0, 0, 0, 0, "EMPR  7"},
    {0x2C, 0x10, 2, 0x400, 0, 0, 0, 0, "EMPR  8"},
    {0x2D, 0x12, 2, 0x400, 0, 0, 0, 0, "EMPR  9"},
    {0x2E, 0x14, 2, 0x400, 0, 0, 0, 0, "EMPR 10"},
    {0x2F, 0x16, 2, 0x400, 0, 0, 0, 0, "EMPR 11"},
    {0x30, 0x18, 2, 0x400, 0, 0, 0, 0, "EMPR 12"},
    {0x31, 0x1A, 2, 0x400, 0, 0, 0, 0, "EMPR 13"},
    {0x32, 0x1C, 2, 0x400, 0, 0, 0, 0, "EMPR 14"},
    {0x33, 0x1E, 2, 0x400, 0, 0, 0, 0, "EMPR 15"},
    {0x34, 0x20, 2, 0x400, 0, 0, 0, 0, "EMPR 16"},
    {0x35, 0x22, 2, 0x400, 0, 0, 0, 0, "EMPR 17"},
    {0x36, 0x24, 2, 0x400, 0, 0, 0, 0, "EMPR 18"},
    {0x37, 0x26, 2, 0x400, 0, 0, 0, 0, "EMPR 19"},
    {0x38, 0x28, 2, 0x400, 0, 0, 0, 0, "EMPR 20"},
    {0x39, 0x2A, 2, 0x400, 0, 0, 0, 0, "EMPR 21"},
    {0x3A, 0x2C, 2, 0x400, 0, 0, 0, 0, "EMPR 22"},
    {0x3B, 0x2E, 2, 0x400, 0, 0, 0, 0, "EMPR 23"},
    {0x3C, 0x30, 2, 0x400, 0, 0, 0, 0, "EMPR 24"},
    {0x3D, 0x32, 2, 0x400, 0, 0, 0, 0, "EMPR 25"},
    {0x3E, 0x34, 2, 0x400, 0, 0, 0, 0, "EMPR 26"},
    {0x3F, 0x36, 2, 0x400, 0, 0, 0, 0, "EMPR 27"},
    {0x40, 0x38, 2, 0x400, 0, 0, 0, 0, "EMPR 28"},
    {0x41, 0x3A, 2, 0x400, 0, 0, 0, 0, "EMPR 29"},
    {0x42, 0x3C, 2, 0x400, 0, 0, 0, 0, "EMPR 30"},
    {0x43, 0x3E, 2, 0x400, 0, 0, 0, 0, "EMPR 31"}
};

static struct nvp_zone_info_entry_t nvp_zone_bti[] =
{
    {1, 0, 0x20, 0x40000, 0, 0, 0, 0, "boot image"}
};

static struct nvp_zone_info_entry_t nvp_zone_hdi[] =
{
    {2, 0, 0x20, 0x40000, 0, 0, 0, 0, "hold image"}
};

static struct nvp_zone_info_entry_t nvp_zone_lbi[] =
{
    {0x14, 0, 0x20, 0x40000, 0, 0, 0, 0, "low battery image"}
};

static struct nvp_zone_info_entry_t nvp_zone_upi[] =
{
    {0x13, 0, 0x20, 0x40000, 0, 0, 0, 0, "update image"}
};

static struct nvp_zone_info_entry_t nvp_zone_eri[] =
{
    {6, 0, 0x20, 0x40000, 0, 0, 0, 0, "update error image"}
};

struct nvp_area_info_entry_t nvp_area_info[NVP_NR_AREAS] =
{
    {2, nvp_zone_ubt, 6, 0, 0, 0, 0, "u-boot parameter"},
    {2, nvp_zone_sys, 0x14, 0, 0, 0, 0, "system parameter"},
    {2, nvp_zone_app, 0x11, 0, 0, 0, 0, "application parameter"},
    {2, nvp_zone_drm, 6, 0, 0, 0, 0, "drm data"},
    {2, nvp_zone_ekb, 3, 0, 0, 0, 0, "ekb data"},
    {2, nvp_zone_emp, 0x20, 0, 0, 0, 0, "empr data"},
    {2, 0, 0, 0, 0, 0, 0, "reserved"},
    {2, 0, 0, 0, 0, 0, 0, "reserved"},
    {1, nvp_zone_bti, 1, 0, 0, 0, 0, "boot image"},
    {1, nvp_zone_hdi, 1, 0, 0, 0, 0, "hold image"},
    {1, nvp_zone_lbi, 1, 0, 0, 0, 0, "low battery image"},
    {1, nvp_zone_upi, 1, 0, 0, 0, 0, "update image"},
    {1, nvp_zone_eri, 1, 0, 0, 0, 0, "update error image"},
    {1, 0, 0, 0, 0, 0, 0, "reserved"},
    {1, 0, 0, 0, 0, 0, 0, "reserved"},
    {1, 0, 0, 0, 0, 0, 0, "reserved"}
};

static int nr_nodes;
static struct nvp_node_info_t *node_info;
static nvp_read_fn_t nvp_read;
static int nvp_size;
static int nr_sectors;
static int nr_clusters;
static uint8_t *nvp_table;
static uint8_t *nvp_shadow;
static uint16_t *nvp_bitmap;

int nvp_get_cluster_status(int cluster)
{
    if(cluster <= 3 || cluster >= nr_clusters)
    {
        cprintf(GREY, "invalid cluster number: cluster=%d\n", cluster);
        return -1;
    }
    return nvp_bitmap[cluster];
}

int nvp_set_cluster_status(int cluster, int status)
{
    if(cluster <= 3 || cluster >= nr_clusters)
    {
        cprintf(GREY, "invalid cluster number: cluster=%d\n", cluster);
        return -1;
    }
    nvp_bitmap[cluster] = status;
    return 0;
}

int nvp_get_sector_status(int sector)
{
    if(sector <= 3 || sector >= nr_sectors)
    {
        cprintf(GREY, "invalid sector number: sector=%d\n", sector);
        return -1;
    }
    return (nvp_bitmap[sector >> 4] >> (sector & 0xf)) & 0x1;
}

int nvp_set_sector_status(int sector, int status)
{
    if(sector <= 3 || sector >= nr_sectors)
    {
        cprintf(GREY, "invalid sector number: sector=%d\n", sector);
        return -1;
    }
    if(status)
        nvp_bitmap[sector >> 4] |= 1 << (sector & 0xf);
    else
        nvp_bitmap[sector >> 4] &= ~(1 << (sector & 0xf));
    return 0;
}

int nvp_get_cluster_number(int shadow, int area, int zone, int index)
{
    int start = nvp_area_info[area].zone_info[zone].start;
    int count = nvp_area_info[area].zone_info[zone].count;
    if(index >= count)
    {
        cprintf(GREY, "invalid index: index=%d\n", index);
        return -1;
    }
    uint8_t *ptr = shadow ? nvp_shadow : nvp_table;
    uint16_t cluster = *(uint16_t *)&ptr[area * NVP_AREA_TABLE_SIZE + (start + index) * 2];
    if(cluster == 0)
        return 0;
    if(cluster <= 3 || cluster >= nr_clusters)
    {
        cprintf(GREY, "invalid cluster: shadow=%d area=%d zone=%d index=%d cluster=%d\n",
            shadow, area, zone, index, cluster);
        return -1;
    }
    return cluster;
}

int nvp_get_sector_number(int shadow, int area, int zone, int index)
{
    int start = nvp_area_info[area].zone_info[zone].start;
    int count = nvp_area_info[area].zone_info[zone].count;
    if(index >= count)
    {
        cprintf(GREY, "invalid index: index=%d\n", index);
        return -1;
    }
    uint8_t *ptr = shadow ? nvp_shadow : nvp_table;
    //cprintf(GREY, "[offset: 0x%x]", area * NVP_AREA_TABLE_SIZE + (start + index) * 4);
    uint32_t sector = *(uint32_t *)&ptr[area * NVP_AREA_TABLE_SIZE + (start + index) * 4];
    if(sector == 0)
        return 0;
    if(sector <= 0x3f || sector >= (unsigned)nr_sectors)
    {
        cprintf(GREY, "invalid sector: shadow=%d area=%d zone=%d index=%d sector=%d\n",
            shadow, area, zone, index, sector);
        return -1;
    }
    return sector;
}

int nvp_read_data(int shadow, int area, int zone, int offset, void *buf, int size)
{
    int large = nvp_area_info[area].kind == NVP_AREA_LARGE_KIND;
    int unit_size = large ? NVP_LARGE_AREA_SIZE : NVP_SMALL_AREA_SIZE;
    int read_size = 0;

    while(size > 0)
    {
        int index = offset / unit_size;
        int unit_offset = offset % unit_size;
        int sec_cluster = large ?
            nvp_get_cluster_number(shadow, area, zone, index) :
            nvp_get_sector_number(shadow, area, zone, index);
        if(sec_cluster == 0)
            break;
        int read = MIN(size, unit_size - unit_offset);
        //cprintf(GREY, "[sec_cluster=%d unit_size=%d read=%d]", sec_cluster, unit_size, read);
        int ret = nvp_read(sec_cluster * unit_size, read, buf);
        if(ret)
            return ret;
        buf += read;
        offset += read;
        size -= read;
        read_size += read;
    }
    return read_size;
}

bool nvp_is_valid_node(int node)
{
    return node >= 0 && node < nr_nodes && node_info[node].area != -1;
}

struct nvp_node_info_t nvp_get_node_info(int node)
{
    return node_info[node];
}

int nvp_get_node_size(int node)
{
    struct nvp_node_info_t i = nvp_get_node_info(node);
    return nvp_area_info[i.area].zone_info[i.zone].size;
}

const char *nvp_get_node_name(int node)
{
    struct nvp_node_info_t i = nvp_get_node_info(node);
    return nvp_area_info[i.area].zone_info[i.zone].name;
}

const char *nvp_get_area_name(int area)
{
    return nvp_area_info[area].name;
}

int nvp_read_node(int node, int offset, void *buffer, int size)
{
    struct nvp_node_info_t i = nvp_get_node_info(node);
    return nvp_read_data(0, i.area, i.zone, offset, buffer, size);
}

int nvp_init(int size, nvp_read_fn_t read, bool debug)
{
    nvp_read = read;
    nvp_size = size;
    nr_sectors = nvp_size / NVP_SECTOR_SIZE;
    nr_clusters = (nr_sectors + NVP_SECTOR_PER_CLUSTER) / NVP_SECTOR_PER_CLUSTER;
    // check that the tables are consistent and compute the number of nodes
    if(debug)
        cprintf(BLUE, "NVP Debug\n");
    for(int i = 0; i < NVP_NR_AREAS; i++)
    {
        if(debug)
        {
            cprintf(RED, "  %s Area: ", nvp_area_info[i].kind == NVP_AREA_SMALL_KIND ? "Small" : "Large");
            cprintf(GREEN, "%s\n", nvp_area_info[i].name);
        }
        if(nvp_area_info[i].zone_info == NULL)
            continue;

        struct nvp_zone_info_entry_t *zones = nvp_area_info[i].zone_info;
        int nr_zones = nvp_area_info[i].nr_zones;
        int kind = nvp_area_info[i].kind;
        if(kind != NVP_AREA_SMALL_KIND && kind != NVP_AREA_LARGE_KIND)
            continue;
        
        uint32_t bitmap[256];
        memset(bitmap, 0, sizeof(bitmap));

        for(int j = 0; j < nr_zones; j++)
        {
            if(debug)
            {
                cprintf_field("    Zone ", "%s", zones[j].name);
                cprintf_field(" Node ", "%d", zones[j].node);
                cprintf_field(" Start ", "%#x", zones[j].start);
                cprintf_field(" Count ", "%#x", zones[j].count);
                cprintf_field(" Size ", "%#x\n", zones[j].size);
            }

            if(kind == NVP_AREA_LARGE_KIND)
            {
                if(zones[j].start >= NVP_LARGE_AREA_MAX_CLUSTER ||
                        zones[j].start + zones[j].count > NVP_LARGE_AREA_MAX_CLUSTER)
                {
                    cprintf(GREY, "Bad zone start/count\n");
                    return 95;
                }
                if(zones[j].size > zones[j].count * NVP_LARGE_AREA_SIZE)
                {
                    cprintf(GREY, "Bad zone size\n");
                    return 96;
                }
            }
            else
            {
                if(zones[j].start >= NVP_SMALL_AREA_MAX_CLUSTER ||
                        zones[j].start + zones[j].count > NVP_SMALL_AREA_MAX_CLUSTER)
                {
                    cprintf(GREY, "Bad zone start/count\n");
                    return 97;
                }
                if(zones[j].size > zones[j].count * NVP_SMALL_AREA_SIZE)
                {
                    cprintf(GREY, "Bad zone size\n");
                    return 98;
                }
            }

            nr_nodes++;
            
            for(int k = 0; k < zones[j].count; k++)
            {
                if(bitmap[zones[j].start + k])
                {
                    cprintf(GREY, "Zone overlap !\n");
                    return 99;
                }
                bitmap[zones[j].start + k] = 0xffffffff;
            }
        }
    }

    // build node table
    nr_nodes++; // nodes start at 1 ?!
    node_info = malloc(nr_nodes * sizeof(struct nvp_node_info_t));
    memset(node_info, 0xff, nr_nodes * sizeof(struct nvp_node_info_t));

    for(int i = 0; i < NVP_NR_AREAS; i++)
    {
        if(nvp_area_info[i].zone_info == NULL)
            continue;

        struct nvp_zone_info_entry_t *zones = nvp_area_info[i].zone_info;
        int nr_zones = nvp_area_info[i].nr_zones;
        int kind = nvp_area_info[i].kind;
        if(kind != NVP_AREA_SMALL_KIND && kind != NVP_AREA_LARGE_KIND)
            continue;

        for(int j = 0; j < nr_zones; j++)
        {
            int node = zones[j].node;
            if(node >= nr_nodes)
            {
                cprintf(GREY, "Node out of bounds !\n");
                return 89;
            }
            if(node_info[node].area != -1 && node_info[node].zone != -1)
            {
                cprintf(GREY, "Node overlap: area=%d zone=%d node=%d to area=%d zone=%d !\n",
                    i, j, node, node_info[node].area, node_info[node].zone);
                return 88;
            }
            node_info[node].area = i;
            node_info[node].zone = j;
        }
    }

    // load allocation table
    nvp_table = malloc(NVP_CLUSTER_SIZE);
    int ret = nvp_read(NVP_TABLE_SECTOR * NVP_SECTOR_SIZE, NVP_CLUSTER_SIZE, nvp_table);
    if(ret) return ret;

    // init shadow table
    nvp_shadow = malloc(NVP_CLUSTER_SIZE);
    memset(nvp_shadow, 0, NVP_CLUSTER_SIZE);

    // init bitmap
    nvp_bitmap = malloc(sizeof(uint16_t) * nr_clusters);
    memset(nvp_bitmap, 0, sizeof(uint16_t) * nr_clusters);

    // read map
    for(int i = 0; i < NVP_NR_AREAS; i++)
    {
        if(nvp_area_info[i].zone_info == NULL)
            continue;
        int kind = nvp_area_info[i].kind;
        if(kind != NVP_AREA_SMALL_KIND && kind != NVP_AREA_LARGE_KIND)
            continue;

        if(kind == NVP_AREA_LARGE_KIND)
        {
            for(int cluster = 0; cluster < NVP_LARGE_AREA_MAX_CLUSTER; cluster++)
            {
                uint16_t entry = *(uint16_t *)&nvp_table[i * NVP_AREA_TABLE_SIZE + cluster * 2];
                if(entry == 0)
                    continue;
                if(nvp_get_cluster_status(entry) != 0)
                {
                    cprintf(GREY, "cluster already used: area=%d cluster=%d entry=%d\n", i, cluster, entry);
                    return 78;
                }
                nvp_set_cluster_status(entry, 0xffff);
            }
        }
        else
        {
            for(int cluster = 0; cluster < NVP_SMALL_AREA_MAX_CLUSTER; cluster++)
            {
                uint32_t entry = *(uint32_t *)&nvp_table[i * NVP_AREA_TABLE_SIZE + cluster * 4];
                if(entry == 0)
                    continue;
                if(nvp_get_sector_status(entry) != 0)
                {
                    cprintf(GREY, "sector already used: area=%d cluster=%d entry=%d\n", i, cluster, entry);
                    return 76;
                }
                nvp_set_sector_status(entry, 1);
            }
        }
    }

    return 0;
}

int nvp_info(void)
{
    uint32_t version;
    int ret = nvp_read(0, sizeof(version), &version);
    if(ret) return ret;

    cprintf(BLUE, "NVP\n");
    cprintf_field("  Version: ", "%x\n", version);

    for(int i = 0; i < NVP_NR_AREAS; i++)
    {
        cprintf(RED, "  Area: ");
        cprintf(GREEN, "%s\n", nvp_area_info[i].name);
        if(nvp_area_info[i].zone_info == NULL)
            continue;

        struct nvp_zone_info_entry_t *zones = nvp_area_info[i].zone_info;
        int nr_zones = nvp_area_info[i].nr_zones;

        for(int j = 0; j < nr_zones; j++)
        {
            cprintf_field("    Zone ", "%s", zones[j].name);
            cprintf(BLUE, " ->");
            uint8_t buf[0x20];
            int sz = 0x20;
            int ret = nvp_read_data(0, i, j, 0, buf, MIN(sz, zones[j].size));
            if(ret <= 0)
            {
                cprintf(RED, " No data\n");
                continue;
            }
            sz = MIN(sz, ret);
            for(int i = 0; i < MIN(sz, zones[j].size); i++)
                cprintf(YELLOW, " %02x", buf[i]);
            cprintf(BLUE, " -> ");
            for(int i = 0; i < MIN(sz, zones[j].size); i++)
                cprintf(YELLOW, "%c", isprint(buf[i]) ? buf[i] : '.');
            printf("\n");
        }
    }
    
    return 0;
}
