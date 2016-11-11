/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Amaury Pouly
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

#include <cstdio>
#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <cstdarg>
#include <string>
#include <fstream>
#include <elf.h>

bool g_verbose = false;
bool g_unsafe = false;

uint8_t *read_file(const std::string& path, size_t& size)
{
    std::ifstream fin(path.c_str(), std::ios::binary);
    if(!fin)
    {
        printf("Error: cannot open '%s'\n", path.c_str());
        return 0;
    }
    fin.seekg(0, std::ios::end);
    size = fin.tellg();
    fin.seekg(0, std::ios::beg);
    uint8_t *buf = new uint8_t[size];
    fin.read((char *)buf, size);
    return buf;
}

bool write_file(const std::string& path, uint8_t *buf, size_t size)
{
    std::ofstream fout(path.c_str(), std::ios::binary);
    if(!fout)
    {
        printf("Error: cannot open '%s'\n", path.c_str());
        return false;
    }
    fout.write((char *)buf, size);
    fout.close();
    return true;
}

/* ELF code */
uint8_t *g_elf_buf;
size_t g_elf_size;
Elf32_Shdr *g_elf_symtab;
Elf32_Shdr *g_elf_symtab_strtab;
Elf32_Shdr *g_elf_shstrtab;

Elf32_Ehdr *elf_ehdr()
{
    return (Elf32_Ehdr *)g_elf_buf;
}

#define NTH_SHDR_OFF(n) \
    (elf_ehdr()->e_shoff + elf_ehdr()->e_shentsize * (n))

Elf32_Shdr *elf_shdr(size_t index)
{
    if(index >= elf_ehdr()->e_shnum)
    {
        printf("Warning: section index is out of bounds\n");
        return nullptr;
    }
    return (Elf32_Shdr *)(g_elf_buf + NTH_SHDR_OFF(index));
}

size_t elf_shnum()
{
    return elf_ehdr()->e_shnum;
}

const char *elf_get_str(Elf32_Shdr *strtab, Elf32_Word index)
{
    /* sanity checks */
    if(strtab->sh_type != SHT_STRTAB)
    {
        printf("Warning: string access to a non-string-table section\n");
        return nullptr;
    }
    if(strtab->sh_offset + strtab->sh_size > g_elf_size)
    {
        printf("Warning: string table section does not fit in the file\n");
        return nullptr;
    }
    if(index >= strtab->sh_size)
    {
        printf("Warning: string access to string table is out of bounds\n");
        return nullptr;
    }
    char *buf = (char *)(g_elf_buf + strtab->sh_offset);
    if(buf[strtab->sh_size - 1] != 0)
    {
        printf("Warning: string table is not zero terminated\n");
        return nullptr;
    }
    return buf + index;
}

const char *elf_get_section_name(size_t index)
{
    Elf32_Shdr *shdr = elf_shdr(index);
    return shdr ? elf_get_str(g_elf_shstrtab, shdr->sh_name) : nullptr;
}

const char *elf_get_symbol_name(Elf32_Sym *sym)
{
    if(ELF32_ST_TYPE(sym->st_info) == STT_SECTION)
        return elf_get_section_name(sym->st_shndx);
    else
        return elf_get_str(g_elf_symtab_strtab, sym->st_name);
}

Elf32_Sym *elf_get_symbol_by_name(const char *name)
{
    Elf32_Sym *sym = (Elf32_Sym *)(g_elf_buf + g_elf_symtab->sh_offset);
    size_t nr_syms = g_elf_symtab->sh_size / sizeof(Elf32_Sym);
    for(size_t i = 0; i < nr_syms; i++)
    {
        const char *s = elf_get_symbol_name(&sym[i]);
        if(s != nullptr && strcmp(name, s) == 0)
            return &sym[i];
    }
    return nullptr;
}

Elf32_Sym *elf_get_symbol_by_address(size_t shndx, Elf32_Word address)
{
    Elf32_Sym *sym = (Elf32_Sym *)(g_elf_buf + g_elf_symtab->sh_offset);
    size_t nr_syms = g_elf_symtab->sh_size / sizeof(Elf32_Sym);
    for(size_t i = 0; i < nr_syms; i++)
    {
        if(sym[i].st_shndx == shndx && sym[i].st_value == address)
            return &sym[i];
    }
    return nullptr;
}

