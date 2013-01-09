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
#ifndef _WIN32
#include <scsi/scsi.h>
#endif
#include <scsi/sg_lib.h>
#include <scsi/sg_pt.h>
#include "misc.h"
#include "para_noise.h"

/* the windows port doesn't have scsi.h and GOOD */
#ifndef GOOD
#define GOOD                 0x00
#endif

bool g_debug = false;
bool g_force = false;
char *g_out_prefix = NULL;
int g_dev_fd = 0;

#define let_the_force_flow(x) do { if(!g_force) return x; } while(0)
#define continue_the_force(x) if(x) let_the_force_flow(x)

#define check_field(v_exp, v_have, str_ok, str_bad) \
    if((v_exp) != (v_have)) \
    { cprintf(RED, str_bad); let_the_force_flow(__LINE__); } \
    else { cprintf(RED, str_ok); }

#define errorf(...) do { cprintf(GREY, __VA_ARGS__); return __LINE__; } while(0)

#if 0
void *buffer_alloc(int sz)
{
#ifdef SG_LIB_MINGW
    unsigned psz = getpagesize();
#else
    unsigned psz = sysconf(_SC_PAGESIZE); /* was getpagesize() */
#endif
    void *buffer = malloc(sz + psz);
    return (void *)(((ptrdiff_t)(buffer + psz - 1)) & ~(psz - 1));
}
#else
void *buffer_alloc(int sz)
{
    return malloc(sz);
}
#endif

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
    char error[256];
    struct sg_pt_base *obj = construct_scsi_pt_obj();
    if(obj == NULL)
    {
        cprintf(GREY, "construct_scsi_pt_obj failed\n");
        return 1;
    }
    set_scsi_pt_cdb(obj, cdb, cdb_size);
    if(sense)
        set_scsi_pt_sense(obj, sense, *sense_size);
    if(flags & DO_READ)
        set_scsi_pt_data_in(obj, buffer, *buf_size);
    if(flags & DO_WRITE)
        set_scsi_pt_data_out(obj, buffer, *buf_size);
    int ret = do_scsi_pt(obj, g_dev_fd, 1, 0);
    switch(get_scsi_pt_result_category(obj))
    {
        case SCSI_PT_RESULT_SENSE:
        case SCSI_PT_RESULT_GOOD:
            ret = get_scsi_pt_status_response(obj);
            break;
        case SCSI_PT_RESULT_STATUS:
            cprintf(GREY, "Status error: %d (", get_scsi_pt_status_response(obj));
            sg_print_scsi_status(get_scsi_pt_status_response(obj));
            printf(")\n");
            break;
        case SCSI_PT_RESULT_TRANSPORT_ERR:
            cprintf(GREY, "Transport error: %s\n", get_scsi_pt_transport_err_str(obj, 256, error));
            ret = -2;
            break;
        case SCSI_PT_RESULT_OS_ERR:
            cprintf(GREY, "OS error: %s\n", get_scsi_pt_os_err_str(obj, 256, error));
            ret = -3;
            break;
        default:
            cprintf(GREY, "Unknown error\n");
            break;
    }

    if(sense)
        *sense_size = get_scsi_pt_sense_len(obj);
    if(flags & (DO_WRITE | DO_READ))
        *buf_size -= get_scsi_pt_resid(obj);
    
    destruct_scsi_pt_obj(obj);
    return ret;
}

int do_sense_analysis(int status, uint8_t *sense, int sense_size)
{
    if(status != GOOD || g_debug)
    {
        cprintf_field("Status:", " "); fflush(stdout);
        sg_print_scsi_status(status);
        cprintf_field("\nSense:", " "); fflush(stdout);
        sg_print_sense(NULL, sense, sense_size, 0);
    }
    if(status == GOOD)
        return 0;
    return status;
}

