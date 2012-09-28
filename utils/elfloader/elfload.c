/*-
 * Copyright (c) 2005-2006, Kohsuke Ohtani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * elf.c - ELF file format support
 */
#include <endian.h>

#include <ctype.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "elf.h"
#include "elfload.h"

#define DEBUGF printf
//#define DEBUGF(...) 
/* forward declarations */
static int load_executable(int fd, struct mem_info_t *m);
static int load_relocatable(int fd, struct mem_info_t *m);

#define MAX_ELF_SECTIONS 32
#define COMMON_SYMBOLS_IDX 32

#define ADDRESS_SPACE_SPLIT 0x80000000
#define ADDRESS_MASK        0x7fffffff

static char *sect_addr[MAX_ELF_SECTIONS+1];	/* array of section address */
static char *sect_addr_img[MAX_ELF_SECTIONS+1];   /* array of pointers to mem img */

static Elf32_Sym *symtab = NULL;    /* ptr to symtab */
static Elf32_Word symtab_size = 0;
static Elf32_Word common_size;

static Elf32_Ehdr elf_header;

/* byteoreder shuffling functions */
uint32_t (*elf2h32)(uint32_t) = NULL;
uint32_t (*h2elf32)(uint32_t) = NULL;

uint16_t (*elf2h16)(uint16_t) = NULL;
uint16_t (*h2elf16)(uint16_t) = NULL;

/* wrappers as acutal byte conversion
 * is implemented as macro
 */
static uint32_t __le32toh(uint32_t val)
{
    return le32toh(val);
}

static uint32_t __be32toh(uint32_t val)
{
    return be32toh(val);
}

static uint32_t __htole32(uint32_t val)
{
    return htole32(val);
}

static uint32_t __htobe32(uint32_t val)
{
    return htobe32(val);
}


static uint16_t __le16toh(uint16_t val)
{
    return le16toh(val);
}

static uint16_t __be16toh(uint16_t val)
{
    return be16toh(val);
}

static uint16_t __htole16(uint16_t val)
{
    return htole16(val);
}

static uint16_t __htobe16(uint16_t val)
{
    return htobe16(val);
}

/*
 * Load the program from specified ELF file.
 */
void *elf_open(const char *filename, struct mem_info_t *m)
{
    Elf32_Ehdr *ehdr;

    int fd = open(filename, O_RDONLY);

    if (fd < 0)
    {
        DEBUGF("Could not open file %s", filename);
        goto error;
    }

    /* Read elf header */
    lseek(fd, 0, SEEK_SET);
    size_t bytes_read = read(fd, &elf_header, sizeof(Elf32_Ehdr));

    if (bytes_read != sizeof(Elf32_Ehdr))
    {
        DEBUGF("Error reading elf header");
        goto error_fd;
    }

    /* to simplify porting */
    ehdr = &elf_header;

    if (ehdr->e_ident[EI_DATA] == ELFDATA2LSB)
    {
        /* little endian elf */
        elf2h32 = &__le32toh;
        elf2h16 = &__le16toh;
        h2elf32 = &__htole32;
        h2elf16 = &__htole16;
    }
    else
    {
        /* big endian elf */
        elf2h32 = &__be32toh;
        elf2h16 = &__be16toh;
        h2elf32 = &__htobe32;
        h2elf16 = &__htobe16;
    }

    /*  Check ELF header */
    if ((ehdr->e_ident[EI_MAG0] != ELFMAG0) ||
        (ehdr->e_ident[EI_MAG1] != ELFMAG1) ||
        (ehdr->e_ident[EI_MAG2] != ELFMAG2) ||
        (ehdr->e_ident[EI_MAG3] != ELFMAG3))
    {
        DEBUGF(("Invalid ELF image\n"));
        goto error_fd;
    }

    switch (elf2h16(ehdr->e_type))
    {
        case ET_EXEC:
            if (load_executable(fd, m) != 0)
                goto error_fd;
            break;

        case ET_REL:
            if (load_relocatable(fd, m) != 0)
                goto error_fd;
            break;

        default:
            DEBUGF("Unsupported elf type 0x%x\n", ehdr->e_type);
            goto error_fd;
	}

    close(fd);
    return m->dram;

error_fd:
    close(fd);
error:
    return NULL;
}

