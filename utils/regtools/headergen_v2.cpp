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

#define HEADERGEN_VERSION   "3.0.0"

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
        error_t e = ctx.get(j);
        switch(e.level())
        {
            case error_t::INFO: printf("[INFO]"); break;
            case error_t::WARNING: printf("[WARN]"); break;
            case error_t::FATAL: printf("[FATAL]"); break;
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
 * headergen version: " HEADERGEN_VERSION "\n";
    for(size_t i = 0; i < socs.size(); i++)
    {
        soc_t& s = *socs[i].get();
        if(!s.version.empty())
            fout << " * " << s.name << " version: " << s.version << "\n";
        if(!s.author.empty())
        {
            fout << " * " << s.name << " authors:";
            for(size_t j = 0; j < s.author.size(); j++)
            {
                if(j != 0)
                    fout << ",";
                fout << " " << s.author[j];
            }
            fout << "\n";
        }
    }
    fout << "\
 *\n\
 * Copyright (C) 2015 by the authors\n\
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

/**
 * Generator interface
 */

class abstract_generator
{
public:
    /// set output directory (default is current directory)
    void set_output_dir(const std::string& dir);
    /// add a SoC to the list
    void add_soc(const soc_t& soc);
    /// generate headers, returns true on success
    virtual bool generate(error_context_t& ctx) = 0;
protected:
    std::vector< soc_t > m_soc; /// list of socs
    std::string m_outdir; /// output directory path
};

void abstract_generator::set_output_dir(const std::string& dir)
{
    m_outdir = dir;
}

void abstract_generator::add_soc(const soc_t& soc)
{
    m_soc.push_back(soc);
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
 * - TYPED: typed macro for easy acess, typically (*(volatile uint32 *)addr)
 * - for each field F:
 *   - BP: this macro contains the bit position of F
 *   - BM: this macro contains the bit mask of F
 *   - for each field enum value V:
 *     - BV: this macro contains the value of V (not shifted to the position)
 *   - BF: this macro generate the mask for a value: (((v) << pos) & mask)
 *   - BFV: same as BF but the parameter is the name of an enum value
 *   - BW: write OR, shorthand for HW_<reg> = BF_OR(<reg>, ...)
 *   - BR: read field
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
 *
 * More macros are constructed for convenience. The BW macro is a shorthand
 * for register write with ORing:
 *   BW(<reg>)(<f1>,<f2>,...) expands to TYPED(<reg>) = BF_OR(<reg>, <f1>, <f2>, ...)
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
        MN_BF_OR, /// field ORing
        MN_BM_OR, /// mask ORing
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
        MT_REG_TYPED, /// typed register: *(volatile uintn_t *)(addr)
        MT_REG_BW, /// write with OR
        MT_REG_BR, /// read field
        MT_FIELD_BP, /// bit position of a field
        MT_FIELD_BM, /// bit mask of a field
        MT_FIELD_BV, /// bit value of a field enum
        MT_FIELD_BF, /// bit field value: (v << pos) & mask
        MT_FIELD_BFV, /// bit field enum value: (enum_v << pos) & mask
    };
    /// return type prefix/suffix for register macro
    virtual std::string type_xfix(macro_type_t type, bool prefix) const = 0;
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
        std::vector< std::string >& params);
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
    std::vector< std::string >& params)
{
    std::ostringstream oss;
    unsigned counter = 1;
    oss << "(";
    for(size_t i = 0; i < reg.parametric.size(); i++)
    {
        if(i != 0)
            oss << " + ";
        node_inst_t ninst = reg.inst.parent(reg.parametric.size() - i - 1);
        instance_t& inst = *ninst.get();
        if(reg.parametric[i])
            params.push_back("_n" + to_str(counter++));
        if(inst.type == instance_t::RANGE)
        {
            if(inst.range.type == range_t::STRIDE)
            {
                if(reg.parametric[i])
                {
                    oss << to_hex(inst.range.base) << " + ";
                    oss << "(" << params.back() << ") * " << to_hex(inst.range.stride);
                }
                else
                    oss << to_hex(inst.range.base + ninst.index() * inst.range.stride);
            }
            else if(inst.range.type == range_t::FORMULA)
            {
                if(!reg.parametric[i])
                {
                    soc_word_t res;
                    std::map< std::string, soc_word_t > vars;
                    vars[inst.range.variable] = ninst.index();
                    error_context_t ctx;
                    if(!evaluate_formula(inst.range.formula, vars, res, "", ctx))
                        oss << "#error cannot evaluate formula";
                    else
                        oss << to_hex(res);
                }
                else
                    oss << strsubst(inst.range.formula, inst.range.variable,
                        "(" + params.back() + ")");
            }
        }
        else if(inst.type == instance_t::SINGLE)
            oss << to_hex(inst.addr);
        else
            return "#error unknown instance type";
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
    std::string macro_addr = macro_basename(reg, MT_REG_ADDR) + type_xfix(MT_REG_ADDR, false);
    std::string macro_typed = macro_basename(reg, MT_REG_TYPED) + type_xfix(MT_REG_TYPED, false);
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
    std::string bfv_prefix = macro_basename(reg, MT_FIELD_BFV) + field_prefix();

    /* print ADDR macro: addr */
    ctx.add(macro_addr + param_str, addr);
    /* print TYPED macro: (*(volatile uintN_t *)(addr)) */
    unsigned width = reg.inst.node().reg().get()->width;
    ctx.add(macro_typed + param_str,
        "(*(volatile uint" + to_str(width) + "_t *)" + macro_addr + param_str + ")");
    /* print BW macro: TYPED(reg) = BF_OR(reg, ...) */
    ctx.add(macro_basename(reg, MT_REG_BW) + type_xfix(MT_REG_BW, false) + "(" +
        param_str_no_paren_comma + "...)",
        macro_typed + param_str + " = " + macro_name(MN_BF_OR) +
        "(" + macro_basename(reg) + ", __VA_ARGS__)");
    /* print BR macro: ((TYPED(reg) & BM(reg, field)) >> BP(reg, field)) */
    ctx.add(macro_basename(reg, MT_REG_BR) + type_xfix(MT_REG_BW, false) + "(" +
        param_str_no_paren_comma + "_f)",
        "((" + macro_typed + param_str +
        " & " + bm_prefix + "##_f) & " + bp_prefix + "##_f)");
    /* print fields */
    std::vector< field_ref_t > fields = reg.inst.node().reg().fields();
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
        /* print BF macro: (((v) << pos) & mask) */
        ctx.add(bf_prefix + fname + type_xfix(MT_FIELD_BF, false) + "(_v)",
            "(((_v) << " + bp_prefix + fname + type_xfix(MT_FIELD_BP, false) + ") & " +
            bm_prefix + fname + type_xfix(MT_FIELD_BM, false) + ")");
        /* print BFV macro: ((enum_v) << pos) & mask) */
        ctx.add(bf_prefix + fname + type_xfix(MT_FIELD_BFV, false) + "(_e)",
            bf_prefix + fname + type_xfix(MT_FIELD_BF, false) + "(" +
            bv_prefix + "##_e)");
    }

    ctx.print(os);
    return true;
}