Elf32_Sym *elf_get_symbol_by_index(size_t index)
{
    Elf32_Sym *sym = (Elf32_Sym *)(g_elf_buf + g_elf_symtab->sh_offset);
    size_t nr_syms = g_elf_symtab->sh_size / sizeof(Elf32_Sym);
    if(index >= nr_syms)
        return nullptr;
    return &sym[index];
}

void *elf_get_section_ptr(size_t shndx, Elf32_Word address, size_t size)
{
    Elf32_Shdr *shdr = elf_shdr(shndx);
    if(shdr == nullptr)
        return nullptr;
    if(address + size > shdr->sh_size)
        return nullptr;
    if(shdr->sh_offset + shdr->sh_size > g_elf_size)
        return nullptr;
    return g_elf_buf + shdr->sh_offset + address;
}

/* make sure the string has a final zero in the section, optionally check characters
 * are printable */
const char *elf_get_string_ptr_safe(size_t shndx, Elf32_Word offset, bool want_print = true)
{
    Elf32_Shdr *shdr = elf_shdr(shndx);
    if(shdr == nullptr)
        return nullptr;
    /* address must be in the section */
    if(offset >= shdr->sh_size)
        return nullptr;
    /* determine maximum size */
    size_t max_sz = shdr->sh_size - offset;
    const char *ptr = (const char *)(g_elf_buf + shdr->sh_offset + offset);
    for(size_t i = 0; i < max_sz; i++)
    {
        if(ptr[i] == 0) /* found final 0, everything is fine */
            return ptr;
        if(want_print && !isprint(ptr[i]))
            return nullptr;
    }
    return nullptr;
}

size_t elf_find_reloc_section(size_t shndx)
{
    /* find the relocation section */
    for(size_t i = 0; i < elf_ehdr()->e_shnum; i++)
    {
        Elf32_Shdr *shdr = elf_shdr(i);
        if(shdr->sh_type != SHT_REL && shdr->sh_type != SHT_RELA)
            continue;
        if(shdr->sh_info != shndx)
            continue;
        return i;
    }
    return 0;
}

void *elf_get_symbol_ptr(Elf32_Sym *sym, size_t size)
{
    /* NOTE: also works for STT_SECTION since offset will be 0 */
    return elf_get_section_ptr(sym->st_shndx, sym->st_value, size);
}

/* take the position of a 32-bit address in the section and apply relocation if
 * any */
void *elf_reloc_addr32(size_t shndx, Elf32_Word offset)
{
    /* read value */
    uint32_t *val = (uint32_t *)elf_get_section_ptr(shndx, offset, 4);
    if(val == nullptr)
        return 0; /* invalid */
    /* find reloc section if any */
    size_t relshndx = elf_find_reloc_section(shndx);
    if(relshndx == 0)
        return g_elf_buf + *val; /* no relocation applies */
    Elf32_Shdr *shdr = elf_shdr(relshndx);
    /* find relocation that applies */
    if(shdr->sh_type == SHT_RELA)
    {
        printf("Warning: unsupported RELA relocation type\n");
        return 0;
    }
    Elf32_Rel *rel = (Elf32_Rel *)elf_get_section_ptr(relshndx, 0, shdr->sh_size);
    if(rel == nullptr)
    {
        printf("Warning: invalid relocation section\n");
        return 0;
    }
    size_t sym_count = shdr->sh_size / sizeof(Elf32_Rel);
    for(size_t i = 0; i < sym_count; i++)
    {
        /* for relocatable files, r_offset is the offset in the section */
        if(rel[i].r_offset != offset)
            continue;
        /* find symbol, ignore shdr->sh_link and assume it is g_elf_symtab
         * since the file should have only one symbol table anyway */
        Elf32_Sym *sym = elf_get_symbol_by_index(ELF32_R_SYM(rel[i].r_info));
        /* found it! */
        if(g_verbose)
        {
            printf("[section %zu (%s) offset %#x reloc val %#x type %d sym %d (%s)]\n",
                shndx, elf_get_section_name(shndx), offset, *val,
                ELF32_R_TYPE(rel[i].r_info), ELF32_R_SYM(rel[i].r_info),
                sym ? elf_get_symbol_name(sym) : "<undef>");
        }
        /* apply reloc */
        if(ELF32_R_TYPE(rel[i].r_info) == R_ARM_ABS32)
        {
            if(sym == nullptr)
            {
                printf("Warning: R_ARM_ABS32 reloc with invalid symbol reference\n");
                return 0;
            }
            return *val + (uint8_t *)elf_get_symbol_ptr(sym, 0);
        }
        else
        {
            printf("Warning: unsupported relocation type %d\n", ELF32_R_TYPE(rel[i].r_info));
            return 0;
        }
    }
    /* no reloc applies */
    if(g_verbose)
    {
        printf("[section %zu (%s) offset %#x no reloc found]\n", shndx,
            elf_get_section_name(shndx), offset);
    }
    return g_elf_buf + *val; /* no relocation applies */
}