static int load_executable(int fd, struct mem_info_t *m)
{
    Elf32_Ehdr *ehdr;
    Elf32_Phdr program_header, *phdr;
    int i;

    ehdr = &elf_header;
    phdr = &program_header;

    /* loop through program headers */
    for (i = 0; i < (int)ehdr->e_phnum; i++)
    {
        lseek(fd, ehdr->e_phoff + (i * sizeof(Elf32_Phdr)), SEEK_SET);
        read(fd, &program_header, sizeof(Elf32_Phdr));

        if (phdr->p_type != PT_LOAD)
            continue;

#if 0
        /* check mem requirements valid for PP */
        if (phdr->p_paddr < (uintptr_t)m->dram ||
            (phdr->p_paddr + phdr->p_memsz) >
                ((uintptr_t)m->dram + m->dram_size))
        {
            if (phdr->p_paddr < (uintptr_t)m->iram ||
               (phdr->p_paddr + phdr->p_memsz) >
                   ((uintptr_t)m->iram + m->iram_size))
            {
                DEBUGF("Not enough memory to execute\n");
                return -1;
            }
        }
#endif
        if (phdr->p_filesz > 0)
        {
            DEBUGF("loading at addr: 0x%0x size: 0x%0x file offset: 0x%0x\n",
                   phdr->p_paddr, phdr->p_filesz, phdr->p_offset);

            //lseek(fd, phdr->p_offset, SEEK_SET);
            //read(fd, (void *)phdr->p_paddr, phdr->p_filesz);
        }

        if (phdr->p_filesz != phdr->p_memsz)
        {
            DEBUGF("clearing mem at addr: 0x%0x size: 0x%0x\n",
                   phdr->p_paddr+phdr->p_filesz, phdr->p_memsz-phdr->p_filesz);

            //memset((void *)(phdr->p_paddr + phdr->p_filesz), 0, (phdr->p_memsz - phdr->p_filesz));
        }
                
    } /* for */

    return 0;
}

static Elf32_Word process_common_symbols(int fd, Elf32_Shdr *shdr)
{
    Elf32_Sym *sym = symtab;
    int num_symbols = elf2h16(shdr->sh_size)/elf2h32(shdr->sh_entsize);
    Elf32_Word common_size = 0;
    Elf32_Word bound, common_offset;
    int i;

    for (i=0; i<num_symbols; i++)
    {
        if (elf2h16(sym->st_shndx) == SHN_COMMON)
        {
            DEBUGF("COMMON sym[%d] st_value=0x%0x st_size=0x%0x\n",
                   i, elf2h32(sym->st_value), elf2h32(sym->st_size));

            bound = elf2h32(sym->st_value) - 1;
            /* align according to constraint stored in st_value */
            common_offset = (common_size + bound) & ~bound;
            common_size = common_offset + elf2h32(sym->st_size);

            /* patch symbol entry for further use */
            sym->st_shndx = h2elf16(COMMON_SYMBOLS_IDX);
            sym->st_value = h2elf32(common_offset);
        }

        sym++;
    }

    /* Word align */
    return ((common_size + 3) & ~3);
}

/* Updates symbol value *symval associated with relocation entry *rela
 * returns 0 on success, -1 on error
 */
static int get_symbol_value_rela(int fd, Elf32_Rela *rela, Elf32_Addr *symval)
{
    Elf32_Sym *sym = (symtab + ELF32_R_SYM(elf2h32(rela->r_info)));

    DEBUGF("symbol[%d]: st_name=0x%0x st_value=0x%0x "
           "st_size=0x%0x st_info=0x%0x st_shndx=0x%0x\n",
           ELF32_R_SYM(elf2h32(rela->r_info)), elf2h32(sym->st_name), elf2h32(sym->st_value),
           elf2h32(sym->st_size), sym->st_info, elf2h16(sym->st_shndx));

    if (elf2h16(sym->st_shndx) != STN_UNDEF)
    {
        *symval = (Elf32_Addr)sect_addr[elf2h16(sym->st_shndx)] + elf2h32(sym->st_value);
    }
    else
    {
        return -1;
    }

    return 0;
}

/* Updates symbol value *symval associated with relocation entry *rel
 * returns 0 on success, -1 on error
 */
static int get_symbol_value_rel(int fd, Elf32_Rel *rel, Elf32_Addr *symval)
{
    Elf32_Sym *sym = (symtab + ELF32_R_SYM(elf2h32(rel->r_info)));

    DEBUGF("symbol[%d]: st_name=0x%0x st_value=0x%0x "
           "st_size=0x%0x st_info=0x%0x st_shndx=0x%0x\n",
           ELF32_R_SYM(elf2h32(rel->r_info)), elf2h32(sym->st_name), elf2h32(sym->st_value),
           elf2h32(sym->st_size), sym->st_info, elf2h16(sym->st_shndx));

    if (elf2h16(sym->st_shndx) != STN_UNDEF)
    {
        *symval = (Elf32_Addr)sect_addr[elf2h16(sym->st_shndx)] + elf2h32(sym->st_value);
    }
    else
    {
        return -1;
    }

    return 0;
}

