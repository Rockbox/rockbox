/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#include <dirent.h>
#include <getopt.h>
#include <regex>

using namespace soc_desc;
int g_verbose = 0;
bool g_inline = false;
bool g_print_zero = false;
bool g_regex_mode = false;
std::regex_constants::syntax_option_type g_regex_flags;

std::regex_constants::syntax_option_type parse_regex_mode(const std::string& mode)
{
    std::regex_constants::syntax_option_type flags;
    size_t index = 0;
    while(index < mode.size())
    {
        size_t end = mode.find(',', index);
        if(end == std::string::npos)
            end = mode.size();
        std::string opt = mode.substr(index, end - index);
        if(opt == "icase")
            flags |= std::regex_constants::icase;
        else if(opt == "ECMAScript")
            flags |= std::regex_constants::ECMAScript;
        else if(opt == "basic")
            flags |= std::regex_constants::basic;
        else if(opt == "extended")
            flags |= std::regex_constants::extended;
        else if(opt == "awk")
            flags |= std::regex_constants::awk;
        else if(opt == "grep")
            flags |= std::regex_constants::grep;
        else if(opt == "egrep")
            flags |= std::regex_constants::egrep;
        else
        {
            fprintf(stderr, "Invalid regex option '%s'\n", opt.c_str());
            exit(1);
        }
        index = end + 1;
    }
    return flags;
}

void print_path(node_ref_t node, bool nl = true)
{
    printf("%s", node.soc().get()->name.c_str());
    std::vector< std::string > path = node.path();
    for(size_t i = 0; i < path.size(); i++)
        printf(".%s", path[i].c_str());
    if(nl)
        printf("\n");
}

template<typename T>
std::string to_str(const T& t)
{
    std::ostringstream oss;
    oss << t;
    return oss.str();
}

std::string get_path(node_inst_t inst)
{
    if(!inst.is_root())
    {
        std::string path = get_path(inst.parent()) + "." + inst.name();
        if(inst.is_indexed())
            path += "[" + to_str(inst.index()) + "]";
        return path;
    }
    else
        return inst.soc().get()->name;
}

void print_inst(node_inst_t inst, bool addr = true, bool nl = true)
{
    if(!inst.is_root())
    {
        print_inst(inst.parent(), false);
        printf(".%s", inst.name().c_str());
        if(inst.is_indexed())
            printf("[%u]", (unsigned)inst.index());
    }
    else
        printf("%s", inst.soc().get()->name.c_str());
    if(addr)
    {
        printf(" @ %#x", inst.addr());
        if(nl)
            printf("\n");
    }
}

void find_insts(std::vector< node_inst_t >& list, node_inst_t inst, soc_addr_t addr)
{
    if(inst.addr() == addr)
    {
        /* only keep matches that are registers */
        if(inst.node().reg().valid())
            list.push_back(inst);
    }
    std::vector< node_inst_t > children = inst.children();
    for(size_t i = 0; i < children.size(); i++)
        find_insts(list, children[i], addr);
}

void find_insts(std::vector< node_inst_t >& insts,
    std::vector< soc_t >& soc_list, soc_addr_t addr)
{
    for(size_t i = 0; i < soc_list.size(); i++)
        find_insts(insts, soc_ref_t(&soc_list[i]).root_inst(), addr);
}

const size_t NO_INDEX = (size_t)-1;
/* index is set to NO_INDEX if there is no index */
bool parse_name(std::string& name, std::string& component, size_t& index)
{
    size_t i = 0;
    /* name must be of the form [a-zA-Z0-9_]+ */
    while(i < name.size() && (isalnum(name[i]) || name[i] == '_'))
        i++;
    if(i == 0)
        return false;
    component = name.substr(0, i);
    /* must stop at the end, or on '.' or on '[' */
    if(name.size() == i)
    {
        index = NO_INDEX;
        name = name.substr(i);
        return true;
    }
    else if(name[i] == '.')
    {
        index = NO_INDEX;
        name = name.substr(i + 1);
        return true;
    }
    else if(name[i] == '[')
    {
        /* parse index */
        char *end;
        index = strtoul(name.c_str() + i + 1, &end, 0);
        /* must stop on ']'. Also strtoul is happy with an empty string, check for this */
        if(*end != ']' || end == name.c_str() + i + 1)
            return false;
        i = end + 1 - name.c_str();
        /* check if we have a '.' after that, or the end */
        if(i < name.size() && name[i] != '.')
            return false;
        name = name.substr(i + 1);
        return true;
    }
    else
        return false;
}

