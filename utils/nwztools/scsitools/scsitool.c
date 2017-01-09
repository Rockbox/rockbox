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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "rbscsi.h"
#include "misc.h"
#include "para_noise.h"
#include "nwz_db.h"

bool g_debug = false;
const char *g_force_series = NULL;
char *g_out_prefix = NULL;
rb_scsi_device_t g_dev;

static void print_hex(void *_buffer, int buffer_size)
{
    uint8_t *buffer = _buffer;
    for(int i = 0; i < buffer_size; i += 16)
    {
        for(int j = 0; j < 16; j++)
        {
            if(i + j < buffer_size)
                cprintf(YELLOW, " %02x", buffer[i + j]);
            else
                cprintf(YELLOW, "   ");
        }
        printf(" ");
        for(int j = 0; j < 16; j++)
        {
            if(i + j < buffer_size)
                cprintf(RED, "%c", isprint(buffer[i + j]) ? buffer[i + j] : '.');
            else
                cprintf(RED, " ");
        }
        printf("\n");
    }
}

/* Do read */
#define DO_READ     (1 << 1)
/* Do write */
#define DO_WRITE    (1 << 2)

/* returns <0 on error and status otherwise */
int do_scsi(uint8_t *cdb, int cdb_size, unsigned flags, void *sense, int *sense_size, void *buffer, int *buf_size)
{
    struct rb_scsi_raw_cmd_t raw;
    raw.dir = RB_SCSI_NONE;
    if(flags & DO_READ)
        raw.dir = RB_SCSI_READ;
    if(flags & DO_WRITE)
        raw.dir = RB_SCSI_WRITE;
    raw.cdb_len = cdb_size;
    raw.cdb = cdb;
    raw.buf = buffer;
    raw.buf_len = *buf_size;
    raw.sense_len = *sense_size;
    raw.sense = sense;
    raw.tmo = 5;
    int ret = rb_scsi_raw_xfer(g_dev, &raw);
    *sense_size = raw.sense_len;
    *buf_size = raw.buf_len;
    return ret == RB_SCSI_OK || ret == RB_SCSI_SENSE ? raw.status : -ret;
}

int do_sense_analysis(int status, uint8_t *sense, int sense_size)
{
    if(status != 0 && g_debug)
    {
        cprintf(GREY, "Status: %d\n", status);
        cprintf(GREY, "Sense:");
        for(int i = 0; i < sense_size; i++)
            cprintf(GREY, " %02x", sense[i]);
        cprintf(GREY, "\n");
        rb_scsi_decode_sense(g_dev, sense, sense_size);
    }
    return status;
}

/*
 * SCSI commands
 */
#define CMD_A3          0xa3 /* start a complicated, authenticated, session to do things */
#define CMD_A4          0xa4 /* start a complicated, authenticated, session to do things */
#define CMD_EMPR_DPCC   0xd7
#define CMD_DNK         0xdd
#define CMD_DPCC        0xfb

/*
 * DNK: command is in cdb[10], subcommand in cdb[11], cdb[7] must be 0xbc
 */

int do_dnk_cmd(bool read, uint32_t cmd, uint8_t sub_cmd, uint16_t arg, void *buffer, int *buffer_size)
{
    uint8_t cdb[12] = {CMD_DNK, 0, 0, 0, 0, 0, 0, 0xbc, 0, 0, 0, 0};
    cdb[10] = cmd;
    cdb[11] = sub_cmd;
    cdb[8] = (*buffer_size) >> 8;
    cdb[9] = (*buffer_size) & 0xff;
    cdb[4] = (arg >> 8) & 0xff;
    cdb[5] = arg & 0xff;

    uint8_t sense[32];
    int sense_size = 32;

    int ret = do_scsi(cdb, 12, read ? DO_READ : DO_WRITE, sense, &sense_size, buffer, buffer_size);
    if(ret < 0)
        return ret;
    ret = do_sense_analysis(ret, sense, sense_size);
    if(ret)
        return ret;
    return 0;
}

#define DNK_EXACT_LENGTH    (1 << 0)
#define DNK_STRING          (1 << 1)
#define DNK_UINT32          (1 << 2)
#define DNK_HEX             (1 << 3)

struct dnk_prop_t
{
    const char *name;
    const char *desc;
    uint8_t cmd;
    uint8_t subcmd;
    int size;
    unsigned flags;
};

