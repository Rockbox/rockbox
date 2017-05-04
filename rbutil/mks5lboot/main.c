/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2015 by Cástor Muñoz
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mks5lboot.h"

/* Win32 compatibility */
#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#else
#include <time.h>
static void sleep_ms(unsigned int ms)
{
   struct timespec req;
   req.tv_sec = ms / 1000;
   req.tv_nsec = (ms % 1000) * 1000000;
   nanosleep(&req, NULL);
}
#endif

#define DEFAULT_LOOP_PERIOD 1  /* seconds */

#define _ERR(format, ...) \
    do { \
        snprintf(errstr, errstrsize, "[ERR] "format, __VA_ARGS__); \
        goto error; \
    } while(0)

static int write_file(char *outfile, unsigned char* buf,
                            int bufsize, char* errstr, int errstrsize)
{
    int fd = open(outfile, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, 0666);
    if (fd < 0)
        _ERR("Could not open %s for writing", outfile);

    if (write(fd, buf, bufsize) != bufsize)
        _ERR("Could not write file %s", outfile);

    return 1;

error:
    return 0;
}

static unsigned char *read_file(char *infile, int *bufsize,
                                    char* errstr, int errstrsize)
{
    unsigned char *buf;
    int fd;
    struct stat s;

    fd = open(infile, O_RDONLY|O_BINARY);
    if (fd < 0)
        _ERR("Could not open %s for reading", infile);

    if (fstat(fd, &s) < 0)
        _ERR("Checking size of input file %s", infile);

    *bufsize = s.st_size;

    buf = malloc(*bufsize);
    if (buf == NULL)
        _ERR("Could not allocate memory for %s", infile);

    if (read(fd, buf, *bufsize) != *bufsize)
        _ERR("Could not read file %s", infile);

    return buf;

error:
    return NULL;
}

static void usage(void)
{
    fprintf(stderr,
    "Usage:\n"
    "  mks5lboot --bl-inst <bootloader.ipod> [-p <pid>] [--single]\n"
    "            --bl-uninst <platform> [-p <pid>]\n"
    "            --dfuscan [--loop [<sec>]] [-p <pid>]\n"
    "            --dfusend <infile.dfu> [-p <pid>]\n"
    "            --dfureset [--loop [<sec>]] [-p <pid>]\n"
    "            --mkdfu-inst <bootloader.ipod> <outfile.dfu> [--single]\n"
    "            --mkdfu-uninst <platform> <outfile.dfu>\n"
    "            --mkdfu-raw <filename.bin> <outfile.dfu>\n"
    "\n"
    "Commands:\n"
    "  --bl-inst    Install file <bootloader.ipod> into an iPod device\n"
    "               (same as --mkdfu-inst and --dfusend).\n"
    "  --bl-uninst  Remove a bootloader from an iPod device (same as\n"
    "               --mkdfu-uninst and --dfusend).\n"
    "\n"
    "  --dfuscan    scan for DFU USB devices and outputs the status.\n"
    "  --dfusend    send DFU image <infile.dfu> to the device.\n"
    "  --dfureset   reset DFU USB device bus.\n"
    "\n"
    "  --mkdfu-inst    Build a DFU image containing an installer for\n"
    "                  <bootloader.ipod>, save it as <outfile.dfu>.\n"
    "  --mkdfu-uninst  Build a DFU image containing an uninstaler for\n"
    "                  <platform> devices, save it as <outfile.dfu>.\n"
    "  --mkdfu-raw     Build a DFU image containing raw executable\n"
    "                  code, save it ass <outfile.dfu>. <infile.bin>\n"
    "                  is the code you want to run, it is loaded at\n"
    "                  address 0x%08x and executed.\n"
    "\n"
    "  <bootloader.ipod> is the rockbox bootloader that you want to\n"
    "  install (previously scrambled with tools/scramble utility).\n"
    "\n"
    "  <platform> is the name of the platform (type of device) for\n"
    "  which the DFU uninstaller will be built. Currently supported\n"
    "  platform names are:\n"
    "    ipod6g:  iPod Classic 6G\n"
    "\n"
    "Options:\n"
    "  -p, --pid <pid>   Use a specific <pid> (Product Id) USB device,\n"
    "                    if this option is ommited then it uses the\n"
    "                    first USB DFU device found.\n"
    "  -l, --loop <sec>  Run the command every <sec> seconds, default\n"
    "                    period (<sec> ommited) is %d seconds.\n"
    "  -S, --single      Be careful using this option. The bootloader\n"
    "                    is installed for single boot, the original\n"
    "                    Apple NOR boot is destroyed (if it exists),\n"
    "                    and only Rockbox can be used.\n"
    , DFU_LOADADDR + BIN_OFFSET
    , DEFAULT_LOOP_PERIOD);

    exit(1);
}

