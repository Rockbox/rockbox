/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 by Amaury Pouly
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
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>
#include <fstream>
#include <set>

using namespace soc_desc;

#define HEADERGEN_VERSION   "3.1.0"

/**
 * Useful stuff
 */

std::string tolower(const std::string s)
{
    std::string res = s;
    for(size_t i = 0; i < s.size(); i++)
        res[i] = ::tolower(res[i]);
    return res;
}

std::string toupper(const std::string& s)
{
    std::string res = s;
    for(size_t i = 0; i < s.size(); i++)
        res[i] = ::toupper(res[i]);
    return res;
}

template< typename T >
std::string to_str(const T& v)
{
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

template< typename T >
std::string to_hex(const T& v)
{
    std::ostringstream oss;
    oss << "0x" << std::hex << v;
    return oss.str();
}

template< typename T >
std::string to_hex_nice(const T& v)
{
    std::ostringstream oss;
    // don't print 0x for single-digit numbers
    if(v >= 0 && v < 10)
        oss << v;
    else
    oss << "0x" << std::hex << v;
    return oss.str();
}

std::string strsubst(const std::string& str, const std::string& pattern,
    const std::string& subst)
{
    std::string res = str;
    size_t pos = 0;
    while((pos = res.find(pattern, pos)) != std::string::npos)
    {
        res.replace(pos, pattern.size(), subst);
        pos += subst.size();
    }
    return res;
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
        printf("\n");
}

void create_dir(const std::string& path)
{
    mkdir(path.c_str(), 0755);
}

void create_alldir(const std::string& prefix, const std::string& path)
{
    size_t p = path.find('/');
    if(p == std::string::npos)
        return;
    create_dir(prefix + "/" + path.substr(0, p));
    create_alldir(prefix + "/" + path.substr(0, p), path.substr(p + 1));
}

/**
 * Printer utils
 */

struct limited_column_context_t
{
    limited_column_context_t(size_t nr_col = 80)
        :m_nr_col(nr_col), m_prevent_wordcut(true) {}
    void set_prefix(const std::string& prefix) { m_prefix = prefix; }
    void add(const std::string& text)
    {
        for(size_t i = 0; i < text.size();)
        {
            size_t offset = 0;
            if(m_cur_line.size() == 0)
                m_cur_line = m_prefix;
            size_t len = std::min(text.size() - i, m_nr_col - m_cur_line.size());
            // prevent word cut
            if(m_prevent_wordcut && !isspace(text[i + len - 1]) && 
                    i + len < text.size() && !isspace(text[i + len]))
            {
                size_t pos = text.find_last_of(" \t\n\v\r\f", i + len - 1);
                if(pos == std::string::npos || pos < i)
                    len = 0;
                else
                    len = pos - i + 1;
            }
            size_t pos = text.find('\n', i);
            if(pos != std::string::npos && pos <= i + len)
            {
                offset = 1;
                len = pos - i;
            }
            m_cur_line += text.substr(i, len);
            // len == 0 means we need a new line
            if(m_cur_line.size() == m_nr_col || len == 0)
            {
                m_lines.push_back(m_cur_line);
                m_cur_line = "";
            }
            i += len + offset;
        }
    }

    std::ostream& print(std::ostream& oss)
    {
        for(size_t i = 0; i < m_lines.size(); i++)
            oss << m_lines[i] << "\n";
        if(m_cur_line.size() != 0)
            oss << m_cur_line << "\n";
        return oss;
    }

    std::string str()
    {
        std::ostringstream oss;
        print(oss);
        return oss.str();
    }

    std::vector< std::string > m_lines;
    std::string m_cur_line;
    std::string m_prefix;
    size_t m_nr_col;
    bool m_prevent_wordcut;
};

struct define_align_context_t
{
    define_align_context_t():m_max_name(0) {}
    void add(const std::string& name, const std::string& val)
    {
        m_lines.push_back(std::make_pair(name, val));
        m_max_name = std::max(m_max_name, name.size());
    }

    void add(const define_align_context_t& o)
    {
        m_lines.insert(m_lines.end(), o.m_lines.begin(), o.m_lines.end());
        m_max_name = std::max(m_max_name, o.m_max_name);
    }

    void add_raw(const std::string& line)
    {
        m_lines.push_back(std::make_pair("", line));
    }

    std::ostream& print(std::ostream& oss)
    {
        std::string define = "#define ";
        size_t align = define.size() + m_max_name + 1;
        align = ((align + 3) / 4) * 4;

        for(size_t i = 0; i < m_lines.size(); i++)
        {
            std::string name = m_lines[i].first;
            // raw entry ?
            if(name.size() != 0)
            {
                name.insert(name.end(), align - define.size() - name.size(), ' ');
                oss << define << name << m_lines[i].second << "\n";
            }
            else
                oss << m_lines[i].second;
        }
        return oss;
    }

    void clear()
    {
        m_max_name = 0;
        m_lines.clear();
    }

    size_t m_max_name;
    std::vector< std::pair< std::string, std::string > > m_lines;
};

limited_column_context_t print_description(const std::string& desc, const std::string& prefix)
{
    limited_column_context_t ctx;
    if(desc.size() == 0)
        return ctx;
    ctx.set_prefix(prefix);
    ctx.add(desc);
    return ctx;
}

void print_copyright(std::ostream& fout, const std::vector< soc_ref_t >& socs)
{
    fout << "\
/***************************************************************************\n\
 *             __________               __   ___.\n\
 *   Open      \\______   \\ ____   ____ |  | _\\_ |__   _______  ___\n\
 *   Source     |       _//  _ \\_/ ___\\|  |/ /| __ \\ /  _ \\  \\/  /\n\
 *   Jukebox    |    |   (  <_> )  \\___|    < | \\_\\ (  <_> > <  <\n\
 *   Firmware   |____|_  /\\____/ \\___  >__|_ \\|___  /\\____/__/\\_ \\\n\
 *                     \\/            \\/     \\/    \\/            \\/\n\
 * This file was automatically generated by headergen, DO NOT EDIT it.\n\
 *\n";
    std::set< std::string > authors;
    for(auto& soc : socs)
        for(auto& author : soc.get()->author)
            authors.insert(author);
    for(auto& author : authors)
        fout << " * Copyright (C) by " << author << "\n";
    fout << "\
 *\n\
 * This program is free software; you can redistribute it and/or\n\
 * modify it under the terms of the GNU General Public License\n\
 * as published by the Free Software Foundation; either version 2\n\
 * of the License, or (at your option) any later version.\n\
 *\n\
 * This software is distributed on an \"AS IS\" basis, WITHOUT WARRANTY OF ANY\n\
 * KIND, either express or implied.\n\
 *\n\
 ****************************************************************************/\n";
}

void print_guard(std::ostream& fout, const std::string& guard, bool begin)
{
    if(begin)
    {
        fout << "#ifndef " << guard << "\n";
        fout << "#define " << guard << "\n";
    }
    else
        fout << "\n#endif /* " << guard << "*/\n";
}

std::string call_arg_string(std::vector< std::string >& args)
{
    if(args.empty())
        return "";
    std::ostringstream oss;
    oss << "(";
    for(size_t i = 0; i < args.size(); i++)
    {
        if(i != 0)
            oss << ", ";
        oss << args[i];
    }
    oss << ")";
    return oss.str();
}

/**
 * Generator interface
 */

class abstract_generator
{
public:
    abstract_generator();
    /// set output directory (default is current directory)
    void set_output_dir(const std::string& dir);
    /// add a SoC to the list
    void add_soc(const soc_t& soc);
    /// generate headers, returns true on success
    virtual bool generate(error_context_t& ctx) = 0;
    /// set list of "things" to print and matching policy, if only=true, the generator prints only
    /// things list, otherwise it prints things NOT in the list
    /// the interpretation of "things" is specific to each generator
    void set_print_list(const std::set< std::string >& list, bool only);
protected:
    /// generate a string computing the address of a node, relative to its parent
    /// if the index is NO_INDEX and the node has a range, a parametric formula is generated, in this
    /// case the variable name from the available names array is poped, used in the formula and added
    /// to the array of used variables
    static const size_t NO_INDEX = std::numeric_limits<std::size_t>::max();
    std::string inst_relative_address(const instance_t& node, size_t index,
        std::vector< std::string >& vars, std::vector< std::string >& fresh_vars) const;

    std::vector< soc_t > m_soc; /// list of socs
    std::string m_outdir; /// output directory path
    std::set< std::string > m_print_list; // blocks to print (or not print)
    bool m_print_list_only; // if true, only print list
};

abstract_generator::abstract_generator()
{
    // print everything by default
    m_print_list_only = false;
}

void abstract_generator::set_print_list(const std::set< std::string >& list, bool only)
{
    m_print_list = list;
    m_print_list_only = only;
}

void abstract_generator::set_output_dir(const std::string& dir)
{
    m_outdir = dir;
}

void abstract_generator::add_soc(const soc_t& soc)
{
    m_soc.push_back(soc);
}

std::string abstract_generator::inst_relative_address(const instance_t& inst, size_t index,
    std::vector< std::string >& vars, std::vector< std::string >& fresh_vars) const
{
    std::ostringstream oss;

    if(inst.type == instance_t::RANGE)
    {
        vars.push_back(fresh_vars.back());
        fresh_vars.pop_back();
        if(inst.range.type == range_t::STRIDE)
        {
            if(index == NO_INDEX)
            {
                std::string min_first;
                if(inst.range.first != 0)
                    min_first = " - " + to_str(inst.range.first);
                oss << to_hex(inst.range.base) << " + ";
                oss << "(" << vars.back() << min_first << ") * " << to_hex(inst.range.stride);
            }
            else
                oss << to_hex(inst.range.base + (index - inst.range.first) * inst.range.stride);
        }
        else if(inst.range.type == range_t::FORMULA)
        {
            if(index != NO_INDEX)
            {
                soc_word_t res;
                std::map< std::string, soc_word_t > vars;
                vars[inst.range.variable] = index;
                error_context_t ctx;
                if(!evaluate_formula(inst.range.formula, vars, res, "", ctx))
                    oss << "#error cannot evaluate formula";
                else
                    oss << to_hex(res);
            }
            else
                oss << strsubst(inst.range.formula, inst.range.variable,
                    "(" + vars.back() + ")");
        }
        else if(inst.range.type == range_t::LIST)
        {
            if(index == NO_INDEX)
            {
                oss << "(";
                for(size_t j = 0; j + 1 < inst.range.list.size(); j++)
                {
                    oss << "(" << vars.back() << ") == " << (inst.range.first + j)
                        << " ? " << to_hex(inst.range.list[j]) << " : ";
                }
                oss << to_hex(inst.range.list.back()) << ")";
            }
            else
                oss << to_hex(inst.range.list[index - inst.range.first]);
        }
        else
        {
            return "#error unknown range type";
        }
    }
    else if(inst.type == instance_t::SINGLE)
        oss << to_hex(inst.addr);
    else
        return "#error unknown instance type";
    return oss.str();
}

/**
 * Common Generator
 *
 * This generator is an abstract class which can generate register headers while
 * giving the user a lot of control on how the macros are named and generated.
 *
 * The first thing the generator will want to know is whether you want to generate
 * selector headers or not. Selector headers are used when generating headers from
 * several SoCs: the selector will include the right header based on some user-defined
 * logic. For example, imagine you have two socs vsoc1000 and vsoc2000, you could
 * generate the following files and directories:
 *
 *   regs/
 *     regs-vsoc.h [selector]
 *     vsoc1000/
 *       regs-vsoc.h [vsoc1000 header]
 *     vsoc2000/
 *       regs-vsoc.h [vsoc2000 header]
 *
 * The generator will call has_selectors() to determine if it should generate
 * selector files or not. If it returns true, it will call selector_soc_dir()
 * for each of the socs to know in which subdirectory it should include the headers.
 * The generator will create one macro per soc, which name is given by
 * selector_soc_macro() and it will finally include the select header YOU will
 * have to write, which name is given by selector_include_header()
 * The selector file will typically look like this:
 *
 *   [regs-vsoc.h]
 *   #define VSOC1000_INCLUDE "vsoc1000/regs-vsoc.h" // returned by selector_soc_macro(vsoc1000)
 *   #define VSOC2000_INCLUDE "vsoc2000/regs-vsoc.h" // returned by selector_soc_macro(vsoc2000)
 *   #include "regs-select.h" // returned by selector_include_header()
 *
 * NOTE: it may happen that some header only exists in some soc (for example
 * in vsoc2000 but not in vsoc1000), in this case the macros will only be
 * generated for the socs in which it exists.
 * NOTE: and this is *VERY* important: the register selector file MUST NOT
 * be protected by include guards since it will included several times, once
 * for each register file.
 *
 * The generator will create one set of files per soc. For each register, it
 * will call register_header() to determine in which file you want the register
 * to be put. You can put everything in one file, or one register per file,
 * that's entirely up to you. Here is an example:
 *
 *   register_header(vsoc1000.cpu.ctrl) -> "regs-cpu.h"
 *   register_header(vsoc1000.cpu.reset) -> "regs-cpu.h"
 *   register_header(vsoc1000.cop.ctrl) -> "regs-cop.h"
 *   register_header(vsoc1000.cop.magic) -> "regs-magic.h"
 *
 *   regs-cpu.h:
 *     vsoc1000.cpu.ctrl
 *     vsoc1000.cpu.reset
 *   regs-cop.h:
 *     vsoc1000.cop.ctrl
 *   regs-magic.h
 *     vsoc1000.cop.magic
 *
 * Once the list of register files is determine, it will begin generating each
 * file. A register header is always protected from double inclusion by an
 * include guard. You can tweak the name by redefining header_include_guard()
 * or keep the default behaviour which generates something like this:
 *
 *   #ifndef __HEADERGEN_REGS_CPU_H__
 *   #define __HEADERGEN_REGS_CPU_H__
 *   ...
 *   #endif
 *
 * NOTE: the tool will also generate a copyright header which contains the Rockbox
 * logo, a list of soc names and version, and authors in the description file.
 * This part cannot be customised at the moment.
 *
 * In order to list all registers, the generator will recursively enumerate all
 * nodes and instances. When a instance is of RANGE type, the generator can generate
 * two types of macros:
 * - one for each instance
 * - one with an index parameter
 * The generator will ask you if you want to generate one for each instance by
 * calling register_flag(RF_GENERATE_ALL_INST) and if you want to generate a
 * parametrized one by calling register_flag(RF_GENERATE_PARAM_INST). You can
 * generate either one or both (or none).
 *
 *   register_flag(vsoc.int.enable, RF_GENERATE_ALL_INST) -> true
 *   => will generate vsoc.int.enable[1], vsoc.int.enable[2], ...
 *   register_flag(vsoc.int.enable, RF_GENERATE_PARAM_INST) -> true
 *   => will generate vsoc.int.enable[n]
 *
 * This process will give a list of pseudo-instances: these are register instances
 * where some ranges have a given index and some ranges are parametric. The generator
 * will create one set of macro per pseudo-instance. The exact list of macros
 * generated is the following:
 * - ADDR: this macro will contain the address of the register (possibly parametrized)
 * - TYPE: this macro will contain the type of the register (width, access)
 * - NAME: this macro expands to the name of the register (useful for fields and other variants)
 * - INDEX: this macro expands to the index of the register (or empty is not indexed)
 * - VAR: this macros expands to a fake variable that can be read/written
 * - for each field F:
 *   - BP: this macro contains the bit position of F
 *   - BM: this macro contains the bit mask of F
 *   - for each field enum value V:
 *     - BV: this macro contains the value of V (not shifted to the position)
 *   - BF: this macro generate the mask for a value: (((v) << pos) & mask)
 *   - BFV: same as BF but the parameter is the name of an enum value
 *   - BFM: this macro is like BF but ignores value returns mask
 *   - BFMV: like BFV but returns masks like BFM
 * The generator will also create the following macros for each variant:
 * - ADDR: same as ADDR but with offset
 * - TYPE: same as TYPE but depends on variant
 * - NAME: same as NAME (ie same fields)
 * - INDEX: same as INDEX
 *
 * In order to generate the macro name, the generate relies on you providing detailled
 * information. Given an pseudo-instance I and a macro type MT, the generator
 * will always call type_xfix(MT) to know which prefix/suffix you want for the 
 * macro and generate names of the form:
 *   type_xfix(MT, true)basename(I)type_xfix(MT, false)
 * The basename() functions will call inst_prefix() for each each instance on the
 * pseudo-instance path, and then instance_name() to know the name. You can
 * (and must) create a different name for parametrised instances. The basename
 * looks like this for pseudo-inst i1.i2.i3....in:
 *   instance_name(i1)inst_prefix(i1.i2)instance_name(i1.i2)...inst_name(i1.i2...in)
 * For field macros, the process is the same with an extra prefix returned by
 * field_prefix() and you can select the field name with field_name() to obtain
 * for example for field F in I:
 *   type_xfix(MT,true)basename(I)field_prefix()field_name(I.F)type_xfix(MT,false)
 * For field enum macros, there is an extra prefix given by enum_prefix()
 * and the enum name is given by enum_name()
 * For variants, the basename is surrounded by prefix/suffix given by variant_xfix():
 *   variant_xfix(var, true)basename(I)variant_xfix(var, false)
 *
 * Let's illustrate this on example
 *   type_xfix(MT_REG_ADDR, true) -> "RA_x"
 *   type_xfix(MT_REG_ADDR, false) -> "x_AR"
 *   instance_name(vsoc1000.cpu) -> "CPU"
 *   inst_prefix(vsoc1000.cpu.ctrl) -> "_"
 *   instance_name(vsoc1000.cpu.ctrl) -> "CTRL"
 *   => RA_xCPU_CTRLx_AR
 *   type_xfix(MT_FIELD_BF, true) -> "BF_"
 *   type_xfix(MT_FIELD_BF, false) -> ""
 *   field_prefix() -> "__"
 *   field_name(vsoc1000.cpu.ctrl.speed) -> "SPEED"
 *   => BF_CPU_CTRL__SPEED
 *   variant_xfix("set", true) -> "VAR_"
 *   variant_xfix("set", false) -> "_SET"
 *   => HW_VAR_CPU_CTRL_SET
 *
 * The generator will also create a macro header file, it will call macro_header()
 * once to know the name of this file.
 * The macro header contains useful macro to read, write and manipulate registers
 * and fields. You must implement the macro_header_macro() method to let the
 * generator know the name of each macro. Note that the macro header macros
 * depend on the specific naming of the other macros, which are given by
 * type_xfix() and field_prefix() most notably. The exact list of generated macros
 * is the following:
 * - BF_OR: field ORing (see details below)
 * - BF_OM: mask ORing (same as BF_OR but except field value is the mask)
 *
 * The BF_OR macro is one of the most important and useful macro. It allows for
 * compact ORing of register field, both for immediate values or value names.
 * For this macro to work properly, there are constraints that the generator
 * must satisfy. Notably, type_xfix(MT_FIELD_BF, true) == type_xfix(MT_FIELD_BFV, true)
 * and similarly for MT_FIELD_BM
 * The general format is as follows:
 *
 *   BF_OR(<reg>, <f1>, <f2>, ...) expands to BF(<reg>,<f1>) | BF(<reg>,<f2>) | ...
 *   BM_OR(<reg>, <f1>, <f2>, ...) expands to BM(<reg>,<f1>) | BM(<reg>,<f2>) | ...
 *
 * Typical usages of this macro will be of the form:
 *
 * BF_OR(<reg>, <f1>(<v1>), <f2>(<v2>), <f3name>(<v3name>), ...)
 * BM_OR(<reg>, <f1>, <f2>, <f3>, ...)
 *
 *
 * Let's illustrate this on example.
 *   type_xfix(MT_FIELD_BF, true) -> "BF_"
 *   type_xfix(MT_FIELD_BFV, true) -> "BF_"
 *   type_xfix(MT_FIELD_BF, false) -> "BF_"
 *   type_xfix(MT_FIELD_BF, false) -> "_V"
 *   field_prefix() -> "__"
 *   enum_prefix() -> "___"
 *   macro_header_macro(MMM_BF_OR) -> "BF_OR"
 *   => BF_OR(<reg>, <f1>, <f2>, ...) expands to BF_<reg>__<f1> | BF_<reg>__<f2> | ...
 *   => BF_OR(CPU_CTRL, DIVIDER(1), PRIO_V(HIGH), SPEED(100))
 *      expands to
 *      BF_CPU_CTRL__DIVIDER(1) | BF_CPU_CTRL__DIVIDER_V(HIGH) | BF_CPU_CTRL__SPEED(1000)
 *      expands to
 *      BF_CPU_CTRL__DIVIDER(1) | BF_CPU_CTRL__DIVIDER(BV_CPU_CTRL__DIVIDER___HIGH) | BF_CPU_CTRL__SPEED(1000)
 *      and so on...
 *
 */
class common_generator : public abstract_generator
{
    struct pseudo_node_inst_t
    {
        node_inst_t inst;
        std::vector< bool > parametric;
    };
public:
    virtual bool generate(error_context_t& ctx);
private:
    void gather_files(const pseudo_node_inst_t& inst, const std::string& prefix,
        std::map< std::string, std::vector< pseudo_node_inst_t > >& map);
    void print_inst(const pseudo_node_inst_t& inst, bool end = true); // debug
    std::vector< soc_ref_t > list_socs(const std::vector< pseudo_node_inst_t >& list);
    bool generate_register(std::ostream& os, const pseudo_node_inst_t& reg);
    bool generate_macro_header(error_context_t& ectx);
protected:
    /// return true to generate support macros
    /// if false then all macros are disabled except MT_REG_ADDR, MT_REG_VAR,
    /// and MT_FIELD_*. Also the macro header is not created
    virtual bool has_support_macros() const = 0;
    /// return true to generate selector files
    virtual bool has_selectors() const = 0;
    /// [selector only] return the directory name for the soc
    virtual std::string selector_soc_dir(const soc_ref_t& ref) const = 0;
    /// [selector only] return the header to include to select betweens socs
    virtual std::string selector_include_header() const = 0;
    /// [selector only] return the name of the macro to emit for each soc
    virtual std::string selector_soc_macro(const soc_ref_t& ref) const = 0;
    /// return the relative filename in which to put the register
    virtual std::string register_header(const node_inst_t& inst) const = 0;
    /// return the include guard for a file (default does its best)
    virtual std::string header_include_guard(const std::string& filename);
    /// return the name of the macros header to generate
    virtual std::string macro_header() const = 0;
    /// macro from macro header
    enum macro_name_t
    {
        MN_REG_READ, /// register read
        MN_FIELD_READ, /// register's field read
        MN_FIELD_READX, /// register value field read
        MN_REG_WRITE, /// register write
        MN_REG_SET, /// register set
        MN_REG_CLEAR, /// register clear
        MN_REG_CLEAR_SET, /// register clear then set
        MN_FIELD_WRITE, /// register's field(s) write
        MN_FIELD_WRITEX, /// register's field(s) write
        MN_FIELD_OVERWRITE, /// register full write
        MN_FIELD_SET, /// register's field(s) set
        MN_FIELD_CLEAR, /// register's field(s) clear
        MN_FIELD_TOG, /// register's field(s) toggle
        MN_FIELD_CLEAR_SET, /// register's field(s) clear then set
        MN_FIELD_OR, /// field ORing
        MN_FIELD_OR_MASK, /// field ORing but in fact mask ORing
        MN_MASK_OR, /// mask ORing
        MN_GET_VARIANT, /// get register variant
        MN_VARIABLE, /// return variable-like for register
    };
    /// return macro name defined in the macro header
    virtual std::string macro_name(macro_name_t macro) const = 0;
    /// flag to consider
    enum register_flag_t
    {
        RF_GENERATE_ALL_INST, /// for range instance, generate one macro set per instance ?
        RF_GENERATE_PARAM_INST, /// for range instance, generate a parametrised macro set ?
    };
    /** tweak instance generaton and its children
     * NOTE: for range flags, although the instance will be numbered, the index is
     * to be ignored */
    virtual bool register_flag(const node_inst_t& inst, register_flag_t flag) const = 0;
    /// prefix/suffix type
    enum macro_type_t
    {
        MT_REG_ADDR, /// register address
        MT_REG_TYPE, /// register type
        MT_REG_NAME, /// register prefix for fields
        MT_REG_INDEX, /// register index/indices
        MT_REG_VAR, /// variable-like macro
        MT_FIELD_BP, /// bit position of a field
        MT_FIELD_BM, /// bit mask of a field
        MT_FIELD_BV, /// bit value of a field enum
        MT_FIELD_BF, /// bit field value: (v << pos) & mask
        MT_FIELD_BFM, /// ignore value and return mask
        MT_FIELD_BFV, /// bit field enum value: (enum_v << pos) & mask
        MT_FIELD_BFMV, /// ignore value and return mask
        MT_IO_TYPE, /// register io type
    };
    /// register access types
    enum access_type_t
    {
        AT_RO, /// read-only: write are disallowed
        AT_RW, /// read-write: read/writes are allowed, field write by generated a RMW
        AT_WO, /// write-only: read are disallowed, field write will set other fields to 0
    };
    /// register operation
    enum register_op_t
    {
        RO_VAR, /// variable-like
        RO_READ, /// read
        RO_WRITE, /// write
        RO_RMW, /// read-modify-write
    };
    /// return type prefix/suffix for register macro
    virtual std::string type_xfix(macro_type_t type, bool prefix) const = 0;
    /// return variant prefix/suffix
    virtual std::string variant_xfix(const std::string& variant, bool prefix) const = 0;
    /// return instance prefix in macro name
    virtual std::string inst_prefix(const node_inst_t& inst) const = 0;
    /// return field prefix in field macro names
    virtual std::string field_prefix() const = 0;
    /// return field enum prefix in field macro names
    virtual std::string enum_prefix() const = 0;
    /// return the field enum name in field macro names
    virtual std::string enum_name(const enum_ref_t& enum_) const = 0;
    /// return instance name in macro, default is instance name then index if any
    virtual std::string instance_name(const node_inst_t& inst, bool parametric) const;
    /// return field name in macro, default is field name
    virtual std::string field_name(const field_ref_t& field) const;
    /// return the complete macro name with prefix, default uses all the other functions
    virtual std::string macro_basename(const pseudo_node_inst_t& inst) const;
    /// return the complete macro name with prefix, default uses macro_basename()
    virtual std::string macro_basename(const pseudo_node_inst_t& inst, macro_type_t type) const;
    /// generate address string for a register instance, and fill the parametric
    /// argument list, default does it the obvious way and parameters are _n1, _n2, ...
    virtual std::string register_address(const pseudo_node_inst_t& reg,
        std::vector< std::string >& params) const;
    /// return access type for a variant and a given register access
    /// NOTE variant with the unspecified access type will be promoted to register access
    virtual access_type_t register_access(const std::string& variant, access_t access) const = 0;
    /// return register type name
    virtual std::string register_type_name(access_type_t access, int width) const;
    /// get register operation name
    virtual std::string register_op_name(register_op_t op) const;
    /// register operation prefix
    virtual std::string register_op_prefix() const;
    /// generate a macro pasting that is safe even when macro is empty
    /// ie: safe_macro_paste(false, "A") -> ##A
    /// ie: safe_macro_paste(false, "A") ->
    std::string safe_macro_paste(bool left, const std::string& macro) const;
    /// generate SCT macros using register variant ?
    virtual bool has_sct() const = 0;
    /// return the associated SCT variant (only if RF_GENERATE_SCT)
    /// empty means don't generate
    virtual std::string sct_variant(macro_name_t name) const = 0;
};

std::string common_generator::instance_name(const node_inst_t& inst, bool parametric) const
{
    std::ostringstream oss;
    oss << inst.name();
    if(inst.is_indexed() && !parametric)
        oss << inst.index();
    return oss.str();
}

std::string common_generator::field_name(const field_ref_t& field) const
{
    return field.get()->name;
}

std::string common_generator::macro_basename(const pseudo_node_inst_t& inst_) const
{
    pseudo_node_inst_t inst = inst_;
    std::string str;
    while(!inst.inst.is_root())
    {
        str = instance_name(inst.inst, inst.parametric.back()) + str;
        inst.inst = inst.inst.parent();
        inst.parametric.pop_back();
        if(!inst.inst.is_root())
            str = inst_prefix(inst.inst) + str;
    }
    return str;
}

std::string common_generator::macro_basename(const pseudo_node_inst_t& inst, macro_type_t type) const
{
    return type_xfix(type, true) + macro_basename(inst);
}

void common_generator::print_inst(const pseudo_node_inst_t& inst, bool end)
{
    if(!inst.inst.is_root())
    {
        pseudo_node_inst_t p = inst;
        p.inst = p.inst.parent();
        p.parametric.pop_back();
        print_inst(p, false);
        printf(".%s", inst.inst.name().c_str());
        if(inst.inst.is_indexed())
        {
            if(inst.parametric.back())
                printf("[]");
            else
                printf("[%u]", (unsigned)inst.inst.index());
        }
    }
    else
    {
        printf("%s", inst.inst.soc().get()->name.c_str());
    }
    if(end)
        printf("\n");
}

std::string common_generator::register_type_name(access_type_t access, int width) const
{
    std::ostringstream oss;
    oss << width << "_";
    switch(access)
    {
        case AT_RO: oss << "RO"; break;
        case AT_RW: oss << "RW"; break;
        case AT_WO: oss << "WO"; break;
        default: oss << "error"; break;
    }
    return type_xfix(MT_IO_TYPE, true) + oss.str() + type_xfix(MT_IO_TYPE, false);
}

std::string common_generator::register_op_name(register_op_t op) const
{
    switch(op)
    {
        case RO_VAR: return "VAR";
        case RO_READ: return "RD";
        case RO_WRITE: return "WR";
        case RO_RMW: return "RMW";
        default: return "<op>";
    }
}

std::string common_generator::register_op_prefix() const
{
    return "_";
}

std::string common_generator::safe_macro_paste(bool left, const std::string& macro) const
{
    if(macro.size() == 0)
        return "";
    return left ? "##" + macro : macro + "##";
}

void common_generator::gather_files(const pseudo_node_inst_t& inst, const std::string& prefix,
    std::map< std::string, std::vector< pseudo_node_inst_t > >& map)
{
    if(inst.inst.node().reg().valid())
        map[prefix + register_header(inst.inst)].push_back(inst);
    // if asked, generate one for each instance
    std::vector< node_inst_t > list = inst.inst.children();
    for(size_t i = 0; i < list.size(); i++)
    {
        pseudo_node_inst_t c = inst;
        c.inst = list[i];
        if(!list[i].is_indexed() || register_flag(inst.inst, RF_GENERATE_ALL_INST))
        {
            c.parametric.push_back(false);
            gather_files(c, prefix, map);
            c.parametric.pop_back();
        }
        if(list[i].is_indexed() && register_flag(list[i], RF_GENERATE_PARAM_INST))
        {
            bool first = list[i].index() == list[i].get()->range.first;
            if(first)
            {
                c.parametric.push_back(true);
                gather_files(c, prefix, map);
            }
        }
    }
}

std::vector< soc_ref_t > common_generator::list_socs(const std::vector< pseudo_node_inst_t >& list)
{
    std::set< soc_ref_t > socs;
    std::vector< pseudo_node_inst_t >::const_iterator it = list.begin();
    for(; it != list.end(); ++it)
        socs.insert(it->inst.soc());
    std::vector< soc_ref_t > soc_list;
    for(std::set< soc_ref_t >::iterator jt = socs.begin(); jt != socs.end(); ++jt)
        soc_list.push_back(*jt);
    return soc_list;
}

std::string common_generator::header_include_guard(const std::string& filename)
{
    std::string guard = "__HEADERGEN_" + toupper(filename) + "__";
    for(size_t i = 0; i < guard.size(); i++)
        if(!isalnum(guard[i]))
            guard[i] = '_';
    return guard;
}

std::string common_generator::register_address(const pseudo_node_inst_t& reg,
    std::vector< std::string >& params) const
{
    std::ostringstream oss;
    std::vector< std::string > vars;
    size_t cnt = reg.parametric.size();
    while(cnt > 0)
        vars.push_back("_n" + to_str(cnt--));
    oss << "(";
    for(size_t i = 0; i < reg.parametric.size(); i++)
    {
        if(i != 0)
            oss << " + ";
        node_inst_t ninst = reg.inst.parent(reg.parametric.size() - i - 1);
        instance_t& inst = *ninst.get();
        size_t index = ninst.index();
        if(reg.parametric[i])
            index = NO_INDEX;
        oss << inst_relative_address(inst, index, params, vars);
    }
    oss << ")";
    return oss.str();
}

bool common_generator::generate_register(std::ostream& os, const pseudo_node_inst_t& reg)
{
    os << "\n";
    define_align_context_t ctx;
    std::vector< std::string > params;
    std::string addr = register_address(reg, params);
    std::string basename = macro_basename(reg);
    std::string param_str;
    std::string param_str_no_paren;
    std::string param_str_no_paren_comma;
    if(params.size() > 0)
    {
        for(size_t i = 0; i < params.size(); i++)
            param_str_no_paren += (i == 0 ? "" : ",") + params[i];
        param_str = "(" + param_str_no_paren + ")";
        param_str_no_paren_comma = param_str_no_paren + ",";
    }
    std::string bp_prefix = macro_basename(reg, MT_FIELD_BP) + field_prefix();
    std::string bm_prefix = macro_basename(reg, MT_FIELD_BM) + field_prefix();
    std::string bf_prefix = macro_basename(reg, MT_FIELD_BF) + field_prefix();
    std::string bfm_prefix = macro_basename(reg, MT_FIELD_BFM) + field_prefix();
    std::string bfv_prefix = macro_basename(reg, MT_FIELD_BFV) + field_prefix();
    std::string bfmv_prefix = macro_basename(reg, MT_FIELD_BFMV) + field_prefix();

    register_ref_t regr = reg.inst.node().reg();
    /* handle register the same way as variants */
    std::vector< std::string > var_prefix;
    std::vector< std::string > var_suffix;
    std::vector< std::string > var_addr;
    std::vector< access_type_t > var_access;
    var_prefix.push_back("");
    var_suffix.push_back("");
    var_access.push_back(register_access("", regr.get()->access));
    var_addr.push_back(addr);
    std::vector< variant_ref_t > variants = regr.variants();
    for(size_t i = 0; i < variants.size(); i++)
    {
        var_prefix.push_back(variant_xfix(variants[i].type(), true));
        var_suffix.push_back(variant_xfix(variants[i].type(), false));
        var_addr.push_back("(" + type_xfix(MT_REG_ADDR, true) + basename +
            type_xfix(MT_REG_ADDR, false) + param_str + " + " +
            to_hex(variants[i].offset()) + ")");
        access_t acc = variants[i].get()->access;
        if(acc == UNSPECIFIED)
            acc = regr.get()->access; // fallback to register access
        var_access.push_back(register_access(variants[i].type(), acc));
    }

    for(size_t i = 0; i < var_prefix.size(); i++)
    {
        std::string var_basename = var_prefix[i] + basename + var_suffix[i];
        std::string macro_var = type_xfix(MT_REG_VAR, true) + var_basename +
            type_xfix(MT_REG_VAR, false);
        std::string macro_addr = type_xfix(MT_REG_ADDR, true) + var_basename +
            type_xfix(MT_REG_ADDR, false);
        std::string macro_type = type_xfix(MT_REG_TYPE, true) + var_basename +
            type_xfix(MT_REG_TYPE, false);
        std::string macro_prefix = type_xfix(MT_REG_NAME, true) + var_basename +
            type_xfix(MT_REG_NAME, false);
        std::string macro_index = type_xfix(MT_REG_INDEX, true) + var_basename +
            type_xfix(MT_REG_INDEX, false);
        /* print VAR macro:
         * if we have support macros then we generate something like HW(basename)
         * where HW is some support macros to support complex operations. Otherwise
         * we just generate something like (*(volatile unsigned uintN_t)basename_addr) */
        if(!has_support_macros())
        {
            std::ostringstream oss;
            oss << "(*(volatile uint" << regr.get()->width << "_t *)" << macro_addr + param_str << ")";
            ctx.add(macro_var + param_str, oss.str());
        }
        else
            ctx.add(macro_var + param_str, macro_name(MN_VARIABLE) + "(" + var_basename + param_str + ")");
        /* print ADDR macro */
        ctx.add(macro_addr + param_str, var_addr[i]);
        /* disable macros if needed */
        if(has_support_macros())
        {
            /* print TYPE macro */
            ctx.add(macro_type + param_str, register_type_name(var_access[i], regr.get()->width));
            /* print PREFIX macro */
            ctx.add(macro_prefix + param_str, basename);
            /* print INDEX macro */
            ctx.add(macro_index + param_str, param_str);
        }
    }
    /* print fields */
    std::vector< field_ref_t > fields = regr.fields();
    for(size_t i = 0; i < fields.size(); i++)
    {
        /* print BP macro: pos */
        std::string fname = field_name(fields[i]);
        ctx.add(bp_prefix + fname + type_xfix(MT_FIELD_BP, false),
            to_str(fields[i].get()->pos));
        /* print BM macro: mask */
        ctx.add(bm_prefix + fname + type_xfix(MT_FIELD_BM, false),
            to_hex(fields[i].get()->bitmask()));
        std::vector< enum_ref_t > enums = fields[i].enums();
        std::string bv_prefix = macro_basename(reg, MT_FIELD_BV) + field_prefix()
            + fname + enum_prefix();
        /* print BV macros: enum_v */
        for(size_t j = 0; j < enums.size(); j++)
            ctx.add(bv_prefix + enum_name(enums[j]) + type_xfix(MT_FIELD_BV, false),
                to_hex(enums[j].get()->value));
        /* print BF macro: (((v) & mask) << pos) */
        ctx.add(bf_prefix + fname + type_xfix(MT_FIELD_BF, false) + "(v)",
            "(((v) & " + to_hex(fields[i].get()->unshifted_bitmask()) + ") << " +
            to_str(fields[i].get()->pos) + ")");
        /* print BFM macro: masl */
        ctx.add(bfm_prefix + fname + type_xfix(MT_FIELD_BFM, false) + "(v)",
            bm_prefix + fname + type_xfix(MT_FIELD_BM, false));
        /* print BFV macro: ((enum_v) << pos) & mask) */
        ctx.add(bfv_prefix + fname + type_xfix(MT_FIELD_BFV, false) + "(e)",
            bf_prefix + fname + type_xfix(MT_FIELD_BF, false) + "(" +
            bv_prefix + "##e)");
        /* print BFMV macro: masl */
        ctx.add(bfmv_prefix + fname + type_xfix(MT_FIELD_BFMV, false) + "(v)",
            bm_prefix + fname + type_xfix(MT_FIELD_BM, false));
    }

    ctx.print(os);
    return true;
}

bool common_generator::generate_macro_header(error_context_t& ectx)
{
    /* only generate if we need support macros */
    if(!has_support_macros())
        return true;
    std::ofstream fout((m_outdir + "/" + macro_header()).c_str());
    if(!fout)
    {
        printf("Cannot create '%s'\n", (m_outdir + "/" + macro_header()).c_str());
        return false;
    }
    print_copyright(fout, std::vector< soc_ref_t >());
    std::string guard = header_include_guard(macro_header());
    print_guard(fout, guard, true);
    fout << "\n";

    /* print variadic OR macros:
     * __VAR_OR1(prefix, suffix) expands to prefix##suffix
     * and more n>=2, using multiple layers of macros:
     * __VAR_ORn(pre, s01, s02, ..., sn) expands to pre##s01 | .. | pre##sn */
    std::string var_or = "__VAR_OR";
    const int MAX_N = 13;

    fout << "#define " << var_or << "1(prefix, suffix) \\\n";
    fout << "    (prefix##suffix)\n";
    for(int n = 2; n <= MAX_N; n++)
    {
        fout << "#define " << var_or << n << "(pre";
        for(int j = 1; j <= n; j++)
            fout << ", s" << j;
        fout << ") \\\n";
        /* dichotmoty: expands ORn using ORj and ORk where j=n/2 and k=n-j */
        int half = n / 2;
        fout << "    (" << var_or << half << "(pre";
        for(int j = 1; j <= half; j++)
            fout << ", s" << j;
        fout << ") | " << var_or << (n - half) << "(pre";
        for(int j = half + 1; j <= n; j++)
            fout << ", s" << j;
        fout << "))\n";
    }
    fout << "\n";

    /* print macro to compute number of arguments */
    std::string var_nargs = "__VAR_NARGS";

    fout << "#define " << var_nargs << "(...) " << var_nargs << "_(__VA_ARGS__";
    for(int i = MAX_N; i >= 1; i--)
            fout << ", " << i;
    fout << ")\n";
    fout << "#define " << var_nargs << "_(";
    for(int i = 1; i <= MAX_N; i++)
        fout << "_" << i << ", ";
    fout << "N, ...) N\n\n";

    /* print macro for variadic register macros */
    std::string var_expand = "__VAR_EXPAND";

    fout << "#define " << var_expand << "(macro, prefix, ...) "
        << var_expand << "_(macro, " << var_nargs << "(__VA_ARGS__), prefix, __VA_ARGS__)\n";
    fout << "#define " << var_expand << "_(macro, cnt, prefix, ...) "
        << var_expand << "__(macro, cnt, prefix, __VA_ARGS__)\n";
    fout << "#define " << var_expand << "__(macro, cnt, prefix, ...) "
        << var_expand << "___(macro##cnt, prefix, __VA_ARGS__)\n";
    fout << "#define " << var_expand << "___(macro, prefix, ...) "
        << "macro(prefix, __VA_ARGS__)\n\n";

    /* print type macros */
    define_align_context_t ctx;
    access_type_t at[3] = { AT_RO, AT_RW, AT_WO };
    int width[3] = { 8, 16, 32 };
    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            std::string io_type = register_type_name(at[i], width[j]);
            ctx.add(io_type + "(op, name, ...)", io_type + register_op_prefix() + "##op(name, __VA_ARGS__)");
            // read
            std::ostringstream oss;
            std::string reg_addr = safe_macro_paste(false, type_xfix(MT_REG_ADDR, true))
                + "name" + safe_macro_paste(true, type_xfix(MT_REG_ADDR, false));
            if(at[i] == AT_RO)
                oss << "(*(const volatile uint" << width[j] << "_t *)(" << reg_addr << "))";
            else if(at[i] == AT_RW)
                oss << "(*(volatile uint" << width[j] << "_t *)(" << reg_addr << "))";
            else
                oss << "({_Static_assert(0, #name \" is write-only\"); 0;})";
            ctx.add(io_type + register_op_prefix() + register_op_name(RO_READ) + "(name, ...)",
                oss.str());
            // write
            oss.str("");
            if(at[i] != AT_RO)
                oss << "(*(volatile uint" << width[j] << "_t *)(" << reg_addr << ")) = (val)";
            else
                oss << "_Static_assert(0, #name \" is read-only\")";
            ctx.add(io_type + register_op_prefix() + register_op_name(RO_WRITE) + "(name, val)",
                oss.str());
            // read-modify-write
            oss.str("");
            if(at[i] == AT_RW)
            {
                oss << io_type << register_op_prefix() << register_op_name(RO_WRITE) + "(name, ";
                oss << "(" << io_type << register_op_prefix() << register_op_name(RO_READ) + "(name) & (vand))";
                oss << " | (vor))";
            }
            else if(at[i] == AT_WO)
                oss << io_type << register_op_prefix() << register_op_name(RO_WRITE) + "(name, vor)";
            else
                oss << "_Static_assert(0, #name \" is read-only\")";
            ctx.add(io_type + register_op_prefix() + register_op_name(RO_RMW) + "(name, vand, vor)",
                oss.str());
            // variable
            oss.str("");
            if(at[i] == AT_RO)
                oss << "(*(const volatile uint" << width[j] << "_t *)(" << reg_addr << "))";
            else
                oss << "(*(volatile uint" << width[j] << "_t *)(" << reg_addr << "))";
            ctx.add(io_type + register_op_prefix() + register_op_name(RO_VAR) + "(name, ...)",
                oss.str());

            ctx.add_raw("\n");
        }
    }
    ctx.print(fout);
    fout << "\n";

    /* print GET_VARIANT macro */
    std::string get_var = macro_name(MN_GET_VARIANT);
    fout << "/** " <<  get_var << "\n";
    fout << " *\n";
    fout << " * usage: " << get_var << "(register, variant_prefix, variant_postfix)\n";
    fout << " *\n";
    fout << " * effect: expands to register variant given as argument\n";
    fout << " * note: internal usage\n";
    fout << " * note: register must be fully qualified if indexed\n";
    fout << " *\n";
    fout << " * example: " << get_var << "(ICOLL_CTRL, , _SET)\n";
    fout << " * example: " << get_var << "(ICOLL_ENABLE(3), , _CLR)\n";
    fout << " */\n";
    fout << "#define " << get_var << "(name, varp, vars) "
        << get_var << "_(" << safe_macro_paste(false, type_xfix(MT_REG_NAME, true))
        << "name" << safe_macro_paste(true, type_xfix(MT_REG_NAME, false)) << ", "
        << safe_macro_paste(false, type_xfix(MT_REG_INDEX, true))
        << "name" << safe_macro_paste(true, type_xfix(MT_REG_INDEX, false)) << ", varp, vars)\n";
    fout << "#define " << get_var << "_(...) " << get_var << "__(__VA_ARGS__)\n";
    fout << "#define " << get_var << "__(name, index, varp, vars) "
        << "varp##name##vars index\n";
    fout << "\n";

    /* print BF_OR macro */
    std::string bf_or = macro_name(MN_FIELD_OR);
    fout << "/** " <<  bf_or << "\n";
    fout << " *\n";
    fout << " * usage: " << bf_or << "(register, f1(v1), f2(v2), ...)\n";
    fout << " *\n";
    fout << " * effect: expands to the register value where each field fi has value vi.\n";
    fout << " *         Informally: reg_f1(v1) | reg_f2(v2) | ...\n";
    fout << " * note: enumerated values for fields can be obtained by using the syntax:\n";
    fout << " *          f1" << type_xfix(MT_FIELD_BFV, false) << "(name)\n";
    fout << " *\n";
    fout << " * example: " << bf_or << "(ICOLL_CTRL, SFTRST(1), CLKGATE(0), TZ_LOCK_V(UNLOCKED))\n";
    fout << " */\n";
    fout << "#define " << bf_or << "(reg, ...) "
        << var_expand << "(" << var_or << ", " << type_xfix(MT_FIELD_BF, true)
        << "##reg##" << field_prefix() << ", __VA_ARGS__)\n";
    fout << "\n";

    /* print BF_OR macro */
    std::string bfm_or = macro_name(MN_FIELD_OR_MASK);
    fout << "/** " <<  bfm_or << "\n";
    fout << " *\n";
    fout << " * usage: " << bfm_or << "(register, f1(v1), f2(v2), ...)\n";
    fout << " *\n";
    fout << " * effect: expands to the register value where each field fi has maximum value (vi is ignored).\n";
    fout << " * note: internal usage\n";
    fout << " *\n";
    fout << " * example: " << bfm_or << "(ICOLL_CTRL, SFTRST(1), CLKGATE(0), TZ_LOCK_V(UNLOCKED))\n";
    fout << " */\n";
    fout << "#define " << bfm_or << "(reg, ...) "
        << var_expand << "(" << var_or << ", " << type_xfix(MT_FIELD_BFM, true)
        << "##reg##" << field_prefix() << ", __VA_ARGS__)\n";
    fout << "\n";

    /* print BM_OR macro */
    std::string bm_or = macro_name(MN_MASK_OR);
    fout << "/** " << bm_or << "\n";
    fout << " *\n";
    fout << " * usage: " << bm_or << "(register, f1, f2, ...)\n";
    fout << " *\n";
    fout << " * effect: expands to the register value where each field fi is set to its maximum value.\n";
    fout << " *         Informally: reg_f1_mask | reg_f2_mask | ...\n";
    fout << " *\n";
    fout << " * example: " << bm_or << "(ICOLL_CTRL, SFTRST, CLKGATE)\n";
    fout << " */\n";
    fout << "#define " << bm_or << "(reg, ...) "
        << var_expand << "(" << var_or << ", " << type_xfix(MT_FIELD_BM, true)
        << "##reg##" << field_prefix() << ", __VA_ARGS__)\n\n";
    fout << "\n";

    /* print REG_READ macro */
    std::string reg_read = macro_name(MN_REG_READ);
    fout << "/** " << reg_read << "\n";
    fout << " *\n";
    fout << " * usage: " << reg_read << "(register)\n";
    fout << " *\n";
    fout << " * effect: read a register and return its value\n";
    fout << " * note: register must be fully qualified if indexed\n";
    fout << " *\n";
    fout << " * example: " << reg_read << "(ICOLL_STATUS)\n";
    fout << " *          " << reg_read << "(ICOLL_ENABLE(42))\n";
    fout << " */\n";
    fout << "#define " << reg_read << "(name) "
        << safe_macro_paste(false, type_xfix(MT_REG_TYPE, true)) << "name"
        << safe_macro_paste(true, type_xfix(MT_REG_TYPE, false))
        << "(" << register_op_name(RO_READ) << ", name)\n";
    fout << "\n";

    /* print FIELD_READX macro */
    std::string bf_readx = macro_name(MN_FIELD_READX);
    fout << "/** " << bf_readx << "\n";
    fout << " *\n";
    fout << " * usage: " << bf_readx << "(value, register, field)\n";
    fout << " *\n";
    fout << " * effect: given a register value, return the value of a particular field\n";
    fout << " * note: this macro does NOT read any register\n";
    fout << " *\n";
    fout << " * example: " << bf_readx << "(0xc0000000, ICOLL_CTRL, SFTRST)\n";
    fout << " *          " << bf_readx << "(0x46ff, ICOLL_ENABLE, CPU0_PRIO)\n";
    fout << " */\n";
    fout << "#define " << bf_readx << "(val, name, field) "
        << "(((val) & " << safe_macro_paste(false, type_xfix(MT_FIELD_BM, true))
        << "name" << safe_macro_paste(true, type_xfix(MT_FIELD_BM, false))
        << safe_macro_paste(true, field_prefix()) << "##field"
        << ") >> " << safe_macro_paste(false, type_xfix(MT_FIELD_BP, true))
        << "name" << safe_macro_paste(true, type_xfix(MT_FIELD_BP, false))
        << safe_macro_paste(true, field_prefix()) << "##field"
        << ")\n";
    fout << "\n";

    /* print FIELD_READ macro */
    std::string bf_read = macro_name(MN_FIELD_READ);
    fout << "/** " << bf_read << "\n";
    fout << " *\n";
    fout << " * usage: " << bf_read << "(register, field)\n";
    fout << " *\n";
    fout << " * effect: read a register and return the value of a particular field\n";
    fout << " * note: register must be fully qualified if indexed\n";
    fout << " *\n";
    fout << " * example: " << bf_read << "(ICOLL_CTRL, SFTRST)\n";
    fout << " *          " << bf_read << "(ICOLL_ENABLE(3), CPU0_PRIO)\n";
    fout << " */\n";
    fout << "#define " << bf_read << "(name, field) " << bf_read << "_("
        << reg_read << "(name), " << safe_macro_paste(false, type_xfix(MT_REG_NAME, true))
        << "name" << safe_macro_paste(true, type_xfix(MT_REG_NAME, false)) << ", field)\n";
    fout << "#define " << bf_read << "_(...) " << bf_readx << "(__VA_ARGS__)\n";
    fout << "\n";

    /* print REG_WRITE macro */
    std::string reg_write = macro_name(MN_REG_WRITE);
    fout << "/** " << reg_write << "\n";
    fout << " *\n";
    fout << " * usage: " << reg_write << "(register, value)\n";
    fout << " *\n";
    fout << " * effect: write a register\n";
    fout << " * note: register must be fully qualified if indexed\n";
    fout << " *\n";
    fout << " * example: " << reg_write << "(ICOLL_CTRL, 0x42)\n";
    fout << " *          " << reg_write << "(ICOLL_ENABLE_SET(3), 0x37)\n";
    fout << " */\n";
    fout << "#define " << reg_write << "(name, val) "
        << safe_macro_paste(false, type_xfix(MT_REG_TYPE, true)) << "name"
        << safe_macro_paste(true, type_xfix(MT_REG_TYPE, false))
        << "(" << register_op_name(RO_WRITE) << ", name, val)\n";
    fout << "\n";

    /* print FIELD_WRITE macro */
    std::string bf_write = macro_name(MN_FIELD_WRITE);
    fout << "/** " << bf_write << "\n";
    fout << " *\n";
    fout << " * usage: " << bf_write << "(register, f1(v1), f2(v2), ...)\n";
    fout << " *\n";
    fout << " * effect: change the register value so that field fi has value vi\n";
    fout << " * note: register must be fully qualified if indexed\n";
    fout << " * note: this macro may perform a read-modify-write\n";
    fout << " *\n";
    fout << " * example: " << bf_write << "(ICOLL_CTRL, SFTRST(1), CLKGATE(0), TZ_LOCK_V(UNLOCKED))\n";
    fout << " *          " << bf_write << "(ICOLL_ENABLE(3), CPU0_PRIO(1), CPU0_TYPE_V(FIQ))\n";
    fout << " */\n";
    fout << "#define " << bf_write << "(name, ...) "
        << bf_write << "_(name, " << safe_macro_paste(false, type_xfix(MT_REG_NAME, true))
        << "name" << safe_macro_paste(true, type_xfix(MT_REG_NAME, false)) << ", __VA_ARGS__)\n";
    fout << "#define " << bf_write << "_(name, name2, ...) "
        << safe_macro_paste(false, type_xfix(MT_REG_TYPE, true)) << "name"
        << safe_macro_paste(true, type_xfix(MT_REG_TYPE, false))
        << "(" << register_op_name(RO_RMW) << ", name, "
        << "~" << macro_name(MN_FIELD_OR_MASK) << "(name2, __VA_ARGS__), "
        << macro_name(MN_FIELD_OR) << "(name2, __VA_ARGS__))\n";
    fout << "\n";

    /* print FIELD_OVERWRITE macro */
    std::string bf_overwrite = macro_name(MN_FIELD_OVERWRITE);
    fout << "/** " << bf_overwrite << "\n";
    fout << " *\n";
    fout << " * usage: " << bf_overwrite << "(register, f1(v1), f2(v2), ...)\n";
    fout << " *\n";
    fout << " * effect: change the register value so that field fi has value vi and other fields have value zero\n";
    fout << " *         thus this macro is equivalent to:\n";
    fout << " *         " << reg_write << "(register, " << bf_or << "(register, f1(v1), ...))\n";
    fout << " * note: register must be fully qualified if indexed\n";
    fout << " * note: this macro will overwrite the register (it is NOT a read-modify-write)\n";
    fout << " *\n";
    fout << " * example: " << bf_overwrite << "(ICOLL_CTRL, SFTRST(1), CLKGATE(0), TZ_LOCK_V(UNLOCKED))\n";
    fout << " *          " << bf_overwrite << "(ICOLL_ENABLE(3), CPU0_PRIO(1), CPU0_TYPE_V(FIQ))\n";
    fout << " */\n";
    fout << "#define " << bf_overwrite << "(name, ...) "
        << bf_overwrite << "_(name, " << safe_macro_paste(false, type_xfix(MT_REG_NAME, true))
        << "name" << safe_macro_paste(true, type_xfix(MT_REG_NAME, false)) << ", __VA_ARGS__)\n";
    fout << "#define " << bf_overwrite << "_(name, name2, ...) "
        << safe_macro_paste(false, type_xfix(MT_REG_TYPE, true)) << "name"
        << safe_macro_paste(true, type_xfix(MT_REG_TYPE, false))
        << "(" << register_op_name(RO_WRITE) << ", name, "
        << macro_name(MN_FIELD_OR) << "(name2, __VA_ARGS__))\n";
    fout << "\n";

    /* print FIELD_WRITEX macro */
    std::string bf_writex = macro_name(MN_FIELD_WRITEX);
    fout << "/** " << bf_writex << "\n";
    fout << " *\n";
    fout << " * usage: " << bf_writex << "(var, register, f1(v1), f2(v2), ...)\n";
    fout << " *\n";
    fout << " * effect: change the variable value so that field fi has value vi\n";
    fout << " * note: this macro will perform a read-modify-write\n";
    fout << " *\n";
    fout << " * example: " << bf_writex << "(var, ICOLL_CTRL, SFTRST(1), CLKGATE(0), TZ_LOCK_V(UNLOCKED))\n";
    fout << " *          " << bf_writex << "(var, ICOLL_ENABLE, CPU0_PRIO(1), CPU0_TYPE_V(FIQ))\n";
    fout << " */\n";
    fout << "#define " << bf_writex << "(var, name, ...) "
        << "(var) = " << macro_name(MN_FIELD_OR) << "(name, __VA_ARGS__) | (~"
        << macro_name(MN_FIELD_OR_MASK) << "(name, __VA_ARGS__) & (var))\n";
    fout << "\n";

    /* print FIELD_SET/FIELD_CLEAR macro */
    for(int i = 0; i < 2; i++)
    {
        macro_name_t n = (i == 0) ? MN_FIELD_SET : MN_FIELD_CLEAR;
        std::string bf_set = macro_name(n);
        std::string set_var = sct_variant(n);

        fout << "/** " << bf_set << "\n";
        fout << " *\n";
        fout << " * usage: " << bf_set << "(register, f1, f2, ...)\n";
        fout << " *\n";
        fout << " * effect: change the register value so that field fi has ";
        if(i == 0)
            fout << "maximum value\n";
        else
            fout << "value zero\n";
        if(has_sct())
            fout << " * IMPORTANT: this macro performs a write to the " << set_var << " variant of the register\n";
        else
            fout << " * note: this macro will perform a read-modify-write\n";

        fout << " * note: register must be fully qualified if indexed\n";
        fout << " *\n";
        fout << " * example: " << bf_set << "(ICOLL_CTRL, SFTRST, CLKGATE)\n";
        fout << " *          " << bf_set << "(ICOLL_ENABLE(3), CPU0_PRIO, CPU0_TYPE)\n";
        fout << " */\n";

        if(has_sct())
        {
            fout << "#define " << bf_set << "(name, ...) " << bf_set << "_("
                << macro_name(MN_GET_VARIANT) << "(name, " << variant_xfix(set_var, true)
                << ", " << variant_xfix(set_var, false) << "), "
                << safe_macro_paste(false, type_xfix(MT_REG_NAME, true))
                << "name" << safe_macro_paste(true, type_xfix(MT_REG_NAME, false))
                << ", __VA_ARGS__)\n";
            fout << "#define " << bf_set << "_(name, name2, ...) " << macro_name(MN_REG_WRITE)
                << "(name, " << macro_name(MN_MASK_OR) << "(name2, __VA_ARGS__))\n";
        }
        else
        {
            fout << "#define " << bf_set << "(name, ...) "
                << bf_set << "_(name, " << safe_macro_paste(false, type_xfix(MT_REG_NAME, true))
                << "name" << safe_macro_paste(true, type_xfix(MT_REG_NAME, false)) << ", __VA_ARGS__)\n";
            fout << "#define " << bf_set << "_(name, name2, ...) "
                << safe_macro_paste(false, type_xfix(MT_REG_TYPE, true)) << "name"
                << safe_macro_paste(true, type_xfix(MT_REG_TYPE, false))
                << "(" << register_op_name(RO_RMW) << ", name, ~";
            if(i == 0)
                fout << "0," << macro_name(MN_MASK_OR) << "(name2, __VA_ARGS__))\n";
            else
                fout << macro_name(MN_MASK_OR) << "(name2, __VA_ARGS__), 0)\n";
        }
        fout << "\n";
    }

    if(has_sct())
    {
        std::string set_var = sct_variant(MN_FIELD_SET);
        std::string clr_var = sct_variant(MN_FIELD_CLEAR);

        /* print REG_SET */
        std::string reg_set = macro_name(MN_REG_SET);
        if(!reg_set.empty())
        {
            fout << "/** " << reg_set << "\n";
            fout << " *\n";
            fout << " * usage: " << reg_set << "(register, set_value)\n";
            fout << " *\n";
            fout << " * effect: set some bits using " << set_var << " variant\n";
            fout << " * note: register must be fully qualified if indexed\n";
            fout << " *\n";
            fout << " * example: " << reg_set << "(ICOLL_CTRL, 0x42)\n";
            fout << " *          " << reg_set << "(ICOLL_ENABLE(3), 0x37)\n";
            fout << " */\n";
            fout << "#define " << reg_set << "(name, sval) "
                << reg_set << "_(" << macro_name(MN_GET_VARIANT) << "(name, "
                << variant_xfix(set_var, true) << ", " << variant_xfix(set_var, false)
                << "), sval)\n";
            fout << "#define " << reg_set << "_(sname, sval) "
                << macro_name(MN_REG_WRITE) << "(sname, sval)\n";
            fout << "\n";
        }

        /* print REG_CLR */
        std::string reg_clr = macro_name(MN_REG_CLEAR);
        if(!reg_clr.empty())
        {
            fout << "/** " << reg_clr << "\n";
            fout << " *\n";
            fout << " * usage: " << reg_clr << "(register, clr_value)\n";
            fout << " *\n";
            fout << " * effect: clear some bits using " << clr_var << " variant\n";
            fout << " * note: register must be fully qualified if indexed\n";
            fout << " *\n";
            fout << " * example: " << reg_clr << "(ICOLL_CTRL, 0x42)\n";
            fout << " *          " << reg_clr << "(ICOLL_ENABLE(3), 0x37)\n";
            fout << " */\n";
            fout << "#define " << reg_clr << "(name, cval) "
                << reg_clr << "_(" << macro_name(MN_GET_VARIANT) << "(name, "
                << variant_xfix(clr_var, true) << ", " << variant_xfix(clr_var, false)
                << "), cval)\n";
            fout << "#define " << reg_clr << "_(cname, cval) "
                << macro_name(MN_REG_WRITE) << "(cname, cval)\n";
            fout << "\n";
        }

        /* print REG_CS */
        std::string reg_cs = macro_name(MN_REG_CLEAR_SET);
        fout << "/** " << reg_cs << "\n";
        fout << " *\n";
        fout << " * usage: " << reg_cs << "(register, clear_value, set_value)\n";
        fout << " *\n";
        fout << " * effect: clear some bits using " << clr_var << " variant and then set some using " << set_var << " variant\n";
        fout << " * note: register must be fully qualified if indexed\n";
        fout << " *\n";
        fout << " * example: " << reg_cs << "(ICOLL_CTRL, 0xff, 0x42)\n";
        fout << " *          " << reg_cs << "(ICOLL_ENABLE(3), 0xff, 0x37)\n";
        fout << " */\n";
        fout << "#define " << reg_cs << "(name, cval, sval) "
            << reg_cs << "_(" << macro_name(MN_GET_VARIANT) << "(name, "
            << variant_xfix(clr_var, true) << ", " << variant_xfix(clr_var, false) << "), "
            << macro_name(MN_GET_VARIANT) << "(name, " << variant_xfix(set_var, true)
            << ", " << variant_xfix(set_var, false) << "), cval, sval)\n";
        fout << "#define " << reg_cs << "_(cname, sname, cval, sval) "
            << "do { " << macro_name(MN_REG_WRITE) << "(cname, cval); "
            << macro_name(MN_REG_WRITE) << "(sname, sval); } while(0)\n";
        fout << "\n";

        /* print BF_CS */
        std::string bf_cs = macro_name(MN_FIELD_CLEAR_SET);
        fout << "/** " << bf_cs << "\n";
        fout << " *\n";
        fout << " * usage: " << bf_cs << "(register, f1(v1), f2(v2), ...)\n";
        fout << " *\n";
        fout << " * effect: change the register value so that field fi has value vi using " << clr_var << " and " << set_var << " variants\n";
        fout << " * note: register must be fully qualified if indexed\n";
        fout << " * note: this macro will NOT perform a read-modify-write and is thus safer\n";
        fout << " * IMPORTANT: this macro will set some fields to 0 temporarily, make sure this is acceptable\n";
        fout << " *\n";
        fout << " * example: " << bf_cs << "(ICOLL_CTRL, SFTRST(1), CLKGATE(0), TZ_LOCK_V(UNLOCKED))\n";
        fout << " *          " << bf_cs << "(ICOLL_ENABLE(3), CPU0_PRIO(1), CPU0_TYPE_V(FIQ))\n";
        fout << " */\n";
        fout << "#define " << bf_cs << "(name, ...) "
            << bf_cs << "_(name, " << safe_macro_paste(false, type_xfix(MT_REG_NAME, true))
            << "name" << safe_macro_paste(true, type_xfix(MT_REG_NAME, false)) << ", __VA_ARGS__)\n";
        fout << "#define " << bf_cs << "_(name, name2, ...) "
            << macro_name(MN_REG_CLEAR_SET) << "(name, " << macro_name(MN_FIELD_OR_MASK)
            << "(name2, __VA_ARGS__), " << macro_name(MN_FIELD_OR) << "(name2, __VA_ARGS__))\n";
        fout << "\n";
    }

    /* print REG_VAR macro */
    std::string reg_var = macro_name(MN_VARIABLE);
    fout << "/** " << reg_var << "\n";
    fout << " *\n";
    fout << " * usage: " << reg_var << "(register)\n";
    fout << " *\n";
    fout << " * effect: return a variable-like expression that can be read/written\n";
    fout << " * note: register must be fully qualified if indexed\n";
    fout << " * note: read-only registers will yield a constant expression\n";
    fout << " *\n";
    fout << " * example: unsigned x = " << reg_var << "(ICOLL_STATUS)\n";
    fout << " *          unsigned x = " << reg_var << "(ICOLL_ENABLE(42))\n";
    fout << " *          " << reg_var << "(ICOLL_ENABLE(42)) = 64\n";
    fout << " */\n";
    fout << "#define " << reg_var << "(name) "
        << safe_macro_paste(false, type_xfix(MT_REG_TYPE, true)) << "name"
        << safe_macro_paste(true, type_xfix(MT_REG_TYPE, false))
        << "(" << register_op_name(RO_VAR) << ", name)\n";
    fout << "\n";

    print_guard(fout, guard, false);
    fout.close();
    return true;
}

