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
#include "soc_desc.hpp"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/xmlwriter.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <limits>

namespace soc_desc
{

/**
 * Parser
 */

#define XML_CHAR_TO_CHAR(s) ((const char *)(s))

#define BEGIN_ATTR_MATCH(attr) \
    for(xmlAttr *a = attr; a; a = a->next) { \
        bool used = false;

#define MATCH_UNIQUE_ATTR(attr_name, val, has, parse_fn, ctx) \
    if(strcmp(XML_CHAR_TO_CHAR(a->name), attr_name) == 0) { \
        if(has) \
            return parse_not_unique_attr_error(a, ctx); \
        has = true; \
        xmlChar *str = NULL; \
        if(!parse_text_attr_internal(a, str, ctx) || !parse_fn(a, val, str, ctx)) \
            ret = false; \
        used = true; \
    }

#define MATCH_UNUSED_ATTR(parse_fn, ctx) \
    if(!used) { \
        ret = ret && parse_fn(a, ctx); \
    }

#define END_ATTR_MATCH() \
    }

#define BEGIN_NODE_MATCH(node) \
    for(xmlNode *sub = node; sub; sub = sub->next) { \
        bool used = false; \

#define MATCH_ELEM_NODE(node_name, array, parse_fn, ctx) \
    if(sub->type == XML_ELEMENT_NODE && strcmp(XML_CHAR_TO_CHAR(sub->name), node_name) == 0) { \
        array.resize(array.size() + 1); \
        if(!parse_fn(sub, array.back(), ctx)) \
            ret = false; \
        array.back().id = array.size(); \
        used = true; \
    }

#define MATCH_TEXT_NODE(node_name, array, parse_fn, ctx) \
    if(sub->type == XML_ELEMENT_NODE && strcmp(XML_CHAR_TO_CHAR(sub->name), node_name) == 0) { \
        if(!is_real_text_node(sub)) \
            return parse_not_text_error(sub, ctx); \
        xmlChar *content = xmlNodeGetContent(sub); \
        array.resize(array.size() + 1); \
        ret = ret &&  parse_fn(sub, array.back(), content, ctx); \
        xmlFree(content); \
        used = true; \
    }

#define MATCH_UNIQUE_ELEM_NODE(node_name, val, has, parse_fn, ctx) \
    if(sub->type == XML_ELEMENT_NODE && strcmp(XML_CHAR_TO_CHAR(sub->name), node_name) == 0) { \
        if(has) \
            return parse_not_unique_error(sub, ctx); \
        has = true; \
        if(!parse_fn(sub, val, ctx)) \
            ret = false; \
        used = true; \
    }

#define MATCH_UNIQUE_TEXT_NODE(node_name, val, has, parse_fn, ctx) \
    if(sub->type == XML_ELEMENT_NODE && strcmp(XML_CHAR_TO_CHAR(sub->name), node_name) == 0) { \
        if(has) \
            return parse_not_unique_error(sub, ctx); \
        if(!is_real_text_node(sub)) \
            return parse_not_text_error(sub, ctx); \
        has = true; \
        xmlChar *content = xmlNodeGetContent(sub); \
        ret = ret && parse_fn(sub, val, content, ctx); \
        xmlFree(content); \
        used = true; \
    }

#define MATCH_UNUSED_NODE(parse_fn, ctx) \
    if(!used) { \
        ret = ret && parse_fn(sub, ctx); \
    }

#define END_NODE_MATCH() \
    }

#define CHECK_HAS(node, node_name, has, ctx) \
    if(!has) \
        ret = ret && parse_missing_error(node, node_name, ctx);

#define CHECK_HAS_ATTR(node, attr_name, has, ctx) \
    if(!has) \
        ret = ret && parse_missing_attr_error(node, attr_name, ctx);

namespace
{

bool is_real_text_node(xmlNode *node)
{
    for(xmlNode *sub = node->children; sub; sub = sub->next)
        if(sub->type != XML_TEXT_NODE && sub->type != XML_ENTITY_REF_NODE)
            return false;
    return true;
}

std::string xml_loc(xmlNode *node)
{
    std::ostringstream oss;
    oss << "line " << node->line;
    return oss.str();
}

std::string xml_loc(xmlAttr *attr)
{
    return xml_loc(attr->parent);
}

template<typename T>
bool add_error(error_context_t& ctx, err_t::level_t lvl, T *node,
    const std::string& msg)
{
    ctx.add(err_t(lvl, xml_loc(node), msg));
    return false;
}

template<typename T>
bool add_fatal(error_context_t& ctx, T *node, const std::string& msg)
{
    return add_error(ctx, err_t::FATAL, node, msg);
}

template<typename T>
bool add_warning(error_context_t& ctx, T *node, const std::string& msg)
{
    return add_error(ctx, err_t::WARNING, node, msg);
}

bool parse_wrong_version_error(xmlNode *node, error_context_t& ctx)
{
    std::ostringstream oss;
    oss << "unknown version, only version " << MAJOR_VERSION << " is supported";
    return add_fatal(ctx, node, oss.str());
}

bool parse_not_unique_error(xmlNode *node, error_context_t& ctx)
{
    std::ostringstream oss;
    oss << "there must be a unique <" << XML_CHAR_TO_CHAR(node->name) << "> element";
    if(node->parent->name)
        oss << " in <" << XML_CHAR_TO_CHAR(node->parent->name) << ">";
    else
        oss << " at root level";
    return add_fatal(ctx, node, oss.str());
}

bool parse_not_unique_attr_error(xmlAttr *attr, error_context_t& ctx)
{
    std::ostringstream oss;
    oss << "there must be a unique " << XML_CHAR_TO_CHAR(attr->name) << " attribute";
    oss << " in <" << XML_CHAR_TO_CHAR(attr->parent->name) << ">";
    return add_fatal(ctx, attr, oss.str());
}

bool parse_missing_error(xmlNode *node, const char *name, error_context_t& ctx)
{
    std::ostringstream oss;
    oss << "missing <" << name << "> element";
    if(node->parent->name)
        oss << " in <" << XML_CHAR_TO_CHAR(node->parent->name) << ">";
    else
        oss << " at root level";
    return add_fatal(ctx, node, oss.str());
}

bool parse_missing_attr_error(xmlNode *node, const char *name, error_context_t& ctx)
{
    std::ostringstream oss;
    oss << "missing " << name << " attribute";
    oss << " in <" << XML_CHAR_TO_CHAR(node->name) << ">";
    return add_fatal(ctx, node, oss.str());
}

bool parse_conflict_error(xmlNode *node, const char *name1, const char *name2,
    error_context_t& ctx)
{
    std::ostringstream oss;
    oss << "conflicting <" << name1 << "> and <" << name2 << "> elements";
    if(node->parent->name)
        oss << " in <" << XML_CHAR_TO_CHAR(node->parent->name) << ">";
    else
        oss << " at root level";
    return add_fatal(ctx, node, oss.str());
}

bool parse_not_text_error(xmlNode *node, error_context_t& ctx)
{
    return add_fatal(ctx, node, "this is not a text element");
}

bool parse_not_text_attr_error(xmlAttr *attr, error_context_t& ctx)
{
    return add_fatal(ctx, attr, "this is not a text attribute");
}

bool parse_text_elem(xmlNode *node, std::string& name, xmlChar *content, error_context_t& ctx)
{
    name = XML_CHAR_TO_CHAR(content);
    return true;
}

bool parse_name_elem(xmlNode *node, std::string& name, xmlChar *content, error_context_t& ctx)
{
    name = XML_CHAR_TO_CHAR(content);
    if(name.size() == 0)
        return add_fatal(ctx, node, "name cannot be empty");
    for(size_t i = 0; i < name.size(); i++)
        if(!isalnum(name[i]) && name[i] != '_')
            return add_fatal(ctx, node, "name must only contain alphanumeric characters or _");
    return true;
}

bool parse_unknown_elem(xmlNode *node, error_context_t& ctx)
{
    /* ignore blank nodes */
    if(xmlIsBlankNode(node))
        return true;
    std::ostringstream oss;
    oss << "unknown <" << XML_CHAR_TO_CHAR(node->name) << "> element";
    return add_fatal(ctx, node, oss.str());
}

bool parse_access_elem(xmlNode *node, access_t& acc, xmlChar *content, error_context_t& ctx)
{
    const char *text = XML_CHAR_TO_CHAR(content);
    if(strcmp(text, "read-only") == 0)
        acc = READ_ONLY;
    else if(strcmp(text, "read-write") == 0)
        acc = READ_WRITE;
    else if(strcmp(text, "write-only") == 0)
        acc = WRITE_ONLY;
    else
        return add_fatal(ctx, node, "unknown access type " + std::string(text));
    return true;
}

template<typename T, typename U>
bool parse_unsigned_text(U *node, T& res, xmlChar *content, error_context_t& ctx)
{
    char *end;
    unsigned long uns = strtoul(XML_CHAR_TO_CHAR(content), &end, 0);
    if(*end != 0)
        return add_fatal(ctx, node, "content must be an unsigned integer");
    res = uns;
    if(res != uns)
        return add_fatal(ctx, node, "value does not fit into allowed range");
    return true;
}

template<typename T>
bool parse_unsigned_elem(xmlNode *node, T& res, xmlChar *content, error_context_t& ctx)
{
    return parse_unsigned_text(node, res, content, ctx);
}

template<typename T>
bool parse_unsigned_attr(xmlAttr *attr, T& res, xmlChar *content, error_context_t& ctx)
{
    return parse_unsigned_text(attr, res, content, ctx);
}

bool parse_text_attr_internal(xmlAttr *attr, xmlChar*& res, error_context_t& ctx)
{
    if(attr->children != attr->last)
        return false;
    if(attr->children->type != XML_TEXT_NODE)
        return parse_not_text_attr_error(attr, ctx);
    res = attr->children->content;
    return true;
}

bool parse_text_attr(xmlAttr *attr, std::string& res, xmlChar *content, error_context_t& ctx)
{
    res = XML_CHAR_TO_CHAR(content);
    return true;
}

bool parse_unknown_attr(xmlAttr *attr, error_context_t& ctx)
{
    std::ostringstream oss;
    oss << "unknown '" << XML_CHAR_TO_CHAR(attr->name) << "' attribute";
    return add_fatal(ctx, attr, oss.str());
}


bool parse_enum_elem(xmlNode *node, enum_t& reg, error_context_t& ctx)
{
    bool ret = true;
    bool has_name = false, has_value = false, has_desc = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("name", reg.name, has_name, parse_name_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("value", reg.value, has_value, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("desc", reg.desc, has_desc, parse_text_elem, ctx)
        MATCH_UNUSED_NODE(parse_unknown_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node, "name", has_name, ctx)
    CHECK_HAS(node, "value", has_value, ctx)
    return ret;
}

bool parse_field_elem(xmlNode *node, field_t& field, error_context_t& ctx)
{
    bool ret = true;
    bool has_name = false, has_pos = false, has_desc = false, has_width = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("name", field.name, has_name, parse_name_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("position", field.pos, has_pos, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("width", field.width, has_width, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("desc", field.desc, has_desc, parse_text_elem, ctx)
        MATCH_ELEM_NODE("enum", field.enum_, parse_enum_elem, ctx)
        MATCH_UNUSED_NODE(parse_unknown_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node, "name", has_name, ctx)
    CHECK_HAS(node, "position", has_pos, ctx)
    if(!has_width)
        field.width = 1;
    return ret;
}

bool parse_variant_elem(xmlNode *node, variant_t& variant, error_context_t& ctx)
{
    bool ret = true;
    bool has_type = false, has_offset = false, has_access = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("type", variant.type, has_type, parse_name_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("offset", variant.offset, has_offset, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("access", variant.access, has_access, parse_access_elem, ctx)
        MATCH_UNUSED_NODE(parse_unknown_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node, "type", has_type, ctx)
    CHECK_HAS(node, "offset", has_offset, ctx)
    if(!has_access)
        variant.access = UNSPECIFIED;
    return ret;
}

bool parse_register_elem(xmlNode *node, register_t& reg, error_context_t& ctx)
{
    bool ret = true;
    bool has_width = false, has_desc = false, has_access = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("desc", reg.desc, has_desc, parse_text_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("width", reg.width, has_width, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("access", reg.access, has_access, parse_access_elem, ctx)
        MATCH_ELEM_NODE("field", reg.field, parse_field_elem, ctx)
        MATCH_ELEM_NODE("variant", reg.variant, parse_variant_elem, ctx)
        MATCH_UNUSED_NODE(parse_unknown_elem, ctx)
    END_NODE_MATCH()
    if(!has_width)
        reg.width = 32;
    if(!has_access)
        reg.access = UNSPECIFIED;
    return ret;
}

bool parse_formula_elem(xmlNode *node, range_t& range, error_context_t& ctx)
{
    bool ret = true;
    bool has_var = false;
    BEGIN_ATTR_MATCH(node->properties)
        MATCH_UNIQUE_ATTR("variable", range.variable, has_var, parse_text_attr, ctx)
        MATCH_UNUSED_ATTR(parse_unknown_attr, ctx)
    END_NODE_MATCH()
    CHECK_HAS_ATTR(node, "variable", has_var, ctx)
    return ret;
}

bool parse_range_elem(xmlNode *node, range_t& range, error_context_t& ctx)
{
    bool ret = true;
    bool has_first = false, has_count = false, has_stride = false, has_base = false;
    bool has_formula = false, has_formula_attr = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("first", range.first, has_first, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("count", range.count, has_count, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("base", range.base, has_base, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("stride", range.stride, has_stride, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_ELEM_NODE("formula", range, has_formula_attr, parse_formula_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("formula", range.formula, has_formula, parse_text_elem, ctx)
        MATCH_TEXT_NODE("address", range.list, parse_unsigned_elem, ctx)
        MATCH_UNUSED_NODE(parse_unknown_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node, "first", has_first, ctx)
    if(range.list.size() == 0)
    {
        CHECK_HAS(node, "count", has_count, ctx)
        if(!has_base && !has_formula)
            ret = ret && parse_missing_error(node, "base> or <formula", ctx);
        if(has_base && has_formula)
            return parse_conflict_error(node, "base", "formula", ctx);
        if(has_base)
            CHECK_HAS(node, "stride", has_stride, ctx)
        if(has_stride && !has_base)
            ret = ret && parse_conflict_error(node, "stride", "formula", ctx);
        if(has_stride)
            range.type = range_t::STRIDE;
        else
            range.type = range_t::FORMULA;
    }
    else
    {
        if(has_base)
            ret = ret && parse_conflict_error(node, "base", "addr", ctx);
        if(has_count)
            ret = ret && parse_conflict_error(node, "count", "addr", ctx);
        if(has_formula)
            ret = ret && parse_conflict_error(node, "formula", "addr", ctx);
        if(has_stride)
            ret = ret && parse_conflict_error(node, "stride", "addr", ctx);
        range.type = range_t::LIST;
    }
    return ret;
}

bool parse_instance_elem(xmlNode *node, instance_t& inst, error_context_t& ctx)
{
    bool ret = true;
    bool has_name = false, has_title = false, has_desc = false, has_range = false;
    bool has_address = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("name", inst.name, has_name, parse_name_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("title", inst.title, has_title, parse_text_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("desc", inst.desc, has_desc, parse_text_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("address", inst.addr, has_address, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_ELEM_NODE("range", inst.range, has_range, parse_range_elem, ctx)
        MATCH_UNUSED_NODE(parse_unknown_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node, "name", has_name, ctx)
    if(!has_address && !has_range)
        ret = ret && parse_missing_error(node, "address> or <range", ctx);
    if(has_address && has_range)
        ret = ret && parse_conflict_error(node, "address", "range", ctx);
    if(has_address)
        inst.type = instance_t::SINGLE;
    else
        inst.type = instance_t::RANGE;
    return ret;
}

bool parse_node_elem(xmlNode *node_, node_t& node, error_context_t& ctx)
{
    bool ret = true;
    register_t reg;
    bool has_title = false, has_desc = false, has_register = false, has_name = false;
    BEGIN_NODE_MATCH(node_->children)
        MATCH_UNIQUE_TEXT_NODE("name", node.name, has_name, parse_name_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("title", node.title, has_title, parse_text_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("desc", node.desc, has_desc, parse_text_elem, ctx)
        MATCH_UNIQUE_ELEM_NODE("register", reg, has_register, parse_register_elem, ctx)
        MATCH_ELEM_NODE("node", node.node, parse_node_elem, ctx)
        MATCH_ELEM_NODE("instance", node.instance, parse_instance_elem, ctx)
        MATCH_UNUSED_NODE(parse_unknown_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node_, "name", has_name, ctx)
    if(has_register)
        node.register_.push_back(reg);
    return ret;
}

bool parse_soc_elem(xmlNode *node, soc_t& soc, error_context_t& ctx)
{
    bool ret = true;
    bool has_name = false, has_title = false, has_desc = false, has_version = false;
    bool has_isa = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("name", soc.name, has_name, parse_name_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("title", soc.title, has_title, parse_text_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("desc", soc.desc, has_desc, parse_text_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("version", soc.version, has_version, parse_text_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("isa", soc.isa, has_isa, parse_text_elem, ctx)
        MATCH_TEXT_NODE("author", soc.author, parse_text_elem, ctx)
        MATCH_ELEM_NODE("node", soc.node, parse_node_elem, ctx)
        MATCH_UNUSED_NODE(parse_unknown_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node, "name", has_name, ctx)
    return ret;
}

bool parse_root_elem(xmlNode *node, soc_t& soc, error_context_t& ctx)
{
    size_t ver = 0;
    bool ret = true;
    bool has_soc = false, has_version = false;
    BEGIN_ATTR_MATCH(node->properties)
        MATCH_UNIQUE_ATTR("version", ver, has_version, parse_unsigned_attr, ctx)
        MATCH_UNUSED_ATTR(parse_unknown_attr, ctx)
    END_ATTR_MATCH()
    if(!has_version)
    {
        ctx.add(err_t(err_t::FATAL, xml_loc(node), "no version attribute, is this a v1 file ?"));
        return false;
    }
    if(ver != MAJOR_VERSION)
        return parse_wrong_version_error(node, ctx);
    BEGIN_NODE_MATCH(node)
        MATCH_UNIQUE_ELEM_NODE("soc", soc, has_soc, parse_soc_elem, ctx)
        MATCH_UNUSED_NODE(parse_unknown_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node, "soc", has_soc, ctx)
    return ret;
}

}

bool parse_xml(const std::string& filename, soc_t& soc,
    error_context_t& error_ctx)
{
    LIBXML_TEST_VERSION

    xmlDocPtr doc = xmlReadFile(filename.c_str(), NULL, 0);
    if(doc == NULL)
        return false;

    xmlNodePtr root_element = xmlDocGetRootElement(doc);
    bool ret = parse_root_elem(root_element, soc, error_ctx);

    xmlFreeDoc(doc);

    return ret;
}

/**
 * Normalizer
 */

namespace
{

struct soc_sorter
{
    /* returns the lowest address of an instance, or 0 if none
     * and 0xffffffff if cannot evaluate */
    soc_addr_t first_addr(const instance_t& inst) const
    {
        if(inst.type == instance_t::SINGLE)
            return inst.addr;
        /* sanity check */
        if(inst.type != instance_t::RANGE)
        {
            printf("Warning: unknown instance type %d\n", inst.type);
            return 0;
        }
        if(inst.range.type == range_t::STRIDE)
            return inst.range.base; /* assume positive stride */
        if(inst.range.type == range_t::LIST)
        {
            soc_addr_t min = 0xffffffff;
            for(size_t i = 0; i < inst.range.list.size(); i++)
                if(inst.range.list[i] < min)
                    min = inst.range.list[i];
            return min;
        }
        /* sanity check */
        if(inst.range.type != range_t::FORMULA)
        {
            printf("Warning: unknown range type %d\n", inst.range.type);
            return 0;
        }
        soc_addr_t min = 0xffffffff;
        std::map< std::string, soc_word_t > vars;
        for(size_t i = 0; i < inst.range.count; i++)
        {
            soc_word_t res;
            vars[inst.range.variable] = inst.range.first;
            error_context_t ctx;
            if(evaluate_formula(inst.range.formula, vars, res, "", ctx) && res < min)
                min = res;
        }
        return min;
    }

    /* return smallest address among all instances */
    soc_addr_t first_addr(const node_t& node) const
    {
        soc_addr_t min = 0xffffffff;
        for(size_t i = 0; i < node.instance.size(); i++)
            min = std::min(min, first_addr(node.instance[i]));
        return min;
    }

    /* sort instances by first address */
    bool operator()(const instance_t& a, const instance_t& b) const
    {
        return first_addr(a) < first_addr(b);
    }

    /* sort nodes by first address of first instance (which is the lowest of
     * any instance if instances are sorted) */
    bool operator()(const node_t& a, const node_t& b) const
    {
        soc_addr_t addr_a = first_addr(a);
        soc_addr_t addr_b = first_addr(b);
        /* It may happen that two nodes have the same first instance address,
         * for example if one logically splits a block into two blocks with
         * the same base. In this case, sort by name */
        if(addr_a == addr_b)
            return a.name < b.name;
        return addr_a < addr_b;
    }

    /* sort fields by decreasing position */
    bool operator()(const field_t& a, const field_t& b) const
    {
        /* in the unlikely case where two fields have the same position, use name */
        if(a.pos == b.pos)
            return a.name < b.name;
        return a.pos > b.pos;
    }

    /* sort enum values by value, then by name */
    bool operator()(const enum_t& a, const enum_t& b) const
    {
        if(a.value == b.value)
            return a.name < b.name;
        return a.value < b.value;
    }
};

void normalize(field_t& field)
{
    std::sort(field.enum_.begin(), field.enum_.end(), soc_sorter());
}

void normalize(register_t& reg)
{
    for(size_t i = 0; i < reg.field.size(); i++)
        normalize(reg.field[i]);
    std::sort(reg.field.begin(), reg.field.end(), soc_sorter());
}

void normalize(node_t& node)
{
    for(size_t i = 0; i < node.register_.size(); i++)
        normalize(node.register_[i]);
    for(size_t i = 0; i < node.node.size(); i++)
        normalize(node.node[i]);
    std::sort(node.node.begin(), node.node.end(), soc_sorter());
    std::sort(node.instance.begin(), node.instance.end(), soc_sorter());
}

} /* namespace */

void normalize(soc_t& soc)
{
    for(size_t i = 0; i < soc.node.size(); i++)
        normalize(soc.node[i]);
    std::sort(soc.node.begin(), soc.node.end(), soc_sorter());
}

/**
 * Producer
 */

namespace
{

#define SAFE(x) \
    do{ \
        if((x) < 0) { \
            std::ostringstream oss; \
            oss << __FILE__ << ":" << __LINE__; \
            ctx.add(err_t(err_t::FATAL, oss.str(), "write error")); \
            return -1; \
        } \
    }while(0)

int produce_range(xmlTextWriterPtr writer, const range_t& range, error_context_t& ctx)
{
    /* <range> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "range"));
    /* <first/> */
    SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "first", "%lu", range.first));
    if(range.type == range_t::STRIDE)
    {
        /* <count/> */
        SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "count", "%lu", range.count));
        /* <base/> */
        SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "base", "0x%x", range.base));
        /* <stride/> */
        SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "stride", "0x%x", range.stride));
    }
    /* <formula> */
    else if(range.type == range_t::FORMULA)
    {
        /* <count/> */
        SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "count", "%lu", range.count));
        /* <formula> */
        SAFE(xmlTextWriterStartElement(writer, BAD_CAST "formula"));
        /* variable */
        SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "variable", BAD_CAST range.variable.c_str()));
        /* content */
        SAFE(xmlTextWriterWriteString(writer, BAD_CAST range.formula.c_str()));
        /* </formula> */
        SAFE(xmlTextWriterEndElement(writer));
    }
    else if(range.type == range_t::LIST)
    {
        for(size_t i = 0; i < range.list.size(); i++)
            SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "address", "0x%x", range.list[i]));
    }
    /* </range> */
    SAFE(xmlTextWriterEndElement(writer));

    return 0;
}

int produce_instance(xmlTextWriterPtr writer, const instance_t& inst, error_context_t& ctx)
{
    /* <instance> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "instance"));
    /* <name/> */
    SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "name", BAD_CAST inst.name.c_str()));
    /* <title/> */
    if(!inst.title.empty())
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "title", BAD_CAST inst.title.c_str()));
    /* <desc/> */
    if(!inst.desc.empty())
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "desc", BAD_CAST inst.desc.c_str()));
    /* <address/> */
    if(inst.type == instance_t::SINGLE)
        SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "address", "0x%x", inst.addr));
    /* <range/> */
    else if(inst.type == instance_t::RANGE)
        SAFE(produce_range(writer, inst.range, ctx));
    /* </instance> */
    SAFE(xmlTextWriterEndElement(writer));
    return 0;
}

int produce_enum(xmlTextWriterPtr writer, const enum_t& enum_, error_context_t& ctx)
{
    /* <enum> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "enum"));
    /* <name/> */
    SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "name", BAD_CAST enum_.name.c_str()));
    /* <desc/> */
    if(!enum_.desc.empty())
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "desc", BAD_CAST enum_.desc.c_str()));
    /* <value/> */
    SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "value", "0x%x", enum_.value));
    /* </enum> */
    SAFE(xmlTextWriterEndElement(writer));
    return 0;
}

