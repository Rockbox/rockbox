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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/xmlwriter.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <cctype>

namespace soc_desc_v1
{

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

namespace
{

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
        MATCH_TEXT_ATTR("desc", value.desc)
    END_ATTR_MATCH()

    return true;
}

bool parse_field_elem(xmlNode *node, soc_reg_field_t& field)
{
    BEGIN_ATTR_MATCH(node->properties)
        MATCH_TEXT_ATTR("name", field.name)
        MATCH_BITRANGE_ATTR("bitrange", field.first_bit, field.last_bit)
        MATCH_TEXT_ATTR("desc", field.desc)
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
        MATCH_TEXT_ATTR("desc", reg.desc)
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
        MATCH_TEXT_ATTR("long_name", dev.long_name)
        MATCH_TEXT_ATTR("desc", dev.desc)
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

bool parse_root_elem(xmlNode *node, soc_t& soc)
{
    std::vector< soc_t > socs;
    BEGIN_NODE_MATCH(node)
        MATCH_ELEM_NODE("soc", socs, parse_soc_elem)
    END_NODE_MATCH()
    if(socs.size() != 1)
    {
        fprintf(stderr, "A description file must contain exactly one soc element\n");
        return false;
    }
    soc = socs[0];
    return true;
}

}

bool parse_xml(const std::string& filename, soc_t& socs)
{
    LIBXML_TEST_VERSION

    xmlDocPtr doc = xmlReadFile(filename.c_str(), NULL, 0);
    if(doc == NULL)
        return false;

    xmlNodePtr root_element = xmlDocGetRootElement(doc);
    bool ret = parse_root_elem(root_element, socs);

    xmlFreeDoc(doc);

    return ret;
}

namespace
{

int produce_field(xmlTextWriterPtr writer, const soc_reg_field_t& field)
{
#define SAFE(x) if((x) < 0) return -1;
    /* <field> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "field"));
    /* name */
    SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "name", BAD_CAST field.name.c_str()));
    /* desc */
    SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "desc", BAD_CAST field.desc.c_str()));
    /* bitrange */
    SAFE(xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "bitrange", "%d:%d",
        field.last_bit, field.first_bit));
    /* values */
    for(size_t i = 0; i < field.value.size(); i++)
    {
        /* <value> */
        SAFE(xmlTextWriterStartElement(writer, BAD_CAST "value"));
        /* name */
        SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "name", BAD_CAST field.value[i].name.c_str()));
        /* value */
        SAFE(xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "value", "0x%x", field.value[i].value));
        /* name */
        SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "desc", BAD_CAST field.value[i].desc.c_str()));
        /* </value> */
        SAFE(xmlTextWriterEndElement(writer));
    }
    /* </field> */
    SAFE(xmlTextWriterEndElement(writer));
#undef SAFE
    return 0;
}

int produce_reg(xmlTextWriterPtr writer, const soc_reg_t& reg)
{
#define SAFE(x) if((x) < 0) return -1;
    /* <reg> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "reg"));
    /* name */
    SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "name", BAD_CAST reg.name.c_str()));
    /* name */
    SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "desc", BAD_CAST reg.desc.c_str()));
    /* flags */
    if(reg.flags & REG_HAS_SCT)
        SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "sct", BAD_CAST "yes"));
    /* formula */
    if(reg.formula.type != REG_FORMULA_NONE)
    {
        /* <formula> */
        SAFE(xmlTextWriterStartElement(writer, BAD_CAST "formula"));
        switch(reg.formula.type)
        {
            case REG_FORMULA_STRING:
                SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "string", 
                    BAD_CAST reg.formula.string.c_str()));
                break;
            default:
                break;
        }
        /* </formula> */
        SAFE(xmlTextWriterEndElement(writer));
    }
    /* addresses */
    for(size_t i = 0; i < reg.addr.size(); i++)
    {
        /* <addr> */
        SAFE(xmlTextWriterStartElement(writer, BAD_CAST "addr"));
        /* name */
        SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "name", BAD_CAST reg.addr[i].name.c_str()));
        /* addr */
        SAFE(xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "addr", "0x%x", reg.addr[i].addr));
        /* </addr> */
        SAFE(xmlTextWriterEndElement(writer));
    }
    /* fields */
    for(size_t i = 0; i < reg.field.size(); i++)
        produce_field(writer, reg.field[i]);
    /* </reg> */
    SAFE(xmlTextWriterEndElement(writer));
