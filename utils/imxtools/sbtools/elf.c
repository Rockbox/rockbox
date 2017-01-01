/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Amaury Pouly
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
#define _POSIX_C_SOURCE 200809L /* for strdup */
#include "elf.h"
#include "misc.h"
#include <stdarg.h>
#include <string.h>

/**
 * Definitions
 *   taken from elf.h linux header
 *   based on ELF specification
 *   based on ARM ELF specification
 */
typedef uint16_t Elf32_Half;

typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint16_t Elf32_Section;

#define EI_NIDENT   16

typedef struct
{
    unsigned char   e_ident[EI_NIDENT]; /* Magic number and other info */
    Elf32_Half      e_type; /* Object file type */
    Elf32_Half      e_machine; /* Architecture */
    Elf32_Word      e_version; /* Object file version */
    Elf32_Addr      e_entry; /* Entry point virtual address */
    Elf32_Off       e_phoff; /* Program header table file offset */
    Elf32_Off       e_shoff; /* Section header table file offset */
    Elf32_Word      e_flags; /* Processor-specific flags */
    Elf32_Half      e_ehsize; /* ELF header size in bytes */
    Elf32_Half      e_phentsize; /* Program header table entry size */
    Elf32_Half      e_phnum; /* Program header table entry count */
    Elf32_Half      e_shentsize; /* Section header table entry size */
    Elf32_Half      e_shnum; /* Section header table entry count */
    Elf32_Half      e_shstrndx; /* Section header string table index */
}Elf32_Ehdr;

#define EI_MAG0     0 /* File identification byte 0 index */
#define ELFMAG0     0x7f /* Magic number byte 0 */

#define EI_MAG1     1 /* File identification byte 1 index */
#define ELFMAG1     'E' /* Magic number byte 1 */

#define EI_MAG2     2 /* File identification byte 2 index */
#define ELFMAG2     'L' /* Magic number byte 2 */

#define EI_MAG3     3 /* File identification byte 3 index */
#define ELFMAG3     'F' /* Magic number byte 3 */

#define EI_CLASS    4 /* File class byte index */
#define ELFCLASS32  1 /* 32-bit objects */

#define EI_DATA     5 /* Data encoding byte index */
#define ELFDATA2LSB 1 /* 2's complement, little endian */

#define EI_VERSION  6 /* File version byte index, Value must be EV_CURRENT */

#define EI_OSABI    7 /* OS ABI identification */
#define ELFOSABI_NONE       0 /* UNIX System V ABI */
#define ELFOSABI_ARM_AEABI  64 /* ARM EABI */
#define ELFOSABI_ARM        97 /* ARM */

#define EI_ABIVERSION   8 /* ABI version */

#define EI_PAD      9 /* Byte index of padding bytes */

#define ET_EXEC     2 /* Executable file */

#define EM_ARM      40 /* ARM */

#define EV_CURRENT  1 /* Current version */

#define EF_ARM_HASENTRY     0x00000002

#define SHN_UNDEF   0 /* Undefined section */
#define SHN_ABS     0xfff1 /* Associated symbol is absolute */

typedef struct
{
    Elf32_Word  sh_name; /* Section name (string tbl index) */
    Elf32_Word  sh_type; /* Section type */
    Elf32_Word  sh_flags; /* Section flags */
    Elf32_Addr  sh_addr; /* Section virtual addr at execution */
    Elf32_Off   sh_offset; /* Section file offset */
    Elf32_Word  sh_size; /* Section size in bytes */
    Elf32_Word  sh_link; /* Link to another section */
    Elf32_Word  sh_info; /* Additional section information */
    Elf32_Word  sh_addralign; /* Section alignment */
    Elf32_Word  sh_entsize; /* Entry size if section holds table */
}Elf32_Shdr;