int produce_field(xmlTextWriterPtr writer, const field_t& field, error_context_t& ctx)
{
    /* <field> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "field"));
    /* <name/> */
    SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "name", BAD_CAST field.name.c_str()));
    /* <desc/> */
    if(!field.desc.empty())
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "desc", BAD_CAST field.desc.c_str()));
    /* <position/> */
    SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "position", "%lu", field.pos));
    /* <width/> */
    if(field.width != 1)
        SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "width", "%lu", field.width));
    /* enums */
    for(size_t i = 0; i < field.enum_.size(); i++)
        SAFE(produce_enum(writer, field.enum_[i], ctx));
    /* </field> */
    SAFE(xmlTextWriterEndElement(writer));
    return 0;
}

const char *access_string(access_t acc)
{
    switch(acc)
    {
        case READ_ONLY: return "read-only";
        case READ_WRITE: return "read-write";
        case WRITE_ONLY: return "write-only";
        default: return "bug-invalid-access";
    }
}

int produce_variant(xmlTextWriterPtr writer, const variant_t& variant, error_context_t& ctx)
{
    /* <variant> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "variant"));
    /* <name/> */
    SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "type", BAD_CAST variant.type.c_str()));
    /* <position/> */
    SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "offset", "%lu", (unsigned long)variant.offset));
    /* <access/> */
    if(variant.access != UNSPECIFIED)
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "access", BAD_CAST access_string(variant.access)));
    /* </variant> */
    SAFE(xmlTextWriterEndElement(writer));
    return 0;
}