size_t elf_map_virt_addr(uint32_t address, Elf32_Word& out_off)
{
    /* for relocatable file, this is trivial */
    for(size_t i = 0; i < elf_ehdr()->e_shnum; i++)
    {
        Elf32_Shdr *shdr = elf_shdr(i);
        if(shdr->sh_offset <= address && address < shdr->sh_offset + shdr->sh_size)
        {
            out_off = address - shdr->sh_offset;
            if(g_verbose)
            {
                printf("[map %#x to section %zi (%s) at %#x]\n", address, i,
                    elf_get_section_name(i), out_off);
            }
            return i;
        }
    }
    return 0; /* section 0 is always invalid */
}

size_t elf_map_ptr(void *ptr, Elf32_Word& out_off)
{
    uint32_t addr = (uint32_t)((uint8_t *)ptr - g_elf_buf);
    return elf_map_virt_addr(addr, out_off);
}

/* same as elf_reloc_addr32 but find section automatically from pointer */
void *elf_reloc_addr32_ptr(uint32_t *val)
{
    Elf32_Word off;
    size_t sec = elf_map_ptr((void *)val, off);
    /* if it does not belong to any section, don't do anything */
    if(sec == 0)
    {
        printf("Warning: reloc addr pointer not in any section\n");
        return g_elf_buf + *val;
    }
    return elf_reloc_addr32(sec, off);
}

Elf32_Sym *elf_get_symbol_by_ptr(void *ptr)
{
    Elf32_Word off;
    size_t sec = elf_map_ptr(ptr, off);
    return sec ? elf_get_symbol_by_address(sec, off) : nullptr;
}

/* check if a string is safe */
bool elf_is_str_ptr_safe(const char *str)
{
    Elf32_Word name_off;
    /* find the section it belongs to */
    size_t name_shndx = elf_map_ptr((void *)str, name_off);
    if(name_shndx == 0)
        return false;
    /* check the string fit in the section */
    return elf_get_string_ptr_safe(name_shndx, name_off) != nullptr;
}

bool elf_is_ptr_safe(void *ptr, size_t sz)
{
    Elf32_Word ptr_off;
    /* find the section it belongs to */
    size_t ptr_shndx = elf_map_ptr((void *)ptr, ptr_off);
    if(ptr_shndx == 0)
        return false;
    /* check the string fit in the section */
    return elf_get_section_ptr(ptr_shndx, ptr_off, sz) != nullptr;
}

