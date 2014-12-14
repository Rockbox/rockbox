/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#include "soc_desc_v1.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <cstring>

using namespace soc_desc_v1;

template< typename T >
bool build_map(const char *type, const std::vector< T >& vec,
    std::map< std::string, size_t >& map)
{
    for(size_t i =  0; i < vec.size(); i++)
    {
        if(map.find(vec[i].name) != map.end())
        {
            printf("soc has duplicate %s '%s'\n", type, vec[i].name.c_str());
            return false;
        }
        map[vec[i].name] = i;
    }
    return true;
}

template< typename T >
bool build_map(const char *type, const std::vector< T >& a, const std::vector< T >& b,
        std::vector< std::pair< size_t, size_t > >& m)
{
    std::map< std::string, size_t > ma, mb;
    if(!build_map(type, a, ma) || !build_map(type, b, mb))
        return false;
    std::map< std::string, size_t >::iterator it;
    for(it = ma.begin(); it != ma.end(); ++it)
    {
        if(mb.find(it->first) == mb.end())
        {
            printf("%s '%s' exists in only one file\n", type, it->first.c_str());
            return false;
        }
        m.push_back(std::make_pair(it->second, mb[it->first]));
    }
    for(it = mb.begin(); it != mb.end(); ++it)
    {
        if(ma.find(it->first) == ma.end())
        {
            printf("%s '%s' exists in only one file\n", type, it->first.c_str());
            return false;
        }
    }
    return true;
}

bool compare_value(const soc_t& soc, const soc_dev_t& dev, const soc_reg_t& reg,
        const soc_reg_field_t& field, const soc_reg_field_value_t& a, const soc_reg_field_value_t& b)
{
    if(a.value != b.value)
    {
        printf("register field value '%s.%s.%s.%s.%s' have different values\n", soc.name.c_str(),
            dev.name.c_str(), reg.name.c_str(), field.name.c_str(), a.name.c_str());
        return false;
    }
    if(a.desc != b.desc)
    {
        printf("register field value '%s.%s.%s.%s.%s' have different descriptions\n", soc.name.c_str(),
            dev.name.c_str(), reg.name.c_str(), field.name.c_str(), a.name.c_str());
        return false;
    }
    return true;
}

bool compare_field(const soc_t& soc, const soc_dev_t& dev, const soc_reg_t& reg,
        const soc_reg_field_t& a, const soc_reg_field_t& b)
{
    if(a.first_bit != b.first_bit || a.last_bit != b.last_bit)
    {
        printf("register address '%s.%s.%s.%s' have different bit ranges\n", soc.name.c_str(),
            dev.name.c_str(), reg.name.c_str(), a.name.c_str());
        return false;
    }
    if(a.desc != b.desc)
    {
        printf("register address '%s.%s.%s.%s' have different descriptions\n", soc.name.c_str(),
            dev.name.c_str(), reg.name.c_str(), a.name.c_str());
        return false;
    }
    /* values */
    std::vector< std::pair< size_t, size_t > > map;
    if(!build_map("field value", a.value, b.value, map))
        return false;
    for(size_t i = 0; i < map.size(); i++)
        if(!compare_value(soc, dev, reg, a, a.value[map[i].first], b.value[map[i].second]))
            return false;
    return true;
}

bool compare_reg_addr(const soc_t& soc, const soc_dev_t& dev, const soc_reg_t& reg,
        const soc_reg_addr_t& a, const soc_reg_addr_t& b)
{
    if(a.addr != b.addr)
    {
        printf("register address '%s.%s.%s.%s' have different values\n", soc.name.c_str(),
            dev.name.c_str(), reg.name.c_str(), a.name.c_str());
        return false;
    }
    else
        return true;
}

bool compare_reg(const soc_t& soc, const soc_dev_t& dev, const soc_reg_t& a,
        const soc_reg_t& b)
{
    if(a.desc != b.desc)
    {
        printf("register '%s.%s.%s' have different descriptions\n", soc.name.c_str(),
            dev.name.c_str(), a.name.c_str());
        return false;
    }
    if(a.flags != b.flags)
    {
        printf("device '%s.%s.%s' have different flags\n", soc.name.c_str(),
            dev.name.c_str(), a.name.c_str());
        return false;
    }
    if(a.formula.type != b.formula.type)
    {
        printf("device '%s.%s.%s' have different formula types\n", soc.name.c_str(),
            dev.name.c_str(), a.name.c_str());
        return false;
    }
    if(a.formula.string != b.formula.string)
    {
        printf("device '%s.%s.%s' have different formula string\n", soc.name.c_str(),
            dev.name.c_str(), a.name.c_str());
        return false;
    }
    /* addresses */
    std::vector< std::pair< size_t, size_t > > map;
    if(!build_map("register address", a.addr, b.addr, map))
        return false;
    for(size_t i = 0; i < map.size(); i++)
        if(!compare_reg_addr(soc, dev, a, a.addr[map[i].first], b.addr[map[i].second]))
            return false;
    /* field */
    map.clear();
    if(!build_map("field", a.field, b.field, map))
        return false;
    for(size_t i = 0; i < map.size(); i++)
        if(!compare_field(soc, dev, a, a.field[map[i].first], b.field[map[i].second]))
            return false;
    return true;
}