int find_insts(std::vector< node_inst_t >& matches, node_inst_t inst, std::regex& regex)
{
    /* only keep matches that are registers */
    if(inst.node().reg().valid())
    {
        std::string path = get_path(inst);
        if(regex_match(path, regex))
            matches.push_back(inst);
    }
    std::vector< node_inst_t > children = inst.children();
    for(size_t i = 0; i < children.size(); i++)
        find_insts(matches, children[i], regex);
    return 0;
}

int find_insts_regex(std::vector< node_inst_t >& matches, std::vector< soc_t >& soc_list, std::string str)
{
    auto flags = g_regex_flags | std::regex_constants::optimize; /* we will match a lot */
    /* any error during construction or mayching will throw exception, and print
     * an error message so don't catch them */
    std::regex regex(str, flags);
    for(size_t i = 0; i < soc_list.size(); i++)
        if(find_insts(matches, soc_ref_t(&soc_list[i]).root_inst(), regex) != 0)
            return 1;
    return 0;
}

int find_insts(std::vector< node_inst_t >& matches, node_inst_t inst, std::string name)
{
    if(name.empty())
    {
        if(inst.node().reg().valid())
            matches.push_back(inst);
        return 0;
    }
    std::string component;
    size_t index;
    bool ok = parse_name(name, component, index);
    if(!ok)
    {
        fprintf(stderr, "invalid name '%s'\n", name.c_str());
        return 1;
    }
    if(index == NO_INDEX)
        inst = inst.child(component);
    else
        inst = inst.child(component, index);
    if(inst.valid())
        return find_insts(matches, inst, name);
    else
        return 0;
}

int find_insts(std::vector< node_inst_t >& matches, soc_t& soc, std::string name)
{
    return find_insts(matches, soc_ref_t(&soc).root_inst(), name);
}

int find_insts(std::vector< node_inst_t >& matches, std::vector< soc_t >& soc_list,
    std::string name)
{
    /* regex mode is special */
    if(g_regex_mode)
        return find_insts_regex(matches, soc_list, name);
    /* if name is an integer, parse it */
    char *end;
    unsigned long addr = strtoul(name.c_str(), &end, 0);
    if(*end == 0)
    {
        find_insts(matches, soc_list, addr);
        return 0;
    }
    /* else assume it's a name */
    std::string name_copy = name;
    std::string component;
    size_t index;
    bool ok = parse_name(name_copy, component, index);
    if(!ok)
    {
        fprintf(stderr, "invalid name '%s'\n", name.c_str());
        return 1;
    }
    /* a soc cannot be indexed */
    for(size_t i = 0; i < soc_list.size(); i++)
    {
        int ret;
        if(index == NO_INDEX && soc_list[i].name == component)
            ret = find_insts(matches, soc_list[i], name_copy);
        else
            ret = find_insts(matches, soc_list[i], name);
        if(ret != 0)
            return ret;
    }
    return 0;
}

void do_describe(node_inst_t inst, soc_word_t *decode_val)
{
    if(decode_val)
        printf(" = %#lx", (unsigned long)*decode_val);
    if(g_inline)
        printf(" {");
    else
        printf("\n");
    std::vector< field_ref_t > fields = inst.node().reg().fields();
    bool first = true;
    soc_word_t mask = 0; /* mask of all decoded bits */
    /* special index fields.size() means "undecoded bits" */
    for(size_t i = 0; i <= fields.size(); i++)
    {
        unsigned long val = 0;
        bool dont_print = false;
        field_t f;
        if(i == fields.size())
        {
            /* create fake field */
            f.name = "undecoded bits";
            f.pos = 0;
            f.width = inst.node().reg().get()->width;
            val = decode_val ?  *decode_val & ~mask : 0;
            /* only print if decoding and something was left */
            dont_print = (val == 0);
        }
        else
        {
            f = *fields[i].get();
            val = decode_val ? f.extract(*decode_val) : 0;
            /* don't print zero fields unless asked */
            dont_print = decode_val && !g_print_zero && val == 0;
        }
        if(dont_print)
            continue;
        if(g_inline)
            printf("%s", first ? " " : ", ");
        else
            printf("  ");
        printf("%s", f.name.c_str());
        first = false;
        if(f.width == 1)
            printf("[%zu]", f.pos);
        else
            printf("[%zu:%zu]", f.pos + f.width - 1, f.pos);
        /* decode if needed */
        if(decode_val)
        {
            /* track what we decoded */
            mask |= ~f.bitmask();
            printf(" = %#lx", val);
        }
        /* newline if need */
        if(!g_inline)
            printf("\n");
    }
    if(g_inline)
        printf(" }\n");
}

