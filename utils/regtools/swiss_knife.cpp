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
#include "soc_desc_v1.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cstring>

using namespace soc_desc;

void print_context(const error_context_t& ctx)
{
    for(size_t j = 0; j < ctx.count(); j++)
    {
        err_t e = ctx.get(j);
        switch(e.level())
        {
            case err_t::INFO: printf("[INFO]"); break;
            case err_t::WARNING: printf("[WARN]"); break;
            case err_t::FATAL: printf("[FATAL]"); break;
            default: printf("[UNK]"); break;
        }
        if(e.location().size() != 0)
            printf(" %s:", e.location().c_str());
        printf(" %s\n", e.message().c_str());
    }
}

bool convert_v1_to_v2(const soc_desc_v1::soc_reg_field_value_t& in, enum_t& out, error_context_t& ctx)
{
    out.name = in.name;
    out.desc = in.desc;
    out.value = in.value;
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_reg_field_t& in, field_t& out, error_context_t& ctx)
{
    out.name = in.name;
    out.desc = in.desc;
    out.pos = in.first_bit;
    out.width = in.last_bit - in.first_bit + 1;
    out.enum_.resize(in.value.size());
    for(size_t i = 0; i < in.value.size(); i++)
            if(!convert_v1_to_v2(in.value[i], out.enum_[i], ctx))
                return false;
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_reg_addr_t& in, instance_t& out, error_context_t& ctx)
{
    out.name = in.name;
    out.type = instance_t::SINGLE;
    out.addr = in.addr;
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_reg_formula_t& in, range_t& out, error_context_t& ctx)
{
    out.type = range_t::FORMULA;
    out.formula = in.string;
    out.variable = "n";
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_reg_t& in, node_t& out, error_context_t& ctx,
    std::string _loc)
{
    std::string loc = _loc + "." + in.name;
    out.name = in.name;
    if(in.formula.type == soc_desc_v1::REG_FORMULA_NONE)
    {
        out.instance.resize(in.addr.size());
        for(size_t i = 0; i < in.addr.size(); i++)
            if(!convert_v1_to_v2(in.addr[i], out.instance[i], ctx))
                return false;
    }
    else
    {
        out.instance.resize(1);
        out.instance[0].name = in.name;
        out.instance[0].type = instance_t::RANGE;
        out.instance[0].range.first = 0;
        out.instance[0].range.count = in.addr.size();
        /* check if formula is base/stride */
        bool is_stride = true;
        soc_word_t base = 0, stride = 0;
        if(in.addr.size() <= 1)
        {
            ctx.add(err_t(err_t::WARNING, loc,
                "register uses a formula but has only one instance"));
            is_stride = false;
        }
        else
        {
            base = in.addr[0].addr;
            stride = in.addr[1].addr - base;
            for(size_t i = 0; i < in.addr.size(); i++)
                if(base + i * stride != in.addr[i].addr)
                    is_stride = false;
        }

        if(is_stride)
        {
            ctx.add(err_t(err_t::INFO, loc, "promoted formula to base/stride"));
            out.instance[0].range.type = range_t::STRIDE;
            out.instance[0].range.base = base;
            out.instance[0].range.stride = stride;
        }
        else if(!convert_v1_to_v2(in.formula, out.instance[0].range, ctx))
            return false;
    }
    out.register_.resize(1);
    out.register_[0].width = 32;
    out.register_[0].desc = in.desc;
    out.register_[0].field.resize(in.field.size());
    for(size_t i = 0; i < in.field.size(); i++)
        if(!convert_v1_to_v2(in.field[i], out.register_[0].field[i], ctx))
            return false;
    /* sct */
    if(in.flags & soc_desc_v1::REG_HAS_SCT)
    {
        out.register_[0].variant.resize(3);
        const char *names[3] = {"set", "clr", "tog"};
        for(size_t i = 0; i < 3; i++)
        {
            out.register_[0].variant[i].type = names[i];
            out.register_[0].variant[i].offset = 4 + i *4;
        }
    }
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_dev_addr_t& in, instance_t& out, error_context_t& ctx)
{
    out.name = in.name;
    out.type = instance_t::SINGLE;
    out.addr = in.addr;
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_dev_t& in, node_t& out, error_context_t& ctx,
    std::string _loc)
{
    std::string loc = _loc + "." + in.name;
    if(!in.version.empty())
        ctx.add(err_t(err_t::INFO, loc, "dropped version"));
    out.name = in.name;
    out.title = in.long_name;
    out.desc = in.desc;
    out.instance.resize(1);
    if(in.addr.size() == 1)
    {
        out.instance[0].type = instance_t::SINGLE;
        out.instance[0].name = in.addr[0].name;
        out.instance[0].addr = in.addr[0].addr;
    }
    else
    {
        out.instance[0].type = instance_t::RANGE;
        out.instance[0].name = in.name;
        out.instance[0].range.type = range_t::LIST;
        out.instance[0].range.first = 1;
        out.instance[0].range.list.resize(in.addr.size());
        for(size_t i = 0; i < in.addr.size(); i++)
            out.instance[0].range.list[i] = in.addr[i].addr;
    }
    out.node.resize(in.reg.size());
    for(size_t i = 0; i < in.reg.size(); i++)
        if(!convert_v1_to_v2(in.reg[i], out.node[i], ctx, loc))
            return false;
    return true;
}