struct r_mips_hi16 {
    Elf_Addr *where;
    Elf_Addr symval;
};

#define SIGN_EXTEND16(x) ((x & 0xffff) | \
                         ((x & 0x8000) ? 0xffff0000 : 0x00000000))

#define HI16_TAB_ENTRIES 32
static int relocate_rel_mips(int fd, Elf32_Rel *rel, char *target_sect,
                             char *target_sect_img, struct mem_info_t *m)
{
    static struct r_mips_hi16 r_mips_hi16_tab[HI16_TAB_ENTRIES];
    static int next = 0;

    struct r_mips_hi16 *h;

    Elf32_Word val, instr, addlo;
    Elf32_Addr symval;

    Elf32_Addr where_ptr = ((Elf32_Addr)target_sect + elf2h32(rel->r_offset));
    Elf32_Addr *where = (Elf32_Addr *)(target_sect_img + elf2h32(rel->r_offset));

    switch (ELF32_R_TYPE(elf2h32(rel->r_info)))
    {
        case R_MIPS_NONE:
            break;

        case R_MIPS_32:
            if (get_symbol_value_rel(fd, rel, &symval))
                return -1;

            *where = h2elf32(elf2h32(*where) + symval);
            break;

        case R_MIPS_26:
            if (get_symbol_value_rel(fd, rel, &symval))
                return -1;

            /* Only word aligned addresses can be expressed */
            if (symval % 4)
                return -2;

            /* Check for relocation overflow */
            if ((symval & 0xf0000000) != ((where_ptr + 4) & 0xf0000000))
                return -2;

            *where = h2elf32((elf2h32(*where) & 0xfc000000) | ((symval>>2) & 0x03ffffff));
            break;

        case R_MIPS_HI16:
            /* To perform this relocation we need to know the carry from
             * R_MIPS_LO16, so save the information and perform actual
             * work in R_MIPS_LO16
             */
            if (get_symbol_value_rel(fd, rel, &symval))
                return -1;

            /* We cannot track that much R_MIPS_HI16 entries */
            if (next >= HI16_TAB_ENTRIES)
                return -2;

            h = &r_mips_hi16_tab[next++];
            h->where = where;
            h->symval = symval;
            break;

        case R_MIPS_LO16:
            if (get_symbol_value_rel(fd, rel, &symval))
                return -1;

            /* Sign extend the LO16 addend */
            addlo = SIGN_EXTEND16(elf2h32(*where) & 0xffff);

            while (next--)
            {
                /* List of previous unresolved HI16 relocations */
                h = &r_mips_hi16_tab[next];

                /* it should point to the same symbol */
                if (symval != h->symval)
                    return -2;

                /* Perform associated HI16 relocation */
                instr = elf2h32(*h->where);
                val = ((instr & 0xffff) << 16) + addlo;
                val += symval;

                /* Account for sign extension in low bits */
                if (val & 0x8000)
                    val = (val >> 16) + 1;
                else
                    val = (val >> 16);
                
                instr = (instr & 0xffff0000) | (val & 0x0000ffff);
                *h->where = h2elf32(instr);
            }

            /* perform LO16 relocation */
            val = symval + addlo;
            *where = h2elf32(elf2h32(*where) & 0xffff0000) | (val & 0xffff);
            break;

        default:
            DEBUGF("Unknown relocation type: %d\n", ELF32_R_TYPE(elf2h32(rel->r_info)));
            return -3;
    }

    return 0;
}

static int relocate_rela_sh(int fd, Elf32_Rela *rela, char *target_sect,
                           char *target_sect_img, struct mem_info_t *m)
{
   (void)m;
   (void)target_sect;

    Elf32_Addr symval;
    Elf32_Addr *where = (Elf32_Addr *)(target_sect_img + elf2h32(rela->r_offset));

    switch (ELF32_R_TYPE(elf2h32(rela->r_info)))
    {
        case R_SH_NONE:
            break;

        case R_SH_DIR32:
            if (get_symbol_value_rela(fd, rela, &symval))
                return -1;

            *where = h2elf32(symval + elf2h32(rela->r_addend));
            break;

        default:
            DEBUGF("Unknown relocation type: %d\n", ELF32_R_TYPE(elf2h32(rela->r_info)));
            return -3;
    }

    return 0;
}