int do_find(std::vector< soc_t >& soc_list, int argc, char **argv, bool describe,
    soc_word_t *decode_val = nullptr)
{
    if(argc != 1)
    {
        fprintf(stderr, "action 'find' takes on argument: the name or address of a register\n");
        return 1;
    }
    std::vector< node_inst_t > matches;
    int ret = find_insts(matches, soc_list, argv[0]);
    if(ret != 0)
        return 0;
    /* print matches */
    if(matches.size() > 0)
    {
        for(size_t i = 0; i < matches.size(); i++)
        {
            print_inst(matches[i], true, !describe);
            if(describe)
                do_describe(matches[i], decode_val);
        }
        return 0;
    }
    else
    {
        fprintf(stderr, "No matches\n");
        return 1;
    }
    return 0;
}

int do_decode(std::vector< soc_t >& soc_list, int argc, char **argv)
{
    if(argc != 2)
    {
        fprintf(stderr, "action 'decode' takes two arguments: the register and the value\n");
        return 1;
    }
    char *end;
    soc_word_t val = strtoul(argv[1], &end, 0);
    if(*end)
    {
        fprintf(stderr, "invalid value '%s'\n", argv[1]);
        return 1;
    }
    return do_find(soc_list, argc - 1, argv, true, &val);
}

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

int usage()
{
    printf("usage: regtool [options] <action> [args]\n");
    printf("options:\n");
    printf("  -h                    Display this help\n");
    printf("  -f <regfile>          Load a register file\n");
    printf("  -d <regdir>           Specify a directory where to look for register files\n");
    printf("  -v                    Increase verbosity level\n");
    printf("  -s <soc list>         Limit search to a one or more socs (comma separated)\n");
    printf("  -i                    Describe/decode in one line\n");
    printf("  -z                    Print fields even when the value is zero\n");
    printf("  -r <mode>             Enable regex mode\n");
    printf("\n");
    printf("actions:\n");
    printf("  find <addr>           Find all registers that match this address\n");
    printf("  find <name>           Find the registers that match this name\n");
    printf("  describe <reg>        Describe a register (either found by address or name)\n");
    printf("  decode <reg> <val>    Decode a register value\n");
    printf("By default, regtool will look for register files in desc/, but if\n");
    printf("any file or directory is specified, regtool will ignore the default directory\n");
    printf("Adresses can be in decimal or hexadecimal (using 0x prefix).\n");
    printf("Names can be fully qualified with a soc name or left ambiguous.\n");
    printf("  <reg> ::= <soc>.<regpath> | <regpath>\n");
    printf("  <regpath> ::= (<regcomp>.)*<regcomp>\n");
    printf("  <regcomp> ::= <name> | <name>[<index>]\n");
    printf("In regex mode, all commands expect a regular expression that is matched\n");
    printf("against the full path (<soc>.<regpath>) of every register. The regex must\n");
    printf("follow the C++11 standard and accepts the following, comma-separated options:\n");
    printf("  icase       Case insensitive match\n");
    printf("  ECMAScript  Use ECMAScript grammar\n");
    printf("  basic       Use basic POSIX grammar\n");
    printf("  extended    Use extended basic grammar\n");
    printf("  awk         Use awk grammar\n");
    printf("  grep        Use grep grammar\n");
    printf("  egrep       Use egrep grammar\n");
    printf("Examples:\n");
    printf("  regtool -d desc/ -s stmp3700,imx233 find 0x8001c310\n");
    printf("  regtool find DIGCTL.CHIPID\n");
    printf("  regtool describe imx233.DIGCTL.CHIPID\n");
    printf("  regtool -f desc/regs-stmp3780.xml decode 0x8001c310 0x37800001\n");
    printf("  regtool -r awk find 'imx233\\.LCDIF\\..*'\n");
    return 1;
}

std::string my_dirname(const std::string& filename)
{
    size_t idx = filename.find_last_of("/\\");
    if(idx == std::string::npos)
        return filename;
    else
        return filename.substr(0, idx);
}

/* warn controls whether we warn about error at lowest verbosity: we warn
 * for files explicitely loaded but not the one listed in directories */
