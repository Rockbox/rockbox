/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#include <getopt.h>
#include <stdbool.h>
#include <ctype.h>
#include <iostream>
#include "hwstub.hpp"
#include "hwstub_uri.hpp"

using namespace hwstub;

void usage(void)
{
    fprintf(stderr, "usage: hwstub_ump [options] <addr> <size>\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  --help/-h       Display this help\n");
    fprintf(stderr, "  --verbose/-v    Verbose output\n");
    fprintf(stderr, "  --dev/-d <uri>  Device URI (see below)\n");
    //hwstub::usage_uri(stdout);
    exit(1);
}

int main(int argc, char **argv)
{
    const char *uri = hwstub::uri::default_uri().full_uri().c_str();
    bool verbose = false;

    // parse command line
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"dev", required_argument, 0, 'd'},
            {"verbose", no_argument, 0, 'v'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "hd:v", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'v':
                verbose = true;
                break;
            case 'h':
                usage();
                break;
            case 'd':
                uri = optarg;
                break;
            default:
                abort();
        }
    }

    if(optind + 2 != argc)
        usage();

    char *end;
    unsigned long addr = strtoul(argv[optind], &end, 0);
    if(*end)
    {
        fprintf(stderr, "Invalid dump address\n");
        return 2;
    }
    unsigned long size = strtoul(argv[optind + 1], &end, 0);
    if(*end)
    {
        fprintf(stderr, "Invalid dump size\n");
        return 2;
    }

    // create usb context
    std::string errstr;
    std::shared_ptr<context> hwctx = uri::create_context(uri::uri(uri), &errstr);
    if(!hwctx)
    {
        fprintf(stderr, "Cannot create context: %s\n", errstr.c_str());
        return 1;
    }
    if(verbose)
        hwctx->set_debug(std::cerr);
    std::vector<std::shared_ptr<hwstub::device>> list;
    hwstub::error ret = hwctx->get_device_list(list);
    if(ret != hwstub::error::SUCCESS)
    {
        fprintf(stderr, "Cannot get device list: %d\n", (int)ret);
        return 1;
    }
    if(list.size() == 0)
    {
        fprintf(stderr, "No hwstub device detected!\n");
        return 1;
    }
    /* open first device */
    std::shared_ptr<hwstub::handle> hwdev;
    ret = list[0]->open(hwdev);
    if(ret != hwstub::error::SUCCESS)
    {
        fprintf(stderr, "Cannot open device: %d\n", (int)ret);
        return 1;
    }

    /* load */

    uint8_t *buffer = new uint8_t[size];
    size_t out_size = size;
    ret = hwdev->read(addr, buffer, out_size, false);
    if(ret != hwstub::error::SUCCESS || out_size != size)
    {
        fprintf(stderr, "Dump failed: %s, %zu/%zu\n", error_string(ret).c_str(),
            out_size, size);
        goto Lerr;
    }

    fwrite(buffer, 1, size, stdout);

    return 0;

    Lerr:
    // display log if handled
    fprintf(stderr, "Device log:\n");
    do
    {
        char buffer[128];
        size_t size = sizeof(buffer) - 1;
        hwstub::error err = hwdev->get_log(buffer, size);
        if(err != hwstub::error::SUCCESS)
            break;
        buffer[size] = 0;
        fprintf(stderr, "%s", buffer);
    }while(1);
    return 1;
}