bool common_generator::generate_macro_header(error_context_t& ectx)
{
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
    const int MAX_N = 10;

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

    /* print BF_OR and BM_OR macros */
    fout << "#define " << macro_name(MN_BF_OR) << "(reg, ...) "
        << var_expand << "(" << var_or << ", " << type_xfix(MT_FIELD_BF, true)
        << "##reg##" << field_prefix() << ", __VA_ARGS__)\n";
    fout << "#define " << macro_name(MN_BM_OR) << "(reg, ...) "
        << var_expand << "(" << var_or << ", " << type_xfix(MT_FIELD_BM, true)
        << "##reg##" << field_prefix() << ", __VA_ARGS__)\n\n";

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
         * we include the macro header right here */
        if(!has_selectors())
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
 * Generator: imx233
 */

class imx233_generator : public common_generator
{
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
        return "regs-select.h";
    }

    std::string selector_soc_macro(const soc_ref_t& ref) const
    {
        return toupper(ref.get()->name) + "_INCLUDE";
    }

    std::string register_header(const node_inst_t& inst) const
    {
        if(inst.is_root())
            return "<error>";
        if(inst.parent().is_root())
            return "regs-" + tolower(inst.node().name()) + ".h";
        else
            return register_header(inst.parent());
    }

    std::string macro_name(macro_name_t macro) const
    {
        switch(macro)
        {
            case MN_BF_OR: return "BF_OR";
            case MN_BM_OR: return "BM_OR";
            default: return "";
        }
    }

    std::string macro_header() const
    {
        return "regs-macro.h";
    }

    bool register_flag(const node_inst_t& inst, register_flag_t flag) const
    {
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
            case MT_REG_ADDR: return prefix ? "HW_" : "_ADDR";
            case MT_REG_TYPED: return prefix ? "HW_" : "";
            case MT_REG_BW: return prefix ? "BW_" : "";
            case MT_REG_BR: return prefix ? "BR_" : "";
            case MT_FIELD_BP: return prefix ? "BP_" : "";
            case MT_FIELD_BM: return prefix ? "BM_" : "";
            case MT_FIELD_BV: return prefix ? "BV_" : "";
            case MT_FIELD_BF: return prefix ? "BF_" : "";
            case MT_FIELD_BFV: return prefix ? "BF_" : "_V";
            default: return "";
        }
    }

    std::string inst_prefix(const node_inst_t& inst) const
    {
        return "_";
    }

    std::string field_prefix() const
    {
        return "_";
    }

    std::string enum_prefix() const
    {
        return "__";
    }

    std::string enum_name(const enum_ref_t& enum_) const
    {
        return enum_.get()->name;
    }
};

/**
 * Driver
 */

abstract_generator *get_generator(const std::string& name)
{
    if(name == "imx233")
        return new imx233_generator();
    else
        return 0;
}

void usage()
{
    printf("usage: headergen [options] <desc files...> <output directory>\n");
    printf("options:\n");
    printf("  -?/--help              Dispaly this help\n");
    printf("  -g/--generator <gen>   Select generator (imx233, rk27xx, atj)\n");
    exit(1);
}

int main(int argc, char **argv)
{
    char *generator_name = NULL;
    if(argc <= 1)
        usage();

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"generator", no_argument, 0, 'g'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?g:", long_options, NULL);
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
            default:
                abort();
        }
    }
    if(argc - optind <= 1)
    {
        printf("You need at least one description file and an output directory\n");
        return 3;
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

    gen->set_output_dir(argv[argc - 1]);
    for(int i = optind; i < argc - 1; i++)
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