int produce_register(xmlTextWriterPtr writer, const register_t& reg, error_context_t& ctx)
{
    /* <register> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "register"));
    /* <width/> */
    if(reg.width != 32)
        SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "width", "%lu", reg.width));
    /* <desc/> */
    if(!reg.desc.empty())
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "desc", BAD_CAST reg.desc.c_str()));
    /* <access/> */
    if(reg.access != UNSPECIFIED)
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "access", BAD_CAST access_string(reg.access)));
    /* fields */
    for(size_t i = 0; i < reg.field.size(); i++)
        SAFE(produce_field(writer, reg.field[i], ctx));
    /* variants */
    for(size_t i = 0; i < reg.variant.size(); i++)
        SAFE(produce_variant(writer, reg.variant[i], ctx));
    /* </register> */
    SAFE(xmlTextWriterEndElement(writer));
    return 0;
}

int produce_node(xmlTextWriterPtr writer, const node_t& node, error_context_t& ctx)
{
    /* <node> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "node"));
    /* <name/> */
    SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "name", BAD_CAST node.name.c_str()));
    /* <title/> */
    if(!node.title.empty())
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "title", BAD_CAST node.title.c_str()));
    /* <desc/> */
    if(!node.desc.empty())
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "desc", BAD_CAST node.desc.c_str()));
    /* instances */
    for(size_t i = 0; i < node.instance.size(); i++)
        SAFE(produce_instance(writer, node.instance[i], ctx));
    /* register */
    for(size_t i = 0; i < node.register_.size(); i++)
        SAFE(produce_register(writer, node.register_[i], ctx));
    /* nodes */
    for(size_t i = 0; i < node.node.size(); i++)
        SAFE(produce_node(writer, node.node[i], ctx));
    /* </node> */
    SAFE(xmlTextWriterEndElement(writer));
    return 0;
}