static int relocate_rela_m68k(int fd, Elf32_Rela *rela, char *target_sect,
                             char *target_sect_img, struct mem_info_t *m)
{
    (void)m;

    Elf32_Addr symval;
    Elf32_Addr *where = (Elf32_Addr *)(target_sect_img + elf2h32(rela->r_offset));
    switch (ELF32_R_TYPE(elf2h32(rela->r_info)))
    {
        case R_68K_NONE:
            break;

        case R_68K_32:
            if (get_symbol_value_rela(fd, rela, &symval))
                return -1;
            *where = h2elf32(symval + (Elf32_Sword)elf2h32(rela->r_addend));
            break;

        default:
            DEBUGF("Unknown relocation type: %d\n", ELF32_R_TYPE(elf2h32(rela->r_info)));
            return -3;
    }

    return 0;
}

#define SIGN_EXTEND24(x) ((x & 0x00ffffff) | \
                          ((x & 0x800000) ? 0xff000000 : 0x00000000))

#define VENEER_SIZE 8 /* maybe this needs to be variable */
#define VENEER_SLOTS 16

enum mem_segment_t {
    DRAM_SEGMENT,
    IRAM_SEGMENT,
    UNKNOWN_SEGMENT
};

struct veneer_t
{
    Elf32_Word *where;
    Elf32_Word *where_img;
    Elf32_Addr target;
    enum mem_segment_t mem_segment;
};

static struct veneer_t veneer_tab[VENEER_SLOTS];
static int veneer_idx = 0;

static enum mem_segment_t get_mem_segment(Elf32_Addr addr, struct mem_info_t *m)
{
    if ((addr >= (Elf32_Addr)m->iram) && (addr < (Elf32_Addr)((char *)m->iram + m->iram_size)))
        return IRAM_SEGMENT;
    else if ((addr >= (Elf32_Addr)m->dram) && (addr < (Elf32_Addr)((char *)m->dram + m->dram_size)))
        return DRAM_SEGMENT;
    else
        return UNKNOWN_SEGMENT;
}

static int insert_veneer(Elf32_Addr source, Elf32_Addr target, Elf32_Addr *veneer_addr,
                         struct mem_info_t *m)
{
    Elf32_Addr align;
    Elf32_Word *where, *where_img;
    int i;

    /* Check if we have veneer for this target address already present */
    if (veneer_idx)
        for (i=0; i < veneer_idx; i++)
        {
            if (veneer_tab[i].target == target)
            {
                *veneer_addr = (Elf32_Addr)veneer_tab[i].where;
                return 0;
            }
        }

    /* Code must be word aligned which may not be the case
     * as we insert veneers *AFTER* .bss and COMMON
     * so enforce proper alignment here
     */
    if (get_mem_segment(source, m) == DRAM_SEGMENT)
    {
        align = (m->dram_runtime_used + 3) & ~3;

        if (align + VENEER_SIZE > m->dram_size)
        {
            DEBUGF("Not enough dram mem to insert veneer\n");
            return -1;
        }

        where = (Elf32_Word *)((char *)m->dram + align);
        where_img = (Elf32_Word *)((char *)m->dram_img + align);

        /* Account for inserted veneer */
        m->dram_runtime_used = align + VENEER_SIZE;
    }
    else
    {
        align = (m->iram_runtime_used + 3) & ~3;

        if (align + VENEER_SIZE > m->iram_size)
        {
            DEBUGF("Not enough iram mem to insert veneer\n");
            return -1;
        }

        where = (Elf32_Word *)((char *)m->iram + align);
        where_img = (Elf32_Word *)((char *)m->iram_img + align);

        /* Account for inserted veneer */
        m->iram_runtime_used = align + VENEER_SIZE;
    }

    if (veneer_idx < VENEER_SLOTS)
    {
        veneer_tab[veneer_idx].where = where;
        veneer_tab[veneer_idx].where_img = where_img;
        veneer_tab[veneer_idx].target = target;
        *veneer_addr = (Elf32_Addr)veneer_tab[veneer_idx].where;
        veneer_idx++;

        DEBUGF("Inserting veneer at addr=0x%0x target addr=0x%0x\n",
              (unsigned int)where, (unsigned int)target);
    }
    else
    {
        DEBUGF("Veneer slots exchause\n");
        return -1;
    }