#define SHT_NULL            0 /* Section header table entry unused */
#define SHT_PROGBITS        1 /* Program data */
#define SHT_SYMTAB          2 /* Symbol table */
#define SHT_STRTAB          3 /* String table */
#define SHT_RELA            4 /* Relocation entries with addends */
#define SHT_HASH            5 /* Symbol hash table */
#define SHT_DYNAMIC         6 /* Dynamic linking information */
#define SHT_NOTE            7 /* Notes */
#define SHT_NOBITS          8 /* Program space with no data (bss) */
#define SHT_REL             9 /* Relocation entries, no addends */
#define SHT_SHLIB           10 /* Reserved */
#define SHT_DYNSYM          11 /* Dynamic linker symbol table */
#define SHT_INIT_ARRAY      14 /* Array of constructors */
#define SHT_FINI_ARRAY      15 /* Array of destructors */
#define SHT_PREINIT_ARRAY   16 /* Array of pre-constructors */
#define SHT_GROUP           17 /* Section group */
#define SHT_SYMTAB_SHNDX    18 /* Extended section indeces */
#define	SHT_NUM             19 /* Number of defined types.  */

#define SHF_WRITE       (1 << 0) /* Writable */
#define SHF_ALLOC       (1 << 1) /* Occupies memory during execution */
#define SHF_EXECINSTR   (1 << 2) /* Executable */
#define SHF_MERGE       (1 << 4) /* Might be merged */
#define SHF_STRINGS     (1 << 5) /* Contains nul-terminated strings */

typedef struct
{
    Elf32_Word  p_type; /* Segment type */
    Elf32_Off   p_offset; /* Segment file offset */
    Elf32_Addr  p_vaddr; /* Segment virtual address */
    Elf32_Addr  p_paddr; /* Segment physical address */
    Elf32_Word  p_filesz; /* Segment size in file */
    Elf32_Word  p_memsz; /* Segment size in memory */
    Elf32_Word  p_flags; /* Segment flags */
    Elf32_Word  p_align; /* Segment alignment */
}Elf32_Phdr;

#define PT_LOAD     1 /* Loadable program segment */

#define PF_X    (1 << 0) /* Segment is executable */
#define PF_W    (1 << 1) /* Segment is writable */
#define PF_R    (1 << 2) /* Segment is readable */

typedef struct
{
    Elf32_Word    st_name; /* Symbol name (string tbl index) */
    Elf32_Addr    st_value; /* Symbol value */
    Elf32_Word    st_size; /* Symbol size */
    unsigned char st_info; /* Symbol type and binding */
    unsigned char st_other; /* Symbol visibility */
    Elf32_Section st_shndx; /* Section index */
}Elf32_Sym;

#define ELF32_ST_BIND(val)          (((unsigned char) (val)) >> 4)
#define ELF32_ST_TYPE(val)          ((val) & 0xf)
#define ELF32_ST_INFO(bind, type)   (((bind) << 4) + ((type) & 0xf))

#define STB_LOCAL   0       /* Local symbol */
#define STB_GLOBAL  1       /* Global symbol */
#define STB_WEAK    2       /* Weak symbol */
#define STB_NUM     3       /* Number of defined types.  */
#define STB_LOOS    10      /* Start of OS-specific */
#define STB_GNU_UNIQUE  10      /* Unique symbol.  */
#define STB_HIOS    12      /* End of OS-specific */
#define STB_LOPROC  13      /* Start of processor-specific */
#define STB_HIPROC  15      /* End of processor-specific */

#define STT_NOTYPE  0       /* Symbol type is unspecified */
#define STT_OBJECT  1       /* Symbol is a data object */
#define STT_FUNC    2       /* Symbol is a code object */
#define STT_SECTION 3       /* Symbol associated with a section */
#define STT_FILE    4       /* Symbol's name is file name */
#define STT_COMMON  5       /* Symbol is a common data object */
#define STT_TLS     6       /* Symbol is thread-local data object*/
#define STT_NUM     7       /* Number of defined types.  */
#define STT_LOOS    10      /* Start of OS-specific */
#define STT_GNU_IFUNC   10      /* Symbol is indirect code object */
#define STT_HIOS    12      /* End of OS-specific */
#define STT_LOPROC  13      /* Start of processor-specific */
#define STT_HIPROC  15      /* End of processor-specific */