#undef SAFE
    return 0;
}

int produce_dev(xmlTextWriterPtr writer, const soc_dev_t& dev)
{
#define SAFE(x) if((x) < 0) return -1;
    /* <dev> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "dev"));
    /* name */
    SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "name", BAD_CAST dev.name.c_str()));
    /* long_name */
    SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "long_name", BAD_CAST dev.long_name.c_str()));
    /* desc */
    SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "desc", BAD_CAST dev.desc.c_str()));
    /* version */
    SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "version", BAD_CAST dev.version.c_str()));
    /* addresses */
    for(size_t i = 0; i < dev.addr.size(); i++)
    {
        /* <addr> */
        SAFE(xmlTextWriterStartElement(writer, BAD_CAST "addr"));
        /* name */
        SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "name", BAD_CAST dev.addr[i].name.c_str()));
        /* addr */
        SAFE(xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "addr", "0x%x", dev.addr[i].addr));
        /* </addr> */
        SAFE(xmlTextWriterEndElement(writer));
    }
    /* registers */
    for(size_t i = 0; i < dev.reg.size(); i++)
        produce_reg(writer, dev.reg[i]);
    /* </dev> */
    SAFE(xmlTextWriterEndElement(writer));
#undef SAFE
    return 0;
}

}

bool produce_xml(const std::string& filename, const soc_t& soc)
{
    LIBXML_TEST_VERSION

    xmlTextWriterPtr writer = xmlNewTextWriterFilename(filename.c_str(), 0);
    if(writer == NULL)
        return false;
#define SAFE(x) if((x) < 0) goto  Lerr
    SAFE(xmlTextWriterSetIndent(writer, 1));
    SAFE(xmlTextWriterSetIndentString(writer, BAD_CAST "  "));
    /* <xml> */
    SAFE(xmlTextWriterStartDocument(writer, NULL, NULL, NULL));
    /* <soc> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "soc"));
    /* name */
    SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "name", BAD_CAST soc.name.c_str()));
    /* desc */
    SAFE(xmlTextWriterWriteAttribute(writer,  BAD_CAST "desc", BAD_CAST soc.desc.c_str()));
    /* devices */
    for(size_t i = 0; i < soc.dev.size(); i++)
        SAFE(produce_dev(writer, soc.dev[i]));
    /* end <soc> */
    SAFE(xmlTextWriterEndElement(writer));
    /* </xml> */
    SAFE(xmlTextWriterEndDocument(writer));
    xmlFreeTextWriter(writer);
    return true;
#undef SAFE
Lerr:
    xmlFreeTextWriter(writer);
    return false;
}

namespace
{

struct soc_sorter
{
    bool operator()(const soc_dev_t& a, const soc_dev_t& b) const
    {
        return a.name < b.name;
    }

    bool operator()(const soc_dev_addr_t& a, const soc_dev_addr_t& b) const
    {
        return a.name < b.name;
    }

    bool operator()(const soc_reg_t& a, const soc_reg_t& b) const
    {
        soc_addr_t aa = a.addr.size() > 0 ? a.addr[0].addr : 0;
        soc_addr_t ab = b.addr.size() > 0 ? b.addr[0].addr : 0;
        return aa < ab;
    }

    bool operator()(const soc_reg_addr_t& a, const soc_reg_addr_t& b) const
    {
        return a.addr < b.addr;
    }

    bool operator()(const soc_reg_field_t& a, const soc_reg_field_t& b) const
    {
        return a.last_bit > b.last_bit;
    }

