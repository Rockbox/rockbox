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

/** Bare representation of the format */
struct enum_t
{
    std::string name, desc;
    soc_word_t value;
};

struct field_t
{
    std::string name, desc;
    size_t pos, width;
    std::vector< enum_t > enum_;
};

struct register_t
{
    size_t width;
    std::vector< field_t > field;
};

struct range_t
{
    enum
    {
        STRIDE,
        FORMULA
    }type;
    size_t first, count;
    soc_word_t base, stride;
    std::string formula, variable;
};

struct instance_t
{
    std::string name, title, desc;
    enum
    {
        SINGLE,
        RANGE
    }type;
    soc_word_t addr;
    range_t range;
};

struct node_t
{
    std::string title, desc;
    std::vector< register_t> register_;
    std::vector< instance_t> instance;
    std::vector< node_t > node;
};

struct soc_t
{
    std::string name, title, desc;
    std::string isa, version;
    std::vector< std::string > author;
    std::vector< node_t > node;
};

/** Parse a SoC description from a XML file, put it into <soc>. */
bool parse_xml(const std::string& filename, soc_t& soc, error_context_t& error_ctx);
/** Write a SoC description to a XML file, overwriting it. A file can contain
 * multiple Soc descriptions */
bool produce_xml(const std::string& filename, const soc_t& soc, error_context_t& error_ctx);
/** Formula parser: try to parse and evaluate a formula with some variables */
bool evaluate_formula(const std::string& formula,
    const std::map< std::string, soc_word_t>& var, soc_word_t& result,
    error_context_t& error_ctx);

} // soc_desc

#endif /* __SOC_DESC__ */
