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

const size_t MAJOR_VERSION = 2;
const size_t MINOR_VERSION = 0;
const size_t REVISION_VERSION = 0;

/** Typedef for SoC types: word, address and flags */
typedef uint32_t soc_addr_t;
typedef uint32_t soc_word_t;
typedef int soc_id_t;

/** Default value for IDs */
const soc_id_t DEFAULT_ID = 0xcafebabe;

/** Error class */
class err_t
{
public:
    enum level_t
    {
        INFO,
        WARNING,
        FATAL
    };
    err_t(level_t lvl, const std::string& loc, const std::string& msg)
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
    void add(const err_t& err) { m_list.push_back(err); }
    size_t count() const { return m_list.size(); }
    err_t get(size_t i) const { return m_list[i]; }
protected:
    std::vector< err_t > m_list;
};

/**
 * Bare representation of the format
 */

/** Register access type and rules
 *
 * Access can be specified on registers and register variants. When left
 * unspecified (aka DEFAULT), a register variant inherit the access from
 * the register, and a register defaults to read-write if unspecified.
 * When specified, the register variant access takes precedence over the register
 * access. */
enum access_t
{
    UNSPECIFIED = 0, /** Register: read-write, fields: inherit from register */
    READ_ONLY, /** Read-only */
    READ_WRITE, /** Read-write */
    WRITE_ONLY, /** Write-only */
};

/** Enumerated value (aka named value), represents a special value for a field */
struct enum_t
{
    soc_id_t id; /** ID (must be unique among field enums) */
    std::string name; /** Name (must be unique among field enums) */
    std::string desc; /** Optional description of the meaning of this value */
    soc_word_t value; /** Value of the field */

    /** Default constructor: default ID and value is 0 */
    enum_t():id(DEFAULT_ID), value(0) {}
};

/** Register field information */
struct field_t
{
    soc_id_t id; /** ID (must be unique among register fields) */
    std::string name; /** Name (must be unique among register fields) */
    std::string desc; /** Optional description of the field */
    size_t pos; /** Position of the least significant bit */
    size_t width; /** Width of the field in bits  */
    std::vector< enum_t > enum_; /** List of special values */

    /** Default constructor: default ID, position is 0, width is 1 */
    field_t():id(DEFAULT_ID), pos(0), width(1) {}

    /** Returns the bit mask of the field within the register */
    soc_word_t bitmask() const
    {
        // WARNING beware of the case where width is 32
        if(width == 32)
            return 0xffffffff;
        else
            return ((1 << width) - 1) << pos;
    }

    /** Returns the unshifted bit mask of the field */
    soc_word_t unshifted_bitmask() const
    {
        // WARNING beware of the case where width is 32
        if(width == 32)
            return 0xffffffff;
        else
            return (1 << width) - 1;
    }

    /** Extract field value from register value */
    soc_word_t extract(soc_word_t reg_val) const
    {
        return (reg_val & bitmask()) >> pos;
    }

    /** Replace the field value in a register value */
    soc_word_t replace(soc_word_t reg_val, soc_word_t field_val) const
    {
        return (reg_val & ~bitmask()) | ((field_val << pos) & bitmask());
    }

    /** Return field value index, or -1 if none */
    int find_value(soc_word_t v) const
    {
        for(size_t i = 0; i < enum_.size(); i++)
            if(enum_[i].value == v)
                return i;
        return -1;
    }
};

/** Register variant information
 *
 * A register variant provides an alternative access to the register, potentially
 * with special semantics. Although there are no constraints on the type string,
 * the following types have well-defined semantics:
 * - alias: the same register at another address
 * - set: writing to this register will set the 1s bits and ignore the 0s
 * - clr: writing to this register will clear the 1s bits and ignore the 0s
 * - tog: writing to this register will toggle the 1s bits and ignore the 0s
 * Note that by default, variants inherit the access type of the register but
 * can override it.
 */
struct variant_t
{
    soc_id_t id; /** ID (must be unique among register variants) */
    std::string type; /** type of the variant */
    soc_addr_t offset; /** offset of the variant */
    access_t access; /** Access type */

    /** Default constructor: default ID, offset is 0, access is unspecified */
    variant_t():id(DEFAULT_ID), offset(0), access(UNSPECIFIED) {}
};

/** Register information */
struct register_t
{
    size_t width; /** Size in bits */
    access_t access; /** Access type */
    std::string desc; /** Optional description of the register */
    std::vector< field_t > field; /** List of fields */
    std::vector< variant_t > variant; /** List of variants */