    bool operator()(const soc_reg_field_value_t a, const soc_reg_field_value_t& b) const
    {
        return a.value < b.value;
    }
};

void normalize(soc_reg_field_t& field)
{
    std::sort(field.value.begin(), field.value.end(), soc_sorter());
}

void normalize(soc_reg_t& reg)
{
    std::sort(reg.addr.begin(), reg.addr.end(), soc_sorter());
    std::sort(reg.field.begin(), reg.field.end(), soc_sorter());
    for(size_t i = 0; i < reg.field.size(); i++)
        normalize(reg.field[i]);
}

void normalize(soc_dev_t& dev)
{
    std::sort(dev.addr.begin(), dev.addr.end(), soc_sorter());
    std::sort(dev.reg.begin(), dev.reg.end(), soc_sorter());
    for(size_t i = 0; i < dev.reg.size(); i++)
        normalize(dev.reg[i]);
}

}

void normalize(soc_t& soc)
{
    std::sort(soc.dev.begin(), soc.dev.end(), soc_sorter());
    for(size_t i = 0; i < soc.dev.size(); i++)
        normalize(soc.dev[i]);
}

namespace
{
    soc_error_t make_error(soc_error_level_t lvl, std::string at, std::string what)
    {
        soc_error_t err;
        err.level = lvl;
        err.location = at;
        err.message = what;
        return err;
    }

    soc_error_t make_warning(std::string at, std::string what)
    {
        return make_error(SOC_ERROR_WARNING, at, what);
    }

    soc_error_t make_fatal(std::string at, std::string what)
    {
        return make_error(SOC_ERROR_FATAL, at, what);
    }

    soc_error_t prefix(soc_error_t err, const std::string& prefix_at)
    {
        err.location = prefix_at + "." + err.location;
        return err;
    }

    void add_errors(std::vector< soc_error_t >& errors,
        const std::vector< soc_error_t >& new_errors, const std::string& prefix_at)
    {
        for(size_t i = 0; i < new_errors.size(); i++)
            errors.push_back(prefix(new_errors[i], prefix_at));
    }

    std::vector< soc_error_t > no_error()
    {
        std::vector< soc_error_t > s;
        return s;
    }

    std::vector< soc_error_t > one_error(const soc_error_t& err)
    {
        std::vector< soc_error_t > s;
        s.push_back(err);
        return s;
    }

    bool name_valid(char c)
    {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') || c == '_';
    }

    bool name_valid(const std::string& s)
    {
        for(size_t i = 0; i < s.size(); i++)
            if(!name_valid(s[i]))
                return false;
        return true;
    }
}

std::vector< soc_error_t > soc_reg_field_value_t::errors(bool recursive)
{
    (void) recursive;
    if(name.size() == 0)
        return one_error(make_fatal(name, "empty name"));
    else if(!name_valid(name))
        return one_error(make_fatal(name, "invalid name"));
    else
        return no_error();
}

std::vector< soc_error_t > soc_reg_field_t::errors(bool recursive)
{
    std::vector< soc_error_t >  err;
    std::string at(name);
    if(name.size() == 0)
        err.push_back(make_fatal(at, "empty name"));
    else if(!name_valid(name))
        err.push_back(make_fatal(at, "invalid name"));
    if(last_bit > 31)
        err.push_back(make_fatal(at, "last bit is greater than 31"));
    if(first_bit > last_bit)
        err.push_back(make_fatal(at, "last bit is greater than first bit"));
    for(size_t i = 0; i < value.size(); i++)
    {
        for(size_t j = 0; j < value.size(); j++)
        {
            if(i == j)
                continue;
            if(value[i].name == value[j].name)
                err.push_back(prefix(make_fatal(value[i].name, 
                    "there are several values with the same name"), at));
            if(value[i].value == value[j].value)
                err.push_back(prefix(make_warning(value[i].name, 
                    "there are several values with the same value"), at));
        }
        if(value[i].value > (bitmask() >> first_bit))
            err.push_back(prefix(make_warning(at, "value doesn't fit into the field"), value[i].name));
        if(recursive)
            add_errors(err, value[i].errors(true), at);
    }
    return err;
}

std::vector< soc_error_t > soc_reg_addr_t::errors(bool recursive)
{
    (void) recursive;
    if(name.size() == 0)
        return one_error(make_fatal("", "empty name"));
    else if(!name_valid(name))
        return one_error(make_fatal(name, "invalid name"));
    else
        return no_error();
}

std::vector< soc_error_t > soc_reg_formula_t::errors(bool recursive)
{
    (void) recursive;
    if(type == REG_FORMULA_STRING && string.size() == 0)
        return one_error(make_fatal("", "empty string formula"));
    else
        return no_error();
}

