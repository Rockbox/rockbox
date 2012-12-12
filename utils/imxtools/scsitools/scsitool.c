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
#include <scsi/scsi.h>
#include <scsi/sg_lib.h>
#include <scsi/sg_pt.h>
#include "misc.h"
#include "stmp_scsi.h"

bool g_debug = false;
bool g_force = false;
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

int stmp_inquiry(uint8_t *dev_type, char vendor[9], char product[17])
{
    unsigned char buffer[36];
    uint8_t cdb[10];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = 0x12;
    cdb[4] = sizeof(buffer);

    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int buf_sz = sizeof(buffer);
    int ret = do_scsi(cdb, sizeof(cdb), DO_READ, sense, &sense_size, buffer, &buf_sz);
    if(ret < 0)
        return ret;
    ret = do_sense_analysis(ret, sense, sense_size);
    if(ret)
        return ret;
    if(buf_sz != sizeof(buffer))
        return -1;
    *dev_type = buffer[0];
    memcpy(vendor, buffer + 8, 8);
    vendor[8] = 0;
    memcpy(product, buffer + 16, 16);
    product[16] = 0;
    return 0;
}

static int stmp_get_protocol_version(struct scsi_stmp_protocol_version_t *ver)
{
    uint8_t cdb[10];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_STMP_READ;
    cdb[1] = SCSI_STMP_CMD_GET_PROTOCOL_VERSION;
    
    uint8_t sense[32];
    int sense_size = sizeof(sense);

    int buf_sz = sizeof(struct scsi_stmp_protocol_version_t);
    int ret = do_scsi(cdb, sizeof(cdb), DO_READ, sense, &sense_size, ver, &buf_sz);
    if(ret < 0)
        return ret;
    ret = do_sense_analysis(ret, sense, sense_size);
    if(ret)
        return ret;
    if(buf_sz != sizeof(struct scsi_stmp_protocol_version_t))
        return -1;
    return 0;
}

static int do_work(void)
{
    cprintf(BLUE, "Information\n");
    
    uint8_t dev_type;
    char vendor[9];
    char product[17];
    int ret = stmp_inquiry(&dev_type, vendor, product);
    if(ret)
        errorf("Cannot get inquiry data: %d\n", ret);
    cprintf_field("  Vendor: ", "%s\n", vendor);
    cprintf_field("  Product: ", "%s\n", product);
    
    struct scsi_stmp_protocol_version_t ver;
    ret = stmp_get_protocol_version(&ver);
    if(ret)
        errorf("Cannot get protocol version: %d\n", ret);
    
    cprintf_field("  Protocol: ", "%x.%x\n", ver.major, ver.minor);

    return 0;
}

static void usage(void)
{
    printf("Usage: scsitool [options] <dev>\n");
    printf("Options:\n");
    printf("  -f/--force\tForce to continue on errors\n");
    printf("  -?/--help\tDisplay this message\n");
    printf("  -d/--debug\tDisplay debug messages\n");
    printf("  -c/--no-color\tDisable color output\n");
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

        int c = getopt_long(argc, argv, "?dcf", long_options, NULL);
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
            default:
                abort();
        }
    }

    if(argc - optind != 1)
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

    do_work();

    scsi_pt_close_device(g_dev_fd);
Lend:
    color(OFF);

    return ret;
}