bool common_generator::generate(error_context_t& ectx)
{
    /* first find which inst goes to which file */
    std::map< std::string, std::vector< pseudo_node_inst_t > > regmap;
    for(size_t i = 0; i < m_soc.size(); i++)
    {
        soc_ref_t soc(&m_soc[i]);
        pseudo_node_inst_t inst;
        inst.inst = soc.root_inst();
        gather_files(inst, has_selectors() ? selector_soc_dir(soc) + "/" :
            "", regmap);
    }
    /* create output directory */
    create_dir(m_outdir);
    /* print files */
    std::map< std::string, std::vector< pseudo_node_inst_t > >::iterator it = regmap.begin();
    for(; it != regmap.end(); ++it)
    {
        create_alldir(m_outdir, it->first.c_str());
        std::ofstream fout((m_outdir + "/" + it->first).c_str());
        if(!fout)
        {
            printf("Cannot create '%s'\n", (m_outdir + "/" + it->first).c_str());
            return false;
        }
        std::vector< soc_ref_t > soc_list = list_socs(it->second);
        print_copyright(fout, soc_list);
        std::string guard = header_include_guard(it->first);
        print_guard(fout, guard, true);

        /* if we generate selectors, we include the macro header in them, otherwise
         * we include the macro header right here. If we don't need support macros,
         * we also don't include it. */
        if(!has_selectors() && has_support_macros())
        {
            fout << "\n";
            fout << "#include \"" << macro_header() << "\"\n";
        }

        for(size_t i = 0; i < it->second.size(); i++)
        {
            if(!generate_register(fout, it->second[i]))
            {
                printf("Cannot generate register");
                print_inst(it->second[i]);
                return false;
            }
        }

        print_guard(fout, guard, false);
        fout.close();
    }
    /* for selectors only */
    if(has_selectors())
    {
        /* list all possible headers and per soc headers */
        std::map< std::string, std::set< soc_ref_t > > headers;
        for(it = regmap.begin(); it != regmap.end(); ++it)
        {
            /* pick the first instance in the file to extract the file name */
            node_inst_t inst = it->second[0].inst;
            headers[register_header(inst)].insert(inst.node().soc());
        }
        /* create selector headers */
        std::map< std::string, std::set< soc_ref_t > >::iterator jt;
        for(jt = headers.begin(); jt != headers.end(); ++jt)
        {
            std::ofstream fout((m_outdir + "/" + jt->first).c_str());
            if(!fout)
            {
                printf("Cannot create selector '%s'\n", (m_outdir + "/" + jt->first).c_str());
                return false;
            }
            print_copyright(fout, std::vector< soc_ref_t >());
            std::string guard = header_include_guard(jt->first);
            print_guard(fout, guard, true);

            /* if we generate selectors, we include the macro header in them */
            fout << "\n";
            fout << "#include \"" << macro_header() << "\"\n";

            std::set< soc_ref_t >::iterator kt;
            define_align_context_t ctx;
            for(kt = jt->second.begin(); kt != jt->second.end(); ++kt)
            {
                std::ostringstream oss;
                oss << "\"" << selector_soc_dir(*kt) << "/" << jt->first << "\"";
                ctx.add(selector_soc_macro(*kt), oss.str());
            }
            fout << "\n";
            ctx.print(fout);
            fout << "\n";
            fout << "#include \"" << selector_include_header() << "\"\n";
            fout << "\n";
            for(kt = jt->second.begin(); kt != jt->second.end(); ++kt)
                fout << "#undef " << selector_soc_macro(*kt) << "\n";

            print_guard(fout, guard, false);
            fout.close();
        }
    }
    /* generate macro header */
    if(!generate_macro_header(ectx))
        return false;
    return true;
}