bool elf_init()
{
    if(g_elf_size < sizeof(Elf32_Ehdr))
    {
        printf("Invalid ELF file: too small\n");
        return false;
    }
    Elf32_Ehdr *ehdr = elf_ehdr();
    if(ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
            ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
            ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
            ehdr->e_ident[EI_MAG3] != ELFMAG3)
    {
        printf("Invalid ELF file: invalid ident\n");
        return false;
    }
    /* we only support relocatable files */
    if(ehdr->e_type != ET_REL)
    {
        printf("Unsupported ELF file: this is not a relocatable file\n");
        return false;
    }
    if(ehdr->e_ident[EI_CLASS] != ELFCLASS32 || ehdr->e_machine != EM_ARM)
    {
        printf("Unsupported ELF file: this is not a 32-bit ARM ELF file\n");
        return false;
    }
    /* go through sections */
    if(ehdr->e_shoff == 0)
    {
        printf("Invalid ELF file: no sections\n");
        return false;
    }
    if(ehdr->e_shentsize < sizeof(Elf32_Shdr))
    {
        printf("Invalid ELF file: section entry size too small\n");
        return false;
    }
    if(NTH_SHDR_OFF(ehdr->e_shnum) > g_elf_size)
    {
        printf("Invalid ELF file: sections header does not fit in the file\n");
        return false;
    }
    for(size_t i = 0; i < ehdr->e_shnum; i++)
    {
        Elf32_Shdr *shdr = (Elf32_Shdr *)(g_elf_buf + NTH_SHDR_OFF(i));
        if(shdr->sh_type == SHT_SYMTAB)
            g_elf_symtab = shdr;
    }
    /* handle symbol table */
    if(g_elf_symtab)
    {
        if(g_elf_symtab->sh_offset + g_elf_symtab->sh_size > g_elf_size)
        {
            printf("Invalid ELF file: symtab does not file in the file\n");
            return false;
        }
        g_elf_symtab_strtab = elf_shdr(g_elf_symtab->sh_link);
        if(g_elf_symtab_strtab == nullptr)
        {
            printf("Invalid ELF file: symtab's strtab is not valid\n");
        }
        if(g_elf_symtab_strtab->sh_type != SHT_STRTAB)
        {
            printf("Invalid ELF file: symtab's strtab is not a string table\n");
            return false;
        }
    }
    /* handle section string table */
    if(ehdr->e_shstrndx != SHN_UNDEF)
    {
        g_elf_shstrtab = elf_shdr(ehdr->e_shstrndx);
        if(g_elf_shstrtab == nullptr)
        {
            printf("Invalid ELF file: section string table is invalid\n");
            return false;
        }
    }

    return true;
}

/* main code */

void usage()
{
    printf("usage: nvptool [options] inputs...\n");
    printf("options:\n");
    printf("  -h/--help        Display help\n");
    printf("  -x/--extract     Extract nvp map from icx_nvp_emmc.ko\n");
    printf("  -o/--output      Set output file\n");
    printf("  -v/--verbose     Enable debug output\n");
    printf("  -u/--unsafe      Perform potentially unsafe operations\n");
    exit(1);
}

struct zone_info_v1_t
{
    uint32_t node;
    uint32_t start;
    uint32_t count;
    uint32_t size;
    uint32_t semaphore[4]; /* a 16-byte structure, useless for us */
    uint32_t name; /* pointer to string */
} __attribute__((packed));

struct zone_info_v2_t
{
    uint32_t node;
    uint32_t start;
    uint32_t count;
    uint32_t size;
    uint32_t semaphore[3]; /* a 12-byte structure, useless for us */
    uint32_t name; /* pointer to string */
} __attribute__((packed));

struct area_info_v1_t
{
    uint32_t type; /* 1 = large, 2 = small */
    uint32_t zoneinfo; /* pointer to zone_info_t[] */
    uint32_t zonecount;
    uint32_t semaphore[4]; /* a 16-byte structure, useless for us */
    uint32_t name; /* pointer to string */
} __attribute__((packed));

struct area_info_v2_t
{
    uint32_t type; /* 1 = large, 2 = small */
    uint32_t zoneinfo; /* pointer to zone_info_t[] */
    uint32_t zonecount;
    uint32_t semaphore[3]; /* a 16-byte structure, useless for us */
    uint32_t name; /* pointer to string */
} __attribute__((packed));

int guess_version(void *area_info_ptr)
{
    /* the "semaphore" part is always filled with zeroes, so simply check if there
     * are 3 or 4 of them */
    area_info_v1_t *ai_v1 = (area_info_v1_t *)area_info_ptr;
    if(ai_v1->semaphore[3] == 0)
        return 1; /* v1: semaphore has 4 fields */
    else
        return 2; /* v2: semaphore has 3 fields */
}

