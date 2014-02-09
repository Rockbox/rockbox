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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <string.h>

#define XML_CHAR_TO_CHAR(s) ((const char *)(s))

#define BEGIN_ATTR_MATCH(attr) \
    for(xmlAttr *a = attr; a; a = a->next) {

#define MATCH_X_ATTR(attr_name, hook, ...) \
    if(strcmp(XML_CHAR_TO_CHAR(a->name), attr_name) == 0) { \
        std::string s; \
        if(!parse_text_attr(a, s) || !hook(s, __VA_ARGS__)) \
            return false; \
    }

#define SOFT_MATCH_X_ATTR(attr_name, hook, ...) \
    if(strcmp(XML_CHAR_TO_CHAR(a->name), attr_name) == 0) { \
        std::string s; \
        if(parse_text_attr(a, s)) \
            hook(s, __VA_ARGS__); \
    }

#define SOFT_MATCH_SCT_ATTR(attr_name, var) \
    SOFT_MATCH_X_ATTR(attr_name, validate_sct_hook, var)

#define MATCH_TEXT_ATTR(attr_name, var) \
    MATCH_X_ATTR(attr_name, validate_string_hook, var)

#define MATCH_UINT32_ATTR(attr_name, var) \
    MATCH_X_ATTR(attr_name, validate_uint32_hook, var)

#define MATCH_BITRANGE_ATTR(attr_name, first, last) \
    MATCH_X_ATTR(attr_name, validate_bitrange_hook, first, last)

#define END_ATTR_MATCH() \
    }

#define BEGIN_NODE_MATCH(node) \
    for(xmlNode *sub = node; sub; sub = sub->next) {

#define MATCH_ELEM_NODE(node_name, array, parse_fn) \
    if(sub->type == XML_ELEMENT_NODE && strcmp(XML_CHAR_TO_CHAR(sub->name), node_name) == 0) { \
        array.resize(array.size() + 1); \
        if(!parse_fn(sub, array.back())) \
            return false; \
    }

#define SOFT_MATCH_ELEM_NODE(node_name, array, parse_fn) \
    if(sub->type == XML_ELEMENT_NODE && strcmp(XML_CHAR_TO_CHAR(sub->name), node_name) == 0) { \
        array.resize(array.size() + 1); \
        if(!parse_fn(sub, array.back())) \
            array.pop_back(); \
    }

#define END_NODE_MATCH() \
    }

bool validate_string_hook(const std::string& str, std::string& s)
{
    s = str;
    return true;
}

bool validate_sct_hook(const std::string& str, soc_reg_flags_t& flags)
{
    if(str == "yes") flags |= REG_HAS_SCT;
    else if(str != "no") return false;
    return true;
}

bool validate_unsigned_long_hook(const std::string& str, unsigned long& s)
{
    char *end;
    s = strtoul(str.c_str(), &end, 0);
    return *end == 0;
}

bool validate_uint32_hook(const std::string& str, uint32_t& s)
{
    unsigned long u;
    if(!validate_unsigned_long_hook(str, u)) return false;
#if ULONG_MAX > 0xffffffff
    if(u > 0xffffffff) return false;
#endif
    s = u;
    return true;
}

bool validate_bitrange_hook(const std::string& str, unsigned& first, unsigned& last)
{
    unsigned long a, b;
    size_t sep = str.find(':');
    if(sep == std::string::npos) return false;
    if(!validate_unsigned_long_hook(str.substr(0, sep), a)) return false;
    if(!validate_unsigned_long_hook(str.substr(sep + 1), b)) return false;
    if(a > 31 || b > 31 || a < b) return false;
    first = b;
    last = a;
    return true;
}

bool parse_text_attr(xmlAttr *attr, std::string& s)
{
    if(attr->children != attr->last)
        return false;
    if(attr->children->type != XML_TEXT_NODE)
        return false;
    s = XML_CHAR_TO_CHAR(attr->children->content);
    return true;
}

bool parse_value_elem(xmlNode *node, soc_reg_field_value_t& value)
{
    BEGIN_ATTR_MATCH(node->properties)
        MATCH_TEXT_ATTR("name", value.name)
        MATCH_UINT32_ATTR("value", value.value)
    END_ATTR_MATCH()

    return true;
}

