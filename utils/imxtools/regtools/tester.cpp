/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Amaury Pouly
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
#include "desc_parser.hpp"
#include <stdio.h>
#include <stdlib.h>

void print_value_desc(const soc_reg_field_value_t& value)
{
    printf("    VALUE %s (%#x)\n", value.name.c_str(), value.value);
}

void print_field_desc(const soc_reg_field_t& field)
{
    printf("   FIELD %s (%d:%d)\n", field.name.c_str(), field.last_bit,
        field.first_bit);
    for(size_t i = 0; i < field.values.size(); i++)
        print_value_desc(field.values[i]);
}

std::string compute_sct(soc_reg_flags_t f)
{
    if(f & REG_HAS_SCT) return "SCT";
    else return "";
}

void print_reg_desc(const soc_reg_t& reg, bool in_multi)
{
    if(in_multi)
    {
        printf("   REG %s (%#x)\n", reg.name.c_str(), reg.addr);
    }
    else
    {
        std::string sct = compute_sct(reg.flags);
        printf("  REG %s %s(%#x)\n", reg.name.c_str(), sct.c_str(), reg.addr);
        for(size_t i = 0; i < reg.fields.size(); i++)
            print_field_desc(reg.fields[i]);
    }
}

void print_multireg_desc(const soc_multireg_t& mreg)
{
    std::string sct = compute_sct(mreg.flags);
    printf("  MULTIREG %s %s(%#x * %d, +%#x)\n", mreg.name.c_str(), sct.c_str(),
        mreg.base, mreg.count, mreg.offset);
    for(size_t i = 0; i < mreg.regs.size(); i++)
        print_reg_desc(mreg.regs[i], true);
    for(size_t i = 0; i < mreg.fields.size(); i++)
        print_field_desc(mreg.fields[i]);
}


void print_dev_desc(const soc_dev_t& dev, bool in_multi)
{
    if(in_multi)
    {
        printf("  DEV %s (%#x)\n", dev.name.c_str(), dev.addr);
    }
    else
    {
        printf(" DEV %s (%#x, %s, %s)\n", dev.name.c_str(), dev.addr,
            dev.long_name.c_str(), dev.desc.c_str());
        for(size_t i = 0; i < dev.multiregs.size(); i++)
            print_multireg_desc(dev.multiregs[i]);
        for(size_t i = 0; i < dev.regs.size(); i++)
            print_reg_desc(dev.regs[i], false);
    }
}

void print_multidev_desc(const soc_multidev_t& dev)
{
    printf(" MULTIDEV %s (%s, %s)\n", dev.name.c_str(), dev.long_name.c_str(),
        dev.desc.c_str());
    for(size_t i = 0; i < dev.devs.size(); i++)
        print_dev_desc(dev.devs[i], true);
    for(size_t i = 0; i < dev.multiregs.size(); i++)
        print_multireg_desc(dev.multiregs[i]);
    for(size_t i = 0; i < dev.regs.size(); i++)
        print_reg_desc(dev.regs[i], false);
}

void print_soc_desc(const soc_t& soc)
{
    printf("SOC %s (%s)\n", soc.name.c_str(), soc.desc.c_str());
    for(size_t i = 0; i < soc.devs.size(); i++)
        print_dev_desc(soc.devs[i], false);
    for(size_t i = 0; i < soc.multidevs.size(); i++)
        print_multidev_desc(soc.multidevs[i]);
}

void usage()
{
    printf("usage: tester <desc file>\n");
    exit(1);
}

int main(int argc, char **argv)
{
    if(argc != 2)
        usage();
    std::vector< soc_t > socs;
    bool ret = parse_soc_desc(argv[1], socs);
    printf("parse result: %d\n", ret);
    if(ret)
        for(size_t i = 0; i < socs.size(); i++)
            print_soc_desc(socs[i]);
    return 0;
}