bool convert_v1_to_v2(const soc_desc_v1::soc_t& in, soc_t& out, error_context_t& ctx)
{
    out.name = in.name;
    out.title = in.desc;
    out.node.resize(in.dev.size());
    for(size_t i = 0; i < in.dev.size(); i++)
        if(!convert_v1_to_v2(in.dev[i], out.node[i], ctx, in.name))
            return false;
    return true;
}

int do_convert(int argc, char **argv)
{
    std::vector< std::string > authors;
    std::string version;
    while(argc >= 2)
    {
        if(strcmp(argv[0], "--author") == 0)
            authors.push_back(argv[1]);
        else if(strcmp(argv[0], "--version") == 0)
            version = argv[1];
        else
            break;
        argc -= 2;
        argv += 2;
    }
    if(argc < 2)
        return printf("convert mode expects at least one description file and an output file\n");
    soc_desc_v1::soc_t soc;
    if(!soc_desc_v1::parse_xml(argv[0], soc))
        return printf("cannot read file '%s'\n", argv[0]);
    error_context_t ctx;
    soc_t new_soc;
    if(!convert_v1_to_v2(soc, new_soc, ctx))
    {
        print_context(ctx);
        return printf("cannot convert from v1 to v2\n");
    }
    new_soc.author = authors;
    new_soc.version = version;
    if(!produce_xml(argv[1], new_soc, ctx))
    {
        print_context(ctx);
        return printf("cannot write file '%s'\n", argv[1]);
    }
    print_context(ctx);
    return 0;
}

int do_read(int argc, char **argv)
{
    for(int i = 0; i < argc; i++)
    {
        error_context_t ctx;
        soc_t soc;
        bool ret = parse_xml(argv[i], soc, ctx);
        if(ctx.count() != 0)
            printf("In file %s:\n", argv[i]);
        print_context(ctx);
        if(!ret)
        {
            printf("cannot parse file '%s'\n", argv[i]);
            continue;
        }
    }
    return 0;
}

int do_eval(int argc, char **argv)
{
    std::map< std::string, soc_word_t > map;
    for(int i = 0; i < argc; i++)
    {
        std::string formula(argv[i]);
        soc_word_t result;
        if(strcmp(argv[i], "--var") == 0)
        {
            if(i + 1 >= argc)
                break;
            i++;
            std::string str(argv[i]);
            size_t pos = str.find('=');
            if(pos == std::string::npos)
            {
                printf("invalid variable string '%s'\n", str.c_str());
                continue;
            }
            std::string name = str.substr(0, pos);
            std::string val = str.substr(pos + 1);
            char *end;
            soc_word_t v = strtoul(val.c_str(), &end, 0);
            if(*end)
            {
                printf("invalid variable string '%s'\n", str.c_str());
                continue;
            }
            printf("%s = %#lx\n", name.c_str(), (unsigned long)v);
            map[name] = v;
            continue;
        }
        error_context_t ctx;
        if(!evaluate_formula(formula, map, result, "", ctx))
        {
            print_context(ctx);
            printf("cannot parse '%s'\n", formula.c_str());
        }
        else
            printf("result: %lu (%#lx)\n", (unsigned long)result, (unsigned long)result);
    }
    return 0;
}