struct dnk_prop_t dnk_prop_list[] =
{
    { "serial_num", "Serial number", 0x23, 1, 8, DNK_STRING},
    { "storage_size", "Storage size(GB)", 0x23, 4, 4, DNK_EXACT_LENGTH | DNK_UINT32},
    { "product_id", "Product ID", 0x23, 6, 12, DNK_STRING},
    { "destination", "Destination", 0x23, 8, 4, DNK_EXACT_LENGTH | DNK_UINT32},
    { "model_id", "Model ID", 0x23, 9, 4, DNK_EXACT_LENGTH | DNK_UINT32 | DNK_HEX},
    { "model_name", "Model Name", 0x12, 0, 64, DNK_STRING},
    /* there are more obscure commands:
     * - 0x11 returns a 10-byte packet containing a 8-byte "LeftIdl8", scrambled
     *   with para_noise (the 2-byte padding is random so that output is random
     *   until unscrambled)
     * - 0x21 returns a 0x2b2 packet contaning a 0x2b0 "DNK", scrambled similarly
     * - 0x22 can write the DNK (sending scrambled data again)
     * - 0x23 has more subproperties:
     *   - 5 is "eDKS"
     *   - 7 is "ProductGroup"
     *   - 10 is nvp properties (see get_dnk_nvp)
     *   - 11 seems to read something from nvp and encrypt it with AES, not sure what
     * - 0x24 can write the same properties read by 0x23 */
};

#define NR_DNK_PROPS   (sizeof(dnk_prop_list) / sizeof(dnk_prop_list[0]))

uint16_t get_big_endian16(void *_buf)
{
    uint8_t *buf = _buf;
    return buf[0] << 16 | buf[1];
}

uint32_t get_big_endian32(void *_buf)
{
    uint8_t *buf = _buf;
    return buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
}

void set_big_endian16(void *_buf, uint16_t val)
{
    uint8_t *buf = _buf;
    buf[1] = val & 0xff;
    buf[0] = (val >> 8) & 0xff;
}

void set_big_endian32(void *_buf, uint32_t val)
{
    uint8_t *buf = _buf;
    buf[3] = val & 0xff;
    buf[2] = (val >> 8) & 0xff;
    buf[1] = (val >> 16) & 0xff;
    buf[0] = (val >> 24) & 0xff;
}

uint32_t get_little_endian32(void *_buf)
{
    uint8_t *buf = _buf;
    return buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0];
}

void set_little_endian32(void *_buf, uint32_t val)
{
    uint8_t *buf = _buf;
    buf[0] = val & 0xff;
    buf[1] = (val >> 8) & 0xff;
    buf[2] = (val >> 16) & 0xff;
    buf[3] = (val >> 24) & 0xff;
}

int get_dnk_prop(int argc, char **argv)
{
    if(argc != 1 && argc != 4)
    {
        printf("You must specify a known property name or a full property specification:\n");
        printf("Full usage: <cmd> <subcmd> <size> <flags>\n");
        printf("Property usage: <prop>\n");
        printf("Properties:");
        for(unsigned i = 0; i < NR_DNK_PROPS; i++)
            printf(" %s", dnk_prop_list[i].name);
        printf("\n");
        return 1;
    }

    struct dnk_prop_t prop;
    memset(&prop, 0, sizeof(prop));
    if(argc == 1)
    {
        for(unsigned i = 0; i < NR_DNK_PROPS; i++)
            if(strcmp(dnk_prop_list[i].name, argv[0]) == 0)
                prop = dnk_prop_list[i];
        if(prop.name == NULL)
        {
            cprintf(GREY, "Unknown property '%s'\n", argv[0]);
            return 1;
        }
    }
    else
    {
        prop.desc = "Property";
        prop.cmd = strtoul(argv[0], NULL, 0);
        prop.subcmd = strtoul(argv[1], NULL, 0);
        prop.size = strtoul(argv[2], NULL, 0);
        prop.flags = strtoul(argv[3], NULL, 0);
    }

    char *buffer = malloc(prop.size + 1);
    int buffer_size = prop.size;
    int ret = do_dnk_cmd(true, prop.cmd, prop.subcmd, 0, buffer, &buffer_size);
    if(ret)
        return ret;
    if(buffer_size == 0)
    {
        cprintf(GREY, "Device didn't send any data\n");
        return 1;
    }
    if((prop.flags & DNK_EXACT_LENGTH) && buffer_size != prop.size)
    {
        cprintf(GREY, "Device didn't send the expected amount of data\n");
        return 2;
    }
    buffer[buffer_size] = 0;
    cprintf(GREEN, "%s:", prop.desc);
    if(prop.flags & DNK_STRING)
        cprintf(YELLOW, " %s\n", buffer);
    else if(prop.flags & DNK_UINT32)
    {
        uint32_t val = get_big_endian32(buffer);
        if(prop.flags & DNK_HEX)
            cprintf(YELLOW, " 0x%x\n", val);
        else
            cprintf(YELLOW, " %u\n", val);
    }
    else
    {
        printf(YELLOW, "\n");
        print_hex(buffer, buffer_size);
    }
    return 0;
}

