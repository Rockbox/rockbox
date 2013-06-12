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
#include "soc_desc.hpp"
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
    for(size_t i = 0; i < field.value.size(); i++)
        print_value_desc(field.value[i]);
}

std::string compute_sct(soc_reg_flags_t f)
{
    if(f & REG_HAS_SCT) return "SCT";
    else return "";
}

void print_reg_addr_desc(const soc_reg_addr_t& reg)
{
    printf("   ADDR %s %#x\n", reg.name.c_str(), reg.addr);
}

void print_reg_desc(const soc_reg_t& reg)
{
    std::string sct = compute_sct(reg.flags);
    printf("  REG %s %s\n", reg.name.c_str(), sct.c_str());
    for(size_t i = 0; i < reg.addr.size(); i++)
        print_reg_addr_desc(reg.addr[i]);
    for(size_t i = 0; i < reg.field.size(); i++)
        print_field_desc(reg.field[i]);
}

void print_dev_addr_desc(const soc_dev_addr_t& dev)
{
    printf("  ADDR %s %#x\n", dev.name.c_str(), dev.addr);
}

void print_dev_desc(const soc_dev_t& dev)
{
    printf(" DEV %s\n", dev.name.c_str());
    for(size_t i = 0; i < dev.addr.size(); i++)
        print_dev_addr_desc(dev.addr[i]);
    for(size_t i = 0; i < dev.reg.size(); i++)
        print_reg_desc(dev.reg[i]);
}

void print_soc_desc(const soc_t& soc)
{
    printf("SOC %s (%s)\n", soc.name.c_str(), soc.desc.c_str());
    for(size_t i = 0; i < soc.dev.size(); i++)
        print_dev_desc(soc.dev[i]);
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
    bool ret = soc_desc_parse_xml(argv[1], socs);
    printf("parse result: %d\n", ret);
    if(ret)
        for(size_t i = 0; i < socs.size(); i++)
            print_soc_desc(socs[i]);
    return 0;
}