int do_dnk_cmd(uint32_t cmd, uint8_t sub_cmd, uint16_t arg, void *buffer, int *buffer_size)
{
    uint8_t cdb[12] = {0xdd, 0, 0, 0, 0, 0, 0, 0xbc, 0, 0, 0, 0};
    cdb[10] = cmd;
    cdb[11] = sub_cmd;
    cdb[8] = (*buffer_size) >> 8;
    cdb[9] = (*buffer_size) & 0xff;
    cdb[4] = (arg >> 8) & 0xff;
    cdb[5] = arg & 0xff;
    
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

#define DNK_EXACT_LENGTH    (1 << 0)
#define DNK_STRING          (1 << 1)
#define DNK_UINT32          (1 << 2)

struct dnk_prop_t
{
    char *name;
    uint8_t cmd;
    uint8_t subcmd;
    int size;
    unsigned flags;
};

struct dnk_prop_t dnk_prop_list[] =
{
    { "serial_num", 0x23, 1, 8, DNK_STRING},
    { "model_id", 0x23, 4, 4, DNK_EXACT_LENGTH | DNK_UINT32},
    { "product_id", 0x23, 6, 12, DNK_STRING},
    { "destination", 0x23, 8, 4, DNK_EXACT_LENGTH | DNK_UINT32},
    { "model_id2", 0x23, 9, 4, DNK_EXACT_LENGTH | DNK_UINT32},
    { "dev_info", 0x12, 0, 64, DNK_STRING},
};

#define NR_DNK_PROPS   (sizeof(dnk_prop_list) / sizeof(dnk_prop_list[0]))

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
        prop.cmd = strtoul(argv[0], NULL, 0);
        prop.subcmd = strtoul(argv[1], NULL, 0);
        prop.size = strtoul(argv[2], NULL, 0);
        prop.flags = strtoul(argv[3], NULL, 0);
    }

    char *buffer = buffer_alloc(prop.size + 1);
    int buffer_size = prop.size;
    int ret = do_dnk_cmd(prop.cmd, prop.subcmd, 0, buffer, &buffer_size);
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
    if(prop.flags & DNK_STRING)
        cprintf_field("Property: ", "%s\n", buffer);
    else if(prop.flags & DNK_UINT32)
        cprintf_field("Property: ", "0x%x\n", *(uint32_t *)buffer);
    else
    {
        cprintf(GREEN, "Property:\n");
        print_hex(buffer, buffer_size);
    }
    return 0;
}

#define NVP_EXACT_LENGTH    (1 << 0)
#define NVP_STRING          (1 << 1)
#define NVP_UINT32          (1 << 2)

struct nvp_prop_t
{
    const char *name;
    const char *desc;
    uint8_t node;
    int size;
    unsigned flags;
};

#define NVP_ENTRY(node, name,desc,size,flag) { name, desc, node, size, flag }

