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

#define VERSION "1.0 with v1 bootloader"


static void print_usage(void)
{
    fprintf(stderr,"Usage: beastpatcher [action]\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"Where [action] is one of the following options:\n");
    fprintf(stderr,"  -i,   --install (default)\n");
    fprintf(stderr,"  -h,   --help\n");
    fprintf(stderr,"  -s,   --send              nk.bin\n");
    fprintf(stderr,"\n");
}


int main(int argc, char* argv[])
{
    int res;
    char yesno[4];
    struct mtp_info_t mtp_info;

    fprintf(stderr,"beastpatcher v" VERSION " - (C) 2009 by the Rockbox developers\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

    if(argc == 1 || strcmp(argv[1],"-i")==0 || strcmp(argv[1],"--install")==0) {
        res = beastpatcher();
        /* don't ask for enter if started with command line arguments */
        if(argc == 1) {
            printf("\nPress ENTER to exit beastpatcher: ");
            fgets(yesno,4,stdin);
        }
    }
    else if((argc > 2) && ((strcmp(argv[1],"-s")==0) || (strcmp(argv[1],"--send")==0))) {
        mtp_init(&mtp_info);
        mtp_scan(&mtp_info);
        res = mtp_send_file(&mtp_info, argv[2]);
        mtp_finished(&mtp_info);
    }
    else {
        print_usage();
        res = -1;
    }

    return res;
}