/**
 * Simple generator:
 * - does not support fancy macros
 * - only supports only soc at a time
 * - expect a classical soc/block/register hierarchy
 * - block can be indexed: MMC[0], MMC[1], etc
 * - registers can be indexed: GPIO.IN[0], GPIO.IN[1], GPIO[3].PIN[4]
 * - the "print list" is interpreted as a list of blocks
 */
class simple_generator : public abstract_generator
{
public:
    virtual bool generate(error_context_t& ctx);
protected:
    /// if true, put all definitions in a single header, otherwise create one header per block
    virtual bool single_header(soc_ref_t soc) const = 0;
    /// return name of the register header: in single_header mode this is the header name, in
    /// non-single_header mode, returning a non-empty string will create a header that includes
    /// all other headers
    virtual std::string header_name(soc_ref_t soc) const = 0;
    /// return name of the block header: ignored in single-header mode
    virtual std::string header_name(const std::string& block_name) const = 0;
    /// return the name of the macro holding the block address
    /// #define GPIO_ADDR 0x42000000
    /// if empty, the generator will not create this macro and inline all addresses
    virtual std::string macro_block_addr(const std::string& block_name) const = 0;
    /// return the name of the macro holding the register address
    /// #define GPIO_IN_ADDR    0x42000800                   (if block address is empty)
    /// #define GPIO_IN_ADDR    (BLOCK_ADDR + 0x800)      (otherwise)
    /// if empty, the generator will not create this macro and  inline all addresses
    virtual std::string macro_reg_addr(const std::string& block_name, const std::string& reg_name,
        register_ref_t reg) const = 0;
    /// return the name of the macro holding the register "variable-like"
    /// #define GPIO_IN         (*(volatile uint32_t *)0x42000800)   (if BLOCK_ADDR is empty)
    /// #define GPIO_IN         (*(volatile uint32_t *)REG_ADDR)  (otherwise)
    /// if empty, the generator will not create this macro
    virtual std::string macro_reg_var(const std::string& block_name, const std::string& reg_name,
        register_ref_t reg) const = 0;
    /// return the name of the macro holding the register address for the variant
    /// #define GPIO_IN_ADDR_SET    0x42000804
    /// if empty, the generator will not create this macro and  inline all addresses
    virtual std::string macro_reg_addr_variant(const std::string& block_name, const std::string& reg_name,
        const std::string& var_name, variant_ref_t var) const = 0;
    /// return the name of the macro holding the register "variable-like" for the variant
    /// #define GPIO_IN_SET     REG32(0x42000804)   (if BLOCK_ADDR is empty)
    /// #define GPIO_IN_SET     REG32(REG_ADDR_SET)  (otherwise)
    /// if empty, the generator will not create this macro
    virtual std::string macro_reg_var_variant(const std::string& block_name, const std::string& reg_name,
        const std::string& var_name, variant_ref_t var) const = 0;
    /// return the name of the macro holding the bit position of a field
    /// #define GPIO_CTRL_MUX_BP    4
    /// if empty, the generator will no create this macro and inline it everywhere
    virtual std::string macro_field_pos(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, field_ref_t field) const = 0;
    /// return the name of the macro holding the bit mask of a field
    /// #define GPIO_CTRL_MUX_BM    (0x3 << 4)
    /// if empty, the generator will no create this macro and inline it everywhere
    virtual std::string macro_field_mask(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, field_ref_t field) const = 0;
    /// return the name of the macro to form a field value
    /// #define GPIO_CTRL_MUX(x)    (((x) << FIELD_BP) & FIELD_BM)
    /// if empty, the generator will no create this macro
    virtual std::string macro_field_form(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, field_ref_t field) const = 0;
    /// return the name of the macro to extract a field from the register value
    /// #define GPIO_CTRL_MUX_RD(x)    (((x) & FIELD_BM) >> FIELD_BP)
    /// if empty, the generator will no create this macro
    virtual std::string macro_field_read(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, field_ref_t field) const = 0;
    /// return the name of the  macro holding an enumerated value
    /// #define GPIO_CTRL_MUX__MMC  1
    /// #define GPIO_CTRL_MUX__LCD  2
    /// if empty, the generator will no create this macro
    virtual std::string macro_enum_val(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, const std::string& enum_name, enum_ref_t field) const = 0;
    /// return the name of the macro that generates forms a field value from an enumerated value
    /// #define GPIO_CTRL_MUX_V(bla)    ((GPIO_CTRL_MUX__##bla) << 4)
    /// if empty, the generator will no create this macro
    virtual std::string macro_field_form_enum(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, field_ref_t field) const = 0;
    /// in macro_field_form_enum, return the concatenation string use to form the enumerated value
    /// GPIO_CTRL_MUX__##bla in the example above
    virtual std::string macro_field_form_enum_concat(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, field_ref_t field, const std::string& enum_arg_name) const = 0;
    /// given a string and a width, generate the code to create a variable-like register, default is:
    ///   (*(volatile uintN_t *)(some addr))
    virtual std::string expand_var_addr(const std::string& addr, size_t width);
private:
    bool generate_block(error_context_t& ctx, define_align_context_t& top_ctx,
        node_ref_t& block);
};

