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
#include "elf.h"
#include "misc.h"

static char *strdup(const char *str)
{
    int len = strlen(str);
    char *s = malloc(len + 1);
    memcpy(s, str, len + 1);
    return s;
}

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

void elf_init(struct elf_params_t *params)
{
    memset(params, 0, sizeof(struct elf_params_t));
}

extern void *xmalloc(size_t s);

static struct elf_section_t *elf_add_section(struct elf_params_t *params)
{
    struct elf_section_t *sec = xmalloc(sizeof(struct elf_section_t));
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
    char buffer[32];
    if(name == NULL)
    {
        sprintf(buffer, ".text%d", params->unique_index++);
        name = buffer;
    }

    sec->type = EST_LOAD;
    sec->addr = load_addr;
    sec->size = size;
    sec->section = xmalloc(size);
    sec->name = strdup(name);
    memcpy(sec->section, section, size);
}

void elf_add_fill_section(struct elf_params_t *params,
    uint32_t fill_addr, uint32_t size, uint32_t pattern)
{
    char buffer[32];
    sprintf(buffer, ".bss%d", params->unique_index++);

    if(pattern != 0x00)
    {
        printf("oops, non-zero filling, ignore fill section\n");
        return;
    }

    struct elf_section_t *sec = elf_add_section(params);

    sec->type = EST_FILL;
    sec->addr = fill_addr;
    sec->size = size;
    sec->pattern = pattern;
    sec->name = strdup(buffer);
}

void elf_write_file(struct elf_params_t *params, elf_write_fn_t write,
    elf_printf_fn_t printf, void *user)
{
    (void) printf;

    Elf32_Ehdr ehdr;
    uint32_t phnum = 0;
    struct elf_section_t *sec = params->first_section;
    uint32_t offset = 0;
    uint32_t strtbl_size = 1; /* offset 0 is for the NULL name */
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
        sec->name_offset = strtbl_size;
        strtbl_size += strlen(sec->name) + 1;

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

    /* the last name offset gives the size of the section, we need to add a small
     * amount of .shstrtab name */
    uint32_t shstrtab_index = strtbl_size;
    strtbl_size += strlen(".shstrtab") + 1;
    char *strtbl_content = malloc(strtbl_size);
    /* create NULL and shstrtab names */
    strtbl_content[0] = '\0';
    strcpy(&strtbl_content[shstrtab_index], ".shstrtab");

    uint32_t data_offset = ehdr.e_ehsize + ehdr.e_phnum * ehdr.e_phentsize +
        ehdr.e_shnum * ehdr.e_shentsize;

    sec = params->first_section;
    offset = ehdr.e_phoff;
    while(sec)
    {
        sec->offset += data_offset;
        strcpy(&strtbl_content[sec->name_offset], sec->name);

        phdr.p_type = PT_LOAD;
        if(sec->type == EST_LOAD)
            phdr.p_offset = sec->offset;
        else
            phdr.p_offset = 0;
        phdr.p_paddr = sec->addr;
        phdr.p_vaddr = phdr.p_paddr; /* assume identity map ? */
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
        shdr.sh_name = sec->name_offset;
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
        shdr.sh_name = shstrtab_index;
        shdr.sh_type = SHT_STRTAB;
        shdr.sh_flags = 0;
        shdr.sh_addr = 0;
        shdr.sh_offset = strtbl_offset + data_offset;
        shdr.sh_size = strtbl_size;
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

    write(user, strtbl_offset + data_offset, strtbl_content, strtbl_size);
    free(strtbl_content);
}

bool elf_read_file(struct elf_params_t *params, elf_read_fn_t read,
    elf_printf_fn_t printf, void *user)
{
    #define error_printf(...) ({printf(user, true, __VA_ARGS__); return false;})

    /* read header */
    Elf32_Ehdr ehdr;
    if(!read(user, 0, &ehdr, sizeof(ehdr)))
    {
        printf(user, true, "error reading elf header\n");
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

    char *strtab = NULL;
    if(ehdr.e_shstrndx != SHN_UNDEF)
    {
        Elf32_Shdr shstrtab;
        if(read(user, ehdr.e_shoff + ehdr.e_shstrndx * ehdr.e_shentsize,
                &shstrtab, sizeof(shstrtab)))
        {
            strtab = xmalloc(shstrtab.sh_size);
            if(!read(user, shstrtab.sh_offset, strtab, shstrtab.sh_size))
            {
                free(strtab);
                strtab = NULL;
            }
        }
    }
    /* run through sections */
    printf(user, false, "ELF file:\n");
    for(int i = 1; i < ehdr.e_shnum; i++)
    {
        uint32_t off = ehdr.e_shoff + i * ehdr.e_shentsize;
        Elf32_Shdr shdr;
        memset(&shdr, 0, sizeof(shdr));
        if(!read(user, off, &shdr, sizeof(shdr)))
            error_printf("error reading elf section header");

        if(shdr.sh_type == SHT_PROGBITS && shdr.sh_flags & SHF_ALLOC)
        {
            void *data = xmalloc(shdr.sh_size);
            if(!read(user, shdr.sh_offset, data, shdr.sh_size))
                error_printf("error read self section data\n");
            elf_add_load_section(params, shdr.sh_addr, shdr.sh_size, data, NULL);
            free(data);

            if(strtab)
                printf(user, false, "create load segment for %s\n", &strtab[shdr.sh_name]);
        }
        else if(shdr.sh_type == SHT_NOBITS && shdr.sh_flags & SHF_ALLOC)
        {
            elf_add_fill_section(params, shdr.sh_addr, shdr.sh_size, 0);
            if(strtab)
                printf(user, false, "create fill segment for %s\n", &strtab[shdr.sh_name]);
        }
        else
        {
            if(strtab)
                printf(user, false, "filter out %s\n", &strtab[shdr.sh_name], shdr.sh_type);
        }
    }
    free(strtab);
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
        printf(user, false, "create segment [%#x,+%#x[ -> [%#x,+%#x[\n",
            seg->vaddr, seg->vsize, seg->paddr, seg->psize);
    }
    
    return true;
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

void elf_translate_addresses(struct elf_params_t *params)
{
    struct elf_section_t *sec = params->first_section;
    while(sec)
    {
        sec->addr = elf_translate_virtual_address(params, sec->addr);
        sec = sec->next;
    }
    params->start_addr = elf_translate_virtual_address(params, params->start_addr);
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
}