#undef SAFE

}

bool produce_xml(const std::string& filename, const soc_t& soc, error_context_t& ctx)
{
    LIBXML_TEST_VERSION

    std::ostringstream oss;
    xmlTextWriterPtr writer = xmlNewTextWriterFilename(filename.c_str(), 0);
    if(writer == NULL)
        return false;
#define SAFE(x) do{if((x) < 0) goto  Lerr;}while(0)
    SAFE(xmlTextWriterSetIndent(writer, 1));
    SAFE(xmlTextWriterSetIndentString(writer, BAD_CAST "    "));
    /* <xml> */
    SAFE(xmlTextWriterStartDocument(writer, NULL, NULL, NULL));
    /* <soc> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "soc"));
    /* version */
    oss << MAJOR_VERSION;
    SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "version", BAD_CAST oss.str().c_str()));
    /* <name/> */
    SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "name", BAD_CAST soc.name.c_str()));
    /* <title/> */
    if(!soc.title.empty())
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "title", BAD_CAST soc.title.c_str()));
    /* <desc/> */
    if(!soc.desc.empty())
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "desc", BAD_CAST soc.desc.c_str()));
    /* <author/> */
    for(size_t i = 0; i < soc.author.size(); i++)
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "author", BAD_CAST soc.author[i].c_str()));
    /* <isa/> */
    if(!soc.isa.empty())
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "isa", BAD_CAST soc.isa.c_str()));
    /* <version/> */
    if(!soc.version.empty())
        SAFE(xmlTextWriterWriteElement(writer, BAD_CAST "version", BAD_CAST soc.version.c_str()));
    /* nodes */
    for(size_t i = 0; i < soc.node.size(); i++)
        SAFE(produce_node(writer, soc.node[i], ctx));
    /* </soc> */
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

