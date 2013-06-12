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
#ifndef __SOC_DESC__
#define __SOC_DESC__

#include <stdint.h>
#include <vector>
#include <string>

/**
 * These data structures represent the SoC register in a convenient way.
 * The basic structure is the following:
 * - each SoC has several devices
 * - each device has a generic name, a list of {name,address} and several registers
 * - each register has a generic name, a list of {name,address}, flags,
 *   several fields
 * - each field has a name, a first and last bit position, can apply either
 *   to all addresses of a register or be specific to one only and has several values
 * - each field value has a name and a value
 *
 * All addresses, values and names are relative to the parents. For example a field
 * value BV_LCDIF_CTRL_WORD_LENGTH_18_BIT is represented has:
 * - device LCDIF, register CTRL, field WORD_LENGTH, value 16_BIT
 * The address of CTRL is related to the address of LCDIF, the value of 16_BIT
 * ignores the position of the WORD_LENGTH field in the register.
 */

/**
 * Typedef for SoC types: word, address and flags */
typedef uint32_t soc_addr_t;
typedef uint32_t soc_word_t;
typedef uint32_t soc_reg_flags_t;

/** SoC register generic formula */
enum soc_reg_formula_type_t
{
    REG_FORMULA_NONE, /// register has no generic formula
    REG_FORMULA_STRING, /// register has a generic formula represented by a string
};

/** <soc_reg_t>.<flags> values */
const soc_reg_flags_t REG_HAS_SCT = 1 << 0; /// register SCT variants

/** SoC register field named value */
struct soc_reg_field_value_t
{
    std::string name;
    soc_word_t value;
};

/** SoC register field */
struct soc_reg_field_t
{
    std::string name;
    unsigned first_bit, last_bit;

    soc_word_t bitmask() const
    {
        // WARNING beware of the case where first_bit=0 and last_bit=31
        if(first_bit == 0 && last_bit == 31)
            return 0xffffffff;
        else
            return ((1 << (last_bit - first_bit + 1)) - 1) << first_bit;
    }

    bool is_reserved() const
    {
        return name.substr(0, 4) == "RSVD" || name.substr(0, 5) == "RSRVD";
    }

    std::vector< soc_reg_field_value_t > value;
};

/** SoC register address */
struct soc_reg_addr_t
{
    std::string name; /// actual register name
    soc_addr_t addr;
};

/** SoC register formula */
struct soc_reg_formula_t
{
    enum soc_reg_formula_type_t type;
    std::string string; /// for STRING
};

/** SoC register */
struct soc_reg_t
{
    std::string name; /// generic name (for multi registers) or actual name
    std::vector< soc_reg_addr_t > addr;
    soc_reg_formula_t formula;
    soc_reg_flags_t flags; /// ORed value

    std::vector< soc_reg_field_t > field;
};

/** Soc device address */
struct soc_dev_addr_t
{
    std::string name; /// actual device name
    soc_addr_t addr;
};

/** SoC device */
struct soc_dev_t
{
    std::string name; /// generic name (of multi devices) or actual name
    std::string version; /// description version
    std::vector< soc_dev_addr_t > addr;

    std::vector< soc_reg_t > reg;
};

/** SoC */
struct soc_t
{
    std::string name; /// codename (rockbox)
    std::string desc; /// SoC name

    std::vector< soc_dev_t > dev;
};

/** Parse a SoC description from a XML file, append it to <soc>. A file
 * can contain multiple SoC descriptions */
bool soc_desc_parse_xml(const std::string& filename, std::vector< soc_t >& soc);

#endif /* __SOC_DESC__ */