int get_model_and_series(int *model_index, int *series_index)
{
    /* if the user forced the series, simply match by name, special for '?' which
     * prompts the list */
    if(g_force_series)
    {
        cprintf(RED, "User forced series, auto-detection disabled\n");
        *series_index = -1;
        *model_index = -1;
        for(int i = 0; i < NWZ_SERIES_COUNT; i++)
                if(strcmp(nwz_series[i].codename, g_force_series) == 0)
                    *series_index = i;
        /* display list on error */
        if(*series_index == -1)
        {
            if(strcmp(g_force_series, "?") != 0)
                cprintf(GREY, "Unrecognized series '%s'\n", g_force_series);
            cprintf(OFF, "Series list:\n");
            for(int i = 0; i < NWZ_SERIES_COUNT; i++)
                printf("  %-10s %s\n", nwz_series[i].codename, nwz_series[i].name);
            return -1;
        }
    }
    else
    {
        /* we need to get the model ID: code stolen from get_dnk_prop */
        uint8_t mid_buf[4];
        int mid_buf_size = sizeof(mid_buf);
        int ret = do_dnk_cmd(true, 0x23, 9, 0, mid_buf, &mid_buf_size);
        if(ret)
        {
            cprintf(RED, "Cannot get model ID from device: %d\n", ret);
            return 2;
        }
        if(mid_buf_size != sizeof(mid_buf))
        {
            cprintf(RED, "Cannot get model ID from device: device didn't send the expected amount of data\n");
            return 3;
        }
        unsigned long model_id = get_big_endian32(&mid_buf);
        *model_index = -1;
        for(int i = 0; i < NWZ_MODEL_COUNT; i++)
            if(nwz_model[i].mid == model_id)
                *model_index = i;
        if(*model_index == -1)
        {
            cprintf(RED, "Your device is not supported. Please contact developers.\n");
            return 3;
        }
        *series_index = -1;
        for(int i = 0; i < NWZ_SERIES_COUNT; i++)
            for(int j = 0; j < nwz_series[i].mid_count; j++)
                if(nwz_series[i].mid[j] == model_id)
                    *series_index = i;
        if(*series_index == -1)
        {
            printf("Your device is not supported. Please contact developers.\n");
            return 3;
        }
    }
    cprintf_field("Model: ", "%s\n", *model_index == -1 ? "Unknown" : nwz_model[*model_index].name);
    cprintf_field("Series: ", "%s\n", *series_index == -1 ? "Unknown" : nwz_series[*series_index].name);
    return 0;
}

/* Read nvp node, retrun nonzero on error, update size to actual length. The
 * index is the raw node number sent to the device */
int read_nvp_node(int node_index, void *buffer, size_t *size)
{
    /* the returned data has a 4 byte header:
     * - byte 0/1 is the para_noise index, written as a 16bit big-endian number
     * - byte 2/3 is the node index, written as a 16-bit big-endian number
     *
     * NOTE: byte 0 is always 0 because the OF always picks small para_noise
     * indexes but I guess the actual encoding the one above */
    int xfer_size = *size + 4;
    uint8_t *xfer_buf = malloc(xfer_size);
    int ret = do_dnk_cmd(true, 0x23, 10, node_index, xfer_buf, &xfer_size);
    if(ret)
        return ret;
    if(xfer_size <= 4)
    {
        free(xfer_buf);
        cprintf(GREY, "Device didn't send any data\n");
        return 6;
    }
    if(get_big_endian16(xfer_buf + 2) != node_index)
    {
        free(xfer_buf);
        cprintf(GREY, "Device responded with invalid data\n");
        return 1;
    }
    *size = xfer_size - 4;
    /* unscramble and copy */
    for(int i = 4, idx = get_big_endian16(xfer_buf); i < xfer_size; i++, idx++)
        xfer_buf[i] ^= para_noise[idx % sizeof(para_noise)];
    memcpy(buffer, xfer_buf + 4, *size);
    free(xfer_buf);
    return 0;
}