/**
 * utils
 */

namespace
{

template< typename T >
soc_id_t gen_fresh_id(const std::vector< T >& list)
{
    soc_id_t id = 0;
    for(size_t i = 0; i < list.size(); i++)
        id = std::max(id, list[i].id);
    return id + 1;
}

}

/**
 * soc_ref_t
 */

soc_ref_t::soc_ref_t():m_soc(0)
{
}

soc_ref_t::soc_ref_t(soc_t *soc):m_soc(soc)
{
}

bool soc_ref_t::valid() const
{
    return get() != 0;
}

soc_t *soc_ref_t::get()  const
{
    return m_soc;
}

bool soc_ref_t::operator==(const soc_ref_t& ref) const
{
    return m_soc == ref.m_soc;
}

node_ref_t soc_ref_t::root() const
{
    return node_ref_t(*this);
}

node_inst_t soc_ref_t::root_inst() const
{
    return node_inst_t(*this);
}

void soc_ref_t::reset()
{
    m_soc = 0;
}

/**
 * node_ref_t */

node_ref_t::node_ref_t(soc_ref_t soc):m_soc(soc)
{
}

node_ref_t::node_ref_t(soc_ref_t soc, const std::vector< soc_id_t >& path)
    :m_soc(soc), m_path(path)
{
}