void elf_init(struct elf_params_t *params)
{
    memset(params, 0, sizeof(struct elf_params_t));
}

extern void *xmalloc(size_t s);

static struct elf_section_t *elf_add_section(struct elf_params_t *params)
{
    struct elf_section_t *sec = xmalloc(sizeof(struct elf_section_t));
    memset(sec, 0, sizeof(struct elf_section_t));
    if(params->first_section == NULL)
        params->first_section = params->last_section = sec;
    else
    {
        params->last_section->next = sec;
        params->last_section = sec;
    }
    sec->next = NULL;

    return sec;
}

static struct elf_symbol_t *elf_add_symbol(struct elf_params_t *params)
{
    struct elf_symbol_t *sym = xmalloc(sizeof(struct elf_symbol_t));
    memset(sym, 0, sizeof(struct elf_symbol_t));
    if(params->first_symbol == NULL)
        params->first_symbol = params->last_symbol = sym;
    else
    {
        params->last_symbol->next = sym;
        params->last_symbol = sym;
    }
    sym->next = NULL;

    return sym;
}

static struct elf_segment_t *elf_add_segment(struct elf_params_t *params)
{
    struct elf_segment_t *seg = xmalloc(sizeof(struct elf_section_t));
    if(params->first_segment == NULL)
        params->first_segment = params->last_segment = seg;
    else
    {
        params->last_segment->next = seg;
        params->last_segment = seg;
    }
    seg->next = NULL;

    return seg;
}

void elf_add_load_section(struct elf_params_t *params,
    uint32_t load_addr, uint32_t size, const void *section, const char *name)
{
    struct elf_section_t *sec = elf_add_section(params);

    sec->name = strdup(name);
    sec->type = EST_LOAD;
    sec->addr = load_addr;
    sec->size = size;
    sec->section = xmalloc(size);
    memcpy(sec->section, section, size);
}

void elf_add_fill_section(struct elf_params_t *params,
    uint32_t fill_addr, uint32_t size, uint32_t pattern, const char *name)
{
    if(pattern != 0x00)
    {
        printf("oops, non-zero filling, ignore fill section\n");
        return;
    }

    struct elf_section_t *sec = elf_add_section(params);

    sec->name = strdup(name);
    sec->type = EST_FILL;
    sec->addr = fill_addr;
    sec->size = size;
    sec->pattern = pattern;
}

/* sort by increasing type and then increasing address */
static int elf_simplify_compare(const void *a, const void *b)
{
    const struct elf_section_t *sa = a;
    const struct elf_section_t *sb = b;
    if(sa->type != sb->type)
        return sa->type - sb->type;
    return sa->addr - sb->addr;
}