/* read nvp node, retrun nonzero on error */
int write_nvp_node(int node_index, void *buffer, int size)
{
    /* the data buffer is prepended with a 4 byte header:
     * - byte 0/1 is the para_noise index, written as a 16bit big-endian number
     * - byte 2/3 is the node index, written as a 16-bit big-endian number */
    int xfer_size = size + 4;
    uint8_t *xfer_buf = malloc(xfer_size);
    /* scramble, always use index 0 for para_noise */
    set_big_endian16(xfer_buf, 0); /* para_noise index */
    set_big_endian16(xfer_buf + 2, node_index); /* node index */
    memcpy(xfer_buf + 4, buffer, size);
    for(int i = 4, idx = get_big_endian16(xfer_buf); i < xfer_size; i++, idx++)
        xfer_buf[i] ^= para_noise[idx % sizeof(para_noise)];
    int ret = do_dnk_cmd(false, 0x24, 10, node_index, xfer_buf, &xfer_size);
    if(ret)
        return ret;
    if(xfer_size - 4 != (int)size)
    {
        free(xfer_buf);
        cprintf(GREY, "Wrong transger size\n");
        return 7;
    }
    free(xfer_buf);
    return 0;
}

int get_dnk_nvp(int argc, char **argv)
{
    if(argc != 1 && argc != 2)
    {
        printf("You must specify a known nvp node or a full node specification:\n");
        printf("Node usage: <node>\n");
        printf("Node usage: <node> <size>\n");
        printf("Nodes:\n");
        for(unsigned i = 0; i < NWZ_NVP_COUNT; i++)
            printf("  %-6s%s\n", nwz_nvp[i].name, nwz_nvp[i].desc);
        printf("You can also specify a decimal or hexadecimal value directly\n");
        return 1;
    }
    int series_index, model_index;
    int ret = get_model_and_series(&model_index, &series_index);
    if(ret)
        return ret;
    size_t size = 0;
    /* maybe user specified an explicit size */
    if(argc == 2)
    {
        char *end;
        size = strtoul(argv[1], &end, 0);
        if(*end)
        {
            printf("Invalid user-specified size '%s'\n", argv[1]);
            return 5;
        }
    }
    /* find entry in NVP */
    const char *node_name = argv[0];
    const char *node_desc = NULL;
    int node_index = NWZ_NVP_INVALID;
    for(int i = 0; i < NWZ_NVP_COUNT; i++)
        if(strcmp(nwz_nvp[i].name, node_name) == 0)
        {
            if(nwz_series[series_index].nvp_index)
                node_index = (*nwz_series[series_index].nvp_index)[i];
            if(node_index == NWZ_NVP_INVALID)
            {
                printf("This device doesn't have node '%s'\n", node_name);
                return 5;
            }
            node_desc = nwz_nvp[i].desc;
            /* if not overriden, try to get size from database */
            if(size == 0)
                size = nwz_nvp[i].size;
        }
    /* if we can't find it, maybe check if it's a number */
    if(node_index == NWZ_NVP_INVALID)
    {
        char *end;
        node_index = strtol(node_name, &end, 0);
        if(*end)
            node_index = NWZ_NVP_INVALID; /* string is not a number */
    }
    if(node_index == NWZ_NVP_INVALID)
    {
        printf("I don't know about node '%s'\n", node_name);
        return 4;
    }
    /* if we don't have a size, take a big size to be sure */
    if(size == 0)
    {
        size = 4096;
        printf("Note: node size unknown, trying to read %u bytes\n", (unsigned)size);
    }
    if(g_debug)
        printf("Asking device for %u bytes\n", (unsigned)size);
    /* take the size in the database as a hint of the size, but the device could
     * return less data */
    uint8_t *buffer = malloc(size);
    ret = read_nvp_node(node_index, buffer, &size);
    if(ret != 0)
    {
        free(buffer);
        return ret;
    }
    cprintf(GREEN, "%s (node %d%s%s):\n", node_name, node_index,
        node_desc ? "," : "", node_desc ? node_desc : "");
    print_hex(buffer, size);

    free(buffer);
    return 0;
}

