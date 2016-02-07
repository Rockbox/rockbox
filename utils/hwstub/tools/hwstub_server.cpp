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
#include <cstdio>
#include <thread>
#include <chrono>
#include <cstring>
#include <iostream>
#include "hwstub.hpp"
#include "hwstub_usb.hpp"
#include "hwstub_uri.hpp"
#include "hwstub_net.hpp"
#include <signal.h>
#include <getopt.h>

/* capture CTRL+C */
volatile sig_atomic_t g_exit_loop = 0;

void do_signal(int sig)
{
    g_exit_loop = 1;
}

std::shared_ptr<hwstub::context> g_ctx;
std::shared_ptr<hwstub::net::server> g_srv;

int usage()
{
    printf("usage: hwstub_server [options]\n");
    printf("  --help/-h           Display this help\n");
    printf("  --verbose/-v        Verbose output\n");
    printf("  --context/-c <uri>  Context URI (see below)\n");
    printf("  --server/-s <uri>   Server URI (see below)\n");
    printf("\n");
    hwstub::uri::print_usage(stdout, true, true);
    return 1;
}

int main(int argc, char **argv)
{
    hwstub::uri::uri ctx_uri = hwstub::uri::default_uri();
    hwstub::uri::uri srv_uri = hwstub::uri::default_server_uri();
    bool verbose = false;

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"verbose", no_argument, 0, 'v'},
            {"context", required_argument, 0, 'c'},
            {"server", required_argument, 0, 's'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "hvc:s:", long_options, NULL);
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
                ctx_uri = hwstub::uri::uri(optarg);
                break;
            case 's':
                srv_uri = hwstub::uri::uri(optarg);
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
    g_ctx = hwstub::uri::create_context(ctx_uri, &error);
    if(!g_ctx)
    {
        printf("Cannot create context: %s\n", error.c_str());
        return 1;
    }
    g_ctx->start_polling();

    g_srv = hwstub::uri::create_server(g_ctx, srv_uri, &error);
    if(!g_srv)
    {
        printf("Cannot create server: %s\n", error.c_str());
        return 1;
    }
    if(verbose)
        g_srv->set_debug(std::cout);

    while(!g_exit_loop)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    printf("Shutting down...\n");
    g_srv.reset(); /* will cleanup */
    g_ctx.reset(); /* will cleanup */

    return 0;
}