    /** Default constructor: width is 32 */
    register_t():width(32), access(UNSPECIFIED) {}
};

/** Node address range information */
struct range_t
{
    enum type_t
    {
        STRIDE, /** Addresses are given by a base address and a stride */
        FORMULA, /** Addresses are given by a formula */
        LIST, /** Addresses are given by a list */
    };

    type_t type; /** Range type */
    size_t first; /** First index in the range */
    size_t count; /** Number of indexes in the range (for STRIDE and RANGE) */
    soc_word_t base; /** Base address (for STRIDE) */
    soc_word_t stride; /** Stride value (for STRIDE) */
    std::string formula; /** Formula (for FORMULA) */
    std::string variable; /** Formula variable name (for FORMULA) */
    std::vector< soc_word_t > list; /** Address list (for LIST) */

    /** Default constructor: empty stride */
    range_t():type(STRIDE), first(0), count(0), base(0), stride(0) {}

    /** Return the number of indexes (based on count or list) */
    size_t size()
    {
        return type == LIST ? list.size() : count;
    }
};

/** Node instance information */
struct instance_t
{
    enum type_t
    {
        SINGLE, /** There is a single instance at a specified address */
        RANGE /** There are multiple addresses forming a range */
    };

    soc_id_t id; /** ID (must be unique among node instances) */
    std::string name; /** Name (must be unique among node instances) */
    std::string title; /** Optional instance human name */
    std::string desc; /** Optional description of the instance */
    type_t type; /** Instance type */
    soc_word_t addr; /** Address (for SINGLE) */
    range_t range; /** Range (for RANGE) */

    /** Default constructor: single instance at 0 */
    instance_t():id(DEFAULT_ID), type(SINGLE), addr(0) {}
};

/** Node information */
struct node_t
{
    soc_id_t id; /** ID (must be unique among nodes) */
    std::string name; /** Name (must be unique for the among nodes) */
    std::string title; /** Optional node human name */
    std::string desc; /** Optional description of the node */
    std::vector< register_t> register_; /** Optional register */
    std::vector< instance_t> instance; /** List of instances */
    std::vector< node_t > node; /** List of sub-nodes */

    /** Default constructor: default ID */
    node_t():id(DEFAULT_ID) {}
};

/** System-on-chip information */
struct soc_t
{
    std::string name; /** Codename of the SoC */
    std::string title; /** Human name of the SoC */
    std::string desc; /** Optional description of the SoC */
    std::string isa; /** Instruction Set Assembly */
    std::string version; /** Description version */
    std::vector< std::string > author; /** List of authors of the description */
    std::vector< node_t > node; /** List of nodes */
};

/** Parse a SoC description from a XML file, put it into <soc>. */
bool parse_xml(const std::string& filename, soc_t& soc, error_context_t& error_ctx);
/** Write a SoC description to a XML file, overwriting it. A file can contain
 * multiple Soc descriptions */
bool produce_xml(const std::string& filename, const soc_t& soc, error_context_t& error_ctx);
/** Normalise a soc description by reordering elements so that:
 * - nodes are sorted by lowest address of an instance
 * - instances are sorted by lowest address
 * - fields are sorted by last bit
 * - enum are sorted by value */
void normalize(soc_t& soc);
/** Formula parser: try to parse and evaluate a formula with some variables */
bool evaluate_formula(const std::string& formula,
    const std::map< std::string, soc_word_t>& var, soc_word_t& result,
    const std::string& loc, error_context_t& error_ctx);

/**
 * Convenience API to manipulate the format
 *
 * The idea is that *_ref_t objects are stable pointers: they stay valid even
 * when the underlying soc changes. In particular:
 * - modifying any structure data (except id fields) preserves all references
 * - removing a structure invalidates all references pointing to this structure
 *   and its children
 * - adding any structure preserves all references
 * These references can be used to get pointers to the actual data
 * of the representation when it needs to be read or write.
 */

class soc_ref_t;
class node_ref_t;
class register_ref_t;
class field_ref_t;
class enum_ref_t;
class variant_ref_t;
class node_inst_t;

