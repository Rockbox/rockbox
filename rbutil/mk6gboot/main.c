/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 by Marcin Bukat
 *
 * code taken mostly from mkboot.c
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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

#include "mk6gboot.h"

/* Win32 compatibility */
#ifndef O_BINARY
#define O_BINARY 0
#endif

#define ERROR(format, ...) \
    do { \
        snprintf(errstr, errstrsize, "[ERR] "format, __VA_ARGS__); \
        goto error; \
    } while(0)

static int write_file(char *outfile, unsigned char* buf,
                            int bufsize, char* errstr, int errstrsize)
{
    int fd = open(outfile, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, 0666);
    if (fd < 0)
        ERROR("Could not open %s for writing", outfile);

    if (write(fd, buf, bufsize) != bufsize)
        ERROR("Could not write file %s", outfile);

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
        ERROR("Could not open %s for reading", infile);

    if (fstat(fd, &s) < 0)
        ERROR("Checking size of input file %s", infile);

    *bufsize = s.st_size;

    buf = malloc(*bufsize);
    if (buf == NULL)
        ERROR("Could not allocate memory for %s", infile);

    if (read(fd, buf, *bufsize) != *bufsize)
        ERROR("Could not read file %s", infile);

    return buf;

error:
    return NULL;
}

static void usage(void)
{
    printf("Usage:\n"
        "  mk6gboot --install-bl <bootloader.ipod> [--pid <pid>]\n"
        "  mk6gboot --uninstall-bl <bootloader.ipod> [--pid <pid>]\n"
        "  mk6gboot --mkdfu <bootloader.ipod> <outfile.dfu> [--uninstall]\n"
        "  mk6gboot --dfusend <infile.dfu> [--pid <pid>]\n"
        "  mk6gboot --dfuscan [--loop [seconds]] [--pid <pid>]\n"
        "  mk6gboot --dfureset [--pid <pid>]\n"
        "\n"
        "Commands:\n"
        "  --install-bl    installs bootloader.ipod into the device, if a\n"
        "                  RB bootloader already exists then it is updated.\n"
        "  --uninstall-bl  uninstalls bootloader.ipod from the device.\n"
        "\n"
        "  --mkdfu     creates outfile.dfu, it is a DFU image containing\n"
        "              an installer/uninstaller for bootloader.ipod.\n"
        "  --dfusend   sends a DFU image file to the device.\n"
        "  --dfuscan   scans for DFU USB devices and outputs the status.\n"
        "  --dfureset  resets DFU device USB bus.\n"
        "\n"
        "Options:\n"
        "  -p, --pid <PID>   use a specific PID (Product Id) USB device,\n"
        "                    if this option is ommited then it uses the\n"
        "                    first USB DFU device found.\n"
        "  -l, --loop [sec]  scan for a USB DFU device every 'sec' seconds,\n"
        "                    default period ('sec' ommited) is 3 seconds.\n"
        "  -u, --uninstall   create a file containing a DFU uninstaller.\n");
    exit(1);
}

int main(int argc, char* argv[])
{
    char *blfile = NULL;
    char *dfuoutfile = NULL;
    char *dfuinfile = NULL;
    int n_cmds = 0;
    int uninstall = 0;
    int scan = 0;
    int pid = 0;
    int reset = 0;
    int loop = 0;

    char errstr[200];
    unsigned char *dfubuf;
    int dfusize;

    while (--argc)
    {
        argv++;
        if (!strcmp(*argv, "--install-bl")) {
            if (!--argc) usage();
            blfile = *++argv;
            n_cmds++;
        }
        else if (!strcmp(*argv, "--uninstall-bl")) {
            if (!--argc) usage();
            blfile = *++argv;
            uninstall = 1;
            n_cmds++;
        }
        else if (!strcmp(*argv, "--mkdfu")) {
            if (!--argc) usage();
            blfile = *++argv;
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
                loop = 3; /* default period = 3 seconds */
            }
            else {
                if ((sscanf(*++argv, "%d", &loop) != 1) || !loop) usage();
                argc--;
            }
        }
        else if (!strcmp(*argv, "--uninstall") || !strcmp(*argv, "-u")) {
            uninstall = 1;
        }
        else if (!strcmp(*argv, "--debug")) {
            ipoddfu_debug(1);
        }
        else
            usage();
    }

    if (n_cmds != 1)
        usage();

    if (scan) {
        int state;
        if (loop) {
            int cnt = 0;
            while (1) {
                printf("-- DFU scan %d:\n", ++cnt);
                if (!ipoddfu_scan(pid, &state, reset, errstr, sizeof(errstr)))
                    printf("%s\n", errstr);
                else
                    printf("[INFO] DFU device state: %d\n", state);
                fflush(stdout);
                sleep(loop);
            }
        }
        else {
            if (!ipoddfu_scan(pid, &state, reset, errstr, sizeof(errstr)))
                goto error;
            printf("[INFO] DFU device state: %d\n", state);
            exit(0);
        }
    }

    if (blfile)
        dfubuf = mkdfu(blfile, uninstall, &dfusize, errstr, sizeof(errstr));
    else
        dfubuf = read_file(dfuinfile, &dfusize, errstr, sizeof(errstr));

    if (!dfubuf)
        goto error;

    if (dfuoutfile)
    {
        if (write_file(dfuoutfile, dfubuf, dfusize, errstr, sizeof(errstr))) {
            printf("[INFO] Created file %s, %d bytes\n", dfuoutfile, dfusize);
            exit(0);
        }
    }
    else
    {
        if (ipoddfu_send(pid, dfubuf, dfusize, errstr, sizeof(errstr))) {
            if (blfile)
                printf("[INFO] Bootloader file %s transmitted\n", blfile);
            else
                printf("[INFO] DFU file %s transmitted\n", dfuinfile);
            exit(0);
        }
    }

error:
    printf("%s\n", errstr);
    exit(1);
}
