/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 by Amaury Pouly
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
#include <cstdio>
#include <thread>
#include <chrono>
#include <cstring>
#include <iostream>
#include "hwstub.hpp"
#include "hwstub_usb.hpp"
#include "hwstub_uri.hpp"
#include <signal.h>
#include <getopt.h>

/* capture CTRL+C */
volatile sig_atomic_t g_exit_loop = 0;

void do_signal(int sig)
{
    g_exit_loop = 1;
}

std::shared_ptr<hwstub::context> g_ctx;

void print_error(hwstub::error err, bool nl = true)
{
    switch(err)
    {
        case hwstub::error::SUCCESS: printf("success"); break;
        case hwstub::error::ERROR: printf("error"); break;
        default: printf("unknown(%d)", (int)err); break;
    }
    if(nl)
        printf("\n");
}

const char *target_string(uint32_t id)
{
    switch(id)
    {
        case HWSTUB_TARGET_UNK: return "unknown";
        case HWSTUB_TARGET_STMP: return "stmp";
        case HWSTUB_TARGET_RK27: return "rk27";
        case HWSTUB_TARGET_PP: return "pp";
        case HWSTUB_TARGET_ATJ: return "atj";
        case HWSTUB_TARGET_JZ: return "jz";
        default: return "unknown";
    }
}

void print_dev_details(std::shared_ptr<hwstub::device> dev)
{
    std::shared_ptr<hwstub::handle> h;
    hwstub::error err = dev->open(h);
    if(err != hwstub::error::SUCCESS)
    {
        printf("  [cannot open dev: %s]\n", error_string(err).c_str());
        return;
    }
    /* version */
    struct hwstub_version_desc_t ver_desc;
    err = h->get_version_desc(ver_desc);
    if(err != hwstub::error::SUCCESS)
    {
        printf("  [cannot get version descriptor: %s]\n", error_string(err).c_str());
        return;
    }
    printf("  [version %d.%d.%d]\n", ver_desc.bMajor, ver_desc.bMinor, ver_desc.bRevision);
    /* target */
    struct hwstub_target_desc_t target_desc;
    err = h->get_target_desc(target_desc);
    if(err != hwstub::error::SUCCESS)
    {
        printf("  [cannot get target descriptor: %s]\n", error_string(err).c_str());
        return;
    }
    std::string name(target_desc.bName, sizeof(target_desc.bName));
    printf("  [target %s: %s]\n", target_string(target_desc.dID), name.c_str());
    /* layout */
    struct hwstub_layout_desc_t layout_desc;
    err = h->get_layout_desc(layout_desc);
    if(err != hwstub::error::SUCCESS)
    {
        printf("  [cannot get layout descriptor: %s]\n", error_string(err).c_str());
        return;
    }
    printf("  [code layout %#x bytes @ %#x]\n", layout_desc.dCodeSize, layout_desc.dCodeStart);
    printf("  [stack layout %#x bytes @ %#x]\n", layout_desc.dStackSize, layout_desc.dStackStart);
    printf("  [buffer layout %#x bytes @ %#x]\n", layout_desc.dBufferSize, layout_desc.dBufferStart);
}

void print_device(std::shared_ptr<hwstub::device> dev, bool arrived = true)
{
    hwstub::usb::device *udev = dynamic_cast< hwstub::usb::device* >(dev.get());
    if(arrived)
        printf("--> ");
    else
        printf("<-- ");
    if(udev)
    {
        libusb_device *uudev = udev->native_device();
        struct libusb_device_descriptor dev_desc;
        libusb_get_device_descriptor(uudev, &dev_desc);
        printf("USB device @ %d.%u: ID %04x:%04x\n", udev->get_bus_number(),
               udev->get_address(), dev_desc.idVendor, dev_desc.idProduct);
    }
    else
        printf("Unknown device\n");
    if(arrived)
        print_dev_details(dev);
}

void dev_changed(std::shared_ptr<hwstub::context> ctx, bool arrived, std::shared_ptr<hwstub::device> dev)
{
    print_device(dev, arrived);
}

void print_list()
{
    std::vector<std::shared_ptr<hwstub::device>> list;
    hwstub::error ret = g_ctx->get_device_list(list);
    if(ret != hwstub::error::SUCCESS)
    {
        printf("Cannot get device list: %s\n", error_string(ret).c_str());
        return;
    }
    for(auto d : list)
        print_device(d);
}

int usage()
{
    printf("usage: hwstub_test [options]\n");
    printf("  --help/-h           Display this help\n");
    printf("  --verbose/-v        Verbose output\n");
    printf("  --context/-c <uri>  Context URI (see below)\n");
    printf("\n");
    hwstub::uri::print_usage(stdout, true, false);
    return 1;
}

int main(int argc, char **argv)
{
    hwstub::uri::uri uri = hwstub::uri::default_uri();
    bool verbose = false;

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"verbose", no_argument, 0, 'v'},
            {"context", required_argument, 0, 'c'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "hvc:", long_options, NULL);
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
                return usage();
            case 'c':
                uri = hwstub::uri::uri(optarg);
                break;
            default:
                abort();
        }
    }

    if(optind != argc)
        return usage();

    /* intercept CTRL+C */
    signal(SIGINT, do_signal);

    std::string error;
    g_ctx = hwstub::uri::create_context(uri, &error);
    if(!g_ctx)
    {
        printf("Cannot create context: %s\n", error.c_str());
        return 1;
    }
    if(verbose)
        g_ctx->set_debug(std::cout);
    print_list();
    g_ctx->register_callback(dev_changed);
    g_ctx->start_polling();
    while(!g_exit_loop)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    printf("Shutting down...\n");
    g_ctx.reset(); /* will cleanup */

    return 0;
}