/** SoC reference */
class soc_ref_t
{
    soc_t *m_soc; /* pointer to the soc */
public:
     /** Builds an invalid reference */
    soc_ref_t();
     /** Builds a reference to a soc */
    soc_ref_t(soc_t *soc);
    /** Checks whether this reference is valid */
    bool valid() const;
    /** Returns a pointer to the soc */
    soc_t *get() const;
    /** Returns a reference to the root node */
    node_ref_t root() const;
    /** Returns a reference to the root node instance */
    node_inst_t root_inst() const;
    /** Compare this reference to another */
    bool operator==(const soc_ref_t& r) const;
    inline bool operator!=(const soc_ref_t& r) const { return !operator==(r); }
    bool operator<(const soc_ref_t& r) const { return m_soc < r.m_soc; }
    /** Make this reference invalid */
    void reset();
};

/** SoC node reference
 * NOTE: the root soc node is presented as a node with empty path */
class node_ref_t
{
    friend class soc_ref_t;
    friend class node_inst_t;
    soc_ref_t m_soc; /* reference to the soc */
    std::vector< soc_id_t > m_path; /* path from the root */

    node_ref_t(soc_ref_t soc);
    node_ref_t(soc_ref_t soc, const std::vector< soc_id_t >& path);
public:
    /** Builds an invalid reference */
    node_ref_t();
    /** Check whether this reference is valid */
    bool valid() const;
    /** Check whether this reference is the root node */
    bool is_root() const;
    /** Returns a pointer to the node, or 0 if invalid or root */
    node_t *get() const;
    /** Returns a reference to the soc */
    soc_ref_t soc() const;
    /** Returns a reference to the n-th parent node, 0-th is itself, 1-th is parent */
    node_ref_t parent(unsigned level = 1) const;
    /** Returns reference depth, root is 0, below root is 1 and so on */
    unsigned depth() const;
    /** Returns a reference to the register (which may be on a parent node) */
    register_ref_t reg() const;
    /** Returns a list of references to the sub-nodes */
    std::vector< node_ref_t > children() const;
    /** Returns a reference to a specific child */
    node_ref_t child(const std::string& name) const;
    /** Returns the path of the node, as the list of node names from the root */
    std::vector< std::string > path() const;
    /** Returns the name of the node */
    std::string name() const;
    /** Compare this reference to another */
    bool operator==(const node_ref_t& r) const;
    inline bool operator!=(const node_ref_t& r) const { return !operator==(r); }
    /** Delete the node (and children) pointed by the reference, invalidating it
     * NOTE: if reference points to the root node, deletes all nodes
     * NOTE: does nothing if the reference is not valid */
    void remove();
    /** Create a new child node and returns a reference to it */
    node_ref_t create() const;
    /** Create a register and returns a reference to it */
    register_ref_t create_reg(size_t width = 32) const;
    /** Make this reference invalid */
    void reset();
};

/** SoC register reference */
class register_ref_t
{
    friend class node_ref_t;
    node_ref_t m_node; /* reference to the node owning the register */

    register_ref_t(node_ref_t node);
public:
    /** Builds an invalid reference */
    register_ref_t();
    /** Check whether this reference is valid/exists */
    bool valid() const;
    /** Returns a pointer to the register, or 0 */
    register_t *get() const;
    /** Returns a reference to the node containing the register */
    node_ref_t node() const;
    /** Returns a list of references to the fields of the register */
    std::vector< field_ref_t > fields() const;
    /** Returns a list of references to the variants of the register */
    std::vector< variant_ref_t > variants() const;
    /** Returns a reference to a particular field */
    field_ref_t field(const std::string& name) const;
    /** Returns a reference to a particular variant */
    variant_ref_t variant(const std::string& type) const;
    /** Compare this reference to another */
    bool operator==(const register_ref_t& r) const;
    inline bool operator!=(const register_ref_t& r) const { return !operator==(r); }
    /** Delete the register pointed by the reference, invalidating it
     * NOTE: does nothing if the reference is not valid */
    void remove();
    /** Create a new field and returns a reference to it */
    field_ref_t create_field() const;
    /** Create a new variant and returns a reference to it */
    variant_ref_t create_variant() const;
    /** Make this reference invalid */
    void reset();
};

/** SoC register field reference */
class field_ref_t
{
    friend class register_ref_t;
    register_ref_t m_reg; /* reference to the register */
    soc_id_t m_id; /* field id */