    return 0;
}

static void apply_veneers(void)
{
    int i;
    struct veneer_t *v;

    DEBUGF("Writing %d veneer(s) in dram\n", veneer_idx);

    for (i=0; i < veneer_idx; i++)
    {
        v = &veneer_tab[i];
        *v->where_img = h2elf32(0xe51ff004);    /* ldr pc, [pc, #-4] */
        *(v->where_img+1) = h2elf32(v->target); /* .word TARGET_ADDRESS */
    }
}

static int relocate_rel_arm(int fd, Elf32_Rel *rel, char *target_sect,
                            char *target_sect_img, struct mem_info_t *m)
{
    Elf32_Addr symval;
    Elf32_Sword addend, offset;

    Elf32_Addr where_ptr = ((Elf32_Addr)target_sect + elf2h32(rel->r_offset));
    Elf32_Addr *where = (Elf32_Addr *)(target_sect_img + elf2h32(rel->r_offset));
    switch (ELF32_R_TYPE(elf2h32(rel->r_info)))
    {
        case R_ARM_NONE:
        case R_ARM_V4BX:
            break;

        case R_ARM_ABS32:
            if (get_symbol_value_rel(fd, rel, &symval))
                return -1;

            *where = h2elf32(elf2h32(*where) + symval);
            break;

        case R_ARM_PLT32:
        case R_ARM_CALL:
        case R_ARM_JUMP24:
            if (get_symbol_value_rel(fd, rel, &symval))
                return -1;

            addend = SIGN_EXTEND24(elf2h32(*where) & 0x00ffffff);

            offset = (symval - (Elf32_Addr)where_ptr + (addend << 2));

            /* Only word align addresses are permited */
            if (offset % 4)
                return -2;
            /* Check for overflow */
            if (offset >= 0x02000000 ||
                offset <= -0x02000000)
            {
                if (insert_veneer((Elf32_Addr)where_ptr, symval, &symval, m) != 0)
                    return -2;

                offset = (symval - (Elf32_Addr)where_ptr + (addend << 2));
                DEBUGF("patching call at addr=0x%0x, "
                       "symval=0x%0x offset=0x%0x\n",
                       (unsigned int)where_ptr, symval, offset);
            }

            *where = h2elf32(elf2h32(*where) & 0xff000000) | ((offset>>2) & 0x00ffffff);
            break;

        default:
            DEBUGF("Unknown relocation type: %d\n", ELF32_R_TYPE(elf2h32(rel->r_info)));
            return -3;
    }

    return 0;
}

/* Wrapper to call arch specific functions */
static int relocate_rela(int fd, Elf32_Rela *rela, char *target_sect,
                         char *target_sect_img, struct mem_info_t *m)
{
    Elf32_Addr symval;
    Elf32_Addr where_ptr = ((Elf32_Addr)target_sect + elf2h32(rela->r_offset));
    Elf32_Addr where_val = *(Elf32_Addr *)(target_sect_img + elf2h32(rela->r_offset));

    if (get_symbol_value_rela(fd, rela, &symval) != 0)
        DEBUGF("Relocation: ELF32_R_TYPE: %d, ELF32_R_SYM: 0x%x\n",
               ELF32_R_TYPE(elf2h32(rela->r_info)), ELF32_R_SYM(elf2h32(rela->r_info)));
    else
        DEBUGF("Process relocation: where: 0x%0x, *where: 0x%0x, ELF32_R_TYPE: %d,"
               " ELF32_R_SYM: 0x%x, symval: 0x%x addend: 0x%0x\n",
               where_ptr, where_val, ELF32_R_TYPE(elf2h32(rela->r_info)),
               ELF32_R_SYM(elf2h32(rela->r_info)), symval, elf2h32(rela->r_addend));

    switch (elf2h16(elf_header.e_machine))
    {
        case EM_MIPS:
                return -1;
            break;

        case EM_68K:
            if (relocate_rela_m68k(fd, rela, target_sect, target_sect_img, m) != 0)
                return -1;
            break;

        case EM_SH:
            if (relocate_rela_sh(fd, rela, target_sect, target_sect_img, m) != 0)
            break;

        case EM_ARM:
                return -1;
            break;

        default:
            DEBUGF("Unsupported architecture\n");
            return -1;
    }

    return 0;
}