std::string simple_generator::expand_var_addr(const std::string& addr, size_t width)
{
    return "(*(volatile uint" + to_str(width) + "_t *)(" + addr + "))";
}

bool simple_generator::generate_block(error_context_t& ctx, define_align_context_t& top_ctx,
    node_ref_t& block)
{
    bool single = single_header(block.soc());
    /// only support node with a single instance
    if(block.get()->instance.size() != 1)
    {
        ctx.add(err_t(err_t::WARNING, "", "Skipping multi-instance block " + block.get()->name));
        return true;
    }
    instance_t& block_inst = block.get()->instance[0];
    std::string block_name = block_inst.name;
    bool print = (m_print_list.find(block_name) != m_print_list.end());
    if(!m_print_list_only)
        print = !print;
    if(!print)
        return true;
    /// generate block address string
    std::vector< std::string > fresh_vars;
    std::vector< std::string > args;
    fresh_vars.push_back("m");
    fresh_vars.push_back("n");
    std::string block_addr = inst_relative_address(block_inst, NO_INDEX, args, fresh_vars);
    define_align_context_t block_ctx;
    block_ctx.add_raw("\n"); // empty line for spacing
    /// generate block address macro if necessary
    std::string block_addr_macro = macro_block_addr(block_name);
    if(!block_addr_macro.empty())
    {
        block_addr_macro += call_arg_string(args);
        block_ctx.add(block_addr_macro, block_addr);
        // future use of the address will use the address macro
        block_addr = block_addr_macro;
    }
    /// now generate registers
    std::vector< node_ref_t > regs = block.children();
    for(auto& reg : regs)
    {
        /// only support node with a single instance
        if(reg.get()->instance.size() != 1)
        {
            ctx.add(err_t(err_t::WARNING, "", "Skipping multi-instance node " + block_name + "." + reg.get()->name));
            continue;
        }
        instance_t& reg_inst = reg.get()->instance[0];
        std::string reg_name = reg_inst.name;
        // make sure this is actually a register
        register_ref_t reg_ref = reg.reg();
        if(!reg_ref.valid())
        {
            ctx.add(err_t(err_t::WARNING, "", "Skipping non-register " + block_name + "." + reg_name));
            continue;
        }
        /// make a copy of vars/fresh vars
        std::vector< std::string > fresh_vars_reg(fresh_vars);
        std::vector< std::string > args_reg(args);
        /// generate register address string
        std::string reg_addr_noparen = block_addr + " + " + inst_relative_address(reg_inst,
            NO_INDEX, args_reg, fresh_vars_reg);
        /// generate register address macro if necessary
        std::string reg_addr_macro = macro_reg_addr(block_name, reg_name, reg_ref);
        if(!reg_addr_macro.empty())
        {
            reg_addr_macro += call_arg_string(args_reg);
            block_ctx.add(reg_addr_macro, "(" + reg_addr_noparen + ")");
            // future use of the address will use the address macro
            reg_addr_noparen = reg_addr_macro;
        }
        /// generate register variable-like macro if necessary
        std::string reg_var_macro = macro_reg_var(block_name, reg_name, reg_ref);
        if(!reg_var_macro.empty())
        {
            std::string reg_var = expand_var_addr(reg_addr_noparen, reg_ref.get()->width);
            reg_var_macro += call_arg_string(args_reg);
            block_ctx.add(reg_var_macro, reg_var);
        }
        /// handle variants
        std::vector< variant_ref_t > variants = reg_ref.variants();
        for(auto& variant : variants)
        {
            variant_t& v = *variant.get();
            std::string var_name = v.type;
            /// generate register address macro if necessary
            std::string reg_addr_var_macro = macro_reg_addr_variant(block_name, reg_name, var_name, variant);
            std::string reg_addr_var_noparen = reg_addr_noparen + " + " + to_hex(v.offset);
            if(!reg_addr_var_macro.empty())
            {
                reg_addr_var_macro += call_arg_string(args_reg);
                block_ctx.add(reg_addr_var_macro, "(" + reg_addr_var_noparen + ")");
                reg_addr_var_noparen = reg_addr_var_macro;
            }
            /// generate register variable-like macro if necessary
            std::string reg_var_macro = macro_reg_var_variant(block_name, reg_name, var_name, variant);
            if(!reg_var_macro.empty())
            {
                std::string reg_var_var = expand_var_addr(reg_addr_var_noparen, reg_ref.get()->width);
                reg_var_macro += call_arg_string(args_reg);
                block_ctx.add(reg_var_macro, reg_var_var);
            }
        }
        /// generate fields
        std::vector< field_ref_t > fields = reg_ref.fields();
        for(auto& field : fields)
        {
            field_t& f = *field.get();
            std::string field_name = f.name;
            /// generate bit pos if necessary
            std::string field_bp_macro = macro_field_pos(block_name, reg_name, field_name, field);
            if(!field_bp_macro.empty())
                block_ctx.add(field_bp_macro, to_str(f.pos));
            /// generate bit mask if necessary
            std::string field_bm_macro = macro_field_mask(block_name, reg_name, field_name, field);
            if(!field_bm_macro.empty())
            {
                block_ctx.add(field_bm_macro, 
                    "(" + to_hex_nice(f.unshifted_bitmask()) + " << " + to_str(f.pos) + ")");
            }
            /// generate value forming macro if necessary
            std::string field_bf_macro = macro_field_form(block_name, reg_name, field_name, field);
            if(!field_bf_macro.empty())
            {
                block_ctx.add(field_bf_macro + "(v)",
                    "(((v) & " + to_hex_nice(f.unshifted_bitmask()) + ") << " + to_str(f.pos) + ")");
            }
            /// generate value forming macro for enum if necessary
            std::string field_bfv_macro = macro_field_form_enum(block_name, reg_name, field_name, field);
            if(!field_bfv_macro.empty())
            {
                std::string concat = macro_field_form_enum_concat(block_name, reg_name,
                    field_name, field, "v");
                block_ctx.add(field_bfv_macro + "(v)",
                    "((" + concat + " & " + to_hex_nice(f.unshifted_bitmask()) + ") << " + to_str(f.pos) + ")");
            }
            /// generate value extract macro if necessary
            std::string field_br_macro = macro_field_read(block_name, reg_name, field_name, field);
            if(!field_br_macro.empty())
            {
                block_ctx.add(field_br_macro + "(v)",
                    "(((v) >> " + to_str(f.pos) + ") & " + to_hex_nice(f.unshifted_bitmask()) + ")");
            }
            /// generate enums
            std::vector< enum_ref_t > enums = field.enums();
            for(auto& enum_ : enums)
            {
                enum_t& e = *enum_.get();
                /// generate enum value if necessary
                std::string enum_val_macro = macro_enum_val(block_name, reg_name, field_name, e.name, enum_);
                if(!enum_val_macro.empty())
                {
                    block_ctx.add(enum_val_macro, to_hex_nice(e.value));
                }
            }
        }
    }
    /// create block header if necessary
    if(!single)
    {
        std::string header_block = header_name(block_name);
        std::string guard_block = "__HEADERGEN_" + toupper(block_name) + "_H__";
        std::ofstream fout_block((m_outdir + "/" + header_block).c_str());
        if(!fout_block)
        {
            ctx.add(err_t(err_t::FATAL, "", "Cannot create " + m_outdir + "/" + header_block));
            return false;
        }
        // copyright and guard
        std::vector< soc_ref_t > soc_list;
        soc_list.push_back(block.soc());
        print_copyright(fout_block, soc_list);
        print_guard(fout_block, guard_block, true);
        // write content
        block_ctx.print(fout_block);
        // finish file
        print_guard(fout_block, guard_block, false);
        fout_block.close();
        // print include line in top header if any
        top_ctx.add_raw("#include \"" + header_block + "\"\n");
    }
    else
        top_ctx.add(block_ctx);
    return true;
}