int do_write(int argc, char **argv)
{
    if(argc != 2)
        return printf("write mode expects two arguments\n");
    soc_t soc;
    error_context_t ctx;
    if(!parse_xml(argv[0], soc, ctx))
    {
        print_context(ctx);
        return printf("cannot read file '%s'\n", argv[0]);
    }
    if(!produce_xml(argv[1], soc, ctx))
    {
        print_context(ctx);
        return printf("cannot write file '%s'\n", argv[1]);
    }
    print_context(ctx);
    return 0;
}

void check_name(const std::string& path, const std::string& name, error_context_t& ctx)
{
    if(name.empty())
        ctx.add(err_t(err_t::FATAL, path, "name is empty"));
    for(size_t i = 0; i < name.size(); i++)
        if(!isalnum(name[i]) && name[i] != '_')
            ctx.add(err_t(err_t::FATAL, path, "name '" + name +
                "' must only contain alphanumeric characters or '_'"));
}

void check_instance(const std::string& _path, const instance_t& inst, error_context_t& ctx)
{
    std::string path = _path + "." + inst.name;
    check_name(path, inst.name, ctx);
    if(inst.type == instance_t::RANGE)
    {
        if(inst.range.type == range_t::FORMULA)
        {
            check_name(path + ".<formula variable>", inst.range.variable, ctx);
            /* try to parse formula */
            std::map< std::string, soc_word_t> var;
            var[inst.range.variable] = inst.range.first;
            soc_word_t res;
            if(!evaluate_formula(inst.range.formula, var, res, path + ".<formula>", ctx))
                ctx.add(err_t(err_t::FATAL, path + ".<formula>",
                    "cannot evaluate formula"));
        }
    }
}

void check_field(const std::string& _path, const field_t& field, error_context_t& ctx)
{
    std::string path = _path + "." + field.name;
    check_name(path, field.name, ctx);
    if(field.width == 0)
        ctx.add(err_t(err_t::WARNING, path, "field has width 0"));
    soc_word_t max = field.bitmask() >> field.pos;
    std::set< std::string > names;
    std::map< soc_word_t, std::string > map;
    for(size_t i = 0; i < field.enum_.size(); i++)
    {
        soc_word_t v = field.enum_[i].value;
        std::string n = field.enum_[i].name;
        std::string path_ = path + "." + n;
        check_name(path_, n, ctx);
        if(v > max)
            ctx.add(err_t(err_t::FATAL, path_, "value does not fit into the field"));
        if(names.find(n) != names.end())
            ctx.add(err_t(err_t::FATAL, path, "duplicate name '" + n + "' in enums"));
        names.insert(n);
        if(map.find(v) != map.end())
            ctx.add(err_t(err_t::WARNING, path, "'" + n + "' and '" + map[v] + "' have the same value"));
        map[v] = n;
    }
}

void check_register(const std::string& _path, const soc_desc::register_t& reg, error_context_t& ctx)
{
    std::string path = _path + ".<register>";
    if(reg.width != 8 && reg.width != 16 && reg.width != 32)
        ctx.add(err_t(err_t::WARNING, path, "width is not 8, 16 or 32"));
    for(size_t i = 0; i < reg.field.size(); i++)
        check_field(path, reg.field[i], ctx);
    std::set< std::string > names;
    soc_word_t bitmap = 0;
    for(size_t i = 0; i < reg.field.size(); i++)
    {
        std::string n = reg.field[i].name;
        if(names.find(n) != names.end())
            ctx.add(err_t(err_t::FATAL, path, "duplicate name '" + n + "' in fields"));
        if(reg.field[i].pos + reg.field[i].width > reg.width)
            ctx.add(err_t(err_t::FATAL, path, "field '" + n + "' does not fit into the register"));
        names.insert(n);
        if(bitmap & reg.field[i].bitmask())
        {
            /* find the duplicate to ease debugging */
            for(size_t j = 0; j < i; j++)
                if(reg.field[j].bitmask() & reg.field[i].bitmask())
                    ctx.add(err_t(err_t::FATAL, path, "overlap between fields '" +
                        reg.field[j].name + "' and '" + n + "'"));
        }
        bitmap |= reg.field[i].bitmask();
    }
}

void check_nodes(const std::string& path, const std::vector< node_t >& nodes,
    error_context_t& ctx);