node_ref_t::node_ref_t()
{
}

bool node_ref_t::valid() const
{
    return (m_soc.valid() && is_root()) || get() != 0;
}

bool node_ref_t::is_root() const
{
    return m_path.empty();
}

void node_ref_t::reset()
{
    m_soc.reset();
}

namespace
{

std::vector< node_t > *get_children(node_ref_t node)
{
    if(node.is_root())
        return node.soc().valid() ? &node.soc().get()->node : 0;
    node_t *n = node.get();
    return n == 0 ? 0 : &n->node;
}

node_t *get_child(std::vector< node_t > *nodes, soc_id_t id)
{
    if(nodes == 0)
        return 0;
    for(size_t i = 0; i < nodes->size(); i++)
        if((*nodes)[i].id == id)
            return &(*nodes)[i];
    return 0;
}

node_t *get_child(std::vector< node_t > *nodes, const std::string& name)
{
    if(nodes == 0)
        return 0;
    for(size_t i = 0; i < nodes->size(); i++)
        if((*nodes)[i].name == name)
            return &(*nodes)[i];
    return 0;
}

}

/* NOTE: valid() is implemented using get() != 0, so don't use it in get() ! */
node_t *node_ref_t::get() const
{
    if(!soc().valid())
        return 0;
    /* we could do it recursively but it would make plenty of copies */
    node_t *n = 0;
    std::vector< node_t > *nodes = &soc().get()->node;
    for(size_t i = 0; i < m_path.size(); i++)
    {
        n = get_child(nodes, m_path[i]);
        if(n == 0)
            return 0;
        nodes = &n->node;
    }
    return n;
}

soc_ref_t node_ref_t::soc() const
{
    return m_soc;
}

node_ref_t node_ref_t::parent(unsigned level) const
{
    if(level > depth())
        return node_ref_t();
    std::vector< soc_id_t > path = m_path;
    path.resize(depth() - level);
    return node_ref_t(m_soc, path);
}

unsigned node_ref_t::depth() const
{
    return m_path.size();
}

register_ref_t node_ref_t::reg() const
{
    node_t *n = get();
    if(n == 0)
        return register_ref_t();
    if(n->register_.empty())
        return parent().reg();
    else
        return register_ref_t(*this);
}

register_ref_t node_ref_t::create_reg(size_t width) const
{
    node_t *n = get();
    if(n == 0)
        return register_ref_t();
    if(!n->register_.empty())
        return register_ref_t();
    n->register_.resize(1);
    n->register_[0].width = width;
    return register_ref_t(*this);
}

node_ref_t node_ref_t::child(const std::string& name) const
{
    /* check the node exists */
    node_t *n = get_child(get_children(*this), name);
    if(n == 0)
        return node_ref_t();
    std::vector< soc_id_t > path = m_path;
    path.push_back(n->id);
    return node_ref_t(m_soc, path);
}

std::vector< node_ref_t > node_ref_t::children() const
{
    std::vector< node_ref_t > nodes;
    std::vector< node_t > *children = get_children(*this);
    if(children == 0)
        return nodes;
    for(size_t i = 0; i < children->size(); i++)
    {
        std::vector< soc_id_t > path = m_path;
        path.push_back((*children)[i].id);
        nodes.push_back(node_ref_t(m_soc, path));
    }
    return nodes;
}

std::vector< std::string > node_ref_t::path() const
{
    std::vector< std::string > path;
    if(!soc().valid())
        return path;
    /* we could do it recursively but this is more efficient */
    node_t *n = 0;
    std::vector< node_t > *nodes = &soc().get()->node;
    for(size_t i = 0; i < m_path.size(); i++)
    {
        n = get_child(nodes, m_path[i]);
        if(n == 0)
        {
            path.clear();
            return path;
        }
        path.push_back(n->name);
        nodes = &n->node;
    }
    return path;
}

std::string node_ref_t::name() const
{
    node_t *n = get();
    return n == 0 ? "" : n->name;
}

