/*
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * $Id$
 *
 * Copyright (c) 2009, Dave Chapman
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "beastpatcher.h"
#include "mtp_common.h"

#if defined(__WIN32__) || defined(_WIN32)
#include <windows.h>
#include "../MTP_DLL/MTP_DLL.h"
#endif

#ifdef WITH_BOOTOBJS
#define VERSION "1.0 with v1 bootloader"
#else
#define VERSION "1.0"
#endif

enum actions {
    NONE,
    INSTALL,
    DUALBOOT,
    SEND,
    HELP
};

static void print_usage(void)
{
    fprintf(stderr,"Usage: beastpatcher [action]\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"Where [action] is one of the following options:\n");
#ifdef WITH_BOOTOBJS
    fprintf(stderr,"  -i,   --install           [bootloader.bin]\n");
    fprintf(stderr,"  -d,   --dual-boot         <nk.bin> [bootloader.bin]\n");
#else
    fprintf(stderr,"  -i,   --install           <bootloader.bin>\n");
    fprintf(stderr,"  -d    --dual-boot         <nk.bin> <bootloader.bin>\n");
#endif
    fprintf(stderr,"  -s,   --send              <nk.bin>\n");
    fprintf(stderr,"  -h,   --help\n");
    fprintf(stderr,"\n");
#ifdef WITH_BOOTOBJS
    fprintf(stderr,"With bootloader file omitted the embedded one will be used.\n");
    fprintf(stderr,"\n");
#endif
}


int main(int argc, char* argv[])
{
    int res = 0;
    char yesno[4];
    int i;
    char* bootloader = NULL;
    char* firmware = NULL;
#ifdef WITH_BOOTOBJS
    enum actions action = INSTALL;
    int interactive = 1;
#else
    enum actions action = NONE;
    int interactive = 0;
#endif

    fprintf(stderr,"beastpatcher v" VERSION " - (C) 2009 by the Rockbox developers\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
    if(argc > 1) {
        interactive = 0;
    }
#if defined(__WIN32__) || defined(_WIN32)
    if(mtp_wmp_version() < 11) {
        fprintf(stderr, "beastpacher requires at least Windows Media Player 11 to run!\n");
        fprintf(stderr, "Please update your installation of Windows Media Player.\n");
        return -1;
    }
#endif

    i = 1;
    while(i < argc) {
        if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            action = HELP;
        }
        else if(strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--install") == 0) {
            action = INSTALL;
            if(((i + 1) < argc) && argv[i + 1][0] != '-') {
                bootloader = argv[++i];
            }
#ifndef WITH_BOOTOBJS
            else {
                action = NONE;
            }
#endif
        }
        else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dual-boot") == 0) {
            action = DUALBOOT;
            if(((i + 1) < argc) && argv[i + 1][0] != '-') {
                firmware = argv[++i];
#ifndef WITH_BOOTOBJS
                if(((i + 1) < argc) && argv[i + 1][0] != '-') {
                    bootloader = argv[++i];
                }
#endif
            }
            else {
                action = NONE;
            }
        }
        else if(((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--send") == 0)
            && (i + 1) < argc)) {
            action = SEND;
            firmware = argv[++i];
        }
        i++;
    }

    if(action == NONE) {
        print_usage();
        res = -1;
    }
    else if(action == HELP) {
        print_usage();
        res = 0;
    }
    else if(action == SEND) {
        res = sendfirm(firmware);
    }
    else if(action == DUALBOOT) {
        res = beastpatcher(bootloader, firmware, interactive);
    }
    else if(action == INSTALL) {
        res = beastpatcher(bootloader, NULL, interactive);
        /* don't ask for enter if started with command line arguments */
        if(interactive) {
            printf("\nPress ENTER to exit beastpatcher: ");
            fgets(yesno,4,stdin);
        }
    }
    return res;
}