void check_node(const std::string& _path, const node_t& node, error_context_t& ctx)
{
    std::string path = _path + "." + node.name;
    check_name(_path, node.name, ctx);
    if(node.instance.empty())
        ctx.add(err_t(err_t::WARNING, path, "subnode with no instances"));
    for(size_t j = 0; j < node.instance.size(); j++)
        check_instance(path, node.instance[j], ctx);
    for(size_t i = 0; i < node.register_.size(); i++)
        check_register(path, node.register_[i], ctx);
    check_nodes(path, node.node, ctx);
}

void check_nodes(const std::string& path, const std::vector< node_t >& nodes,
    error_context_t& ctx)
{
    for(size_t i = 0; i < nodes.size(); i++)
        check_node(path, nodes[i], ctx);
    /* gather all instance names */
    std::set< std::string > names;
    for(size_t i = 0; i < nodes.size(); i++)
        for(size_t j = 0; j < nodes[i].instance.size(); j++)
        {
            std::string n = nodes[i].instance[j].name;
            if(names.find(n) != names.end())
                ctx.add(err_t(err_t::FATAL, path, "duplicate instance name '" +
                    n + "' in subnodes"));
            names.insert(n);
        }
    /* gather all node names */
    names.clear();
    for(size_t i = 0; i < nodes.size(); i++)
    {
        std::string n = nodes[i].name;
        if(names.find(n) != names.end())
            ctx.add(err_t(err_t::FATAL, path, "duplicate node name '" + n +
                "' in subnodes"));
        names.insert(n);
    }
}

void do_check(soc_t& soc, error_context_t& ctx)
{
    check_name(soc.name, soc.name, ctx);
    check_nodes(soc.name, soc.node, ctx);
}

int do_check(int argc, char **argv)
{
    for(int i = 0; i < argc; i++)
    {
        error_context_t ctx;
        soc_t soc;
        bool ret = parse_xml(argv[i], soc, ctx);
        if(ret)
            do_check(soc, ctx);
        if(ctx.count() != 0)
            printf("In file %s:\n", argv[i]);
        print_context(ctx);
        if(!ret)
        {
            printf("cannot parse file '%s'\n", argv[i]);
            continue;
        }
    }
    return 0;
}

const unsigned DUMP_NODES = 1 << 0;
const unsigned DUMP_INSTANCES = 1 << 1;
const unsigned DUMP_VERBOSE = 1 << 2;
const unsigned DUMP_REGISTERS = 1 << 3;

void print_path(node_ref_t node, bool nl = true)
{
    printf("%s", node.soc().get()->name.c_str());
    std::vector< std::string > path = node.path();
    for(size_t i = 0; i < path.size(); i++)
        printf(".%s", path[i].c_str());
    if(nl)
        printf("\n");
}

void print_inst(node_inst_t inst, bool end = true)
{
    if(!inst.is_root())
    {
        print_inst(inst.parent(), false);
        printf(".%s", inst.name().c_str());
        if(inst.is_indexed())
            printf("[%u]", (unsigned)inst.index());
    }
    else
    {
        printf("%s", inst.soc().get()->name.c_str());
    }
    if(end)
        printf(" @ %#x\n", inst.addr());
}

void print_reg(register_ref_t reg, unsigned flags)
{
    if(!(flags & DUMP_REGISTERS))
        return;
    node_ref_t node = reg.node();
    soc_desc::register_t *r = reg.get();
    print_path(node, false);
    printf(":width=%u\n", (unsigned)r->width);
    std::vector< field_ref_t > fields = reg.fields();
    for(size_t i = 0; i < fields.size(); i++)
    {
        field_t *f = fields[i].get();
        print_path(node, false);
        if(f->width == 1)
            printf(":[%u]=", (unsigned)f->pos);
        else
            printf(":[%u-%u]=", (unsigned)(f->pos + f->width - 1), (unsigned)f->pos);
        printf("%s\n", f->name.c_str());
    }
    std::vector< variant_ref_t > variants = reg.variants();
    for(size_t i = 0; i < variants.size(); i++)
    {
        print_path(node, false);
        printf(":%s@+0x%x\n", variants[i].type().c_str(), variants[i].offset());
    }
}

void do_dump(node_ref_t node, unsigned flags)
{
    print_path(node);
    if(node.reg().node() == node)
        print_reg(node.reg(), flags);
    std::vector< node_ref_t > children = node.children();
    for(size_t i = 0; i < children.size(); i++)
        do_dump(children[i], flags);
}

void do_dump(node_inst_t inst, unsigned flags)
{
    print_inst(inst);
    std::vector< node_inst_t > children = inst.children();
    for(size_t i = 0; i < children.size(); i++)
        do_dump(children[i], flags);
}