void elf_simplify(struct elf_params_t *params)
{
    /** find all sections of the same types which are contiguous and merge them */

    /* count sections */
    int nr_sections = 0;
    struct elf_section_t *cur_sec = params->first_section;
    while(cur_sec)
    {
        nr_sections++;
        cur_sec = cur_sec->next;
    }

    /* put all sections in an array and free list */
    struct elf_section_t *sections = malloc(sizeof(struct elf_section_t) * nr_sections);
    cur_sec = params->first_section;
    for(int i = 0; i < nr_sections; i++)
    {
        memcpy(&sections[i], cur_sec, sizeof(struct elf_section_t));
        struct elf_section_t *old = cur_sec;
        cur_sec = cur_sec->next;
        free(old);
    }

    /* sort them by type and increasing addresses */
    qsort(sections, nr_sections, sizeof(struct elf_section_t), &elf_simplify_compare);

    /* merge them ! */
    cur_sec = &sections[0];
    for(int i = 1; i < nr_sections; i++)
    {
        /* different type => no */
        if(sections[i].type != cur_sec->type)
            goto Lnext;
        /* (for fill) different pattern => no */
        if(sections[i].type == EST_FILL && sections[i].pattern != cur_sec->pattern)
            goto Lnext;
        /* not contiguous => no */
        if(sections[i].addr != cur_sec->addr + cur_sec->size)
            goto Lnext;
        /* merge !! */
        if(sections[i].type == EST_FILL)
        {
            cur_sec->size += sections[i].size;
            sections[i].size = 0; // will be ignored by rebuilding (see below)
            free(sections[i].name);
        }
        else if(sections[i].type == EST_LOAD)
        {
            // merge data also
            void *data = malloc(cur_sec->size + sections[i].size);
            memcpy(data, cur_sec->section, cur_sec->size);
            memcpy(data + cur_sec->size, sections[i].section, sections[i].size);
            free(cur_sec->section);
            free(sections[i].section);
            free(sections[i].name);
            cur_sec->section = data;
            cur_sec->size += sections[i].size;
            sections[i].size = 0; // will be ignored by rebuilding (see below)
        }
        continue;

        /* update current section to consider */
        Lnext:
        cur_sec = &sections[i];
    }

    /* put back on a list and free array */
    struct elf_section_t **prev_ptr = &params->first_section;
    struct elf_section_t *prev_sec = NULL;
    for(int i = 0; i < nr_sections; i++)
    {
        /* skip empty sections produced by simplification */
        if(sections[i].size == 0)
            continue;

        struct elf_section_t *sec = malloc(sizeof(struct elf_section_t));
        memcpy(sec, &sections[i], sizeof(struct elf_section_t));
        *prev_ptr = sec;
        prev_ptr = &sec->next;
        prev_sec = sec;
    }
    *prev_ptr = NULL;
    params->last_section = prev_sec;
    free(sections);
}

/* sort by increasing address */
static int elf_addr_compare(const void *a, const void *b)
{
    const struct elf_section_t *sa = a;
    const struct elf_section_t *sb = b;
    return sa->addr - sb->addr;
}

void elf_sort_by_address(struct elf_params_t *params)
{
    /** sort sections by address */

    /* count sections */
    int nr_sections = 0;
    struct elf_section_t *cur_sec = params->first_section;
    while(cur_sec)
    {
        nr_sections++;
        cur_sec = cur_sec->next;
    }

    /* put all sections in an array and free list */
    struct elf_section_t *sections = malloc(sizeof(struct elf_section_t) * nr_sections);
    cur_sec = params->first_section;
    for(int i = 0; i < nr_sections; i++)
    {
        memcpy(&sections[i], cur_sec, sizeof(struct elf_section_t));
        struct elf_section_t *old = cur_sec;
        cur_sec = cur_sec->next;
        free(old);
    }

    /* sort them by type and increasing addresses */
    qsort(sections, nr_sections, sizeof(struct elf_section_t), &elf_addr_compare);

    /* put back on a list and free array */
    struct elf_section_t **prev_ptr = &params->first_section;
    struct elf_section_t *prev_sec = NULL;
    for(int i = 0; i < nr_sections; i++)
    {
        /* skip empty sections produced by simplification */
        if(sections[i].size == 0)
            continue;

        struct elf_section_t *sec = malloc(sizeof(struct elf_section_t));
        memcpy(sec, &sections[i], sizeof(struct elf_section_t));
        *prev_ptr = sec;
        prev_ptr = &sec->next;
        prev_sec = sec;
    }
    *prev_ptr = NULL;
    params->last_section = prev_sec;
    free(sections);
}