struct nvp_prop_t nvp_prop_list[] =
{
    NVP_ENTRY(1, "bti", "boot image", 262144, 0),
    NVP_ENTRY(2, "hdi", "hold image", 262144, 0),
    NVP_ENTRY(3, "cng", "aad key", 704, 0),
    NVP_ENTRY(4, "ser", "serial number", 16, NVP_STRING),
    NVP_ENTRY(5, "app", "application parameter", 4096, 0),
    NVP_ENTRY(6, "eri", "update error image", 262144, 0),
    NVP_ENTRY(7, "dcc", "secure clock", 20, 0),
    NVP_ENTRY(8, "mdl", "middleware parameter", 8, 0),
    NVP_ENTRY(9, "fup", "firmware update flag", 4, 0),
    NVP_ENTRY(10, "bok", "beep ok flag", 4, 0),
    NVP_ENTRY(11, "shp", "ship information", 32, 0),
    NVP_ENTRY(12, "dba", "aad icv", 160, 0),
    NVP_ENTRY(13, "dbv", "empr key", 520, 0),
    NVP_ENTRY(14, "tr0", "EKB 0", 16384, 0),
    NVP_ENTRY(15, "tr1", "EKB 1", 16384, 0),
    NVP_ENTRY(16, "mid", "model id", 64, 0),
    NVP_ENTRY(17, "tst", "test mode flag", 4, 0),
    NVP_ENTRY(18, "gty", "getty mode flag", 4, 0),
    NVP_ENTRY(19, "fui", "update image", 262144, 0),
    NVP_ENTRY(20, "lbi", "low battery image", 262144, 0),
    NVP_ENTRY(21, "dor", "key mode (debug/release)", 4, 0),
    NVP_ENTRY(22, "edw", "quick shutdown flag", 4, 0),
    NVP_ENTRY(23, "ubp", "u-boot password", 32, 0),
    NVP_ENTRY(24, "syi", "system information", 4, 0),
    NVP_ENTRY(25, "exm", "exception monitor mode", 4, 0),
    NVP_ENTRY(26, "pcd", "product code", 5, 0),
    NVP_ENTRY(27, "btc", "battery calibration", 4, 0),
    NVP_ENTRY(28, "rnd", "wmt key", 64, 0),
    NVP_ENTRY(29, "ufn", "update file name", 8, 0),
    NVP_ENTRY(30, "sdp", "sound driver parameter", 64, 0),
    NVP_ENTRY(31, "ncp", "noise cancel driver parameter", 64, 0),
    NVP_ENTRY(32, "kas", "key and signature", 64, 0),
    NVP_ENTRY(33, "sfi", "starfish id", 64, 0),
    NVP_ENTRY(34, "rtc", "rtc alarm", 16, 0),
    NVP_ENTRY(35, "bpr", "bluetooth address", 2048, 0),
    NVP_ENTRY(36, "e00", "EMPR  0", 1024, 0),
    NVP_ENTRY(37, "e01", "EMPR  1", 1024, 0),
    NVP_ENTRY(38, "e02", "EMPR  2", 1024, 0),
    NVP_ENTRY(39, "e03", "EMPR  3", 1024, 0),
    NVP_ENTRY(40, "e04", "EMPR  4", 1024, 0),
    NVP_ENTRY(41, "e05", "EMPR  5", 1024, 0),
    NVP_ENTRY(42, "e06", "EMPR  6", 1024, 0),
    NVP_ENTRY(43, "e07", "EMPR  7", 1024, 0),
    NVP_ENTRY(44, "e08", "EMPR  8", 1024, 0),
    NVP_ENTRY(45, "e09", "EMPR  9", 1024, 0),
    NVP_ENTRY(46, "e10", "EMPR 10", 1024, 0),
    NVP_ENTRY(47, "e11", "EMPR 11", 1024, 0),
    NVP_ENTRY(48, "e12", "EMPR 12", 1024, 0),
    NVP_ENTRY(49, "e13", "EMPR 13", 1024, 0),
    NVP_ENTRY(50, "e14", "EMPR 14", 1024, 0),
    NVP_ENTRY(51, "e15", "EMPR 15", 1024, 0),
    NVP_ENTRY(52, "e16", "EMPR 16", 1024, 0),
    NVP_ENTRY(53, "e17", "EMPR 17", 1024, 0),
    NVP_ENTRY(54, "e18", "EMPR 18", 1024, 0),
    NVP_ENTRY(55, "e19", "EMPR 19", 1024, 0),
    NVP_ENTRY(56, "e20", "EMPR 20", 1024, 0),
    NVP_ENTRY(57, "e21", "EMPR 21", 1024, 0),
    NVP_ENTRY(58, "e22", "EMPR 22", 1024, 0),
    NVP_ENTRY(59, "e23", "EMPR 23", 1024, 0),
    NVP_ENTRY(60, "e24", "EMPR 24", 1024, 0),
    NVP_ENTRY(61, "e25", "EMPR 25", 1024, 0),
    NVP_ENTRY(62, "e26", "EMPR 26", 1024, 0),
    NVP_ENTRY(63, "e27", "EMPR 27", 1024, 0),
    NVP_ENTRY(64, "e28", "EMPR 28", 1024, 0),
    NVP_ENTRY(65, "e29", "EMPR 29", 1024, 0),
    NVP_ENTRY(66, "e30", "EMPR 30", 1024, 0),
    NVP_ENTRY(67, "e31", "EMPR 31", 1024, 0),
    NVP_ENTRY(68, "clv", "color variation", 4, 0),
    NVP_ENTRY(69, "slp", "time out to sleep", 4, 0),
    NVP_ENTRY(70, "ipt", "disable iptable flag", 4, 0),
    NVP_ENTRY(71, "mtm", "marlin time", 64, 0),
    NVP_ENTRY(72, "mcr", "marlin crl", 16384, 0),
    NVP_ENTRY(73, "mdk", "marlin device key", 33024, 0),
    NVP_ENTRY(74, "muk", "marlin user key", 24576, 0),
    NVP_ENTRY(75, "pts", "wifi protected setup", 4, 0),
    NVP_ENTRY(76, "skt", "slacker time", 16, 0),
    NVP_ENTRY(77, "mac", "wifi mac address", 6, 0),
    NVP_ENTRY(78, "apd", "application debug mode flag", 4, 0),
    NVP_ENTRY(79, "blf", "browser log mode flag", 4, 0),
    NVP_ENTRY(80, "hld", "hold mode", 4, 0),
    NVP_ENTRY(81, "skd", "slacker id file", 8224, 0),
    NVP_ENTRY(82, "fmp", "fm parameter", 16, 0),
    NVP_ENTRY(83, "sps", "speaker ship info", 4, 0),
    NVP_ENTRY(84, "msc", "mass storage class mode", 4, 0),
    NVP_ENTRY(85, "vrt", "europe vol regulation flag", 4, 0),
    NVP_ENTRY(86, "psk", "bluetooth pskey", 512, 0),
    NVP_ENTRY(87, "bml", "btmw log mode flag", 4, 0),
    NVP_ENTRY(88, "bfd", "btmw factory scdb", 512, 0),
    NVP_ENTRY(89, "bfp", "btmw factory pair info", 512, 0),
};

