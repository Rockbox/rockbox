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
#ifndef __SOC_DESC_V1__
#define __SOC_DESC_V1__

#include <stdint.h>
#include <vector>
#include <list>
#include <string>
#include <map>

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

namespace soc_desc_v1
{

const size_t MAJOR_VERSION = 1;
const size_t MINOR_VERSION = 4;
const size_t REVISION_VERSION = 1;

/**
 * Typedef for SoC types: word, address and flags */
typedef uint32_t soc_addr_t;
typedef uint32_t soc_word_t;
typedef uint32_t soc_reg_flags_t;

/** SoC error gravity level */
enum soc_error_level_t
{
    SOC_ERROR_WARNING,
    SOC_ERROR_FATAL,
};

/** SoC description error */
struct soc_error_t
{
    soc_error_level_t level; /// level (warning, fatal, ...)
    std::string location; /// human description of the location
    std::string message; /// message
};

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
    std::string name; /// name of the value
    soc_word_t value; /// numeric value
    std::string desc; /// human description

    std::vector< soc_error_t > errors(bool recursive);
};

/** SoC register field */
struct soc_reg_field_t
{
    std::string name; /// name of the field
    std::string desc; /// human description
    unsigned first_bit, last_bit; /// bit range of the field

    soc_reg_field_t():first_bit(0), last_bit(31) {}

    /** Return field bitmask in register */
    soc_word_t bitmask() const
    {
        // WARNING beware of the case where first_bit=0 and last_bit=31
        if(first_bit == 0 && last_bit == 31)
            return 0xffffffff;
        else
            return ((1 << (last_bit - first_bit + 1)) - 1) << first_bit;
    }

    /** Extract field value from register value */
    soc_word_t extract(soc_word_t reg_val) const
    {
        return (reg_val & bitmask()) >> first_bit;
    }

    /** Replace the field value in a register value */
    soc_word_t replace(soc_word_t reg_val, soc_word_t field_val) const
    {
        return (reg_val & ~bitmask()) | ((field_val << first_bit) & bitmask());
    }

    bool is_reserved() const
    {
        return name.substr(0, 4) == "RSVD" || name.substr(0, 5) == "RSRVD";
    }

    /** Return field value index, or -1 if none */
    int find_value(soc_word_t v) const
    {
        for(size_t i = 0; i < value.size(); i++)
            if(value[i].value == v)
                return i;
        return -1;
    }

    std::vector< soc_reg_field_value_t > value;

    std::vector< soc_error_t > errors(bool recursive);
};

/** SoC register address */
struct soc_reg_addr_t
{
    std::string name; /// actual register name
    soc_addr_t addr; /// actual register address (relative to device)

    std::vector< soc_error_t > errors(bool recursive);
};

/** SoC register formula */
struct soc_reg_formula_t
{
    enum soc_reg_formula_type_t type;
    std::string string; /// for STRING

    std::vector< soc_error_t > errors(bool recursive);
};

/** SoC register */
struct soc_reg_t
{
    std::string name; /// generic name (for multi registers) or actual name
    std::string desc; /// human description
    std::vector< soc_reg_addr_t > addr; /// instances of the registers
    soc_reg_formula_t formula; /// formula for the instance addresses
    soc_reg_flags_t flags; /// ORed value

    std::vector< soc_reg_field_t > field;

    std::vector< soc_error_t > errors(bool recursive);
};

/** Soc device address */
struct soc_dev_addr_t
{
    std::string name; /// actual device name
    soc_addr_t addr;

    std::vector< soc_error_t > errors(bool recursive);
};

/** SoC device */
struct soc_dev_t
{
    std::string name; /// generic name (of multi devices) or actual name
    std::string long_name; /// human friendly name
    std::string desc; /// human description
    std::string version; /// description version
    std::vector< soc_dev_addr_t > addr;

    std::vector< soc_reg_t > reg;

    std::vector< soc_error_t > errors(bool recursive);
};

/** SoC */
struct soc_t
{
    std::string name; /// codename (rockbox)
    std::string desc; /// SoC name

    std::vector< soc_dev_t > dev;

    std::vector< soc_error_t > errors(bool recursive);
};

/** Parse a SoC description from a XML file, append it to <soc>. */
bool parse_xml(const std::string& filename, soc_t& soc);
/** Write a SoC description to a XML file, overwriting it. A file can contain
 * multiple Soc descriptions */
bool produce_xml(const std::string& filename, const soc_t& soc);
/** Normalise a soc description by reordering elements so that:
 * - devices are sorted by first name
 * - registers are sorted by first address
 * - fields are sorted by last bit
 * - values are sorted by value */
void normalize(soc_t& soc);
/** Formula parser: try to parse and evaluate a formula with some variables */
bool evaluate_formula(const std::string& formula,
    const std::map< std::string, soc_word_t>& var, soc_word_t& result,
    std::string& error);

} // soc_desc_v1

#endif /* __SOC_DESC_V1__ */