namespace
{

bool field_overlap(const soc_reg_field_t& a, const soc_reg_field_t& b)
{
    return !(a.first_bit > b.last_bit || b.first_bit > a.last_bit);
}

}

std::vector< soc_error_t > soc_reg_t::errors(bool recursive)
{
    std::vector< soc_error_t >  err;
    std::string at(name);
    if(name.size() == 0)
        err.push_back(make_fatal(at, "empty name"));
    else if(!name_valid(name))
        err.push_back(make_fatal(at, "invalid name"));
    for(size_t i = 0; i < addr.size(); i++)
    {
        for(size_t j = 0; j < addr.size(); j++)
        {
            if(i == j)
                continue;
            if(addr[i].name == addr[j].name)
                err.push_back(prefix(make_fatal(addr[i].name, 
                    "there are several instances with the same name"), at));
            if(addr[i].addr == addr[j].addr)
                err.push_back(prefix(make_fatal(addr[i].name, 
                    "there are several instances with the same address"), at));
        }
        if(recursive)
            add_errors(err, addr[i].errors(true), at);
    }
    if(recursive)
        add_errors(err, formula.errors(true), at);
    for(size_t i = 0; i < field.size(); i++)
    {
        for(size_t j = 0; j < field.size(); j++)
        {
            if(i == j)
                continue;
            if(field[i].name == field[j].name)
                err.push_back(prefix(make_fatal(field[i].name, 
                    "there are several fields with the same name"), at));
            if(field_overlap(field[i], field[j]))
                err.push_back(prefix(make_fatal(field[i].name, 
                    "there are overlapping fields"), at));
        }
        if(recursive)
            add_errors(err, field[i].errors(true), at);
    }
    return err;
}

std::vector< soc_error_t > soc_dev_addr_t::errors(bool recursive)
{
    (void) recursive;
    if(name.size() == 0)
        return one_error(make_fatal("", "empty name"));
    else if(!name_valid(name))
        return one_error(make_fatal(name, "invalid name"));
    else
        return no_error();
}

std::vector< soc_error_t > soc_dev_t::errors(bool recursive)
{
    std::vector< soc_error_t >  err;
    std::string at(name);
    if(name.size() == 0)
        err.push_back(make_fatal(at, "empty name"));
    else if(!name_valid(name))
        err.push_back(make_fatal(at, "invalid name"));
    for(size_t i = 0; i < addr.size(); i++)
    {
        for(size_t j = 0; j < addr.size(); j++)
        {
            if(i == j)
                continue;
            if(addr[i].name == addr[j].name)
                err.push_back(prefix(make_fatal(addr[i].name, 
                    "there are several instances with the same name"), at));
            if(addr[i].addr == addr[j].addr)
                err.push_back(prefix(make_fatal(addr[i].name, 
                    "there are several instances with the same address"), at));
        }
        if(recursive)
            add_errors(err, addr[i].errors(true), at);
    }
    for(size_t i = 0; i < reg.size(); i++)
    {
        for(size_t j = 0; j < reg.size(); j++)
        {
            if(i == j)
                continue;
            if(reg[i].name == reg[j].name)
                err.push_back(prefix(make_fatal(reg[i].name, 
                    "there are several registers with the same name"), at));
        }
        if(recursive)
            add_errors(err, reg[i].errors(true), at);
    }
    return err;
}

std::vector< soc_error_t > soc_t::errors(bool recursive)
{
    std::vector< soc_error_t >  err;
    std::string at(name);
    for(size_t i = 0; i < dev.size(); i++)
    {
        for(size_t j = 0; j < dev.size(); j++)
        {
            if(i == j)
                continue;
            if(dev[i].name == dev[j].name)
                err.push_back(prefix(make_fatal(dev[i].name, 
                    "there are several devices with the same name"), at));
        }
        if(recursive)
            add_errors(err, dev[i].errors(true), at);
    }
    return err;
}

namespace
{

struct formula_evaluator
{
    std::string formula;
    size_t pos;
    std::string error;

    bool err(const char *fmt, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer,sizeof(buffer), fmt, args);
        va_end(args);
        error = buffer;
        return false;
    }