/* Wrapper to call arch specific functions */
static int relocate_rel(int fd, Elf32_Rel *rel, char *target_sect,
                        char *target_sect_img, struct mem_info_t *m)
{
    Elf32_Addr symval;
    Elf32_Addr where_ptr = ((Elf32_Addr)target_sect + elf2h32(rel->r_offset));
    Elf32_Addr where_val = *(Elf32_Addr *)(target_sect_img + elf2h32(rel->r_offset));

    if (get_symbol_value_rel(fd, rel, &symval) != 0)
        DEBUGF("Relocation: ELF32_R_TYPE: %d, ELF32_R_SYM: 0x%x\n",
               ELF32_R_TYPE(elf2h32(rel->r_info)), ELF32_R_SYM(elf2h32(rel->r_info)));
    else
        DEBUGF("Process relocation: where: 0x%0x, *where: 0x%0x, ELF32_R_TYPE: %d,"
               " ELF32_R_SYM: 0x%x, symval: 0x%x\n",
               where_ptr, where_val, ELF32_R_TYPE(elf2h32(rel->r_info)),
               ELF32_R_SYM(elf2h32(rel->r_info)), symval);

    switch (elf2h16(elf_header.e_machine))
    {
        case EM_MIPS:
            if (relocate_rel_mips(fd, rel, target_sect, target_sect_img, m) != 0)
                return -1;
            break;

        case EM_68K:
            return -1;
            break;

        case EM_SH:
            return -1;
            break;

        case EM_ARM:
            if (relocate_rel_arm(fd, rel, target_sect, target_sect_img, m) != 0)
                return -1;
            break;

        default:
            DEBUGF("Unsupported architecture\n");
            return -1;
    }

    return 0;
}

static int relocate_section_rela(int fd, Elf32_Shdr *shdr, struct mem_info_t *m)
{
    Elf32_Rela rela_entry, *rela;
    int i;
    int nr_reloc = (int)(elf2h32(shdr->sh_size) / elf2h32(shdr->sh_entsize));
    char *target_sect = sect_addr[elf2h32(shdr->sh_info)];
    char *target_sect_img = sect_addr_img[elf2h32(shdr->sh_info)];

    rela = &rela_entry;

    DEBUGF("section[%d]: number of relocations: %d\n", elf2h32(shdr->sh_info), nr_reloc);

    /* Loop through all relocation entries for the section */
    for (i = 0; i < nr_reloc; i++)
    {
        DEBUGF("\nrel[%d] -> ", i);

        /* Read Elf32_Rela entry */
        lseek(fd, elf2h32(shdr->sh_offset) + i*sizeof(Elf32_Rela), SEEK_SET);
        read(fd, &rela_entry, sizeof(Elf32_Rela));

        /* Call architecture specific function */
        if (relocate_rela(fd, rela, target_sect, target_sect_img, m) != 0)
            return -1;
    }

    return 0;
}

static int relocate_section_rel(int fd, Elf32_Shdr *shdr, struct mem_info_t *m)
{
    Elf32_Rel rel_entry, *rel;
    int i;
    int nr_reloc = (int)(elf2h32(shdr->sh_size) / elf2h32(shdr->sh_entsize));
    char *target_sect = sect_addr[elf2h32(shdr->sh_info)];
    char *target_sect_img = sect_addr_img[elf2h32(shdr->sh_info)];

    rel = &rel_entry;

    DEBUGF("section[%d]: number of relocations: %d\n", elf2h32(shdr->sh_info), nr_reloc);

    /* Loop through all relocation entries for the section */
    for (i = 0; i < nr_reloc; i++)
    {
        DEBUGF("\nrel[%d] -> ", i);

        /* Read Elf32_Rel entry */
        lseek(fd, elf2h32(shdr->sh_offset) + i*sizeof(Elf32_Rel), SEEK_SET);
        read(fd, &rel_entry, sizeof(Elf32_Rel));

        /* Call architecture specific function */
        if (relocate_rel(fd, rel, target_sect, target_sect_img, m) != 0)
            return -1;
    }

    return 0;
}

static int relocate_section(int fd, Elf32_Shdr *shdr, struct mem_info_t *m)
{
    int error;

    if (shdr->sh_entsize == 0)
        return 0;

    switch (elf2h32(shdr->sh_type))
    {
        case SHT_REL:
            error = relocate_section_rel(fd, shdr, m);
            break;

        case SHT_RELA:
            error = relocate_section_rela(fd, shdr, m);
            break;

        default:
            return -1;
    }

    return error;
}

