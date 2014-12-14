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
#ifndef __SOC_DESC__
#define __SOC_DESC__

#include <stdint.h>
#include <vector>
#include <list>
#include <string>
#include <map>

namespace soc_desc
{

const size_t MAJOR_VERSION = 2;
const size_t MINOR_VERSION = 0;
const size_t REVISION_VERSION = 0;

/** Typedef for SoC types: word, address and flags */
typedef uint32_t soc_addr_t;
typedef uint32_t soc_word_t;

/** Error class */
class error_t
{
public:
    enum level_t
    {
        INFO,
        WARNING,
        FATAL
    };
    error_t(level_t lvl, const std::string& loc, const std::string& msg)
        :m_level(lvl), m_loc(loc), m_msg(msg) {}
    level_t level() const { return m_level; }
    std::string location() const { return m_loc; }
    std::string message() const { return m_msg; }
protected:
    level_t m_level;
    std::string m_loc, m_msg;
};

/** Error context to log errors */
class error_context_t
{
public:
    void add(const error_t& err) { m_list.push_back(err); }
    size_t count() const { return m_list.size(); }
    error_t get(size_t i) const { return m_list[i]; }
protected:
    std::vector< error_t > m_list;
};

/**
 * Bare representation of the format
 */

/** Enumerated value (aka named value), represents a special value for a field */
struct enum_t
{
    std::string name; /** Name (must be unique for the among field enums) */
    std::string desc; /** Optional description of the meaning of this value */
    soc_word_t value; /** Value of the field */
};

/** Register field information */
struct field_t
{
    std::string name; /** Name (must be unique for the among register fields) */
    std::string desc; /** Optional description of the field */
    size_t pos; /** Position of the least significant bit */
    size_t width; /** Width of the field in bits  */
    std::vector< enum_t > enum_; /** List of special values */

    /** Returns the bit mask of the field within the register */
    soc_word_t bitmask() const
    {
        // WARNING beware of the case where width is 32
        if(width == 32)
            return 0xffffffff;
        else
            return ((1 << width) - 1) << pos;
    }

    /** Extract field value from register value */
    soc_word_t extract(soc_word_t reg_val) const
    {
        return (reg_val & bitmask()) >> pos;
    }

    /** Replace the field value in a register value */
    soc_word_t replace(soc_word_t reg_val, soc_word_t field_val) const
    {
        return (reg_val & ~bitmask()) | ((field_val << pos) & bitmask());
    }

    /** Return field value index, or -1 if none */
    int find_value(soc_word_t v) const
    {
        for(size_t i = 0; i < enum_.size(); i++)
            if(enum_[i].value == v)
                return i;
        return -1;
    }
};

/** Register information */
struct register_t
{
    size_t width; /** Size in bits */
    std::vector< field_t > field; /** List of fields */
};

/** Node address range information */
struct range_t
{
    enum
    {
        STRIDE, /** Addresses are given by a base address and a stride */
        FORMULA /** Addresses are given by a formula */
    }type; /** Range type */
    size_t first; /** First index in the range */
    size_t count; /** Number of indexes in the range */
    soc_word_t base; /** Base address (for STRIDE) */
    soc_word_t stride; /** Stride value (for STRIDE) */
    std::string formula; /** Formula (for FORMULA) */
    std::string variable; /** Formula variable name (for FORMULA) */
};

/** Node instance information */
struct instance_t
{
    std::string name; /** Name (must be unique for the among node instances) */
    std::string title; /** Optional instance human name */
    std::string desc; /** Optional description of the instance */
    enum
    {
        SINGLE, /** There is a single instance at a specified address */
        RANGE /** There are multiple addresses forming a range */
    }type; /** Instance type */
    soc_word_t addr; /** Address (for SINGLE) */
    range_t range; /** Range (for RANGE) */
};

/** Node information */
struct node_t
{
    std::string title; /** Optional node human name */
    std::string desc; /** Optional description of the node */
    std::vector< register_t> register_; /** Optional register */
    std::vector< instance_t> instance; /** List of instances */
    std::vector< node_t > node; /** List of sub-nodes */
};

/** System-on-chip information */
struct soc_t
{
    std::string name; /** Codename of the SoC */
    std::string title; /** Human name of the SoC */
    std::string desc; /** Optional description of the SoC */
    std::string isa; /** Instruction Set Assembly */
    std::string version; /** Description version */
    std::vector< std::string > author; /** List of authors of the description */
    std::vector< node_t > node; /** List of nodes */
};

/** Parse a SoC description from a XML file, put it into <soc>. */
bool parse_xml(const std::string& filename, soc_t& soc, error_context_t& error_ctx);
/** Write a SoC description to a XML file, overwriting it. A file can contain
 * multiple Soc descriptions */
bool produce_xml(const std::string& filename, const soc_t& soc, error_context_t& error_ctx);
/** Formula parser: try to parse and evaluate a formula with some variables */
bool evaluate_formula(const std::string& formula,
    const std::map< std::string, soc_word_t>& var, soc_word_t& result,
    const std::string& loc, error_context_t& error_ctx);

} // soc_desc

#endif /* __SOC_DESC__ */