bool load_file(std::vector< soc_t >& soc_list, const std::string& file, bool user_load)
{
    if(g_verbose >= 2)
        fprintf(stderr, "Loading file '%s'...\n", file.c_str());
    error_context_t ctx;
    soc_t s;
    bool ret = parse_xml(file.c_str(), s, ctx);
    if(g_verbose >= 1 || user_load)
    {
        if(ctx.count() != 0)
            fprintf(stderr, "In file %s:\n", file.c_str());
        print_context(ctx);
    }
    if(!ret)
    {
        if(g_verbose >= 1 || user_load)
            fprintf(stderr, "Cannot parse file '%s'\n", file.c_str());
        return !user_load; /* error only if user loaded */
    }
    soc_list.push_back(s);
    return true;
}

bool load_dir(std::vector< soc_t >& soc_list, const std::string& dirname, bool user_load)
{
    if(g_verbose >= 2)
        fprintf(stderr, "Loading directory '%s'...\n", dirname.c_str());
    DIR *dir = opendir(dirname.c_str());
    if(dir == NULL)
    {
        if(g_verbose >= 1 || user_load)
            fprintf(stderr, "Warning: cannot open directory '%s'\n", dirname.c_str());
        return !user_load; /* error only if user loaded */
    }
    struct dirent *ent;
    while((ent = readdir(dir)))
    {
        std::string name(ent->d_name);
        /* only list *.xml */
        if(name.size() < 4 || name.substr(name.size() - 4) != ".xml")
            continue;
        if(!load_file(soc_list, dirname + "/" + name, false))
        {
            closedir(dir);
            return false;
        }
    }
    closedir(dir);
    return true;
}

void add_socs_to_list(std::set< std::string >& soc_list, const std::string& list)
{
    size_t idx = 0;
    while(idx < list.size())
    {
        size_t next = list.find(',', idx);
        if(next == std::string::npos)
            next = list.size();
        soc_list.insert(list.substr(idx, next - idx));
        idx = next + 1;
    }
}

int main(int argc, char **argv)
{
    std::vector< std::string > g_soc_dir;
    std::vector< std::string > g_soc_files;
    std::vector< soc_t > g_soc_list;
    std::set< std::string> g_allowed_soc;

    if(argc <= 1)
        return usage();
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "hf:d:vs:izr:", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'h':
                return usage();
            case 's':
                add_socs_to_list(g_allowed_soc, optarg);
                break;
            case 'd':
                g_soc_dir.push_back(std::string(optarg));
                break;
            case 'f':
                g_soc_files.push_back(std::string(optarg));
                break;
            case 'v':
                g_verbose++;
                break;
            case 'i':
                g_inline = true;
                break;
            case 'z':
                g_print_zero = true;
                break;
            case 'r':
                g_regex_mode = true;
                g_regex_flags = parse_regex_mode(optarg);
                break;
            default:
                abort();
        }
    }
    if(argc == optind)
    {
        fprintf(stderr, "You need at least one action\n");
        return 2;
    }
    /* if no file or directory, add default */
    if(g_soc_files.empty() && g_soc_dir.empty())
        load_dir(g_soc_list, my_dirname(argv[0]) + "/desc", false);
    /* load directories */
    for(size_t i = 0; i < g_soc_dir.size(); i++)
        load_dir(g_soc_list, g_soc_dir[i], true);
    /* load files */
    for(size_t i = 0; i < g_soc_files.size(); i++)
        load_file(g_soc_list, g_soc_files[i], true);
    /* filter soc list */
    if(g_allowed_soc.size() > 0)
    {
        for(size_t i = 0; i < g_soc_list.size(); i++)
        {
            if(g_allowed_soc.find(g_soc_list[i].name) == g_allowed_soc.end())
            {
                std::swap(g_soc_list[i], g_soc_list.back());
                g_soc_list.pop_back();
                i--;
            }
        }
    }
    /* print */
    if(g_verbose >= 1)
    {
        fprintf(stderr, "Available socs after filtering:");
        for(size_t i = 0; i < g_soc_list.size(); i++)
            fprintf(stderr, " %s", g_soc_list[i].name.c_str());
        fprintf(stderr, "\n");
    }

    std::string action = argv[optind];
    argc -= optind + 1;
    argv += optind + 1;
    if(action == "find")
        return do_find(g_soc_list, argc, argv, false);
    if(action == "describe")
        return do_find(g_soc_list, argc, argv, true);
    if(action == "decode")
        return do_decode(g_soc_list, argc, argv);
    fprintf(stderr, "unknown action '%s'\n", action.c_str());
    return 1;
}