bool parse_field_elem(xmlNode *node, soc_reg_field_t& field)
{
    BEGIN_ATTR_MATCH(node->properties)
        MATCH_TEXT_ATTR("name", field.name)
        MATCH_BITRANGE_ATTR("bitrange", field.first_bit, field.last_bit)
    END_ATTR_MATCH()

    BEGIN_NODE_MATCH(node->children)
        SOFT_MATCH_ELEM_NODE("value", field.value, parse_value_elem)
    END_NODE_MATCH()

    return true;
}

bool parse_reg_addr_elem(xmlNode *node, soc_reg_addr_t& addr)
{
    BEGIN_ATTR_MATCH(node->properties)
        MATCH_TEXT_ATTR("name", addr.name)
        MATCH_UINT32_ATTR("addr", addr.addr)
    END_ATTR_MATCH()

    return true;
}

bool parse_reg_formula_elem(xmlNode *node, soc_reg_formula_t& formula)
{
    BEGIN_ATTR_MATCH(node->properties)
        MATCH_TEXT_ATTR("string", formula.string)
    END_ATTR_MATCH()

    formula.type = REG_FORMULA_STRING;

    return true;
}

bool parse_add_trivial_addr(const std::string& str, soc_reg_t& reg)
{
    soc_reg_addr_t a;
    a.name = reg.name;
    if(!validate_uint32_hook(str, a.addr))
        return false;
    reg.addr.push_back(a);
    return true;
}

bool parse_reg_elem(xmlNode *node, soc_reg_t& reg)
{
    std::list< soc_reg_formula_t > formulas;
    BEGIN_ATTR_MATCH(node->properties)
        MATCH_TEXT_ATTR("name", reg.name)
        SOFT_MATCH_SCT_ATTR("sct", reg.flags)
        SOFT_MATCH_X_ATTR("addr", parse_add_trivial_addr, reg)
    END_ATTR_MATCH()

    BEGIN_NODE_MATCH(node->children)
        MATCH_ELEM_NODE("addr", reg.addr, parse_reg_addr_elem)
        MATCH_ELEM_NODE("formula", formulas, parse_reg_formula_elem)
        MATCH_ELEM_NODE("field", reg.field, parse_field_elem)
    END_NODE_MATCH()

    if(formulas.size() > 1)
    {
        fprintf(stderr, "Only one formula is allowed per register\n");
        return false;
    }
    if(formulas.size() == 1)
        reg.formula = formulas.front();

    return true;
}

bool parse_dev_addr_elem(xmlNode *node, soc_dev_addr_t& addr)
{
    BEGIN_ATTR_MATCH(node->properties)
        MATCH_TEXT_ATTR("name", addr.name)
        MATCH_UINT32_ATTR("addr", addr.addr)
    END_ATTR_MATCH()

    return true;
}

bool parse_dev_elem(xmlNode *node, soc_dev_t& dev)
{
    BEGIN_ATTR_MATCH(node->properties)
        MATCH_TEXT_ATTR("name", dev.name)
        MATCH_TEXT_ATTR("version", dev.version)
    END_ATTR_MATCH()

    BEGIN_NODE_MATCH(node->children)
        MATCH_ELEM_NODE("addr", dev.addr, parse_dev_addr_elem)
        MATCH_ELEM_NODE("reg", dev.reg, parse_reg_elem)
    END_NODE_MATCH()

    return true;
}

bool parse_soc_elem(xmlNode *node, soc_t& soc)
{
    BEGIN_ATTR_MATCH(node->properties)
        MATCH_TEXT_ATTR("name", soc.name)
        MATCH_TEXT_ATTR("desc", soc.desc)
    END_ATTR_MATCH()

    BEGIN_NODE_MATCH(node->children)
        MATCH_ELEM_NODE("dev", soc.dev, parse_dev_elem)
    END_NODE_MATCH()

    return true;
}

bool parse_root_elem(xmlNode *node, std::list< soc_t >& soc)
{
    BEGIN_NODE_MATCH(node)
        MATCH_ELEM_NODE("soc", soc, parse_soc_elem)
    END_NODE_MATCH()
    return true;
}

bool soc_desc_parse_xml(const std::string& filename, std::list< soc_t >& socs)
{
    LIBXML_TEST_VERSION

    xmlDoc *doc = xmlReadFile(filename.c_str(), NULL, 0);
    if(doc == NULL)
        return false;

    xmlNode *root_element = xmlDocGetRootElement(doc);

    bool ret = parse_root_elem(root_element, socs);

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return ret;
}