#define NR_NVP_PROPS   (sizeof(nvp_prop_list) / sizeof(nvp_prop_list[0]))

int get_dnk_nvp(int argc, char **argv)
{
    if(argc != 1 && argc != 3)
    {
        printf("You must specify a known nvp node or a full node specification:\n");
        printf("Full usage: <node> <size> <flags>\n");
        printf("Node usage: <node>\n");
        printf("Nodes:\n");
        for(unsigned i = 0; i < NR_NVP_PROPS; i++)
            printf("  %s\t%s\n", nvp_prop_list[i].name, nvp_prop_list[i].desc);
        return 1;
    }

    struct nvp_prop_t prop;
    memset(&prop, 0, sizeof(prop));
    if(argc == 1)
    {
        for(unsigned i = 0; i < NR_NVP_PROPS; i++)
            if(strcmp(nvp_prop_list[i].name, argv[0]) == 0)
                prop = nvp_prop_list[i];
        if(prop.name == NULL)
        {
            cprintf(GREY, "Unknown node '%s'\n", argv[0]);
            return 1;
        }
    }
    else
    {
        prop.node = strtoul(argv[0], NULL, 0);
        prop.size = strtoul(argv[1], NULL, 0);
        prop.flags = strtoul(argv[2], NULL, 0);
    }

    int buffer_size = prop.size + 4;
    uint8_t *buffer = buffer_alloc(buffer_size + 1);
    int ret = do_dnk_cmd(0x23, 10, prop.node, buffer, &buffer_size);
    if(ret)
        return ret;
    if(buffer_size <= 4)
    {
        cprintf(GREY, "Device didn't send any data\n");
        return 1;
    }
    if(buffer[0] != 0 || buffer[2] != 0 || buffer[3] != prop.node)
    {
        cprintf(GREY, "Device responded with invalid data\n");
        return 1;
    }

    for(int i = 4, idx = buffer[1]; i < buffer_size; i++, idx++)
        buffer[i] ^= para_noise[idx];
    buffer[buffer_size] = 0;
    buffer += 4;
    buffer_size -= 4;
    if((prop.flags & DNK_EXACT_LENGTH) && buffer_size != prop.size)
    {
        cprintf(GREY, "Device didn't send the expected amount of data\n");
        return 2;
    }
    
    if(prop.flags & DNK_STRING)
        cprintf_field("Node: ", "%s\n", buffer);
    else if(prop.flags & DNK_UINT32)
        cprintf_field("Node: ", "0x%x\n", *(uint32_t *)buffer);
    else
    {
        cprintf(GREEN, "Node:\n");
        print_hex(buffer, buffer_size);
    }
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

    char *buffer = buffer_alloc(prop.size);
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

    void *buffer = buffer_alloc(32);
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

    char *buffer = buffer_alloc(0x81);
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
    uint8_t cdb[12] = {0xfc, 0, 0x04, 'd', 'b', 'm', 'n', 0, 0x80, 0, 0, 0};

    char *buffer = buffer_alloc(0x81);
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
    printf("  -o <prefix>\tSet output prefix\n");
    printf("  -f/--force\tForce to continue on errors\n");
    printf("  -?/--help\tDisplay this message\n");
    printf("  -d/--debug\tDisplay debug messages\n");
    printf("  -c/--no-color\tDisable color output\n");
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
            {"force", no_argument, 0, 'f'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?dcfo:", long_options, NULL);
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
            case 'f':
                g_force = true;
                break;
            case '?':
                usage();
                break;
            case 'o':
                g_out_prefix = optarg;
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
    g_dev_fd = scsi_pt_open_device(argv[optind], false, true);
    if(g_dev_fd < 0)
    {
        cprintf(GREY, "Cannot open device: %m\n");
        ret = 1;
        goto Lend;
    }

    ret = process_cmd(argv[optind + 1], argc - optind - 2, argv + optind + 2);

    scsi_pt_close_device(g_dev_fd);
Lend:
    color(OFF);

    return ret;
}

