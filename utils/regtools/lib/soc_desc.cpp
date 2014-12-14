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
    for(xmlAttr *a = attr; a; a = a->next) {

#define MATCH_UNIQUE_ATTR(attr_name, val, has, parse_fn, ctx) \
    if(strcmp(XML_CHAR_TO_CHAR(a->name), attr_name) == 0) { \
        if(has) \
            return parse_not_unique_attr_error(a, ctx); \
        has = true; \
        xmlChar *str = NULL; \
        if(!parse_text_attr_internal(a, str, ctx) || !parse_fn(a, val, str, ctx)) \
            return false; \
    }

#define END_ATTR_MATCH() \
    }

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

#define CHECK_HAS_ATTR(node, attr_name, has, ctx) \
    if(!has) \
        return parse_missing_attr_error(node, attr_name, ctx);

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
bool add_error(error_context_t& ctx, error_t::level_t lvl, T *node,
    const std::string& msg)
{
    ctx.add(error_t(lvl, xml_loc(node), msg));
    return false;
}

template<typename T>
bool add_fatal(error_context_t& ctx, T *node, const std::string& msg)
{
    return add_error(ctx, error_t::FATAL, node, msg);
}

template<typename T>
bool add_warning(error_context_t& ctx, T *node, const std::string& msg)
{
    return add_error(ctx, error_t::WARNING, node, msg);
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

bool parse_field_elem(xmlNode *node, field_t& field, error_context_t& ctx)
{
    bool has_name = false, has_pos = false, has_desc = false, has_width = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("name", field.name, has_name, parse_name_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("position", field.pos, has_pos, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("width", field.width, has_width, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("desc", field.desc, has_desc, parse_text_elem, ctx)
        MATCH_ELEM_NODE("enum", field.enum_, parse_enum_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node, "name", has_name, ctx)
    CHECK_HAS(node, "position", has_pos, ctx)
    if(!has_width)
        field.width = 1;
    return true;
}

bool parse_register_elem(xmlNode *node, register_t& reg, error_context_t& ctx)
{
    bool has_width = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("width", reg.width, has_width, parse_unsigned_elem, ctx)
        MATCH_ELEM_NODE("field", reg.field, parse_field_elem, ctx)
    END_NODE_MATCH()
    if(!has_width)
        reg.width = 32;
    return true;
}

bool parse_formula_elem(xmlNode *node, range_t& range, error_context_t& ctx)
{
    bool has_var = false;
    BEGIN_ATTR_MATCH(node->properties)
        MATCH_UNIQUE_ATTR("variable", range.variable, has_var, parse_text_attr, ctx)
    END_NODE_MATCH()
    CHECK_HAS_ATTR(node, "variable", has_var, ctx)
    return true;
}

bool parse_range_elem(xmlNode *node, range_t& range, error_context_t& ctx)
{
    bool has_first = false, has_count = false, has_stride = false, has_base = false;
    bool has_formula = false, has_formula_attr = false;
    BEGIN_NODE_MATCH(node->children)
        MATCH_UNIQUE_TEXT_NODE("first", range.first, has_first, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("count", range.count, has_count, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("base", range.base, has_base, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("stride", range.stride, has_stride, parse_unsigned_elem, ctx)
        MATCH_UNIQUE_ELEM_NODE("formula", range, has_formula_attr, parse_formula_elem, ctx)
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
    if(has_stride)
        range.type = range_t::STRIDE;
    else
        range.type = range_t::FORMULA;
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
    bool has_title = false, has_desc = false, has_register = false, has_name = false;
    BEGIN_NODE_MATCH(node_->children)
        MATCH_UNIQUE_TEXT_NODE("name", node.name, has_name, parse_name_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("title", node.title, has_title, parse_text_elem, ctx)
        MATCH_UNIQUE_TEXT_NODE("desc", node.desc, has_desc, parse_text_elem, ctx)
        MATCH_UNIQUE_ELEM_NODE("register", reg, has_register, parse_register_elem, ctx)
        MATCH_ELEM_NODE("node", node.node, parse_node_elem, ctx)
        MATCH_ELEM_NODE("instance", node.instance, parse_instance_elem, ctx)
    END_NODE_MATCH()
    CHECK_HAS(node_, "name", has_name, ctx)
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
    size_t ver = 0;
    bool has_soc = false, has_version = false;
    BEGIN_ATTR_MATCH(node->properties)
        MATCH_UNIQUE_ATTR("version", ver, has_version, parse_unsigned_attr, ctx)
    END_ATTR_MATCH()
    if(!has_version)
        ctx.add(error_t(error_t::INFO, xml_loc(node), "no version attribute, is this a v1 file ?"));
    CHECK_HAS_ATTR(node, "version", has_version, ctx)
    if(ver != MAJOR_VERSION)
        return parse_wrong_version_error(node, ctx);
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
            ctx.add(error_t(error_t::FATAL, oss.str(), "write error")); \
            return -1; \
        } \
    }while(0)

int produce_range(xmlTextWriterPtr writer, const range_t& range, error_context_t& ctx)
{
    /* <range> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "range"));
    /* <first/> */
    SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "first", "%lu", range.first));
    /* <count/> */
    SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "count", "%lu", range.count));
    /* <base/><stride/> */
    if(range.type == range_t::STRIDE)
    {
        SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "base", "0x%x", range.base));
        SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "stride", "0x%x", range.stride));
    }
    /* <formula> */
    else if(range.type == range_t::FORMULA)
    {
        /* <formula> */
        SAFE(xmlTextWriterStartElement(writer, BAD_CAST "formula"));
        /* variable */
        SAFE(xmlTextWriterWriteAttribute(writer, BAD_CAST "variable", BAD_CAST range.variable.c_str()));
        /* content */
        SAFE(xmlTextWriterWriteString(writer, BAD_CAST range.formula.c_str()));
        /* </formula> */
        SAFE(xmlTextWriterEndElement(writer));
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

int produce_register(xmlTextWriterPtr writer, const register_t& reg, error_context_t& ctx)
{
    /* <register> */
    SAFE(xmlTextWriterStartElement(writer, BAD_CAST "register"));
    /* <width/> */
    if(reg.width != 32)
        SAFE(xmlTextWriterWriteFormatElement(writer, BAD_CAST "width", "%lu", reg.width));
    /* fields */
    for(size_t i = 0; i < reg.field.size(); i++)
        SAFE(produce_field(writer, reg.field[i], ctx));
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
    SAFE(xmlTextWriterSetIndentString(writer, BAD_CAST "  "));
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
 * Node API
 */

soc_ref_t::soc_ref_t(soc_t *soc):m_soc(soc)
{
}

soc_t *soc_ref_t::get()
{
    return m_soc;
}

node_ref_t soc_ref_t::root()
{
    return node_ref_t(*this);
}

node_inst_t soc_ref_t::root_inst()
{
    return node_inst_t(*this);
}

node_ref_t::node_ref_t(soc_ref_t soc):m_soc(soc)
{
}

node_ref_t::node_ref_t(soc_ref_t soc, const std::vector< std::string >& path)
    :m_soc(soc), m_path(path)
{
}

node_ref_t node_ref_t::make_invalid(soc_ref_t soc)
{
    /* builds a reference with a path which is clearly invalid */
    return node_ref_t(soc, std::vector< std::string >(1, "<invalid>"));
}

bool node_ref_t::valid()
{
    return is_root() || get() != 0;
}

bool node_ref_t::is_root()
{
    return m_path.empty();
}

namespace
{

std::vector< node_t > *get_children(node_ref_t node)
{
    if(node.is_root())
        return &node.soc().get()->node;
    node_t *n = node.get();
    return n == 0 ? 0 : &n->node;
}

node_t *get_child(std::vector< node_t > *nodes, std::string name)
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
node_t *node_ref_t::get()
{
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

soc_ref_t node_ref_t::soc()
{
    return m_soc;
}

node_ref_t node_ref_t::parent()
{
    std::vector< std::string > path = m_path;
    if(!path.empty())
        path.pop_back();
    return node_ref_t(m_soc, path);
}

register_ref_t node_ref_t::reg()
{
    node_t *n = get();
    if(n == 0)
        return register_ref_t::make_invalid(m_soc);
    if(n->register_.empty())
        return parent().reg();
    else
        return register_ref_t(*this);
}

node_ref_t node_ref_t::child(const std::string& name)
{
    /* check the node exists */
    if(get_child(get_children(*this), name) == 0)
        return node_ref_t::make_invalid(m_soc);
    std::vector< std::string > path = m_path;
    path.push_back(name);
    return node_ref_t(m_soc, path);
}

std::vector< node_ref_t > node_ref_t::children()
{
    std::vector< node_ref_t > nodes;
    std::vector< node_t > *children = get_children(*this);
    if(children == 0)
        return nodes;
    for(size_t i = 0; i < children->size(); i++)
    {
        std::vector< std::string > path = m_path;
        path.push_back((*children)[i].name);
        nodes.push_back(node_ref_t(m_soc, path));
    }
    return nodes;
}

std::vector< std::string > node_ref_t::path()
{
    return m_path;
}

std::string node_ref_t::name()
{
    return m_path.empty() ? "" : m_path.back();
}

register_ref_t::register_ref_t(node_ref_t node)
    :m_node(node)
{
}

register_ref_t register_ref_t::make_invalid(soc_ref_t soc)
{
    return register_ref_t(node_ref_t::make_invalid(soc));
}

bool register_ref_t::valid()
{
    return get() != 0;
}

register_t *register_ref_t::get()
{
    node_t *n = m_node.get();
    if(n == 0 || n->register_.empty())
        return 0;
    return &n->register_[0];
}

node_ref_t register_ref_t::node()
{
    return m_node;
}

std::vector< field_ref_t > register_ref_t::fields()
{
    std::vector< field_ref_t > fields;
    return fields;
}

field_ref_t::field_ref_t(register_ref_t reg, const std::string& name)
    :m_reg(reg), m_name(name)
{
}

field_ref_t field_ref_t::make_invalid(soc_ref_t soc)
{
    return field_ref_t(register_ref_t::make_invalid(soc), "<invalid>");
}

bool field_ref_t::valid()
{
    return get() != 0;
}

field_t *field_ref_t::get()
{
    register_t *reg = m_reg.get();
    if(reg == 0)
        return 0;
    for(size_t i = 0; i < reg->field.size(); i++)
        if(reg->field[i].name == m_name)
            return &reg->field[i];
    return 0;
}

register_ref_t field_ref_t::reg()
{
    return m_reg;
}

namespace
{

const size_t INST_NO_INDEX = std::numeric_limits<std::size_t>::max();

instance_t *get_inst(std::vector< instance_t > *insts, std::string name)
{
    if(insts == 0)
        return 0;
    for(size_t i = 0; i < insts->size(); i++)
        if((*insts)[i].name == name)
            return &(*insts)[i];
    return 0;
}

bool get_inst_addr(range_t& range, size_t index, soc_addr_t& addr)
{
    if(index < range.first || index >= range.first + range.count)
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

instance_t *find_inst(soc_ref_t soc, const std::vector< std::string >& node_path,
    const std::vector< std::string >& name_path,
    const std::vector< size_t >& index_path, soc_addr_t& addr)
{
    addr = 0;
    instance_t *inst = 0;
    std::vector< node_t > *nodes = &soc.get()->node;
    if(node_path.size() != name_path.size() || name_path.size() != index_path.size())
        return 0;
    for(size_t i = 0; i < node_path.size(); i++)
    {
        node_t *n = get_child(nodes, node_path[i]);
        if(n == 0)
            return 0;
        nodes = &n->node;
        inst = get_inst(&n->instance, name_path[i]);
        if(!get_inst_addr(inst, index_path[i], addr))
            return 0;
    }
    return inst;
}

void add_instances(std::vector< node_inst_t >& list, instance_t& inst, node_ref_t node,
    std::vector< std::string > n_path, std::vector< size_t > i_path)
{
    n_path.push_back(inst.name);
    switch(inst.type)
    {
        case instance_t::SINGLE:
            i_path.push_back(INST_NO_INDEX);
            list.push_back(node_inst_t(node, n_path, i_path));
            break;
        case instance_t::RANGE:
            for(size_t i = 0; i < inst.range.count; i++)
            {
                i_path.push_back(inst.range.first + i);
                list.push_back(node_inst_t(node, n_path, i_path));
                i_path.pop_back();
            }
            break;
        default:
            break;
    }
}

}

node_inst_t::node_inst_t(soc_ref_t soc)
    :m_node(node_ref_t(soc))
{
}

node_inst_t::node_inst_t(node_ref_t node, const std::vector< std::string >& names,
        const std::vector< size_t >& indexes)
    :m_node(node), m_name_path(names), m_index_path(indexes)
{
}

node_inst_t node_inst_t::make_invalid(soc_ref_t soc)
{
    return node_inst_t(node_ref_t::make_invalid(soc), std::vector< std::string >(),
        std::vector< size_t >());
}

bool node_inst_t::valid()
{
    return is_root() || get() != 0;
}

node_ref_t node_inst_t::node()
{
    return m_node;
}

soc_ref_t node_inst_t::soc()
{
    return m_node.soc();
}

bool node_inst_t::is_root()
{
    return m_node.is_root();
}

node_inst_t node_inst_t::parent()
{
    std::vector< std::string > names = m_name_path;
    std::vector< size_t > indexes = m_index_path;
    if(!names.empty())
        names.pop_back();
    if(!indexes.empty())
        indexes.pop_back();
    return node_inst_t(m_node.parent(), names, indexes);
}

instance_t *node_inst_t::get()
{
    soc_addr_t addr;
    return find_inst(soc(), m_node.m_path, m_name_path, m_index_path, addr);
}

soc_addr_t node_inst_t::addr()
{
    soc_addr_t addr;
    if(find_inst(soc(), m_node.m_path, m_name_path, m_index_path, addr))
        return addr;
    else
        return 0;
}

node_inst_t node_inst_t::child(const std::string& name)
{
    return child(name, INST_NO_INDEX);
}


node_inst_t node_inst_t::child(const std::string& name, size_t index)
{
    std::vector< node_t > *nodes = get_children(m_node);
    for(size_t i = 0; i < nodes->size(); i++)
    {
        if(get_inst(&(*nodes)[i].instance, name))
        {
            std::vector< std::string > n_path = m_name_path;
            std::vector< size_t > i_path = m_index_path;
            n_path.push_back(name);
            i_path.push_back(index);
            return node_inst_t(m_node.child((*nodes)[i].name), n_path, i_path);
        }
    }
    return node_inst_t::make_invalid(soc());
}

std::vector< node_inst_t > node_inst_t::children()
{
    std::vector< node_inst_t > list;
    std::vector< node_t > *nodes = get_children(m_node);
    for(size_t i = 0; i < nodes->size(); i++)
    {
        node_t& node = (*nodes)[i];
        node_ref_t n = m_node.child(node.name);
        for(size_t j = 0; j < node.instance.size(); j++)
            add_instances(list, node.instance[j], n, m_name_path, m_index_path);
    }
    return list;
}

std::string node_inst_t::name()
{
    return m_name_path.empty() ? "" : m_name_path.back();
}

bool node_inst_t:: is_indexed()
{
    return !m_index_path.empty() && m_index_path.back() != INST_NO_INDEX;
}

size_t node_inst_t::index()
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