void do_dump(soc_t& soc, unsigned flags)
{
    soc_ref_t ref(&soc);
    if(flags & DUMP_NODES)
        do_dump(ref.root(), flags);
    if(flags & DUMP_INSTANCES)
        do_dump(ref.root_inst(), flags);
}

int do_dump(int argc, char **argv)
{
    unsigned flags = 0;
    int i = 0;
    for(; i < argc; i++)
    {
        if(strcmp(argv[i], "--nodes") == 0)
            flags |= DUMP_NODES;
        else if(strcmp(argv[i], "--instances") == 0)
            flags |= DUMP_INSTANCES;
        else if(strcmp(argv[i], "--verbose") == 0)
            flags |= DUMP_VERBOSE;
        else if(strcmp(argv[i], "--registers") == 0)
            flags |= DUMP_REGISTERS;
        else
            break;
    }
    if(i == argc)
    {
        printf("you must specify at least one file\n");
        return 1;
    }
    for(; i < argc; i++)
    {
        error_context_t ctx;
        soc_t soc;
        bool ret = parse_xml(argv[i], soc, ctx);
        if(ret)
            do_dump(soc, flags);
        if(ctx.count() != 0)
            printf("In file %s:\n", argv[i]);
        print_context(ctx);
        if(!ret)
        {
            printf("cannot parse file '%s'\n", argv[i]);
            continue;
        }
    }
    return 0;
}

std::string trim(const std::string& s)
{
    std::string ss = s.substr(s.find_first_not_of(" \t"));
    return ss.substr(0, ss.find_last_not_of(" \t") + 1);
}

bool parse_key(const std::string& key, std::string& dev, std::string& reg)
{
    if(key.substr(0, 3) != "HW.")
        return false;
    std::string s = key.substr(3);
    size_t idx = s.find('.');
    if(idx == std::string::npos)
        return false;
    dev = s.substr(0, idx);
    reg = s.substr(idx + 1);
    return true;
}

bool find_addr(const soc_desc_v1::soc_dev_t& dev,
    const std::string& reg, soc_desc_v1::soc_addr_t& addr)
{
    for(size_t i = 0; i < dev.reg.size(); i++)
        for(size_t j = 0; j < dev.reg[i].addr.size(); j++)
            if(dev.reg[i].addr[j].name == reg)
            {
                addr += dev.reg[i].addr[j].addr;
                return true;
            }
    return false;
}

bool find_addr(const soc_desc_v1::soc_t& soc, const std::string& dev,
    const std::string& reg, soc_desc_v1::soc_addr_t& addr)
{
    addr = 0;
    for(size_t i = 0; i < soc.dev.size(); i++)
        for(size_t j = 0; j < soc.dev[i].addr.size(); j++)
            if(soc.dev[i].addr[j].name == dev)
            {
                addr += soc.dev[i].addr[j].addr;
                return find_addr(soc.dev[i], reg, addr);
            }
    return false;
}

int convert_dump(const std::map< std::string, std::string >& entries,
    const soc_desc_v1::soc_t& soc, std::ofstream& fout)
{
    std::map< std::string, std::string >::const_iterator it = entries.begin();
    for(; it != entries.end(); ++it)
    {
        char *end;
        soc_desc_v1::soc_word_t v = strtoul(it->second.c_str(), &end, 0);
        if(*end != 0)
        {
            printf("because of invalid value '%s': ignore key '%s'\n",
                it->second.c_str(), it->first.c_str());
            continue;
        }
        std::string dev, reg;
        if(!parse_key(it->first, dev, reg))
        {
            printf("invalid key format, ignore key '%s'\n", it->first.c_str());
            continue;
        }
        soc_desc_v1::soc_addr_t addr;
        if(!find_addr(soc, dev, reg, addr))
        {
            printf("cannot find register in description, ignore key '%s'\n",
                it->first.c_str());
            continue;
        }
        fout << "0x" << std::hex << addr << " = 0x" << std::hex << v << "\n";
    }
    return 0;
}