void elf_write_file(struct elf_params_t *params, elf_write_fn_t write,
    generic_printf_t printf, void *user)
{
    (void) printf;

    Elf32_Ehdr ehdr;
    uint32_t phnum = 0;
    struct elf_section_t *sec = params->first_section;
    uint32_t offset = 0;
    Elf32_Phdr phdr;
    Elf32_Shdr shdr;
    memset(&ehdr, 0, EI_NIDENT);

    while(sec)
    {
        if(sec->type == EST_LOAD)
        {
            sec->offset = offset;
            offset += sec->size;
        }
        else
        {
            sec->offset = 0;
        }

        phnum++;
        sec = sec->next;
    }

    uint32_t strtbl_offset = offset;

    ehdr.e_ident[EI_MAG0] = ELFMAG0;
    ehdr.e_ident[EI_MAG1] = ELFMAG1;
    ehdr.e_ident[EI_MAG2] = ELFMAG2;
    ehdr.e_ident[EI_MAG3] = ELFMAG3;
    ehdr.e_ident[EI_CLASS] = ELFCLASS32;
    ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
    ehdr.e_ident[EI_VERSION] = EV_CURRENT;
    ehdr.e_ident[EI_OSABI] = ELFOSABI_NONE;
    ehdr.e_ident[EI_ABIVERSION] = 0;
    ehdr.e_type = ET_EXEC;
    ehdr.e_machine = EM_ARM;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_entry = params->start_addr;
    ehdr.e_flags = 0;
    if(params->has_start_addr)
        ehdr.e_flags |= EF_ARM_HASENTRY;
    ehdr.e_ehsize = sizeof ehdr;
    ehdr.e_phentsize = sizeof phdr;
    ehdr.e_phnum = phnum;
    ehdr.e_shentsize = sizeof shdr;
    ehdr.e_shnum = phnum + 2; /* one for section 0 and one for string table */
    ehdr.e_shstrndx = ehdr.e_shnum - 1;
    ehdr.e_phoff = ehdr.e_ehsize;
    ehdr.e_shoff = ehdr.e_ehsize + ehdr.e_phnum * ehdr.e_phentsize;

    write(user, 0, &ehdr, sizeof ehdr);

    /* allocate enough size for the string table:
     * - one empty name ("\0")
     * - one name ".shstrtab\0"
     * - all section names with zeroes */
    size_t strtbl_size = 1+ strlen(".shstrtab") + 1;
    sec = params->first_section;
    while(sec)
    {
        strtbl_size += strlen(sec->name) + 1;
        sec = sec->next;
    }

    char *strtbl_content = malloc(strtbl_size);

    strtbl_content[0] = '\0';
    strcpy(&strtbl_content[1], ".shstrtab");
    uint32_t strtbl_index = 1 + strlen(".shstrtab") + 1;

    uint32_t data_offset = ehdr.e_ehsize + ehdr.e_phnum * ehdr.e_phentsize +
        ehdr.e_shnum * ehdr.e_shentsize;

    sec = params->first_section;
    offset = ehdr.e_phoff;
    while(sec)
    {
        sec->offset += data_offset;

        phdr.p_type = PT_LOAD;
        if(sec->type == EST_LOAD)
            phdr.p_offset = sec->offset;
        else
            phdr.p_offset = 0;
        phdr.p_paddr = elf_translate_virtual_address(params, sec->addr);
        phdr.p_vaddr = sec->addr; /* assume identity map ? */
        phdr.p_memsz = sec->size;
        if(sec->type == EST_LOAD)
            phdr.p_filesz = phdr.p_memsz;
        else
            phdr.p_filesz = 0;
        phdr.p_flags = PF_X | PF_W | PF_R;
        phdr.p_align = 0;

        write(user, offset, &phdr, sizeof phdr);

        offset += sizeof(Elf32_Phdr);
        sec = sec->next;
    }

    sec = params->first_section;
    offset = ehdr.e_shoff;

    {
        shdr.sh_name = 0;
        shdr.sh_type = SHT_NULL;
        shdr.sh_flags = 0;
        shdr.sh_addr = 0;
        shdr.sh_offset = 0;
        shdr.sh_size = 0;
        shdr.sh_link = SHN_UNDEF;
        shdr.sh_info = 0;
        shdr.sh_addralign = 0;
        shdr.sh_entsize = 0;

        write(user, offset, &shdr, sizeof shdr);

        offset += sizeof(Elf32_Shdr);
    }

    while(sec)
    {
        shdr.sh_name = strtbl_index;
        strtbl_index += 1 + sprintf(&strtbl_content[strtbl_index], "%s", sec->name);
        if(sec->type == EST_LOAD)
            shdr.sh_type = SHT_PROGBITS;
        else
            shdr.sh_type = SHT_NOBITS;
        shdr.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
        shdr.sh_addr = sec->addr;
        shdr.sh_offset = sec->offset;
        shdr.sh_size = sec->size;
        shdr.sh_link = SHN_UNDEF;
        shdr.sh_info = 0;
        shdr.sh_addralign = 1;
        shdr.sh_entsize = 0;

        write(user, offset, &shdr, sizeof shdr);

        offset += sizeof(Elf32_Shdr);
        sec = sec->next;
    }

    {
        shdr.sh_name = 1;
        shdr.sh_type = SHT_STRTAB;
        shdr.sh_flags = 0;
        shdr.sh_addr = 0;
        shdr.sh_offset = strtbl_offset + data_offset;
        shdr.sh_size = strtbl_index;
        shdr.sh_link = SHN_UNDEF;
        shdr.sh_info = 0;
        shdr.sh_addralign = 1;
        shdr.sh_entsize = 0;

        write(user, offset, &shdr, sizeof shdr);

        offset += sizeof(Elf32_Shdr);
    }

    sec = params->first_section;
    while(sec)
    {
        if(sec->type == EST_LOAD)
            write(user, sec->offset, sec->section, sec->size);
        sec = sec->next;
    }

    write(user, strtbl_offset + data_offset, strtbl_content, strtbl_index);
    free(strtbl_content);
}