bool node_ref_t::operator==(const node_ref_t& ref) const
{
    return m_soc == ref.m_soc && m_path == ref.m_path;
}

void node_ref_t::remove()
{
    if(is_root())
    {
        soc_t *s = soc().get();
        if(s)
            s->node.clear();
    }
    else
    {
        std::vector< node_t > *list = get_children(parent());
        if(list == 0)
            return;
        for(size_t i = 0; i < list->size(); i++)
            if((*list)[i].id == m_path.back())
            {
                list->erase(list->begin() + i);
                return;
            }
    }
}

node_ref_t node_ref_t::create() const
{
    std::vector< node_t > *list = get_children(*this);
    if(list == 0)
        return node_ref_t();
    node_t n;
    n.id = gen_fresh_id(*list);
    list->push_back(n);
    std::vector< soc_id_t > path = m_path;
    path.push_back(n.id);
    return node_ref_t(soc(), path);
}

/**
 * register_ref_t
 */

register_ref_t::register_ref_t(node_ref_t node)
    :m_node(node)
{
}

register_ref_t::register_ref_t()
{
}

bool register_ref_t::valid() const
{
    return get() != 0;
}

void register_ref_t::reset()
{
    m_node.reset();
}

register_t *register_ref_t::get() const
{
    node_t *n = m_node.get();
    if(n == 0 || n->register_.empty())
        return 0;
    return &n->register_[0];
}

node_ref_t register_ref_t::node() const
{
    return m_node;
}

std::vector< field_ref_t > register_ref_t::fields() const
{
    std::vector< field_ref_t > fields;
    register_t *r = get();
    if(r == 0)
        return fields;
    for(size_t i = 0; i < r->field.size(); i++)
        fields.push_back(field_ref_t(*this, r->field[i].id));
    return fields;
}

std::vector< variant_ref_t > register_ref_t::variants() const
{
    std::vector< variant_ref_t > variants;
    register_t *r = get();
    if(r == 0)
        return variants;
    for(size_t i = 0; i < r->variant.size(); i++)
        variants.push_back(variant_ref_t(*this, r->variant[i].id));
    return variants;
}

field_ref_t register_ref_t::field(const std::string& name) const
{
    register_t *r = get();
    if(r == 0)
        return field_ref_t();
    for(size_t i = 0; i < r->field.size(); i++)
        if(r->field[i].name == name)
            return field_ref_t(*this, r->field[i].id);
    return field_ref_t();
}

variant_ref_t register_ref_t::variant(const std::string& type) const
{
    register_t *r = get();
    if(r == 0)
        return variant_ref_t();
    for(size_t i = 0; i < r->variant.size(); i++)
        if(r->variant[i].type == type)
            return variant_ref_t(*this, r->variant[i].id);
    return variant_ref_t();
}

void register_ref_t::remove()
{
    node_t *n = node().get();
    if(n)
        n->register_.clear();
}

field_ref_t register_ref_t::create_field() const
{
    register_t *r = get();
    if(r == 0)
        return field_ref_t();
    field_t f;
    f.id = gen_fresh_id(r->field);
    r->field.push_back(f);
    return field_ref_t(*this, f.id);
}

variant_ref_t register_ref_t::create_variant() const
{
    register_t *r = get();
    if(r == 0)
        return variant_ref_t();
    variant_t v;
    v.id = gen_fresh_id(r->variant);
    r->variant.push_back(v);
    return variant_ref_t(*this, v.id);
}

/**
 * field_ref_t
 */

field_ref_t::field_ref_t(register_ref_t reg, soc_id_t id)
    :m_reg(reg), m_id(id)
{
}

field_ref_t::field_ref_t()
{
}

bool field_ref_t::valid() const
{
    return get() != 0;
}

void field_ref_t::reset()
{
    m_reg.reset();
}

field_t *field_ref_t::get() const
{
    register_t *reg = m_reg.get();
    if(reg == 0)
        return 0;
    for(size_t i = 0; i < reg->field.size(); i++)
        if(reg->field[i].id == m_id)
            return &reg->field[i];
    return 0;
}

std::vector< enum_ref_t > field_ref_t::enums() const
{
    std::vector< enum_ref_t > enums;
    field_t *f = get();
    if(f == 0)
        return enums;
    for(size_t i = 0; i < f->enum_.size(); i++)
        enums.push_back(enum_ref_t(*this, f->enum_[i].id));
    return enums;
}

register_ref_t field_ref_t::reg() const
{
    return m_reg;
}

enum_ref_t field_ref_t::create_enum() const
{
    field_t *f = get();
    if(f == 0)
        return enum_ref_t();
    enum_t e;
    e.id = gen_fresh_id(f->enum_);
    f->enum_.push_back(e);
    return enum_ref_t(*this, e.id);
}

/**
 * enum_ref_t
 */

enum_ref_t::enum_ref_t(field_ref_t field, soc_id_t id)
    :m_field(field), m_id(id)
{
}

enum_ref_t::enum_ref_t()
{
}

bool enum_ref_t::valid() const
{
    return get() != 0;
}

void enum_ref_t::reset()
{
    m_field.reset();
}

enum_t *enum_ref_t::get() const
{
    field_t *field = m_field.get();
    if(field == 0)
        return 0;
    for(size_t i = 0; i < field->enum_.size(); i++)
        if(field->enum_[i].id == m_id)
            return &field->enum_[i];
    return 0;
}

field_ref_t enum_ref_t::field() const
{
    return m_field;
}

/**
 * variant_ref_t
 */

variant_ref_t::variant_ref_t(register_ref_t reg, soc_id_t id)
    :m_reg(reg), m_id(id)
{
}

variant_ref_t::variant_ref_t()
{
}

bool variant_ref_t::valid() const
{
    return get() != 0;
}

void variant_ref_t::reset()
{
    m_reg.reset();
}

variant_t *variant_ref_t::get() const
{
    register_t *reg = m_reg.get();
    if(reg == 0)
        return 0;
    for(size_t i = 0; i < reg->variant.size(); i++)
        if(reg->variant[i].id == m_id)
            return &reg->variant[i];
    return 0;
}