int do_extract(const char *output, int argc, char **argv)
{
    if(argc != 1)
    {
        printf("You need to specify exactly one input file to extract from.\n");
        return 3;
    }
    FILE *fout = NULL;
    if(output)
    {
        fout = fopen(output, "w");
        if(fout == NULL)
        {
            printf("Cannot open output file '%s'\n", output);
            return 4;
        }
    }
    /* read elf file */
    g_elf_buf = read_file(argv[0], g_elf_size);
    if(g_elf_buf == nullptr)
    {
        printf("Cannot open input file '%s'\n", argv[0]);
        return 1;
    }
    if(!elf_init())
    {
        printf("This is not a valid ELF file\n");
        return 1;
    }
    if(g_elf_symtab == nullptr)
    {
        printf("This ELF file does not have a symbol table\n");
        return 1;
    }
    /* look for symbol 'AreaInfo' */
    Elf32_Sym *sym_AreaInfo = elf_get_symbol_by_name("AreaInfo");
    if(sym_AreaInfo == nullptr)
    {
        printf("Cannot find symbol 'AreaInfo'\n");
        return 1;
    }
    printf("AreaInfo:\n");
    if(g_verbose)
    {
        printf("[%u bytes at address %#x in section %u (%s)]\n",
            (unsigned)sym_AreaInfo->st_size, (unsigned)sym_AreaInfo->st_value,
            (unsigned)sym_AreaInfo->st_shndx, elf_get_section_name(sym_AreaInfo->st_shndx));
    }
    /* guess version */
    int ver = guess_version(elf_get_symbol_ptr(sym_AreaInfo, sizeof(area_info_v1_t)));
    if(g_verbose)
        printf("[guessed version: %d]\n", ver);
    size_t sizeof_area_info = (ver == 1) ? sizeof(area_info_v1_t) : sizeof(area_info_v2_t);
    size_t sizeof_zone_info = (ver == 1) ? sizeof(zone_info_v1_t) : sizeof(zone_info_v2_t);
    /* sanity check AreaInfo */
    size_t area_count = sym_AreaInfo->st_size / sizeof_area_info;
    if(!g_unsafe && (sym_AreaInfo->st_size % sizeof_area_info) != 0)
    {
        printf("AreaInfo size (%u) is a not a multiple of area_info_t size (%zu).\n",
            (unsigned)sym_AreaInfo->st_size, sizeof_area_info);
        printf("Use unsafe option to override this check\n");
        return 1;
    }
    area_info_v1_t *AreaInfo_v1 = (area_info_v1_t *)elf_get_symbol_ptr(sym_AreaInfo,
        sym_AreaInfo->st_size);
    area_info_v2_t *AreaInfo_v2 = (area_info_v2_t *)AreaInfo_v1;
    if(AreaInfo_v1 == nullptr)
    {
        printf("Symbol does not point to a valid address\n");
        return 1;
    }
    for(size_t i = 0; i < area_count; i++)
    {
        uint32_t type;
        uint32_t *zoneinfo_ptr;
        uint32_t zonecount;
        uint32_t *name_ptr;

        if(ver == 1)
        {
            type = AreaInfo_v1[i].type;
            zoneinfo_ptr = &AreaInfo_v1[i].zoneinfo;
            zonecount = AreaInfo_v1[i].zonecount;
            name_ptr = &AreaInfo_v1[i].name;
        }
        else
        {
            type = AreaInfo_v2[i].type;
            zoneinfo_ptr = &AreaInfo_v2[i].zoneinfo;
            zonecount = AreaInfo_v2[i].zonecount;
            name_ptr = &AreaInfo_v2[i].name;
        }

        if(g_verbose)
        {
            printf("  [type=%u info=%#x count=%u name=%#x]\n", type, *zoneinfo_ptr,
                zonecount, *name_ptr);
        }
        /* translate name address */
        const char *name = (const char *)elf_reloc_addr32_ptr(name_ptr);
        if(name == nullptr || !elf_is_str_ptr_safe(name))
        {
            printf("  Entry name is not a string\n");
            continue;
        }
        /* skip reserved entries */
        if(*zoneinfo_ptr == 0)
        {
            printf("  %s\n", name);
            continue;
        }
        /* relocate the zoneinfo pointer */
        void *Zone = elf_reloc_addr32_ptr(zoneinfo_ptr);;
        if(Zone == nullptr)
        {
            printf("  %s\n", name);
            printf("  Zone info pointer is not valid\n");
            continue;
        }
        /* in safe mode, make sure the zone info pointer is a symbol */
        Elf32_Sym *zoneinfo_sym = elf_get_symbol_by_ptr((void *)Zone);
        const char *zoneinfo_sym_name = "<no symbol>";
        if(zoneinfo_sym)
            zoneinfo_sym_name = elf_get_symbol_name(zoneinfo_sym);
        printf("  %s (%s)\n", name, zoneinfo_sym_name);
        if(!g_unsafe && !zoneinfo_sym)
        {
            printf("  Zone info pointer does not correspond to any symbol.\n");
            printf("  Use unsafe option to override this check\n");
            continue;
        }
        /* if we have the symbol, make sure the claimed size match */
        if(!g_unsafe && zoneinfo_sym)
        {
            if(zoneinfo_sym->st_size != sizeof_zone_info * zonecount)
            {
                printf("  Zone info symbol size (%u) does not match expected size (%zu)\n",
                    (unsigned)zoneinfo_sym->st_size, sizeof_zone_info * zonecount);
                printf("  Use unsafe option to override this check\n");
                continue;
            }
        }
        /* sanity check */
        if(!elf_is_ptr_safe((void *)Zone, sizeof_zone_info * zonecount))
        {
            printf("  Zone info pointer is not valid\n");
            continue;
        }
        /* read zone */
        zone_info_v1_t *Zone_v1 = (zone_info_v1_t *)Zone;
        zone_info_v2_t *Zone_v2 = (zone_info_v2_t *)Zone;
        for(size_t j = 0; j < zonecount; j++)
        {
            uint32_t node, start, count, size;
            uint32_t *name_ptr;

            if(ver == 1)
            {
                node = Zone_v1[j].node;
                start = Zone_v1[j].start;
                count = Zone_v1[j].count;
                size = Zone_v1[j].size;
                name_ptr = &Zone_v1[j].name;
            }
            else
            {
                node = Zone_v2[j].node;
                start = Zone_v2[j].start;
                count = Zone_v2[j].count;
                size = Zone_v2[j].size;
                name_ptr = &Zone_v2[j].name;
            }

            if(g_verbose)
            {
                printf("    [node=%u start=%#x count=%u size=%u name=%#x]\n",
                    node, start, count, size, *name_ptr);
            }
            /* translate name address */
            const char *name = (const char *)elf_reloc_addr32_ptr(name_ptr);
            if(name == nullptr || !elf_is_str_ptr_safe(name))
            {
                printf("    Entry name is not a string\n");
                continue;
            }
            printf("    %s: node %03u, size %u\n", name, node, size);
            if(fout)
                fprintf(fout, "%u,%u,%s\n", node, size, name);
        }
    }
    if(fout)
        fclose(fout);
    /* success */
    return 0;
}

int main(int argc, char **argv)
{
    const char *output = NULL;
    bool extract = false;

    if(argc <= 1)
        usage();

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"extract", no_argument, 0, 'x'},
            {"output", required_argument, 0, 'o'},
            {"verbose", no_argument, 0, 'v'},
            {"unsafe", no_argument, 0, 'u'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "hxo:vu", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'h':
                usage();
                break;
            case 'o':
                output = optarg;
                break;
            case 'x':
                extract = true;
                break;
            case 'v':
                g_verbose = true;
                break;
            case 'u':
                g_unsafe = true;
                break;
            default:
                abort();
        }
    }

    if(extract)
        return do_extract(output, argc - optind, argv + optind);
    printf("You need to specify an operation. Run nvptool -h for help\n");
    return 1;
}