bool simple_generator::generate(error_context_t& ctx)
{
    if(m_soc.size() != 1)
    {
        ctx.add(err_t(err_t::FATAL, "", "This generator only supports one soc"));
        return false;
    }
    soc_t& soc = m_soc[0];
    soc_ref_t soc_ref(&soc);
    /// iterate through all blocks
    bool single = single_header(soc_ref);
    std::vector< node_ref_t > blocks = soc_ref.root().children();
    define_align_context_t top_ctx;
    for(auto& block : blocks)
    {
        if(!generate_block(ctx, top_ctx, block))
            return false;
    }
    /// create top header
    std::string header_top = header_name(soc_ref);
    if(!header_top.empty())
    {
        std::string guard_top = "__HEADERGEN_" + toupper(soc.name) + "_H__";
        std::ofstream fout_top((m_outdir + "/" + header_top).c_str());
        if(!fout_top)
        {
            ctx.add(err_t(err_t::FATAL, "", "Cannot create " + m_outdir + "/" + header_top));
            return false;
        }
        std::vector< soc_ref_t > soc_list;
        soc_list.push_back(soc_ref);
        print_copyright(fout_top, soc_list);
        print_guard(fout_top, guard_top, true);
        // in non-single mode header where top list file, put an empty list to make it more readable
        if(!single && fout_top)
            fout_top << "\n";
        /// print all blocks (single-header mode)
        top_ctx.print(fout_top);
        // finish header
        print_guard(fout_top, guard_top, false);
        fout_top.close();
    }
    return true;
}

