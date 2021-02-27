/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
 *
 * Directly adapted from jz4760_tools/usbboot.c,
 *   Copyright (C) 2015 by Amaury Pouly
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

#include <libusb.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define VR_GET_CPU_INFO     0
#define VR_SET_DATA_ADDRESS 1
#define VR_SET_DATA_LENGTH  2
#define VR_FLUSH_CACHES     3
#define VR_PROGRAM_START1   4
#define VR_PROGRAM_START2   5

/* Global variables */
bool g_verbose = false;
libusb_device_handle* g_usb_dev = NULL;
int g_vid = 0, g_pid = 0;

/* Utility functions */
void die(const char* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

void verbose(const char* msg, ...)
{
    if(!g_verbose)
        return;

    va_list ap;
    va_start(ap, msg);
    vprintf(msg, ap);
    printf("\n");
    va_end(ap);
}

void open_usb(void)
{
    if(g_usb_dev) {
        verbose("Closing USB device");
        libusb_close(g_usb_dev);
    }

    if(g_vid == 0 || g_pid == 0)
        die("Can't open USB device: vendor/product ID not specified");

    verbose("Opening USB device %04x:%04x", g_vid, g_pid);
    g_usb_dev = libusb_open_device_with_vid_pid(NULL, g_vid, g_pid);
    if(!g_usb_dev)
        die("Could not open USB device");

    int ret = libusb_claim_interface(g_usb_dev, 0);
    if(ret != 0) {
        libusb_close(g_usb_dev);
        die("Could not claim interface: %d", ret);
    }
}

void ensure_usb(void)
{
    if(!g_usb_dev)
        open_usb();
}

/* USB communication functions */
void jz_get_cpu_info(void)
{
    ensure_usb();
    verbose("Issue GET_CPU_INFO");

    uint8_t buf[9];
    int ret = libusb_control_transfer(g_usb_dev,
        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_GET_CPU_INFO, 0, 0, buf, 8, 1000);
    if(ret != 0)
        die("Can't get CPU info: %d", ret);

    buf[8] = 0;
    printf("CPU info: %s\n", buf);
}

void jz_upload(const char* filename, int length)
{
    if(length < 0)
        die("invalid upload length: %d", length);

    ensure_usb();
    verbose("Transfer %d bytes from device to host", length);

    void* data = malloc(length);
    int xfered = 0;
    int ret = libusb_bulk_transfer(g_usb_dev, LIBUSB_ENDPOINT_IN | 1,
                                   data, length, &xfered, 10000);
    if(ret != 0)
        die("Transfer failed: %d", ret);
    if(xfered != length)
        die("Transfer error: got %d bytes, expected %d", xfered, length);

    FILE* f = fopen(filename, "wb");
    if(f == NULL)
        die("Can't open file '%s' for writing", filename);

    if(fwrite(data, length, 1, f) != 1)
        die("Error writing transfered data to file");

    fclose(f);
    free(data);
}

void jz_download(const char* filename)
{
    FILE* f = fopen(filename, "rb");
    if(f == NULL)
        die("Can't open file '%s' for reading", filename);

    fseek(f, 0, SEEK_END);
    int length = ftell(f);
    fseek(f, 0, SEEK_SET);

    void* data = malloc(length);
    if(fread(data, length, 1, f) != 1)
        die("Error reading data from file");
    fclose(f);

    verbose("Transfer %d bytes from host to device", length);
    int xfered = 0;
    int ret = libusb_bulk_transfer(g_usb_dev, LIBUSB_ENDPOINT_OUT | 1,
                                   data, length, &xfered, 10000);
    if(ret != 0)
        die("Transfer failed: %d", ret);
    if(xfered != length)
        die("Transfer error: %d bytes recieved, expected %d", xfered, length);

    free(data);
}

#define jz_vendor_out_func(name, type, fmt) \
    void name(unsigned long param) { \
        ensure_usb(); \
        verbose("Issue " #type fmt, param); \
        int ret = libusb_control_transfer(g_usb_dev, \
            LIBUSB_ENDPOINT_OUT|LIBUSB_REQUEST_TYPE_VENDOR|LIBUSB_RECIPIENT_DEVICE, \
            VR_##type, param >> 16, param & 0xffff, NULL, 0, 1000); \
        if(ret != 0) \
            die("Request " #type " failed: %d", ret); \
    }

jz_vendor_out_func(jz_set_data_address, SET_DATA_ADDRESS, " 0x%08lx")
jz_vendor_out_func(jz_set_data_length, SET_DATA_LENGTH, " 0x%0lx")
jz_vendor_out_func(_jz_flush_caches, FLUSH_CACHES, "")
jz_vendor_out_func(jz_program_start1, PROGRAM_START1, " 0x%08lx")
jz_vendor_out_func(jz_program_start2, PROGRAM_START2, " 0x%08lx")
#define jz_flush_caches() _jz_flush_caches(0)

/* Default settings */
struct cpu_profile {
    const char* name;
    int vid, pid;
    unsigned long s1_load_addr, s1_exec_addr;
    unsigned long s2_load_addr, s2_exec_addr;
};

static const struct cpu_profile cpu_profiles[] = {
    {"x1000",
     0xa108, 0x1000,
     0xf4001000, 0xf4001800,
     0x80004000, 0x80004000},
    {"jz4760",
     0x601a, 0x4760,
     0x80000000, 0x80000000,
     0x80000000, 0x80000000},
    {NULL}
};

/* Simple "download and run" functions for dev purposes */
unsigned long s1_load_addr = 0, s1_exec_addr = 0;
unsigned long s2_load_addr = 0, s2_exec_addr = 0;

void apply_cpu_profile(const char* name)
{
    const struct cpu_profile* p = &cpu_profiles[0];
    for(p = &cpu_profiles[0]; p->name != NULL; ++p) {
        if(strcmp(p->name, name) != 0)
            continue;

        g_vid = p->vid;
        g_pid = p->pid;
        s1_load_addr = p->s1_load_addr;
        s1_exec_addr = p->s1_exec_addr;
        s2_load_addr = p->s2_load_addr;
        s2_exec_addr = p->s2_exec_addr;
        return;
    }

    die("CPU '%s' not known", name);
}

void run_stage1(const char* filename)
{
    if(s1_load_addr == 0 || s1_exec_addr == 0)
        die("No stage1 binary settings -- did you specify --cpu?");
    jz_set_data_address(s1_load_addr);
    jz_download(filename);
    jz_program_start1(s1_exec_addr);
}

void run_stage2(const char* filename)
{
    if(s2_load_addr == 0 || s2_exec_addr == 0)
        die("No stage2 binary settings -- did you specify --cpu?");
    jz_set_data_address(s2_load_addr);
    jz_download(filename);
    jz_flush_caches();
    jz_program_start2(s2_exec_addr);
}

/* Main functions */
void usage()
{
    printf("\
Usage: usbboot [options]\n\
\n\
Basic options:\n\
  --cpu <cpu>        Select device CPU type\n\
  --stage1 <file>    Download and execute stage1 binary\n\
  --stage2 <file>    Download and execute stage2 binary\n\
\n\
Advanced options:\n\
  --vid <vid>        Specify USB vendor ID\n\
  --pid <pid>        Specify USB product ID\n\
  --cpuinfo          Ask device for CPU info\n\
  --addr <addr>      Set data address\n\
  --length <len>     Set data length\n\
  --upload <file>    Transfer data from device (needs prior --length)\n\
  --download <file>  Transfer data to device\n\
  --start1 <addr>    Execute stage1 code at address\n\
  --start2 <addr>    Execute stage2 code at address\n\
  --flush-caches     Flush device CPU caches\n\
  --renumerate       Close and re-open the USB device\n\
  --wait <time>      Wait for <time> seconds\n\
  -v, --verbose      Be verbose\n\
\n\
Known CPU types and default stage1/stage2 binary settings:\n");
    const struct cpu_profile* p = &cpu_profiles[0];
    for(p = &cpu_profiles[0]; p->name != NULL; ++p) {
        printf("* %s\n", p->name);
        printf("  - USB ID: %04x:%04x\n", p->vid, p->pid);
        printf("  - Stage1: load %#08lx, exec %#08lx\n",
               p->s1_load_addr, p->s1_exec_addr);
        printf("  - Stage2: load %#08lx, exec %#08lx\n",
               p->s2_load_addr, p->s2_exec_addr);
    }

    exit(1);
}

void cleanup()
{
    if(g_usb_dev == NULL)
        libusb_close(g_usb_dev);
    libusb_exit(NULL);
}

int main(int argc, char* argv[])
{
    if(argc <= 1)
        usage();

    libusb_init(NULL);
    atexit(cleanup);

    enum {
        OPT_VID = 0x100, OPT_PID,
        OPT_CPUINFO,
        OPT_START1, OPT_START2, OPT_FLUSH_CACHES,
        OPT_RENUMERATE, OPT_WAIT,
    };

    static const struct option long_options[] = {
        {"cpu", required_argument, 0, 'c'},
        {"stage1", required_argument, 0, '1'},
        {"stage2", required_argument, 0, '2'},
        {"vid", required_argument, 0, OPT_VID},
        {"pid", required_argument, 0, OPT_PID},
        {"cpuinfo", no_argument, 0, OPT_CPUINFO},
        {"addr", required_argument, 0, 'a'},
        {"length", required_argument, 0, 'l'},
        {"upload", required_argument, 0, 'u'},
        {"download", required_argument, 0, 'd'},
        {"start1", required_argument, 0, OPT_START1},
        {"start2", required_argument, 0, OPT_START2},
        {"flush-caches", no_argument, 0, OPT_FLUSH_CACHES},
        {"renumerate", no_argument, 0, OPT_RENUMERATE},
        {"wait", required_argument, 0, OPT_WAIT},
        {"help", no_argument, 0, 'h'},
        {"verbose", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    int data_length = -1;
    while((opt = getopt_long(argc, argv, "hvc:1:2:a:l:u:d:", long_options, NULL)) != -1) {
        unsigned long param;
        char* end;
        switch(opt) {
        case OPT_VID:
        case OPT_PID:
        case 'a':
        case 'l':
        case OPT_START1:
        case OPT_START2:
        case OPT_WAIT:
            param = strtoul(optarg, &end, 0);
            if(*end)
                die("Invalid argument '%s'", optarg);
            break;
        default:
            break;
        }

        switch(opt) {
        case 'h':
            usage();
            break;
        case 'v':
            g_verbose = true;
            break;
        case 'c':
            apply_cpu_profile(optarg);
            break;
        case '1':
            run_stage1(optarg);
            break;
        case '2':
            run_stage2(optarg);
            break;
        case OPT_VID:
            g_vid = param & 0xffff;
            break;
        case OPT_PID:
            g_pid = param & 0xffff;
            break;
        case OPT_CPUINFO:
            jz_get_cpu_info();
            break;
        case 'a':
            jz_set_data_address(param);
            break;
        case 'l':
            data_length = param;
            jz_set_data_length(param);
            break;
        case 'u':
            if(data_length < 0)
                die("Need to specify --length before --upload");
            jz_upload(optarg, data_length);
            break;
        case 'd':
            jz_download(optarg);
            break;
        case OPT_START1:
            jz_program_start1(param);
            break;
        case OPT_START2:
            jz_program_start2(param);
            break;
        case OPT_FLUSH_CACHES:
            jz_flush_caches();
            break;
        case OPT_RENUMERATE:
            open_usb();
            break;
        case OPT_WAIT:
            verbose("Wait %lu seconds", param);
            sleep(param);
            break;
        default:
            /* should only happen due to a bug */
            die("Bad option");
            break;
        }
    }

    if(optind != argc)
        die("Extra arguments on command line");

    return 0;
}
