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
    error_t get(size_t i) { return m_list[i]; }
protected:
    std::vector< error_t > m_list;
};

/** Bare representation of the format */
struct title_t
{
    std::string title;

    void check(error_context_t& ctx);
};

struct desc_t
{
    std::string desc;

    void check(error_context_t& ctx);
};

struct name_t
{
    std::string name;

    void check(error_context_t& ctx);
};

struct enum_t : name_t, desc_t
{
    soc_word_t value;

    void check(error_context_t& ctx);
};

struct field_t : name_t, desc_t
{
    size_t pos, width;
    std::vector< enum_t > enum_;

    void check(error_context_t& ctx);
};

struct register_t
{
    size_t width;
    std::vector< field_t > field;

    void check(error_context_t& ctx);
};

struct range_t
{
    enum
    {
        STRIDE,
        FORMULA
    }type;
    soc_word_t base, stride;
    std::string formula, variable;

    void check(error_context_t& ctx);
};

struct instance_t : name_t, title_t, desc_t
{
    enum
    {
        SINGLE,
        RANGE
    }type;
    soc_word_t addr;
    range_t range;

    void check(error_context_t& ctx);
};

struct node_t : title_t, desc_t
{
    std::vector< register_t> register_;
    std::vector< instance_t> instance;
    std::vector< node_t > node;

    void check(error_context_t& ctx);
};

struct soc_t : name_t, title_t, desc_t
{
    std::string isa, version;
    std::vector< std::string > author;
    std::vector< node_t > node;

    void check(error_context_t& ctx);
};

#if 0
class soc_t
{
public:
    /** getters/setters: name, title, title, authors, ISA, version */
    std::string name() const;
    void set_name(const std::string& name);
    std::string title() const;
    void set_title(const std::string& title);
    std::string desc() const;
    void set_desc(const std::string& desc);
    std::string version() const;
    void set_version(const std::string& version);
    size_t author_count() const;
    std::string author(size_t i) const;
    void add_author(const std::string& author);
    void remove_author(size_t i);
    std::string isa() const;
    void set_isa(const std::string& isa);
    
};
#endif

/** Parse a SoC description from a XML file, put it into <soc>. */
bool parse_xml(const std::string& filename, soc_t& soc);
/** Formula parser: try to parse and evaluate a formula with some variables */
bool evaluate_formula(const std::string& formula,
    const std::map< std::string, soc_word_t>& var, soc_word_t& result,
    error_context_t& error_ctx);

} // soc_desc

#endif /* __SOC_DESC__ */