/**
 * Generator: jz
 */

class jz_generator : public simple_generator
{
    virtual bool single_header(soc_ref_t soc) const
    {
        return true;
    }

    virtual std::string header_name(soc_ref_t soc) const
    {
        return soc.get()->name + "_regs.h";
    }

    virtual std::string header_name(const std::string& block_name) const
    {
        return tolower(block_name) + ".h";
    }

    virtual std::string macro_block_addr(const std::string& block_name) const
    {
        return block_name + "_ADDR";
    }

    virtual std::string macro_reg_addr(const std::string& block_name, const std::string& reg_name,
        register_ref_t reg) const
    {
        return ""; // no register address macro
    }

    virtual std::string macro_reg_var(const std::string& block_name, const std::string& reg_name,
        register_ref_t reg) const
    {
        return block_name + "_" + reg_name;
    }

    virtual std::string macro_reg_addr_variant(const std::string& block_name, const std::string& reg_name,
        const std::string& var_name, variant_ref_t var) const
    {
        return "";
    }

    virtual std::string macro_reg_var_variant(const std::string& block_name, const std::string& reg_name,
        const std::string& var_name, variant_ref_t var) const
    {
        return block_name + "_" + reg_name + "_" + toupper(var_name);
    }

    virtual std::string macro_field_pos(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, field_ref_t field) const
    {
        return ""; // no field bit pos
    }