bool compare_dev_addr(const soc_t& soc, const soc_dev_t& dev, const soc_dev_addr_t& a,
        const soc_dev_addr_t& b)
{
    if(a.addr != b.addr)
    {
        printf("device address '%s.%s.%s' have different values\n", soc.name.c_str(),
            dev.name.c_str(), a.name.c_str());
        return false;
    }
    else
        return true;
}

bool compare_dev(const soc_t& soc, const soc_dev_t& a, const soc_dev_t& b)
{
    if(a.long_name != b.long_name)
    {
        printf("device '%s.%s' have different long names\n", soc.name.c_str(),
            a.name.c_str());
        return false;
    }
    if(a.desc != b.desc)
    {
        printf("device '%s.%s' have different descriptions\n", soc.name.c_str(),
            a.name.c_str());
        return false;
    }
    if(a.version != b.version)
    {
        printf("device '%s.%s' have different versions\n", soc.name.c_str(),
            a.name.c_str());
        return false;
    }
    /* addresses */
    std::vector< std::pair< size_t, size_t > > map;
    if(!build_map("device address", a.addr, b.addr, map))
        return false;
    for(size_t i = 0; i < map.size(); i++)
        if(!compare_dev_addr(soc, a, a.addr[map[i].first], b.addr[map[i].second]))
            return false;
    /* reg */
    map.clear();
    if(!build_map("register", a.reg, b.reg, map))
        return false;
    for(size_t i = 0; i < map.size(); i++)
        if(!compare_reg(soc, a, a.reg[map[i].first], b.reg[map[i].second]))
            return false;
    return true;
}

bool compare_soc(const soc_t& a, const soc_t& b)
{
    if(a.name != b.name)
    {
        return printf("soc have different names\n");
        return false;
    }
    if(a.desc != b.desc)
    {
        printf("soc '%s' have different descriptions\n", a.name.c_str());
        return false;
    }
    std::vector< std::pair< size_t, size_t > > map;
    if(!build_map("device", a.dev, b.dev, map))
        return false;
    for(size_t i = 0; i< map.size(); i++)
        if(!compare_dev(a, a.dev[map[i].first], b.dev[map[i].second]))
            return false;
    return true;
}

int do_compare(int argc, char **argv)
{
    if(argc != 2)
        return printf("compare mode expects two arguments\n");
    soc_t soc[2];
    if(!parse_xml(argv[0], soc[0]))
        return printf("cannot read file '%s'\n", argv[0]);
    if(!parse_xml(argv[1], soc[1]))
        return printf("cannot read file '%s'\n", argv[1]);
    if(compare_soc(soc[0], soc[1]))
        printf("Files are identical.\n");
    return 0;
}

int do_write(int argc, char **argv)
{
    if(argc != 2)
        return printf("write mode expects two arguments\n");
    soc_t soc;
    if(!parse_xml(argv[0], soc))
        return printf("cannot read file '%s'\n", argv[0]);
    if(!produce_xml(argv[1], soc))
        return printf("cannot write file '%s'\n", argv[1]);
    return 0;
}

int do_check(int argc, char **argv)
{
    for(int i = 0; i < argc; i++)
    {
        soc_t soc;
        if(!parse_xml(argv[i], soc))
        {
            printf("cannot read file '%s'\n", argv[i]);
            continue;
        }
        printf("[%s]\n", argv[i]);
        std::vector< soc_error_t > errs = soc.errors(true);
        for(size_t i = 0; i < errs.size(); i++)
        {
            const soc_error_t& e = errs[i];
            switch(e.level)
            {
                case SOC_ERROR_WARNING: printf("[WARN ] "); break;
                case SOC_ERROR_FATAL: printf("[FATAL] "); break;
                default: printf("[ UNK ] "); break;
            }
            printf("%s: %s\n", e.location.c_str(), e.message.c_str());
        }
    }
    return 0;
}

int do_eval(int argc, char **argv)
{
    std::map< std::string, soc_word_t > map;
    for(int i = 0; i < argc; i++)
    {
        std::string error;
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
        if(!evaluate_formula(formula, map, result, error))
            printf("error: %s\n", error.c_str());
        else
            printf("result: %lu (%#lx)\n", (unsigned long)result, (unsigned long)result);
    }
    return 0;
}

void usage()
{
    printf("usage: tester <mode> [options]\n");
    printf("modes:\n");
    printf("  compare <desc file> <desc file>\n");
    printf("  write <read file> <write file>\n");
    printf("  check <files...>\n");
    printf("  eval [<formula>|--var <name>=<val>]...\n");
    exit(1);
}

int main(int argc, char **argv)
{
    if(argc < 2)
        usage();
    std::string mode = argv[1];
    if(mode == "compare")
        return do_compare(argc - 2, argv +  2);
    else if(mode == "write")
        return do_write(argc - 2, argv +  2);
    else if(mode == "check")
        return do_check(argc - 2, argv +  2);
    else if(mode == "eval")
        return do_eval(argc - 2, argv +  2);
    else
        usage();
    return 0;
}