static paddr_t get_address(Elf32_Shdr *shdr, struct mem_info_t *m)
{
    if (elf2h32(shdr->sh_addr) < ADDRESS_SPACE_SPLIT)
        return (paddr_t)m->dram + (elf2h32(shdr->sh_addr));
    else
        return (paddr_t)m->iram + (elf2h32(shdr->sh_addr) & ADDRESS_MASK);
}

static paddr_t get_address_img(Elf32_Shdr *shdr, struct mem_info_t *m)
{
    if (elf2h32(shdr->sh_addr) < ADDRESS_SPACE_SPLIT)
        return (paddr_t)m->dram_img + (elf2h32(shdr->sh_addr) & ~m->uncache_base);
    else
        return (paddr_t)m->iram_img + (elf2h32(shdr->sh_addr) & ADDRESS_MASK);
}

static int check_section_mem_req(Elf32_Shdr *shdr, struct mem_info_t *m, int load)
{
    if (elf2h32(shdr->sh_addr) < ADDRESS_SPACE_SPLIT)
    {
        m->dram_runtime_used += elf2h32(shdr->sh_size);

        /* Check mem requirements */
        if (m->dram_runtime_used > m->dram_size)
        {
            DEBUGF("Not enough dram memory to load section:"
                   " need=0x%0x aval=0x%0x\n",
                   m->dram_runtime_used, m->dram_size);

            return -1;
        }

        if (load)
            m->dram_load_used += elf2h32(shdr->sh_size);
    }
    else
    {
        m->iram_runtime_used += elf2h32(shdr->sh_size);

        /* Check mem requirements */
        if (m->iram_runtime_used > m->iram_size)
        {
            DEBUGF("Not enough iram memory to load section:"
                   " need=0x%0x aval=0x%0x\n",
                   m->iram_runtime_used, m->iram_size);

            return -1;
        }
    }

    return 0;
}