    virtual std::string macro_field_mask(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, field_ref_t field) const
    {
        // for 1-bit field, just use the field name, otherwise add "_bm"
        std::string field_mask = block_name + "_" + reg_name + "_" + field_name;
        if(field.get()->width != 1)
            field_mask += "_bm";
        return field_mask;
    }

    virtual std::string macro_field_form(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, field_ref_t field) const
    {
        // generate a mask only if this is not a 1-bit field
        if(field.get()->width == 1)
            return "";
        return block_name + "_" + reg_name + "_" + field_name;
    }

    virtual std::string macro_field_read(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, field_ref_t field) const
    {
        // generate a mask only if this is not a 1-bit field
        if(field.get()->width == 1)
            return "";
        return block_name + "_" + reg_name + "_" + field_name + "_get";
    }

    virtual std::string macro_enum_val(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, const std::string& enum_name, enum_ref_t field) const
    {
        return block_name + "_" + reg_name + "_" + field_name + "_" + enum_name;
    }

    virtual std::string macro_field_form_enum(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, field_ref_t field) const
    {
        // don't bother generate this macro if there are no enum!
        if(field.get()->enum_.size() == 0)
            return "";
        return block_name + "_" + reg_name + "_" + field_name + "_v";
    }
    virtual std::string macro_field_form_enum_concat(const std::string& block_name, const std::string& reg_name,
        const std::string& field_name, field_ref_t field, const std::string& enum_arg_name) const
    {
        return block_name + "_" + reg_name + "_" + field_name + "_##" + enum_arg_name;
    }