    formula_evaluator(const std::string& s):pos(0)
    {
        for(size_t i = 0; i < s.size(); i++)
            if(!isspace(s[i]))
                formula.push_back(s[i]);
    }

    void adv()
    {
        pos++;
    }

    char cur()
    {
        return end() ? 0 : formula[pos];
    }

    bool end()
    {
        return pos >= formula.size();
    }

    bool parse_digit(char c, int basis, soc_word_t& res)
    {
        c = tolower(c);
        if(isdigit(c))
        {
            res = c - '0';
            return true;
        }
        if(basis == 16 && isxdigit(c))
        {
            res = c + 10 - 'a';
            return true;
        }
        return err("invalid digit '%c'", c);
    }

    bool parse_signed(soc_word_t& res)
    {
        char op = cur();
        if(op == '+' || op == '-')
        {
            adv();
            if(!parse_signed(res))
                return false;
            if(op == '-')
                res *= -1;
            return true;
        }
        else if(op == '(')
        {
            adv();
            if(!parse_expression(res))
                return false;
            if(cur() != ')')
                return err("expected ')', got '%c'", cur());
            adv();
            return true;
        }
        else if(isdigit(op))
        {
            res = op - '0';
            adv();
            int basis = 10;
            if(op == '0' && cur() == 'x')
            {
                basis = 16;
                adv();
            }
            soc_word_t digit = 0;
            while(parse_digit(cur(), basis, digit))
            {
                res = res * basis + digit;
                adv();
            }
            return true;
        }
        else if(isalpha(op) || op == '_')
        {
            std::string name;
            while(isalnum(cur()) || cur() == '_')
            {
                name.push_back(cur());
                adv();
            }
            return get_variable(name, res);
        }
        else
            return err("express signed expression, got '%c'", op);
    }

    bool parse_term(soc_word_t& res)
    {
        if(!parse_signed(res))
            return false;
        while(cur() == '*' || cur() == '/' || cur() == '%')
        {
            char op = cur();
            adv();
            soc_word_t tmp;
            if(!parse_signed(tmp))
                return false;
            if(op == '*')
                res *= tmp;
            else if(tmp != 0)
                res = op == '/' ? res / tmp : res % tmp;
            else
                return err("division by 0");
        }
        return true;
    }

    bool parse_expression(soc_word_t& res)
    {
        if(!parse_term(res))
            return false;
        while(!end() && (cur() == '+' || cur() == '-'))
        {
            char op = cur();
            adv();
            soc_word_t tmp;
            if(!parse_term(tmp))
                return false;
            if(op == '+')
                res += tmp;
            else
                res -= tmp;
        }
        return true;
    }

    bool parse(soc_word_t& res, std::string& _error)
    {
        bool ok = parse_expression(res);
        if(ok && !end())
            err("unexpected character '%c'", cur());
        _error = error;
        return ok && end();
    }

    virtual bool get_variable(std::string name, soc_word_t& res)
    {
        return err("unknown variable '%s'", name.c_str());
    }
};

struct my_evaluator : public formula_evaluator
{
    const std::map< std::string, soc_word_t>& var;

    my_evaluator(const std::string& formula, const std::map< std::string, soc_word_t>& _var)
        :formula_evaluator(formula), var(_var) {}

    virtual bool get_variable(std::string name, soc_word_t& res)
    {
        std::map< std::string, soc_word_t>::const_iterator it = var.find(name);
        if(it == var.end())
            return formula_evaluator::get_variable(name, res);
        else
        {
            res = it->second;
            return true;
        }
    }
};

}

bool evaluate_formula(const std::string& formula,
    const std::map< std::string, soc_word_t>& var, soc_word_t& result, std::string& error)
{
    my_evaluator e(formula, var);
    return e.parse(result, error);
}

/** WARNING we need to call xmlInitParser() to init libxml2 but it needs to
 * called from the main thread, which is a super strong requirement, so do it
 * using a static constructor */
namespace
{
class xml_parser_init
{
public:
    xml_parser_init()
    {
        xmlInitParser();
    }
};

xml_parser_init __xml_parser_init;
}

} // soc_desc_v1