static void *elf_load_section(Elf32_Shdr *sh, elf_read_fn_t read, generic_printf_t printf, void *user)
{
    void *data = xmalloc(sh->sh_size);
    if(!read(user, sh->sh_offset, data, sh->sh_size))
    {
        free(data);
        printf(user, true, OFF, "error reading elf section data\n");
        return NULL;
    }
    return data;
}

bool elf_guess(elf_read_fn_t read, void *user)
{
    /* read header */
    Elf32_Ehdr ehdr;
    if(!read(user, 0, &ehdr, sizeof(ehdr)))
        return false;
    /* basic checks */
    return ehdr.e_ident[EI_MAG0] == ELFMAG0 && ehdr.e_ident[EI_MAG1] == ELFMAG1 &&
            ehdr.e_ident[EI_MAG2] == ELFMAG2 && ehdr.e_ident[EI_MAG3] == ELFMAG3 &&
            ehdr.e_ehsize == sizeof(ehdr) && ehdr.e_phentsize == sizeof(Elf32_Phdr) &&
            ehdr.e_shentsize == sizeof(Elf32_Shdr);
}

bool elf_read_file(struct elf_params_t *params, elf_read_fn_t read,
    generic_printf_t printf, void *user)
{
    #define error_printf(...) ({printf(user, true, GREY, __VA_ARGS__); return false;})

    /* read header */
    Elf32_Ehdr ehdr;
    if(!read(user, 0, &ehdr, sizeof(ehdr)))
    {
        printf(user, true, GREY, "error reading elf header\n");
        return false;
    }
    /* basic checks */
    if(ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
            ehdr.e_ident[EI_MAG2] != ELFMAG2 || ehdr.e_ident[EI_MAG3] != ELFMAG3)
        error_printf("invalid elf header\n");
    if(ehdr.e_ident[EI_CLASS] != ELFCLASS32)
        error_printf("invalid elf class: must be a 32-bit object\n");
    if(ehdr.e_ident[EI_DATA] != ELFDATA2LSB)
        error_printf("invalid elf data encoding: must be 32-bit lsb\n");
    if(ehdr.e_ident[EI_VERSION] != EV_CURRENT)
        error_printf("invalid elf version\n");
    if(ehdr.e_type != ET_EXEC)
        error_printf("invalid elf file: must be an executable file\n");
    if(ehdr.e_machine != EM_ARM)
        error_printf("invalid elf file: must target an arm machine\n");
    if(ehdr.e_ehsize != sizeof(ehdr))
        error_printf("invalid elf file: size header mismatch\n");
    if(ehdr.e_phnum > 0 && ehdr.e_phentsize != sizeof(Elf32_Phdr))
        error_printf("invalid elf file: program header size mismatch\n");
    if(ehdr.e_shnum > 0 && ehdr.e_shentsize != sizeof(Elf32_Shdr))
        error_printf("invalid elf file: section header size mismatch\n");
    elf_set_start_addr(params, ehdr.e_entry);

    /* run through sections */
    printf(user, false, OFF, "ELF file:\n");
    Elf32_Shdr *shdr = xmalloc(sizeof(Elf32_Shdr) * ehdr.e_shnum);
    if(!read(user, ehdr.e_shoff, shdr, sizeof(Elf32_Shdr) * ehdr.e_shnum))
    {
        printf(user, true, GREY, "cannot read elf section headers\n");
        return false;
    }
    char *strtab = elf_load_section(&shdr[ehdr.e_shstrndx], read, printf, user);
    if(!strtab)
        printf(user, false, OFF, "elf file has no valid section string table\n");
    for(int i = 1; i < ehdr.e_shnum; i++)
    {
        const char *sec_name = &strtab[shdr[i].sh_name];
        if(strtab == NULL)
            sec_name = NULL;

        if(shdr[i].sh_type == SHT_PROGBITS && shdr[i].sh_flags & SHF_ALLOC)
        {
            void *data = elf_load_section(&shdr[i], read, printf, user);
            if(!data)
            {
                printf(user, true, GREY, "cannot read elf section %s\n", sec_name);
                goto Lerr;
            }
            elf_add_load_section(params, shdr[i].sh_addr, shdr[i].sh_size, data, sec_name);
            free(data);

            printf(user, false, OFF, "create load segment for %s\n", sec_name);
        }
        else if(shdr[i].sh_type == SHT_NOBITS && shdr[i].sh_flags & SHF_ALLOC)
        {
            elf_add_fill_section(params, shdr[i].sh_addr, shdr[i].sh_size, 0, sec_name);
            printf(user, false, OFF, "create fill segment for %s\n", sec_name);
        }
        else if(shdr[i].sh_type == SHT_SYMTAB)
        {
            // load string table
            char *symstrtab = elf_load_section(&shdr[shdr[i].sh_link], read, printf, user);
            if(!symstrtab)
            {
                printf(user, true, GREY, "cannot load string table for symbol table %s\n", sec_name);
                goto Lerr;
            }
            // load symbol table data
            Elf32_Sym *symdata = elf_load_section(&shdr[i], read, printf, user);
            if(!symdata)
            {
                printf(user, true, GREY, "cannot read elf section %s\n", sec_name);
                free(symstrtab);
                goto Lerr;
            }
            // load symbols (only global ones)
            int nr_symbols = shdr[i].sh_size / sizeof(Elf32_Sym);
            for(int j = shdr[i].sh_info; j < nr_symbols; j++)
            {
                if(ELF32_ST_BIND(symdata[j].st_info) != STB_GLOBAL)
                    continue;
                int type = ELF32_ST_TYPE(symdata[j].st_info);
                if(type != STT_NOTYPE && type != STT_FUNC && type != STT_OBJECT)
                    continue;
                if(symdata[j].st_shndx == SHN_UNDEF)
                    continue;
                struct elf_symbol_t *sym = elf_add_symbol(params);
                sym->name = strdup(&symstrtab[symdata[j].st_name]);
                sym->addr = symdata[j].st_value;
                sym->size = symdata[j].st_size;
                if(symdata[j].st_shndx == SHN_ABS)
                    sym->section = NULL;
                else
                    sym->section = strdup(&strtab[shdr[symdata[j].st_shndx].sh_name]);
                switch(type)
                {
                    case STT_FUNC: sym->type = ESYT_FUNC; break;
                    case STT_OBJECT: sym->type = ESYT_OBJECT; break;
                    case STT_NOTYPE: default: sym->type = ESYT_NOTYPE; break;
                }
                printf(user, false, OFF, "add symbol %s at %#x, type %d, size %d, section %s\n",
                    sym->name, sym->addr, sym->type, sym->size, sym->section);
            }
            free(symdata);
            free(symstrtab);
        }
        else
        {
            printf(user, false, OFF, "filter out %s, type %d\n", sec_name, shdr[i].sh_type);
        }

    }
    free(strtab);
    free(shdr);
    /* run through segments */
    for(int i = 1; i < ehdr.e_phnum; i++)
    {
        uint32_t off = ehdr.e_phoff + i * ehdr.e_phentsize;
        Elf32_Phdr phdr;
        memset(&phdr, 0, sizeof(phdr));
        if(!read(user, off, &phdr, sizeof(phdr)))
            error_printf("error reading elf segment header");
        if(phdr.p_type != PT_LOAD)
            continue;
        struct elf_segment_t *seg = elf_add_segment(params);
        seg->vaddr = phdr.p_vaddr;
        seg->paddr = phdr.p_paddr;
        seg->vsize = phdr.p_memsz;
        seg->psize = phdr.p_filesz;
        printf(user, false, OFF, "create segment [%#x,+%#x[ -> [%#x,+%#x[\n",
            seg->vaddr, seg->vsize, seg->paddr, seg->psize);
    }

    return true;
Lerr:
    free(strtab);
    free(shdr);
    return false;
}