    field_ref_t(register_ref_t reg, soc_id_t id);
public:
    /** Builds an invalid reference */
    field_ref_t();
    /** Check whether this reference is valid/exists */
    bool valid() const;
    /** Returns a pointer to the field, or 0 */
    field_t *get() const;
    /** Returns a reference to the register containing the field */
    register_ref_t reg() const;
    /** Returns a list of references to the enums of the field */
    std::vector< enum_ref_t > enums() const;
    /** Compare this reference to another */
    bool operator==(const field_ref_t& r) const;
    inline bool operator!=(const field_ref_t& r) const { return !operator==(r); }
    /** Make this reference invalid */
    void reset();
    /** Create a new enum and returns a reference to it */
    enum_ref_t create_enum() const;
};

/** SoC register field enum reference */
class enum_ref_t
{
    friend class field_ref_t;
    field_ref_t m_field; /* reference to the field */
    soc_id_t m_id; /* enum id */

    enum_ref_t(field_ref_t reg, soc_id_t id);
public:
    /** Builds an invalid reference */
    enum_ref_t();
    /** Check whether this reference is valid/exists */
    bool valid() const;
    /** Returns a pointer to the enum, or 0 */
    enum_t *get() const;
    /** Returns a reference to the field containing the enum */
    field_ref_t field() const;
    /** Compare this reference to another */
    bool operator==(const field_ref_t& r) const;
    inline bool operator!=(const field_ref_t& r) const { return !operator==(r); }
    /** Make this reference invalid */
    void reset();
};

/** SoC register variant reference */
class variant_ref_t
{
    friend class register_ref_t;
    register_ref_t m_reg; /* reference to the register */
    soc_id_t m_id; /* variant name */

    variant_ref_t(register_ref_t reg, soc_id_t id);
public:
    /** Builds an invalid reference */
    variant_ref_t();
    /** Check whether this reference is valid/exists */
    bool valid() const;
    /** Returns a pointer to the variant, or 0 */
    variant_t *get() const;
    /** Returns a reference to the register containing the field */
    register_ref_t reg() const;
    /** Returns variant type */
    std::string type() const;
    /** Returns variant offset */
    soc_word_t offset() const;
    /** Compare this reference to another */
    bool operator==(const variant_ref_t& r) const;
    inline bool operator!=(const variant_ref_t& r) const { return !operator==(r); }
    /** Make this reference invalid */
    void reset();
};

/** SoC node instance
 * NOTE: the root soc node is presented as a node with a single instance at 0 */
class node_inst_t
{
    friend class node_ref_t;
    friend class soc_ref_t;
    node_ref_t m_node; /* reference to the node */
    std::vector< soc_id_t > m_id_path; /* list of instance IDs */
    std::vector< size_t > m_index_path; /* list of instance indexes */

    node_inst_t(soc_ref_t soc);
    node_inst_t(node_ref_t soc, const std::vector< soc_id_t >& path,
        const std::vector< size_t >& indexes);
public:
    /** Builds an invalid reference */
    node_inst_t();
    /** Check whether this instance is valid/exists */
    bool valid() const;
    /** Returns a reference to the soc */
    soc_ref_t soc() const;
    /** Returns a reference to the node */
    node_ref_t node() const;
    /** Check whether this reference is the root node instance */
    bool is_root() const;
    /** Returns a reference to the n-th parent instance, 0-th is itself, and so on */
    node_inst_t parent(unsigned level = 1) const;
    /** Returns reference depth, 0 is root, and so on */
    unsigned depth() const;
     /** Returns a pointer to the instance of the node, or 0 */
    instance_t *get() const;
    /** Returns the address of this instance */
    soc_addr_t addr() const;
    /** Returns an instance to a child of this node's instance. If the subnode
     * instance is a range, the returned reference is invalid */
    node_inst_t child(const std::string& name) const;
    /** Returns an instance to a child of this node's instance with a range index.
     * If the subnode is not not a range or if the index is out of bounds,
     * the returned reference is invalid */
    node_inst_t child(const std::string& name, size_t index) const;
    /** Returns a list of all instances of subnodes of this node's instance */
    std::vector< node_inst_t > children() const;
    /** Returns the name of the instance */
    std::string name() const;
    /** Checks whether this instance is indexed */
    bool is_indexed() const;
    /** Returns the index of the instance */
    size_t index() const;
    /** Compare this reference to another */
    bool operator==(const node_inst_t& r) const;
    inline bool operator!=(const node_inst_t& r) const { return !operator==(r); }
    /** Make this reference invalid */
    void reset();
};

} // soc_desc

#endif /* __SOC_DESC__ */
