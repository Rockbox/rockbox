/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Amaury Pouly
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
#include "soc_desc.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <cstring>

using namespace soc_desc;

void print_context(const error_context_t& ctx)
{
    for(size_t j = 0; j < ctx.count(); j++)
    {
        error_t e = ctx.get(j);
        switch(e.level())
        {
            case error_t::WARNING: printf("[WARN]"); break;
            case error_t::FATAL: printf("[FATAL]"); break;
            default: printf("[UNK]"); break;
        }
        if(e.location().size() != 0)
            printf(" %s:", e.location().c_str());
        printf(" %s\n", e.message().c_str());
    }
}

int do_read(int argc, char **argv)
{
    for(int i = 0; i < argc; i++)
    {
        error_context_t ctx;
        soc_t soc;
        bool ret = parse_xml(argv[i], soc, ctx);
        if(ctx.count() != 0)
            printf("In file %s:\n", argv[i]);
        print_context(ctx);
        if(!ret)
        {
            printf("cannot parse file '%s'\n", argv[i]);
            continue;
        }
    }
    return 0;
}

int do_eval(int argc, char **argv)
{
    std::map< std::string, soc_word_t > map;
    for(int i = 0; i < argc; i++)
    {
        std::string formula(argv[i]);
        soc_word_t result;
        if(strcmp(argv[i], "--var") == 0)
        {
            if(i + 1 >= argc)
                break;
            i++;
            std::string str(argv[i]);
            size_t pos = str.find('=');
            if(pos == std::string::npos)
            {
                printf("invalid variable string '%s'\n", str.c_str());
                continue;
            }
            std::string name = str.substr(0, pos);
            std::string val = str.substr(pos + 1);
            char *end;
            soc_word_t v = strtoul(val.c_str(), &end, 0);
            if(*end)
            {
                printf("invalid variable string '%s'\n", str.c_str());
                continue;
            }
            printf("%s = %#lx\n", name.c_str(), (unsigned long)v);
            map[name] = v;
            continue;
        }
        error_context_t ctx;
        if(!evaluate_formula(formula, map, result, ctx))
        {
            print_context(ctx);
            printf("cannot parse '%s'\n", formula.c_str());
        }
        else
            printf("result: %lu (%#lx)\n", (unsigned long)result, (unsigned long)result);
    }
    return 0;
}

int do_write(int argc, char **argv)
{
    if(argc != 2)
        return printf("write mode expects two arguments\n");
    soc_t soc;
    error_context_t ctx;
    if(!parse_xml(argv[0], soc, ctx))
    {
        print_context(ctx);
        return printf("cannot read file '%s'\n", argv[0]);
    }
    if(!produce_xml(argv[1], soc, ctx))
    {
        print_context(ctx);
        return printf("cannot write file '%s'\n", argv[1]);
    }
    print_context(ctx);
    return 0;
}

void usage()
{
    printf("usage: tester <mode> [options]\n");
    printf("modes:\n");
    printf("  read <files...>\n");
    printf("  write <read file> <write file>\n");
    printf("  eval [<formula>|--var <name>=<val>]...\n");
    exit(1);
}

int main(int argc, char **argv)
{
    if(argc < 2)
        usage();
    std::string mode = argv[1];
    if(mode == "read")
        return do_read(argc - 2, argv +  2);
    else if(mode == "write")
        return do_write(argc - 2, argv +  2);
    else if(mode == "eval")
        return do_eval(argc - 2, argv +  2);
    else
        usage();
    return 0;
}