register_ref_t variant_ref_t::reg() const
{
    return m_reg;
}

std::string variant_ref_t::type() const
{
    variant_t *v = get();
    return v ? v->type : std::string();
}

soc_word_t variant_ref_t::offset() const
{
    variant_t *v = get();
    return v ? v->offset : 0;
}

/**
 * node_inst_t
 */

namespace
{

const size_t INST_NO_INDEX = std::numeric_limits<std::size_t>::max();

bool get_inst_addr(range_t& range, size_t index, soc_addr_t& addr)
{
    if(index < range.first || index >= range.first + range.size())
                return false;
    switch(range.type)
    {
        case range_t::STRIDE:
            addr += range.base + (index - range.first) * range.stride;
            return true;
        case range_t::FORMULA:
        {
            soc_word_t res;
            std::map< std::string, soc_word_t > vars;
            vars[range.variable] = index;
            error_context_t ctx;
            if(!evaluate_formula(range.formula, vars, res, "", ctx))
                return false;
            addr += res;
            return true;
        }
        case range_t::LIST:
            addr += range.list[index - range.first];
            return true;
        default:
            return false;
    }
}

bool get_inst_addr(instance_t *inst, size_t index, soc_addr_t& addr)
{
    if(inst == 0)
        return false;
    switch(inst->type)
    {
        case instance_t::SINGLE:
            if(index != INST_NO_INDEX)
                return false;
            addr += inst->addr;
            return true;
        case instance_t::RANGE:
            if(index == INST_NO_INDEX)
                return false;
            return get_inst_addr(inst->range, index, addr);
        default:
            return false;
    }
}

}

node_inst_t::node_inst_t(soc_ref_t soc)
    :m_node(soc.root())
{
}

node_inst_t::node_inst_t(node_ref_t node, const std::vector< soc_id_t >& ids,
        const std::vector< size_t >& indexes)
    :m_node(node), m_id_path(ids), m_index_path(indexes)
{
}

node_inst_t::node_inst_t()
{
}

bool node_inst_t::valid() const
{
    return (is_root() && node().valid()) || get() != 0;
}

void node_inst_t::reset()
{
    m_node.reset();
}

node_ref_t node_inst_t::node() const
{
    return m_node;
}

soc_ref_t node_inst_t::soc() const
{
    return m_node.soc();
}

bool node_inst_t::is_root() const
{
    return m_node.is_root();
}

node_inst_t node_inst_t::parent(unsigned level) const
{
    if(level > depth())
        return node_inst_t();
    std::vector< soc_id_t > ids = m_id_path;
    std::vector< size_t > indexes = m_index_path;
    ids.resize(depth() - level);
    indexes.resize(depth() - level);
    return node_inst_t(m_node.parent(level), ids, indexes);
}

unsigned node_inst_t::depth() const
{
    return m_id_path.size();
}

instance_t *node_inst_t::get() const
{
    node_t *n = m_node.get();
    if(n == 0)
        return 0;
    for(size_t i = 0; i < n->instance.size(); i++)
        if(n->instance[i].id == m_id_path.back())
            return &n->instance[i];
    return 0;
}

soc_addr_t node_inst_t::addr() const
{
    if(!valid() || is_root())
        return 0;
    soc_addr_t addr = parent().addr();
    if(!get_inst_addr(get(), m_index_path.back(), addr))
        return 0;
    return addr;
}

node_inst_t node_inst_t::child(const std::string& name) const
{
    return child(name, INST_NO_INDEX);
}

node_inst_t node_inst_t::child(const std::string& name, size_t index) const
{
    std::vector< node_t > *nodes = get_children(m_node);
    if(nodes == 0)
        return node_inst_t();
    node_ref_t child_node = m_node;
    for(size_t i = 0; i < nodes->size(); i++)
    {
        node_t& node = (*nodes)[i];
        child_node.m_path.push_back(node.id);
        for(size_t j = 0; j < node.instance.size(); j++)
        {
            if(node.instance[j].name != name)
                continue;
            std::vector< soc_id_t > ids = m_id_path;
            std::vector< size_t > indexes = m_index_path;
            ids.push_back(node.instance[j].id);
            indexes.push_back(index);
            return node_inst_t(child_node, ids, indexes);
        }
        child_node.m_path.pop_back();
    }
    return node_inst_t();
}

std::vector< node_inst_t > node_inst_t::children() const
{
    std::vector< node_inst_t > list;
    std::vector< node_t > *nodes = get_children(m_node);
    std::vector< soc_id_t > n_path = m_id_path;
    std::vector< size_t > i_path = m_index_path;
    if(nodes == 0)
        return list;
    node_ref_t child_node = m_node;
    for(size_t i = 0; i < nodes->size(); i++)
    {
        node_t& node = (*nodes)[i];
        child_node.m_path.push_back(node.id);
        for(size_t j = 0; j < node.instance.size(); j++)
        {
            instance_t& inst = node.instance[j];
            n_path.push_back(inst.id);
            switch(inst.type)
            {
                case instance_t::SINGLE:
                    i_path.push_back(INST_NO_INDEX);
                    list.push_back(node_inst_t(child_node, n_path, i_path));
                    i_path.pop_back();
                    break;
                case instance_t::RANGE:
                    for(size_t i = 0; i < inst.range.size(); i++)
                    {
                        i_path.push_back(inst.range.first + i);
                        list.push_back(node_inst_t(child_node, n_path, i_path));
                        i_path.pop_back();
                    }
                    break;
                default:
                    break;
            }
            n_path.pop_back();
        }
        child_node.m_path.pop_back();
    }
    return list;
}

std::string node_inst_t::name() const
{
    instance_t *inst = get();
    return inst == 0 ? "" : inst->name;
}

bool node_inst_t:: is_indexed() const
{
    return !m_index_path.empty() && m_index_path.back() != INST_NO_INDEX;
}

size_t node_inst_t::index() const
{
    return m_index_path.empty() ? INST_NO_INDEX : m_index_path.back();
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

