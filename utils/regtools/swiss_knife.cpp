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
#include "soc_desc_v1.hpp"
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
            case error_t::INFO: printf("[INFO]"); break;
            case error_t::WARNING: printf("[WARN]"); break;
            case error_t::FATAL: printf("[FATAL]"); break;
            default: printf("[UNK]"); break;
        }
        if(e.location().size() != 0)
            printf(" %s:", e.location().c_str());
        printf(" %s\n", e.message().c_str());
    }
}

bool convert_v1_to_v2(const soc_desc_v1::soc_reg_field_value_t& in, enum_t& out, error_context_t& ctx)
{
    out.name = in.name;
    out.desc = in.desc;
    out.value = in.value;
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_reg_field_t& in, field_t& out, error_context_t& ctx)
{
    out.name = in.name;
    out.desc = in.desc;
    out.pos = in.first_bit;
    out.width = in.last_bit - in.first_bit + 1;
    out.enum_.resize(in.value.size());
    for(size_t i = 0; i < in.value.size(); i++)
            if(!convert_v1_to_v2(in.value[i], out.enum_[i], ctx))
                return false;
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_reg_addr_t& in, instance_t& out, error_context_t& ctx)
{
    out.name = in.name;
    out.type = instance_t::SINGLE;
    out.addr = in.addr;
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_reg_formula_t& in, range_t& out, error_context_t& ctx)
{
    out.type = range_t::FORMULA;
    out.formula = in.string;
    out.variable = "n";
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_reg_t& in, node_t& out, error_context_t& ctx,
    std::string _loc)
{
    std::string loc = _loc + "." + in.name;
    out.title = in.name;
    out.desc = in.desc;
    if(in.formula.type == soc_desc_v1::REG_FORMULA_NONE)
    {
        out.instance.resize(in.addr.size());
        for(size_t i = 0; i < in.addr.size(); i++)
            if(!convert_v1_to_v2(in.addr[i], out.instance[i], ctx))
                return false;
    }
    else
    {
        out.instance.resize(1);
        out.instance[0].name = in.name;
        out.instance[0].type = instance_t::RANGE;
        out.instance[0].range.first = 0;
        out.instance[0].range.count = in.addr.size();
        /* check if formula is base/stride */
        bool is_stride = true;
        soc_word_t base = 0, stride = 0;
        if(in.addr.size() <= 1)
        {
            ctx.add(error_t(error_t::WARNING, loc,
                "register uses a formula but has only one instance"));
            is_stride = false;
        }
        else
        {
            base = in.addr[0].addr;
            stride = in.addr[1].addr - base;
            for(size_t i = 0; i < in.addr.size(); i++)
                if(base + i * stride != in.addr[i].addr)
                    is_stride = false;
        }

        if(is_stride)
        {
            ctx.add(error_t(error_t::INFO, loc, "promoted formula to base/stride"));
            out.instance[0].range.type = range_t::STRIDE;
            out.instance[0].range.base = base;
            out.instance[0].range.stride = stride;
        }
        else if(!convert_v1_to_v2(in.formula, out.instance[0].range, ctx))
            return false;
    }
    out.register_.resize(1);
    out.register_[0].width = 32;
    out.register_[0].field.resize(in.field.size());
    for(size_t i = 0; i < in.field.size(); i++)
        if(!convert_v1_to_v2(in.field[i], out.register_[0].field[i], ctx))
            return false;
    /* sct */
    if(in.flags & soc_desc_v1::REG_HAS_SCT)
    {
        out.node.resize(3);
        const char *names[3] = {"SET", "CLR", "TOG"};
        for(size_t i = 0; i < 3; i++)
        {
            out.node[i].instance.resize(1);
            out.node[i].instance[0].name = names[i];
            out.node[i].instance[0].type = instance_t::SINGLE;
            out.node[i].instance[0].addr = 4 + i *4;
        }
    }
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_dev_addr_t& in, instance_t& out, error_context_t& ctx)
{
    out.name = in.name;
    out.type = instance_t::SINGLE;
    out.addr = in.addr;
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_dev_t& in, node_t& out, error_context_t& ctx)
{
    out.title = in.long_name;
    out.desc = in.desc;
    out.instance.resize(in.addr.size());
    for(size_t i = 0; i < in.addr.size(); i++)
        if(!convert_v1_to_v2(in.addr[i], out.instance[i], ctx))
            return false;
    out.node.resize(in.reg.size());
    for(size_t i = 0; i < in.reg.size(); i++)
        if(!convert_v1_to_v2(in.reg[i], out.node[i], ctx, in.name))
            return false;
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_t& in, soc_t& out, error_context_t& ctx)
{
    out.name = in.name;
    out.title = in.desc;
    out.node.resize(in.dev.size());
    for(size_t i = 0; i < in.dev.size(); i++)
        if(!convert_v1_to_v2(in.dev[i], out.node[i], ctx))
            return false;
    return true;
}

int do_convert(int argc, char **argv)
{
    if(argc != 2)
        return printf("convert mode expects two arguments\n");
    soc_desc_v1::soc_t soc;
    if(!soc_desc_v1::parse_xml(argv[0], soc))
        return printf("cannot read file '%s'\n", argv[0]);
    error_context_t ctx;
    soc_t new_soc;
    if(!convert_v1_to_v2(soc, new_soc, ctx))
    {
        print_context(ctx);
        return printf("cannot convert from v1 to v2\n");
    }
    if(!produce_xml(argv[1], new_soc, ctx))
    {
        print_context(ctx);
        return printf("cannot write file '%s'\n", argv[1]);
    }
    print_context(ctx);
    return 0;
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
    printf("usage: swiss_knife <mode> [options]\n");
    printf("modes:\n");
    printf("  read <files...>\n");
    printf("  write <read file> <write file>\n");
    printf("  eval [<formula>|--var <name>=<val>]...\n");
    printf("  convert <input file> <output file>\n");
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
    else if(mode == "convert")
        return do_convert(argc - 2, argv +  2);
    else
        usage();
    return 0;
}