struct dpcc_prop_t
{
    char *user_name;
    char name[7];
    uint8_t cdb1;
    int size;
};

struct dpcc_prop_t dpcc_prop_list[] =
{
    { "dev_info", "DEVINFO", 0, 0x80 },
    /* there are more but they are very obscure */
};

#define NR_DPCC_PROPS   (sizeof(dpcc_prop_list) / sizeof(dpcc_prop_list[0]))

int do_dpcc_cmd(uint32_t cmd, struct dpcc_prop_t *prop, void *buffer, int *buffer_size)
{
    uint8_t cdb[12] = {0xfb, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    cdb[2] = cmd;
    if(cmd == 0)
    {
        strncpy((char *)(cdb + 3), prop->name, 7); // warning: erase cdb[10] !
        cdb[1] = prop->cdb1;
        if(prop->cdb1 & 1)
            cdb[10] = (*buffer_size + 15) / 16;
        else
            cdb[10] = *buffer_size;
    }

    uint8_t sense[32];
    int sense_size = 32;

    int ret = do_scsi(cdb, 12, DO_READ, sense, &sense_size, buffer, buffer_size);
    if(ret < 0)
        return ret;
    ret = do_sense_analysis(ret, sense, sense_size);
    if(ret)
        return ret;
    return 0;
}

int get_dpcc_prop(int argc, char **argv)
{
    if(argc != 1 && argc != 3)
    {
        printf("You must specify a known property name or a full property specification:\n");
        printf("Full usage: <prop code> <large> <size>\n");
        printf("Property usage: <prop>\n");
        printf("Properties:");
        for(unsigned i = 0; i < NR_DPCC_PROPS; i++)
            printf(" %s", dpcc_prop_list[i].user_name);
        printf("\n");
        return 1;
    }

    struct dpcc_prop_t prop;
    memset(&prop, 0, sizeof(prop));
    if(argc == 1)
    {
        for(unsigned i = 0; i < NR_DPCC_PROPS; i++)
            if(strcmp(dpcc_prop_list[i].user_name, argv[0]) == 0)
                prop = dpcc_prop_list[i];
        if(prop.user_name[0] == 0)
        {
            cprintf(GREY, "Unknown property '%s'\n", argv[0]);
            return 1;
        }
    }
    else
    {
        strncpy(prop.name, argv[0], 7);
        prop.cdb1 = strtoul(argv[1], NULL, 0);
        prop.size = strtoul(argv[2], NULL, 0);
    }

    char *buffer = malloc(prop.size);
    int buffer_size = prop.size;
    int ret = do_dpcc_cmd(0, &prop, buffer, &buffer_size);
    if(ret)
        return ret;
    if(buffer_size < prop.size)
        buffer[buffer_size] = 0;
    cprintf_field("Property: ", "%s\n", buffer);
    return 0;
}

struct user_timer_t
{
    uint16_t magic;
    uint8_t res[6];
    uint8_t year[2]; // bcd
    uint8_t month; // bcd
    uint8_t day; // bcd
    uint8_t hour; // bcd
    uint8_t min; // bcd
    uint8_t sec; // bcd
    uint8_t res2[17];
} __attribute__((packed));

int get_user_time(int argc, char **argv)
{
    (void) argc;
    (void )argv;

    void *buffer = malloc(32);
    int buffer_size = 32;
    int ret = do_dpcc_cmd(1, NULL, buffer, &buffer_size);
    if(ret)
        return ret;
    struct user_timer_t *time = buffer;
    cprintf_field("User Time: ", "%02x/%02x/%02x%02x %02x:%02x:%02x\n",
        time->day, time->month, time->year[0], time->year[1], time->hour,
        time->min, time->sec);
    return 0;
}

int get_dev_info(int argc, char **argv)
{
    (void) argc;
    (void )argv;
    uint8_t cdb[12] = {0xfc, 0, 0x20, 'd', 'b', 'm', 'n', 0, 0x80, 0, 0, 0};

    char *buffer = malloc(0x81);
    int buffer_size = 0x80;
    uint8_t sense[32];
    int sense_size = 32;

    int ret = do_scsi(cdb, 12, DO_READ, sense, &sense_size, buffer, &buffer_size);
    if(ret < 0)
        return ret;
    ret = do_sense_analysis(ret, sense, sense_size);
    if(ret)
        return ret;
    buffer[buffer_size] = 0;
    cprintf_field("Device Info:", "\n");
    print_hex(buffer, buffer_size);
    return 0;
}

int do_fw_upgrade(int argc, char **argv)
{
    (void) argc;
    (void )argv;
    /* older devices may have used subcommand 3 instead of 4, but this is not
     * supported by any device I have seen */
    uint8_t cdb[12] = {0xfc, 0, 0x04, 'd', 'b', 'm', 'n', 0, 0x80, 0, 0, 0};

    char *buffer = malloc(0x81);
    int buffer_size = 0x80;
    uint8_t sense[32];
    int sense_size = 32;

    int ret = do_scsi(cdb, 12, DO_READ, sense, &sense_size, buffer, &buffer_size);
    if(ret < 0)
        return ret;
    ret = do_sense_analysis(ret, sense, sense_size);
    if(ret)
        return ret;
    buffer[buffer_size] = 0;
    cprintf_field("Result:", "\n");
    print_hex(buffer, buffer_size);
    return 0;
}

static struct
{
    unsigned long dest;
    const char *name;
} g_dest_list[] =
{
    { 0, "J" },
    { 1, "U" },
    { 0x101, "U2" },
    { 0x201, "U3" },
    { 0x301, "CA" },
    { 2, "CEV" },
    { 0x102, "CE7" },
    { 3, "CEW" },
    { 0x103, "CEW2" },
    { 4, "CN" },
    { 5, "KR" },
    { 6, "E" },
    { 0x106, "MX" },
    { 0x206, "E2" },
    { 0x306, "MX3" },
    { 7, "TW" },
};

#define DEST_COUNT (sizeof(g_dest_list) / sizeof(g_dest_list[0]))

int do_dest(int argc, char **argv)
{
    /* it is possile to write any NVP node using the SCSI interface but only
     * give the user the possibility to write destination, because that's the
     * most useful one */
    if(argc != 1 && argc != 3)
    {
        printf("Usage: get\n");
        printf("Usage: set <dest> <sps>\n");
        printf("Destination (<dest>) can be either an integer or one of:\n");
        for(size_t i = 0; i < DEST_COUNT; i++)
            printf("  %s\n", g_dest_list[i].name);
        printf("Sound pressure (<sps>) can be be an integer, 'on' or 'off'\n");
        return 1;
    }
    /* get model/series */
    int model_index, series_index;
    int ret = get_model_and_series(&model_index, &series_index);
    int shp_index = NWZ_NVP_INVALID;
    if(nwz_series[series_index].nvp_index)
        shp_index = (*nwz_series[series_index].nvp_index)[NWZ_NVP_SHP];
    if(shp_index == NWZ_NVP_INVALID)
    {
        printf("This device doesn't have node 'shp'\n");
        return 5;
    }
    /* in all cases, we need to read shp */
    size_t size = nwz_nvp[NWZ_NVP_SHP].size;
    uint8_t *shp = malloc(size);
    ret = read_nvp_node(shp_index, shp, &size);
    if(ret != 0)
    {
        free(shp);
        return ret;
    }
    /* get */
    if(strcmp(argv[0], "get") == 0)
    {
        if(argc != 1)
        {
            printf("Too many arguments for get\n");
            free(shp);
            return 2;
        }
        const char *dst_name = "Unknown";
        unsigned long dst = get_little_endian32(shp);
        for(size_t i = 0; i < DEST_COUNT; i++)
            if(dst == g_dest_list[i].dest)
                dst_name = g_dest_list[i].name;
        printf("Destination: %s (%lx)\n", dst_name, dst);
        unsigned long sps = get_little_endian32(shp + 4);
        printf("Sound pressure: %lu (%s)\n", sps, sps == 0 ? "off" : "on");
        free(shp);
    }
    /* set */
    if(strcmp(argv[0], "set") == 0)
    {
        if(argc != 3)
        {
            printf("Not enough arguments for set\n");
            free(shp);
            return 2;
        }
        /* try to parse dest as integer */
        char *end;
        unsigned long dst = strtoul(argv[1], &end, 0);
        if(*end)
        {
            /* assume string */
            int index = -1;
            for(size_t i = 0; i < DEST_COUNT; i++)
                if(strcmp(argv[1], g_dest_list[i].name) == 0)
                    index = i;
            if(index == -1)
            {
                printf("Unknown destination '%s'\n", argv[1]);
                free(shp);
                return 3;
            }
            dst = g_dest_list[index].dest;
        }
        /* try to parse sps as integer */
        /* try to parse dest as integer */
        unsigned long sps = strtoul(argv[2], &end, 0);
        if(*end)
        {
            /* assume string */
            if(strcmp(argv[2], "on") == 0)
                sps = 1;
            else if(strcmp(argv[2], "off") == 0)
                sps = 0;
            else
            {
                printf("Unknown sound pressure setting '%s'\n", argv[2]);
                free(shp);
                return 3;
            }
        }
        set_little_endian32(shp, dst);
        set_little_endian32(shp + 4, sps);
        int ret = write_nvp_node(shp_index, shp, size);
        free(shp);
        if(ret != 0)
            printf("An error occured when writing node: %d\n", ret);
        else
            printf("Destination successfully changed.\nPlease RESET ALL SETTINGS on your device!\n");
        return ret;
    }
    return 0;
}

typedef int (*cmd_fn_t)(int argc, char **argv);

struct cmd_t
{
    const char *name;
    const char *desc;
    cmd_fn_t fn;
};

struct cmd_t cmd_list[] =
{
    { "get_dnk_prop", "Get DNK property", get_dnk_prop },
    { "get_dnk_nvp", "Get DNK NVP content", get_dnk_nvp },
    { "get_dpcc_prop", "Get DPCC property", get_dpcc_prop },
    { "get_user_time", "Get user time", get_user_time },
    { "get_dev_info", "Get device info", get_dev_info },
    { "do_fw_upgrade", "Do a firmware upgrade", do_fw_upgrade },
    { "dest_tool", "Get/Set destination and sound pressure regulation", do_dest },
};

#define NR_CMDS (sizeof(cmd_list) / sizeof(cmd_list[0]))

int process_cmd(const char *cmd, int argc, char **argv)
{
    for(unsigned i = 0; i < NR_CMDS; i++)
        if(strcmp(cmd_list[i].name, cmd) == 0)
            return cmd_list[i].fn(argc, argv);
    cprintf(GREY, "Unknown command '%s'\n", cmd);
    return 1;
}

static void usage(void)
{
    printf("Usage: scsitool [options] <dev> <command> [arguments]\n");
    printf("Options:\n");
    printf("  -o <prefix>          Set output prefix\n");
    printf("  -?/--help            Display this message\n");
    printf("  -d/--debug           Display debug messages\n");
    printf("  -c/--no-color        Disable color output\n");
    printf("  -s/--series <name>   Force series (disable auto-detection, use '?' for the list)\n");
    printf("Commands:\n");
    for(unsigned i = 0; i < NR_CMDS; i++)
        printf("  %s\t%s\n", cmd_list[i].name, cmd_list[i].desc);
    exit(1);
}

int main(int argc, char **argv)
{
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"no-color", no_argument, 0, 'c'},
            {"series", required_argument, 0, 's'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?dcfo:s:", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'c':
                enable_color(false);
                break;
            case 'd':
                g_debug = true;
                break;
            case '?':
                usage();
                break;
            case 'o':
                g_out_prefix = optarg;
                break;
            case 's':
                g_force_series = optarg;
                break;
            default:
                abort();
        }
    }

    if(argc - optind < 2)
    {
        usage();
        return 1;
    }

    int ret = 0;
    int flags = 0;
    if(g_debug)
        flags |= RB_SCSI_DEBUG;
    g_dev = rb_scsi_open(argv[optind], flags, NULL, NULL);
    if(g_dev == 0)
    {
        cprintf(GREY, "Cannot open device\n");
        ret = 1;
        goto Lend;
    }

    ret = process_cmd(argv[optind + 1], argc - optind - 2, argv + optind + 2);

    rb_scsi_close(g_dev);
Lend:
    color(OFF);

    return ret;
}