    virtual std::string expand_var_addr(const std::string& addr, size_t width)
    {
        return "REG" + to_str(width) + "(" + addr + ")";
    }
};

/**
 * Generator: imx
 */

class imx_generator : public common_generator
{
    bool has_support_macros() const
    {
        return true;
    }

    bool has_selectors() const
    {
        return m_soc.size() >= 2;
    }

    std::string selector_soc_dir(const soc_ref_t& ref) const
    {
        return ref.get()->name;
    }

    std::string selector_include_header() const
    {
        return "select.h";
    }

    std::string selector_soc_macro(const soc_ref_t& ref) const
    {
        return toupper(ref.get()->name) + "_INCLUDE";
    }

    std::string register_header(const node_inst_t& inst) const
    {
        /* one register header per top-level block */
        if(inst.is_root())
            return "<error>";
        if(inst.parent().is_root())
            return tolower(inst.node().name()) + ".h";
        else
            return register_header(inst.parent());
    }

    std::string macro_name(macro_name_t macro) const
    {
        switch(macro)
        {
            case MN_REG_READ: return "REG_RD";
            case MN_FIELD_READ: return "BF_RD";
            case MN_FIELD_READX: return "BF_RDX";
            case MN_REG_WRITE: return "REG_WR";
            case MN_REG_SET: return ""; // no macro for this
            case MN_REG_CLEAR: return ""; // no macro for this
            case MN_FIELD_WRITE: return "BF_WR";
            case MN_FIELD_OVERWRITE: return "BF_WR_ALL";
            case MN_FIELD_WRITEX: return "BF_WRX";
            case MN_FIELD_SET: return "BF_SET";
            case MN_FIELD_CLEAR: return "BF_CLR";
            case MN_FIELD_TOG: return "BF_TOG";
            case MN_FIELD_CLEAR_SET: return "BF_CS";
            case MN_FIELD_OR: return "BF_OR";
            case MN_FIELD_OR_MASK: return "__BFM_OR";
            case MN_MASK_OR: return "BM_OR";
            case MN_REG_CLEAR_SET: return "REG_CS";
            case MN_GET_VARIANT: return "__REG_VARIANT";
            case MN_VARIABLE: return "HW";
            default: return "<macro_name>";
        }
    }

    std::string macro_header() const
    {
        return "macro.h";
    }

    bool register_flag(const node_inst_t& inst, register_flag_t flag) const
    {
        /* make everything parametrized */
        switch(flag)
        {
            case RF_GENERATE_ALL_INST: return false;
            case RF_GENERATE_PARAM_INST: return true;
            default: return false;
        }
    }

    std::string type_xfix(macro_type_t type, bool prefix) const
    {
        switch(type)
        {
            case MT_REG_ADDR: return prefix ? "HWA_" : "";
            case MT_REG_TYPE: return prefix ? "HWT_" : "";
            case MT_REG_NAME: return prefix ? "HWN_" : "";
            case MT_REG_INDEX: return prefix ? "HWI_" : "";
            case MT_REG_VAR: return prefix ? "HW_" : "";
            case MT_FIELD_BP: return prefix ? "BP_" : "";
            case MT_FIELD_BM: return prefix ? "BM_" : "";
            case MT_FIELD_BV: return prefix ? "BV_" : "";
            case MT_FIELD_BF: return prefix ? "BF_" : "";
            case MT_FIELD_BFM: return prefix ? "BFM_" : "";
            case MT_FIELD_BFV: return prefix ? "BF_" : "_V";
            case MT_FIELD_BFMV: return prefix ? "BFM_" : "_V";
            case MT_IO_TYPE: return prefix ? "HWIO_" : "";
            default: return "<xfix>";
        }
    }

    std::string variant_xfix(const std::string& variant, bool prefix) const
    {
        /* variant X -> reg_X */
        if(prefix)
            return "";
        else
            return "_" + toupper(variant);
    }

    std::string inst_prefix(const node_inst_t& inst) const
    {
        /* separate blocks with _: block_reg */
        return "_";
    }

    std::string field_prefix() const
    {
        /* separate fields with _: block_reg_field */
        return "_";
    }

    std::string enum_prefix() const
    {
        /* separate enums with __: block_reg_field__enum */
        return "__";
    }

    std::string enum_name(const enum_ref_t& enum_) const
    {
        return enum_.get()->name;
    }

    access_type_t register_access(const std::string& variant, access_t access) const
    {
        /* SET, CLR and TOG are special and always promoted to WO */
        if(variant == "set" || variant == "clr" || variant == "tog" || access == WRITE_ONLY)
            return AT_WO;
        else if(access == READ_ONLY)
            return AT_RO;
        else
            return AT_RW;
    }

    bool has_sct() const
    {
        return true;
    }

    std::string sct_variant(macro_name_t name) const
    {
        switch(name)
        {
            case MN_FIELD_SET: return "set"; // always use set variant
            case MN_FIELD_CLEAR: return "clr"; // always use clr variant
            case MN_FIELD_TOG: return "tog"; // always use tog variant
            default: return "";
        }
    }
};

/**
 * Generator: atj
 */

class atj_generator : public common_generator
{
    bool has_support_macros() const
    {
        // no support macros
        return false;
    }

    bool has_selectors() const
    {
        return false;
    }

    std::string selector_soc_dir(const soc_ref_t& ref) const
    {
        return ref.get()->name;
    }

    std::string selector_include_header() const
    {
        // unused
        return "<error>";
    }

    std::string selector_soc_macro(const soc_ref_t& ref) const
    {
        // unused
        return "<error>";
    }

    std::string register_header(const node_inst_t& inst) const
    {
        /* one register header per top-level block */
        if(inst.is_root())
            return "<error>";
        if(inst.parent().is_root())
            return tolower(inst.node().name()) + ".h";
        else
            return register_header(inst.parent());
    }

    std::string macro_name(macro_name_t macro) const
    {
        // no macros are generated
        return "<macro_name>";
    }

    std::string macro_header() const
    {
        // unused
        return "<error>";
    }

    bool register_flag(const node_inst_t& inst, register_flag_t flag) const
    {
        /* make everything parametrized */
        switch(flag)
        {
            case RF_GENERATE_ALL_INST: return false;
            case RF_GENERATE_PARAM_INST: return true;
            default: return false;
        }
    }

    std::string type_xfix(macro_type_t type, bool prefix) const
    {
        switch(type)
        {
            case MT_REG_ADDR: return prefix ? "" : "_ADDR";
            case MT_REG_VAR: return prefix ? "" : "";
            case MT_FIELD_BP: return prefix ? "BP_" : "";
            case MT_FIELD_BM: return prefix ? "BM_" : "";
            case MT_FIELD_BV: return prefix ? "BV_" : "";
            case MT_FIELD_BF: return prefix ? "BF_" : "";
            case MT_FIELD_BFM: return prefix ? "BFM_" : "";
            case MT_FIELD_BFV: return prefix ? "BF_" : "_V";
            case MT_FIELD_BFMV: return prefix ? "BFM_" : "_V";
            default: return "<xfix>";
        }
    }

    std::string variant_xfix(const std::string& variant, bool prefix) const
    {
        return "<variant>";
    }

    std::string inst_prefix(const node_inst_t& inst) const
    {
        /* separate blocks with _: block_reg */
        return "_";
    }

    std::string field_prefix() const
    {
        /* separate fields with _: block_reg_field */
        return "_";
    }

    std::string enum_prefix() const
    {
        /* separate enums with __: block_reg_field__enum */
        return "__";
    }

    std::string enum_name(const enum_ref_t& enum_) const
    {
        return enum_.get()->name;
    }

    access_type_t register_access(const std::string& variant, access_t access) const
    {
        return AT_RW;
    }

    bool has_sct() const
    {
        return false;
    }

    std::string sct_variant(macro_name_t name) const
    {
        return "<variant>";
    }
};

/**
 * Driver
 */

abstract_generator *get_generator(const std::string& name)
{
    if(name == "jz")
        return new jz_generator();
    else if(name == "imx")
        return new imx_generator();
    else if(name == "atj")
        return new atj_generator();
    else
        return 0;
}

void usage()
{
    printf("usage: headergen [options] <desc files...>\n");
    printf("options:\n");
    printf("  -?/--help              Dispaly this help\n");
    printf("  -g/--generator <gen>   Select generator (jz, imx)\n");
    printf("  -o/--outdir <dir>      Output directory\n");
    printf("  -l/--list <list>       List of blocks to print\n");
    printf("  -e/--exclude           Blocks listed are excluded (instead of printed)\n");
    printf("Block list format:\n");
    printf("  block1,block2,...,blockN\n");
    exit(1);
}

int main(int argc, char **argv)
{
    char *generator_name = NULL;
    char *outdir = NULL;
    if(argc <= 1)
        usage();
    char *list = NULL;
    bool exclude = false;

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"generator", required_argument, 0, 'g'},
            {"outdir", required_argument, 0, 'o'},
            {"list", required_argument, 0, 'l'},
            {"exclude", no_argument, 0, 'e'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?g:o:l:e", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case '?':
                usage();
                break;
            case 'g':
                generator_name = optarg;
                break;
            case 'o':
                outdir = optarg;
                break;
            case 'l':
                list = optarg;
                break;
            case 'e':
                exclude = true;
                break;
            default:
                abort();
        }
    }
    if(argc == optind)
    {
        printf("You need at least one description file\n");
        return 3;
    }
    if(outdir == 0)
    {
        printf("You need to select an output directory\n");
        return 4;
    }
    if(generator_name == 0)
    {
        printf("You need to select a generator\n");
        return 1;
    }
    abstract_generator *gen = get_generator(generator_name);
    if(gen == 0)
    {
        printf("Unknown generator name '%s'\n", generator_name);
        return 2;
    }
    std::set< std::string > plist;
    if(list)
    {
        char *p = list;
        while(*p)
        {
            char *comma = strchr(p, ',');
            if(comma)
                *comma = 0;
            plist.insert(std::string(p));
            if(comma)
                p = comma + 1;
            else
                break;
        }
    }
    else
        exclude = true; // empty list + exclude => print everything
    gen->set_print_list(plist, !exclude);

    gen->set_output_dir(outdir);
    for(int i = optind; i < argc; i++)
    {
        error_context_t ctx;
        soc_t s;
        bool ret = parse_xml(argv[i], s, ctx);
        if(ctx.count() != 0)
            printf("In file %s:\n", argv[i]);
        print_context(ctx);
        if(!ret)
        {
            printf("Cannot parse file '%s'\n", argv[i]);
            return 1;
        }
        gen->add_soc(s);
    }
    error_context_t ctx;
    bool ret = gen->generate(ctx);
    print_context(ctx);
    if(!ret)
    {
        printf("Cannot generate headers\n");
        return 5;
    }

    return 0;
}