int do_convertdump(int argc, char **argv)
{
    if(argc < 3)
    {
        printf("you must specify at least one description file, one input file and one output file\n");
        return 1;
    }
    std::vector< soc_desc_v1::soc_t > socs;
    for(int i = 0; i < argc - 2; i++)
    {
        socs.resize(socs.size() + 1);
        if(!parse_xml(argv[i], socs.back()))
        {
            socs.pop_back();
            printf("cannot parse description file '%s'\n", argv[i]);
        }
    }
    std::ifstream fin(argv[argc - 2]);
    if(!fin)
    {
        printf("cannot open input file\n");
        return 1;
    }
    std::map< std::string, std::string > entries;
    std::string line;
    while(std::getline(fin, line))
    {
        size_t idx = line.find('=');
        if(idx == std::string::npos)
        {
            printf("ignore invalid line '%s'\n", line.c_str());
            continue;
        }
        std::string key = trim(line.substr(0, idx));
        std::string value = trim(line.substr(idx + 1));
        entries[key] = value;
    }
    if(entries.find("HW") == entries.end())
    {
        printf("invalid dump file: missing HW key\n");
        return 1;
    }
    std::string soc = entries["HW"];
    soc_desc_v1::soc_t *psoc = 0;
    for(size_t i = 0; i < socs.size(); i++)
        if(socs[i].name == soc)
            psoc = &socs[i];
    if(psoc == 0)
    {
        printf("cannot convert dump: please provide the description file for the soc '%s'\n", soc.c_str());
        return 1;
    }
    entries.erase(entries.find("HW"));
    std::ofstream fout(argv[argc - 1]);
    if(!fout)
    {
        printf("cannot open output file\n");
        return 1;
    }
    fout << "soc = " << soc << "\n";
    return convert_dump(entries, *psoc, fout);
}

int do_normalize(int argc, char **argv)
{
    if(argc != 2)
    {
        printf("normalize takes two arguments\n");
        return 1;
    }
    error_context_t ctx;
    soc_t soc;
    bool ret = parse_xml(argv[0], soc, ctx);
    if(ctx.count() != 0)
        printf("In file %s:\n", argv[0]);
    print_context(ctx);
    if(!ret)
    {
        printf("cannot parse file '%s'\n", argv[1]);
        return 2;
    }
    normalize(soc);
    ret = produce_xml(argv[1], soc, ctx);
    if(ctx.count() != 0)
        printf("In file %s:\n", argv[1]);
    print_context(ctx);
    if(!ret)
    {
        printf("cannot write file '%s'\n", argv[1]);
        return 3;
    }
    return 0;
}

void usage()
{
    printf("usage: swiss_knife <mode> [options]\n");
    printf("modes:\n");
    printf("  read <files...>\n");
    printf("  write <read file> <write file>\n");
    printf("  eval [<formula>|--var <name>=<val>]...\n");
    printf("  convert [--author <auth>] [--version <ver>] <input file> <output file>\n");
    printf("  check <files...>\n");
    printf("  dump [--nodes] [--instances] [--registers] [--verbose] <files...>\n");
    printf("  convertdump <desc file> ... <desc file> <input dump file> <output dump file>\n");
    printf("  normalize <desc file> <output desc file>\n");
    printf("\n");
    printf("The following operations are performed in each mode:\n");
    printf("* read: open and parse the files, reports any obvious errors\n");
    printf("* write: open, parse a file and write it back, checks the parser/generator match\n");
    printf("* eval: evaluate a formula with the formula parser\n");
    printf("* convert: convert a description file from version 1 to version 2\n");
    printf("* check: performs deep checks on description files\n");
    printf("* dump: debug tool to dump internal structures\n");
    printf("* convertdump: convert a register dump from version 1 to version 2\n");
    printf("               NOTE: description file must be a v1 file\n");
    printf("* normalize: normalise a description file\n");
    exit(1);
}

int main(int argc, char **argv)
{
    if(argc < 2)
        usage();
    std::string mode = argv[1];
    if(mode == "read")
        return do_read(argc - 2, argv +  2);
    else if(mode == "write")
        return do_write(argc - 2, argv +  2);
    else if(mode == "eval")
        return do_eval(argc - 2, argv +  2);
    else if(mode == "convert")
        return do_convert(argc - 2, argv +  2);
    else if(mode == "check")
        return do_check(argc - 2, argv +  2);
    else if(mode == "dump")
        return do_dump(argc - 2, argv +  2);
    else if(mode == "convertdump")
        return do_convertdump(argc - 2, argv +  2);
    else if(mode == "normalize")
        return do_normalize(argc - 2, argv +  2);
    else
        usage();
    return 0;
}
