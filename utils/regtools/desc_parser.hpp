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
#ifndef __DESC_PARSER__
#define __DESC_PARSER__

#include <stdint.h>
#include <vector>
#include <string>

typedef uint32_t soc_addr_t;
typedef uint32_t soc_word_t;
typedef uint32_t soc_reg_flags_t;

const soc_addr_t SOC_NO_ADDR = 0xffffffff;
const soc_reg_flags_t REG_HAS_SCT = 1 << 0;

struct soc_reg_field_value_t
{
    std::string name;
    soc_word_t value;
};

struct soc_reg_field_t
{
    std::string name;
    unsigned first_bit, last_bit;

    soc_word_t bitmask() const
    {
        return ((1 << (last_bit - first_bit + 1)) - 1) << first_bit;
    }

    std::vector< soc_reg_field_value_t > values;
};

struct soc_reg_t
{
    std::string name;
    soc_addr_t addr;
    soc_reg_flags_t flags;

    std::vector< soc_reg_field_t > fields;
};

struct soc_multireg_t
{
    std::string name;
    soc_addr_t base;
    unsigned count;
    soc_addr_t offset;
    soc_reg_flags_t flags;

    std::vector< soc_reg_t > regs;
    std::vector< soc_reg_field_t > fields;
};

struct soc_dev_t
{
    std::string name;
    std::string long_name;
    std::string desc;
    soc_addr_t addr;

    std::vector< soc_multireg_t > multiregs;
    std::vector< soc_reg_t > regs;
};

struct soc_multidev_t
{
    std::string name;
    std::string long_name;
    std::string desc;

    std::vector< soc_dev_t > devs;
    std::vector< soc_multireg_t > multiregs;
    std::vector< soc_reg_t > regs;
};

struct soc_t
{
    std::string name;
    std::string desc;

    std::vector< soc_dev_t > devs;
    std::vector< soc_multidev_t > multidevs;
};

bool parse_soc_desc(const std::string& filename, std::vector< soc_t >& soc);

#endif /* __DESC_PARSER__ */