int main(int argc, char* argv[])
{
    char *dfuoutfile = NULL;
    char *dfuinfile = NULL;
    char *dfu_arg = NULL;
    int dfu_type = DFU_NONE;
    int n_cmds = 0;
    int scan = 0;
    int pid = 0;
    int reset = 0;
    int loop = 0;
    int single = 0;
    char errstr[200];
    unsigned char *dfubuf;
    int dfusize;

    fprintf(stderr,
#if defined(WIN32) && defined(USE_LIBUSBAPI)
    "mks5lboot Version " VERSION " (libusb)\n"
#else
    "mks5lboot Version " VERSION "\n"
#endif
    "This is free software; see the source for copying conditions.  There is NO\n"
    "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
    "\n");
    fflush(stderr);

    while (--argc)
    {
        argv++;
        if (!memcmp(*argv, "--bl", 4)) {
            if      (!strcmp(*argv+4, "-inst"))   dfu_type = DFU_INST;
            else if (!strcmp(*argv+4, "-uninst")) dfu_type = DFU_UNINST;
            else usage();
            if (!--argc) usage();
            dfu_arg = *++argv;
            n_cmds++;
        }
        else if (!memcmp(*argv, "--mkdfu", 7)) {
            if      (!strcmp(*argv+7, "-inst"))   dfu_type = DFU_INST;
            else if (!strcmp(*argv+7, "-uninst")) dfu_type = DFU_UNINST;
            else if (!strcmp(*argv+7, "-raw"))    dfu_type = DFU_RAW;
            else usage();
            if (!--argc) usage();
            dfu_arg = *++argv;
            if (!--argc) usage();
            dfuoutfile = *++argv;
            n_cmds++;
        }
        else if (!strcmp(*argv, "--dfusend")) {
            if (!--argc) usage();
            dfuinfile = *++argv;
            n_cmds++;
        }
        else if (!strcmp(*argv, "--dfuscan")) {
            scan = 1;
            n_cmds++;
        }
        else if (!strcmp(*argv, "--dfureset")) {
            scan = 1;
            reset = 1;
            n_cmds++;
        }
        else if (!strcmp(*argv, "--pid") || !strcmp(*argv, "-p")) {
            if (!--argc) usage();
            if (sscanf(*++argv, "%x", &pid) != 1) usage();
        }
        else if (!strcmp(*argv, "--loop") || !strcmp (*argv, "-l")) {
            if (!(argc-1) || *(argv+1)[0] == '-') {
                loop = DEFAULT_LOOP_PERIOD;
            }
            else {
                if ((sscanf(*++argv, "%d", &loop) != 1) || !loop) usage();
                argc--;
            }
        }
        else if (!strcmp(*argv, "--single") || !strcmp(*argv, "-S")) {
            single = 1;
        }
        else if (!strcmp(*argv, "--debug")) {
            ipoddfu_debug(1);
        }
        else
            usage();
    }

    if (n_cmds != 1)
        usage();

    if ((dfu_type == DFU_INST) && single)
        dfu_type = DFU_INST_SINGLE;

    if (scan) {
        int cnt = 0;
        while (1) {
            int state, res;
            if (loop) printf("[%d] ", cnt);
            else      printf("[INFO] ");
            printf("DFU %s:\n", reset ? "reset":"scan");
            res = ipoddfu_scan(pid, &state, reset, errstr, sizeof(errstr));
            if (res == 0)
                printf("%s\n", errstr);
            else
                printf("[INFO] DFU device state: %d\n", state);
            if (!loop)
                exit(!res);
            fflush(stdout);
            sleep_ms(loop*1000);
            cnt += loop;
        }
    }

    if (dfuinfile)
        dfubuf = read_file(dfuinfile, &dfusize, errstr, sizeof(errstr));
    else
        dfubuf = mkdfu(dfu_type, dfu_arg, &dfusize, errstr, sizeof(errstr));

    if (!dfubuf)
        goto error;

    if (dfuoutfile) {
        if (write_file(dfuoutfile, dfubuf, dfusize, errstr, sizeof(errstr))) {
            printf("[INFO] Created file %s (%d bytes)\n", dfuoutfile, dfusize);
            exit(0);
        }
    }
    else {
        if (ipoddfu_send(pid, dfubuf, dfusize, errstr, sizeof(errstr))) {
            printf("[INFO] DFU image sent successfully (%d bytes)\n", dfusize);
            exit(0);
        }
    }

error:
    printf("%s\n", errstr);
    exit(1);
}