uint32_t elf_translate_virtual_address(struct elf_params_t *params, uint32_t addr)
{
    struct elf_segment_t *seg = params->first_segment;
    while(seg)
    {
        if(seg->vaddr <= addr && addr < seg->vaddr + seg->vsize)
            return addr - seg->vaddr + seg->paddr;
        seg = seg->next;
    }
    return addr;
}

bool elf_is_empty(struct elf_params_t *params)
{
    return params->first_section == NULL;
}

void elf_set_start_addr(struct elf_params_t *params, uint32_t addr)
{
    params->has_start_addr = true;
    params->start_addr = addr;
}

bool elf_get_start_addr(struct elf_params_t *params, uint32_t *addr)
{
    if(params->has_start_addr && addr != NULL)
        *addr = params->start_addr;
    return params->has_start_addr;
}

int elf_get_nr_sections(struct elf_params_t *params)
{
    int nr = 0;
    struct elf_section_t *sec = params->first_section;
    while(sec)
    {
        nr++;
        sec = sec->next;
    }
    return nr;
}

void elf_release(struct elf_params_t *params)
{
    struct elf_section_t *sec = params->first_section;
    while(sec)
    {
        struct elf_section_t *next_sec = sec->next;
        if(sec->type == EST_LOAD)
            free(sec->section);
        free(sec->name);
        free(sec);
        sec = next_sec;
    }
    struct elf_segment_t *seg = params->first_segment;
    while(seg)
    {
        struct elf_segment_t *next_seg = seg->next;
        free(seg);
        seg = next_seg;
    }
    struct elf_symbol_t *sym = params->first_symbol;
    while(sym)
    {
        free(sym->name);
        free(sym->section);
        struct elf_symbol_t *next_sym = sym->next;
        free(sym);
        sym = next_sym;
    }
}

void elf_std_write(void *user, uint32_t addr, const void *buf, size_t count)
{
    FILE *f = user;
    fseek(f, addr, SEEK_SET);
    fwrite(buf, count, 1, f);
}

bool elf_std_read(void *user, uint32_t addr, void *buf, size_t count)
{
    if(fseek((FILE *)user, addr, SEEK_SET) == -1)
        return false;
    return fread(buf, 1, count, (FILE *)user) == count;
}