static int load_relocatable(int fd, struct mem_info_t *m)
{
    Elf32_Ehdr *ehdr;
    Elf32_Shdr section_header, *shdr;
    int i;

    ehdr = &elf_header;
    shdr = &section_header;

    m->dram_runtime_used = 0;
    m->dram_load_used = 0;
    m->iram_runtime_used = 0;

    if ((int)elf2h16(ehdr->e_shnum) > MAX_ELF_SECTIONS)
    {
        DEBUGF("Can't support more then %d sections\n", MAX_ELF_SECTIONS);
        return -1;
    }

    /* Loop through all sections (section 0 is NULL) */
    for (i = 1; i < (int)elf2h16(ehdr->e_shnum); i++)
    {
        /* Get section header */
        lseek(fd, elf2h32(ehdr->e_shoff) + (i * sizeof(Elf32_Shdr)), SEEK_SET);
        read(fd, &section_header, sizeof(Elf32_Shdr));
        
        sect_addr[i] = (char *)get_address(shdr, m);
        sect_addr_img[i] = (char *)get_address_img(shdr, m);

        DEBUGF("\nsection[%d]: sh_type=0x%0x, sh_flags=0x%0x "
               "sh_addr=0x%0x sh_offset=0x%0x sh_size=0x%0x\n",
               i, elf2h32(shdr->sh_type), elf2h32(shdr->sh_flags), elf2h32(shdr->sh_addr),
               elf2h32(shdr->sh_offset), elf2h32(shdr->sh_size));

        DEBUGF("section[%d]: sect_addr=0x%0x, sect_addr_img=0x%0x\n",
               i, (uintptr_t)sect_addr[i], (uintptr_t)sect_addr_img[i]);

        if (elf2h32(shdr->sh_flags) & SHF_ALLOC)
        {
            if (elf2h32(shdr->sh_type) == SHT_PROGBITS)
            {
                if (elf2h32(shdr->sh_size))
                {
                    if (check_section_mem_req(shdr, m, 1) != 0)
                        return -1;

                    lseek(fd, elf2h32(shdr->sh_offset), SEEK_SET);
                    read(fd, (void *)sect_addr_img[i], (size_t)elf2h32(shdr->sh_size));

                    DEBUGF("loading section at addr=0x%0x size=0x%0x\n",
                           (unsigned int)sect_addr[i], (int)elf2h32(shdr->sh_size));
                } /* if (shdr->sh_size) */

            } /* if (shdr->sh_type == SHT_PROGBITS) */
            else if (elf2h32(shdr->sh_type) == SHT_NOBITS)
            {
                /* .bss */
                if (elf2h32(shdr->sh_size))
                {
                    if (check_section_mem_req(shdr, m, 0) != 0)
                        return -1;

                } /* if (shdr->size) */

            }
        } /* if (shdr->sh_flags & SHF_ALLOC) */
        else
        {
            if (elf2h32(shdr->sh_type) == SHT_SYMTAB)
            {
                /* load symtab into memory */
                if (m->dram_size > m->dram_load_used + elf2h32(shdr->sh_size))
                {
                    symtab = (Elf32_Sym *)((char *)m->dram_img + m->dram_size - elf2h32(shdr->sh_size));
                    symtab_size = elf2h32(shdr->sh_size);
                    DEBUGF("Loading symtab at addr=0x%0x (0x%0x) size=0x%0x\n", 
                           (uintptr_t)((char *)m->dram + m->dram_size - symtab_size),
                           (uintptr_t)symtab, symtab_size);

                    lseek(fd, elf2h32(shdr->sh_offset), SEEK_SET);
                    if (read(fd, (void *)symtab, (size_t)symtab_size) != symtab_size)
                    {
                       DEBUGF("Error loading symtab\n");
                       return -1;
                    }
                }
                else
                {
                    DEBUGF("Not enough dram mem to load symtab need=0x%0x aval=0x%0x\n",
                           m->dram_load_used + elf2h32(shdr->sh_size), m->dram_size);
                    return -1;
                }
 
                common_size = process_common_symbols(fd, shdr);
                DEBUGF("COMMON size=0x%x\n", common_size);

                if (common_size + m->dram_runtime_used <= m->dram_size)
                {
                    /* plug at the end */
                    sect_addr[COMMON_SYMBOLS_IDX] = (char *)m->dram + m->dram_runtime_used;
                    sect_addr_img[COMMON_SYMBOLS_IDX] = (char *)m->dram_img + m->dram_runtime_used;

                    m->dram_runtime_used += common_size;
                }
                else
                {
                    DEBUGF("Not enough dram memory for common symbols need=0x%0x aval=0x%0x",
                           m->dram_runtime_used + common_size, m->dram_size);
                    return -1;
                }
            }
        } /* shdr->sh_flags */	
    } /* for */

    if (symtab == NULL)
    {
        DEBUGF("Can't find symtab.\n");
        return -1;
    }

    DEBUGF("\ndram run size: 0x%x/0x%x, iram run size: 0x%x/0x%x\n",
           m->dram_runtime_used, m->dram_size,  m->iram_runtime_used, m->iram_size);

    DEBUGF("\nProcessing relocations\n");
    /* Process relocation */
    for (i = 1; i < (int)elf2h16(ehdr->e_shnum); i++)
    {
        lseek(fd, elf2h32(ehdr->e_shoff) + (i * sizeof(Elf32_Shdr)), SEEK_SET);
        read(fd, &section_header, sizeof(Elf32_Shdr));
        
        if (elf2h32(shdr->sh_type) == SHT_REL || elf2h32(shdr->sh_type) == SHT_RELA)
        {
            DEBUGF("\nFound relocation info in section[%d]\n", i);

            if (relocate_section(fd, shdr, m) != 0)
            {
                DEBUGF("Relocation error\n");
                return -1;
            }
        }
    }

    /* clearing .bss and COMMON */
    for (i = 1; i < (int)elf2h16(ehdr->e_shnum); i++)
    {
        lseek(fd, elf2h32(ehdr->e_shoff) + (i * sizeof(Elf32_Shdr)), SEEK_SET);
        read(fd, &section_header, sizeof(Elf32_Shdr));

        if ((elf2h32(shdr->sh_flags) & SHF_ALLOC) && (elf2h32(shdr->sh_type) == SHT_NOBITS))
        {
            DEBUGF("Clearing mem at addr=0x%0x size=0x%0x\n",
                   (unsigned int)sect_addr[i], (size_t)elf2h32(shdr->sh_size));
            memset((void *)sect_addr_img[i], 0, (size_t)elf2h32(shdr->sh_size));
        }
    }


    if (common_size)
    {
        DEBUGF("Clearing common symbols area at addr=0x%0x size=0x%0x\n",
               (unsigned int)sect_addr[COMMON_SYMBOLS_IDX],
               (unsigned int)common_size);

        memset((void *)sect_addr_img[COMMON_SYMBOLS_IDX], 0, common_size);
    }

    if (elf2h16(elf_header.e_machine) == EM_ARM)
        apply_veneers();

    return 0;
}
