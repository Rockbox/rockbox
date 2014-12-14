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

namespace soc_desc
{

#define XML_CHAR_TO_CHAR(s) ((const char *)(s))

#define BEGIN_NODE_MATCH(node) \
    for(xmlNode *sub = node; sub; sub = sub->next) {

#define MATCH_ELEM_NODE(node_name, array, parse_fn, ctx) \
    if(sub->type == XML_ELEMENT_NODE && strcmp(XML_CHAR_TO_CHAR(sub->name), node_name) == 0) { \
        array.resize(array.size() + 1); \
        if(!parse_fn(sub, array.back(), ctx)) \
            return false; \
    }

#define MATCH_TEXT_NODE(node_name, array, parse_fn, ctx) \
    if(sub->type == XML_ELEMENT_NODE && strcmp(XML_CHAR_TO_CHAR(sub->name), node_name) == 0) { \
        if(!is_real_text_node(sub)) \
            return parse_not_text_error(sub, ctx); \
        xmlChar *content = xmlNodeGetContent(sub); \
        array.resize(array.size() + 1); \
        bool ret = parse_fn(sub, array.back(), content, ctx); \
        xmlFree(content); \
        if(!ret) \
            return false; \
    }

#define MATCH_UNIQUE_ELEM_NODE(node_name, val, has, parse_fn, ctx) \
    if(sub->type == XML_ELEMENT_NODE && strcmp(XML_CHAR_TO_CHAR(sub->name), node_name) == 0) { \
        if(has) \
            return parse_not_unique_error(sub, ctx); \
        has = true; \
        if(!parse_fn(sub, val, ctx)) \
            return false; \
    }

#define MATCH_UNIQUE_TEXT_NODE(node_name, val, has, parse_fn, ctx) \
    if(sub->type == XML_ELEMENT_NODE && strcmp(XML_CHAR_TO_CHAR(sub->name), node_name) == 0) { \
        if(has) \
            return parse_not_unique_error(sub, ctx); \
        if(!is_real_text_node(sub)) \
            return parse_not_text_error(sub, ctx); \
        has = true; \
        xmlChar *content = xmlNodeGetContent(sub); \
        bool ret = parse_fn(sub, val, content, ctx); \
        xmlFree(content); \
        if(!ret) \
            return false; \
    }

#define END_NODE_MATCH() \
    }

#define CHECK_HAS(node, node_name, has, ctx) \
    if(!has) \
        return parse_missing_error(node, node_name, ctx);

namespace
{

bool is_real_text_node(xmlNode *node)
{
    for(xmlNode *sub = node->children; sub; sub = sub->next)
        if(sub->type != XML_TEXT_NODE && sub->type != XML_ENTITY_REF_NODE)
            return false;
    return true;
}

std::string node_loc(xmlNode *node)
{
    std::ostringstream oss;
    oss << "line " << node->line;
    return oss.str();
}

bool add_error(error_context_t& ctx, error_t::level_t lvl, xmlNode *node,
    const std::string& msg)
{
    ctx.add(error_t(lvl, node_loc(node), msg));
    return false;
}

bool add_fatal(error_context_t& ctx, xmlNode *node, const std::string& msg)
{
    return add_error(ctx, error_t::FATAL, node, msg);
}

bool add_warning(error_context_t& ctx, xmlNode *node, const std::string& msg)
{
    return add_error(ctx, error_t::WARNING, node, msg);
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
    add_warning(ctx, node, std::string("name = ") + XML_CHAR_TO_CHAR(node->name));
    add_warning(ctx, node, std::string("content = ") + XML_CHAR_TO_CHAR(xmlNodeGetContent(node)));
    return add_fatal(ctx, node, "this is not a text element");
}

bool parse_text_elem(xmlNode *node, std::string& name, xmlChar *content, error_context_t& ctx)
{
    name = XML_CHAR_TO_CHAR(content);
    return true;
}

bool parse_name_elem(xmlNode *node, std::string& name, xmlChar *content, error_context_t& ctx)
{
    name = XML_CHAR_TO_CHAR(content);
    /* must begin by alpha, then alphanum */
    if(name.size() == 0)
        return add_fatal(ctx, node, "name cannot be empty");
    if(!isalpha(name[0]) && name[0] != '_' && name[0] != '-')
        return add_fatal(ctx, node, "name must begin by an alphabetic character, - or _");
    for(size_t i = 0; i < name.size(); i++)
        if(!isalnum(name[i]) && name[i] != '_' && name[i] != '-')
            return add_fatal(ctx, node, "name must only contain alphanumeric characters, - or _");
    return true;
}

template<typename T>
bool parse_unsigned_elem(xmlNode *node, T& res, xmlChar *content, error_context_t& ctx)
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

bool parse_enum_elem(xmlNode *node, enum_t& reg, error_context_t& ctx)
{
    bool has_name = false, has_value = false, has_desc = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("name", reg.name, has_name, parse_name_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("value", reg.value, has_value, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("desc", reg.desc, has_desc, parse_text_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node, "name", has_name, ctx)
    CHECK_HAS(node, "value", has_value, ctx)
    return true;
}

bool parse_field_elem(xmlNode *node, field_t& reg, error_context_t& ctx)
{
    bool has_name = false, has_pos = false, has_desc = false, has_width = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("name", reg.name, has_name, parse_name_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("position", reg.pos, has_pos, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("width", reg.width, has_width, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("desc", reg.desc, has_desc, parse_text_elem, ctx)
        MATCH_ELEM_NODE("enum", reg.enum_, parse_enum_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node, "name", has_name, ctx)
    CHECK_HAS(node, "position", has_pos, ctx)
    return true;
}

bool parse_register_elem(xmlNode *node, register_t& reg, error_context_t& ctx)
{
    bool has_width = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("width", reg.width, has_width, parse_unsigned_elem, ctx)
        MATCH_ELEM_NODE("field", reg.field, parse_field_elem, ctx)
    END_NODE_MATCH()
    return true;
}

bool parse_range_elem(xmlNode *node, range_t& range, error_context_t& ctx)
{
    bool has_first = false, has_count = false, has_stride = false, has_base = false;
    bool has_formula = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("first", range.first, has_first, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("count", range.count, has_count, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("base", range.base, has_base, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("stride", range.stride, has_stride, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("formula", range.formula, has_formula, parse_text_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node, "first", has_first, ctx)
    CHECK_HAS(node, "count", has_count, ctx)
    if(!has_base && !has_formula)
        return parse_missing_error(node, "base> or <formula", ctx);
    if(has_base && has_formula)
        return parse_conflict_error(node, "base", "formula", ctx);
    if(has_base)
        CHECK_HAS(node, "stride", has_stride, ctx)
    if(has_stride && !has_base)
        return parse_conflict_error(node, "stride", "formula", ctx);
    return true;
}

bool parse_instance_elem(xmlNode *node, instance_t& inst, error_context_t& ctx)
{
    bool has_name = false, has_title = false, has_desc = false, has_range = false;
    bool has_address = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("name", inst.name, has_name, parse_name_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("title", inst.title, has_title, parse_text_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("desc", inst.desc, has_desc, parse_text_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("address", inst.addr, has_address, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_ELEM_NODE("range", inst.range, has_range, parse_range_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node, "name", has_name, ctx)
    if(!has_address && !has_range)
        return parse_missing_error(node, "address> or <range", ctx);
    if(has_address && has_range)
        return parse_conflict_error(node, "address", "range", ctx);
    if(has_address)
        inst.type = instance_t::SINGLE;
    else
        inst.type = instance_t::RANGE;
    return true;
}

bool parse_node_elem(xmlNode *node_, node_t& node, error_context_t& ctx)
{
    register_t reg;
    bool has_title = false, has_desc = false, has_register = false;
    BEGIN_NODE_MATCH(node_->children)
        MATCH_UNIQUE_TEXT_NODE("title", node.title, has_title, parse_text_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("desc", node.desc, has_desc, parse_text_elem, ctx)
        MATCH_UNIQUE_ELEM_NODE("register", reg, has_register, parse_register_elem, ctx)
        MATCH_ELEM_NODE("node", node.node, parse_node_elem, ctx)
        MATCH_ELEM_NODE("instance", node.instance, parse_instance_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node_, "instance", !node.instance.empty(), ctx)
    if(has_register)
        node.register_.push_back(reg);
    return true;
}

bool parse_soc_elem(xmlNode *node, soc_t& soc, error_context_t& ctx)
{
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
    END_NODE_MATCH()
    CHECK_HAS(node, "name", has_name, ctx)
    return true;
}

bool parse_root_elem(xmlNode *node, soc_t& soc, error_context_t& ctx)
{
    bool has_soc = false;
    BEGIN_NODE_MATCH(node)
        MATCH_UNIQUE_ELEM_NODE("soc", soc, has_soc, parse_soc_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node, "soc", has_soc, ctx)
    return true;
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

