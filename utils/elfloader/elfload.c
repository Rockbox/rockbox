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

    /* will fix later */
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        DEBUGF("Big endian ELFs not supported yet\n");
        goto error_fd;
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

    switch (ehdr->e_type)
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

#if 0
static int
relocate_section_rela(Elf32_Sym *sym_table, Elf32_Rela *rela,
		      char *target_sect, int nr_reloc, char *strtab)
{
	Elf32_Sym *sym;
	Elf32_Addr sym_val;
	int i;

	for (i = 0; i < nr_reloc; i++) {
		sym = &sym_table[ELF32_R_SYM(rela->r_info)];
		ELFDBG(("%s\n", strtab + sym->st_name));
		if (sym->st_shndx != STN_UNDEF) {
			sym_val = (Elf32_Addr)sect_addr[sym->st_shndx]
				+ sym->st_value;
			if (relocate_rela(rela, sym_val, target_sect) != 0)
				return -1;
		} else if (ELF32_ST_BIND(sym->st_info) != STB_WEAK) {
			DPRINTF(("Undefined symbol for rela[%x] sym=%lx\n",
				 i, (long)sym));
			return -1;
		} else {
			DPRINTF(("Undefined weak symbol for rela[%x]\n", i));
		}
		rela++;
	}
	return 0;
}
#endif

static Elf32_Word process_common_symbols(int fd, Elf32_Shdr *shdr)
{
    Elf32_Sym *sym = symtab;
    int num_symbols = shdr->sh_size/shdr->sh_entsize;
    Elf32_Word common_size = 0;
    Elf32_Word bound, common_offset;
    int i;

    for (i=0; i<num_symbols; i++)
    {
        if (sym->st_shndx == SHN_COMMON)
        {
            DEBUGF("COMMON sym[%d] st_value=0x%0x st_size=0x%0x\n",
                   i, sym->st_value, sym->st_size);

            bound = sym->st_value - 1;
            /* align according to constraint stored in st_value */
            common_offset = (common_size + bound) & ~bound;
            common_size = common_offset + sym->st_size;

            /* patch symbol entry for further use */
            sym->st_shndx = COMMON_SYMBOLS_IDX;
            sym->st_value = common_offset;
        }

        sym++;
    }

    return common_size;
}

/* Updates symbol value *symval associated with relocation entry *rel
 * returns 0 on success, -1 on error
 */
static int get_symbol_value(int fd, Elf32_Rel *rel, Elf32_Addr *symval)
{
    Elf32_Sym *sym = (symtab + ELF32_R_SYM(rel->r_info));

    DEBUGF("symbol[%d]: st_name=0x%0x st_value=0x%0x "
           "st_size=0x%0x st_info=0x%0x st_shndx=0x%0x\n",
           ELF32_R_SYM(rel->r_info), sym->st_name, sym->st_value,
           sym->st_size, sym->st_info, sym->st_shndx);

    if (sym->st_shndx != STN_UNDEF)
    {
        *symval = (Elf32_Addr)sect_addr[sym->st_shndx] + sym->st_value;
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

    Elf32_Addr where_ptr = ((Elf32_Addr)target_sect + rel->r_offset);
    Elf32_Addr *where = (Elf32_Addr *)(target_sect_img + rel->r_offset);

    switch (ELF32_R_TYPE(rel->r_info))
    {
        case R_MIPS_NONE:
            break;

        case R_MIPS_32:
            if (get_symbol_value(fd, rel, &symval))
                return -1;

            *where += symval;
            break;

        case R_MIPS_26:
            if (get_symbol_value(fd, rel, &symval))
                return -1;

            /* Only word aligned addresses can be expressed */
            if (symval % 4)
                return -2;

            /* Check for relocation overflow */
            if ((symval & 0xf0000000) != ((where_ptr + 4) & 0xf0000000))
                return -2;

            *where = ((*where & 0xfc000000) | ((symval>>2) & 0x03ffffff));
            break;

        case R_MIPS_HI16:
            /* To perform this relocation we need to know the carry from
             * R_MIPS_LO16, so save the information and perform actual
             * work in R_MIPS_LO16
             */
            if (get_symbol_value(fd, rel, &symval))
                return -1;

            /* We cannot track that much R_MIPS_HI16 entries */
            if (next >= HI16_TAB_ENTRIES)
                return -2;

            h = &r_mips_hi16_tab[next++];
            h->where = where;
            h->symval = symval;
            break;

        case R_MIPS_LO16:
            if (get_symbol_value(fd, rel, &symval))
                return -1;

            /* Sign extend the LO16 addend */
            addlo = SIGN_EXTEND16(*where & 0xffff);

            while (next--)
            {
                /* List of previous unresolved HI16 relocations */
                h = &r_mips_hi16_tab[next];

                /* it should point to the same symbol */
                if (symval != h->symval)
                    return -2;

                /* Perform associated HI16 relocation */
                instr = *h->where;
                val = ((instr & 0xffff) << 16) + addlo;
                val += symval;

                /* Account for sign extension in low bits */
                if (val & 0x8000)
                    val = (val >> 16) + 1;
                else
                    val = (val >> 16);
                
                instr = (instr & 0xffff0000) | (val & 0x0000ffff);
                *h->where = instr;
            }

            /* perform LO16 relocation */
            val = symval + addlo;
            *where = (*where & 0xffff0000) | (val & 0xffff);
            break;

        default:
            DEBUGF("Unknown relocation type: %d\n", ELF32_R_TYPE(rel->r_info));
            return -3;
    }

    return 0;
}

static int relocate_rel_sh(int fd, Elf32_Rel *rel, char *target_sect,
                           char *target_sect_img, struct mem_info_t *m)
{
   (void)m;
   (void)target_sect;

    Elf32_Addr symval;
    Elf32_Addr *where = (Elf32_Addr *)(target_sect_img + rel->r_offset);

    switch (ELF32_R_TYPE(rel->r_info))
    {
        case R_SH_NONE:
            break;

        case R_SH_DIR32:
            if (get_symbol_value(fd, rel, &symval))
                return -1;

            *where += symval;
            break;

        default:
            DEBUGF("Unknown relocation type: %d\n", ELF32_R_TYPE(rel->r_info));
            return -3;
    }

    return 0;
}

static int relocate_rel_m68k(int fd, Elf32_Rel *rel, char *target_sect,
                             char *target_sect_img, struct mem_info_t *m)
{
    (void)m;

    Elf32_Addr symval;
//    Elf32_Addr *where = (Elf32_Addr *)(target_sect + rel->r_offset);
    Elf32_Addr *where = (Elf32_Addr *)(target_sect_img + rel->r_offset);
    switch (ELF32_R_TYPE(rel->r_info))
    {
        case R_68K_NONE:
            break;

        case R_68K_32:
            if (get_symbol_value(fd, rel, &symval))
                return -1;

            *where += symval;
            break;

        default:
            DEBUGF("Unknown relocation type: %d\n", ELF32_R_TYPE(rel->r_info));
            return -3;
    }

    return 0;
}

#define SIGN_EXTEND24(x) ((x & 0x00ffffff) | \
                          ((x & 0x800000) ? 0xff000000 : 0x00000000))

#define VENEER_SIZE 8 /* maybe this needs to be variable */
#define VENEER_SLOTS 16
struct veneer_t
{
    Elf32_Word *where;
    Elf32_Word *where_img;
    Elf32_Addr target;
};

static struct veneer_t veneer_tab[VENEER_SLOTS];
static int veneer_idx = 0;

static int insert_veneer(Elf32_Addr target, Elf32_Addr *veneer_addr,
                         struct mem_info_t *m)
{
    Elf32_Addr align;
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
    align = (m->dram_runtime_used + 3) & ~3;

    if (align + VENEER_SIZE < m->dram_size)
    {
        if (veneer_idx < VENEER_SLOTS)
        {
            /* Create veneer entry */
            veneer_tab[veneer_idx].where =
                (Elf32_Word *)((char *)m->dram + align);

            veneer_tab[veneer_idx].where_img =
                (Elf32_Word *)((char *)m->dram_img + align);

            veneer_tab[veneer_idx].target = target;

            *veneer_addr = (Elf32_Addr)veneer_tab[veneer_idx].where;
            veneer_idx++;
        }
        else
        {
            DEBUGF("Veneer slots exchaused\n");
            return -1;
        }

        /* Account for inserted veneer */
        m->dram_runtime_used = align + VENEER_SIZE;
        DEBUGF("Inserting veneer at addr=0x%0x targeting addr=0x%0x\n",
               (unsigned int)((char *)m->dram + align), (unsigned int)target);
    }
    else
    {
        DEBUGF("Not enough dram mem to insert veneer\n");
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
        *v->where_img = 0xe51ff004;    /* ldr pc, [pc, #-4] */
        *(v->where_img+1) = v->target; /* .word TARGET_ADDRESS */
    }
}

static int relocate_rel_arm(int fd, Elf32_Rel *rel, char *target_sect,
                            char *target_sect_img, struct mem_info_t *m)
{
    Elf32_Addr symval;
    Elf32_Sword addend, offset;

    Elf32_Addr where_ptr = ((Elf32_Addr)target_sect + rel->r_offset);
    Elf32_Addr *where = (Elf32_Addr *)(target_sect_img + rel->r_offset);
    switch (ELF32_R_TYPE(rel->r_info))
    {
        case R_ARM_NONE:
        case R_ARM_V4BX:
            break;

        case R_ARM_ABS32:
            if (get_symbol_value(fd, rel, &symval))
                return -1;

            *where += symval;
            break;

        case R_ARM_PLT32:
        case R_ARM_CALL:
        case R_ARM_JUMP24:
            if (get_symbol_value(fd, rel, &symval))
                return -1;

            addend = SIGN_EXTEND24(*where & 0x00ffffff);

            offset = (symval - (Elf32_Addr)where_ptr + (addend << 2));

            /* Only word align addresses are permited */
            if (offset % 4)
                return -2;
            /* Check for overflow */
            if (offset >= 0x02000000 ||
                offset <= -0x02000000)
            {
                if (insert_veneer(symval, &symval, m) != 0)
                    return -2;

                offset = (symval - (Elf32_Addr)where_ptr + (addend << 2));
                DEBUGF("patching call at addr=0x%0x, "
                       "symval=0x%0x offset=0x%0x\n",
                       (unsigned int)where_ptr, symval, offset);
            }

            *where = (*where & 0xff000000) | ((offset>>2) & 0x00ffffff);
            break;

        default:
            DEBUGF("Unknown relocation type: %d\n", ELF32_R_TYPE(rel->r_info));
            return -3;
    }

    return 0;
}

/* Wrapper to call arch specific functions */
static int relocate_rel(int fd, Elf32_Rel *rel, char *target_sect,
                        char *target_sect_img, struct mem_info_t *m)
{
    Elf32_Addr symval;
    Elf32_Addr where_ptr = ((Elf32_Addr)target_sect + rel->r_offset);
    Elf32_Addr where_val = *(Elf32_Addr *)(target_sect_img + rel->r_offset);

    if (get_symbol_value(fd, rel, &symval) != 0)
        DEBUGF("Relocation: ELF32_R_TYPE: %d, ELF32_R_SYM: 0x%x\n",
               ELF32_R_TYPE(rel->r_info), ELF32_R_SYM(rel->r_info));
    else
        DEBUGF("Process relocation: where: 0x%0x, *where: 0x%0x, ELF32_R_TYPE: %d,"
               " ELF32_R_SYM: 0x%x, symval: 0x%x\n",
               where_ptr, where_val, ELF32_R_TYPE(rel->r_info),
               ELF32_R_SYM(rel->r_info), symval);

    switch (elf_header.e_machine)
    {
        case EM_MIPS:
            if (relocate_rel_mips(fd, rel, target_sect, target_sect_img, m) != 0)
                return -1;
            break;

        case EM_68K:
            if (relocate_rel_m68k(fd, rel, target_sect, target_sect_img, m) != 0)
                return -1;
            break;

        case EM_SH:
            if (relocate_rel_sh(fd, rel, target_sect, target_sect_img, m) != 0)
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

static int relocate_section_rel(int fd, Elf32_Shdr *shdr, struct mem_info_t *m)
{
    Elf32_Rel rel_entry, *rel;
    int i;
    int nr_reloc = (int)(shdr->sh_size / shdr->sh_entsize);
    char *target_sect = sect_addr[shdr->sh_info];
    char *target_sect_img = sect_addr_img[shdr->sh_info];

    rel = &rel_entry;

    DEBUGF("section[%d]: number of relocations: %d\n", shdr->sh_info, nr_reloc);

    /* Loop through all relocation entries for the section */
    for (i = 0; i < nr_reloc; i++)
    {
        DEBUGF("rel[%d] -> ", i);

        /* Read Elf32_Rel entry */
        lseek(fd, shdr->sh_offset + i*sizeof(Elf32_Rel), SEEK_SET);
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

    switch (shdr->sh_type)
    {
        case SHT_REL:
            error = relocate_section_rel(fd, shdr, m);
            break;

        default:
            error = -1;
            break;
    }

    return error;
}

static paddr_t get_address(Elf32_Shdr *shdr, struct mem_info_t *m)
{
    if (shdr->sh_addr < ADDRESS_SPACE_SPLIT)
        return (paddr_t)m->dram + (shdr->sh_addr & ~m->uncache_base);
    else
        return (paddr_t)m->iram + (shdr->sh_addr & ADDRESS_MASK);
}

static paddr_t get_address_img(Elf32_Shdr *shdr, struct mem_info_t *m)
{
    if (shdr->sh_addr < ADDRESS_SPACE_SPLIT)
        return (paddr_t)m->dram_img + (shdr->sh_addr & ~m->uncache_base);
    else
        return (paddr_t)m->iram_img + (shdr->sh_addr & ADDRESS_MASK);
}

static int check_section_mem_req(Elf32_Shdr *shdr, struct mem_info_t *m, int load)
{
    if (shdr->sh_addr < ADDRESS_SPACE_SPLIT)
    {
        m->dram_runtime_used += shdr->sh_size;

        /* Check mem requirements */
        if (m->dram_runtime_used > m->dram_size)
        {
            DEBUGF("Not enough dram memory to load section:"
                   " need=0x%0x aval=0x%0x\n",
                   m->dram_runtime_used, m->dram_size);

            return -1;
        }

        if (load)
            m->dram_load_used += shdr->sh_size;
    }
    else
    {
        m->iram_runtime_used += shdr->sh_size;

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

    if ((int)ehdr->e_shnum > MAX_ELF_SECTIONS)
    {
        DEBUGF("Can't support more then %d sections\n", MAX_ELF_SECTIONS);
        return -1;
    }

    /* Loop through all sections */
    for (i = 0; i < (int)ehdr->e_shnum; i++)
    {
        /* Get section header */
        lseek(fd, ehdr->e_shoff + (i * sizeof(Elf32_Shdr)), SEEK_SET);
        read(fd, &section_header, sizeof(Elf32_Shdr));
        
        sect_addr[i] = (char *)get_address(shdr, m);
        sect_addr_img[i] = (char *)get_address_img(shdr, m);

        DEBUGF("\nsection[%d]: sh_type=0x%0x, sh_flags=0x%0x "
               "sh_addr=0x%0x sh_offset=0x%0x sh_size=0x%0x\n",
               i, shdr->sh_type, shdr->sh_flags, shdr->sh_addr,
               shdr->sh_offset, shdr->sh_size);

        DEBUGF("section[%d]: sect_addr=0x%0x, sect_addr_img=0x%0x\n",
               i, (uintptr_t)sect_addr[i], (uintptr_t)sect_addr_img[i]);

        if (shdr->sh_flags & SHF_ALLOC)
        {
            if (shdr->sh_type == SHT_PROGBITS)
            {
                if (shdr->sh_size)
                {
                    if (check_section_mem_req(shdr, m, 1) != 0)
                        return -1;

                    lseek(fd, shdr->sh_offset, SEEK_SET);
                    read(fd, (void *)sect_addr_img[i], (size_t)shdr->sh_size);

                    DEBUGF("loading section at addr=0x%0x size=0x%0x\n",
                           (unsigned int)sect_addr[i], (int)shdr->sh_size);
                } /* if (shdr->sh_size) */

            } /* if (shdr->sh_type == SHT_PROGBITS) */
            else if (shdr->sh_type == SHT_NOBITS)
            {
                /* .bss */
                if (shdr->sh_size)
                {
                    if (check_section_mem_req(shdr, m, 0) != 0)
                        return -1;

                } /* if (shdr->size) */

            }
        } /* if (shdr->sh_flags & SHF_ALLOC) */
        else
        {
            if (shdr->sh_type == SHT_SYMTAB)
            {
                /* load symtab into memory */
                if (m->dram_size > m->dram_load_used + shdr->sh_size)
                {
                    symtab = (Elf32_Sym *)((char *)m->dram_img + m->dram_size - shdr->sh_size);
                    symtab_size = shdr->sh_size;
                    DEBUGF("Loading symtab at addr=0x%0x (0x%0x) size=0x%0x\n", 
                           (uintptr_t)((char *)m->dram + m->dram_size - symtab_size),
                           (uintptr_t)symtab, symtab_size);

                    lseek(fd, shdr->sh_offset, SEEK_SET);
                    if (read(fd, (void *)symtab, (size_t)symtab_size) != symtab_size)
                    {
                       DEBUGF("Error loading symtab\n");
                       return -1;
                    }
                }
                else
                {
                    DEBUGF("Not enough dram mem to load symtab need=0x%0x aval=0x%0x\n",
                           m->dram_load_used + shdr->sh_size, m->dram_size);
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
    for (i = 0; i < (int)ehdr->e_shnum; i++)
    {
        lseek(fd, ehdr->e_shoff + (i * sizeof(Elf32_Shdr)), SEEK_SET);
        read(fd, &section_header, sizeof(Elf32_Shdr));
        
        if (shdr->sh_type == SHT_REL || shdr->sh_type == SHT_RELA)
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
    for (i = 0; i < (int)ehdr->e_shnum; i++)
    {
        lseek(fd, ehdr->e_shoff + (i * sizeof(Elf32_Shdr)), SEEK_SET);
        read(fd, &section_header, sizeof(Elf32_Shdr));

        if ((shdr->sh_flags & SHF_ALLOC) && (shdr->sh_type == SHT_NOBITS))
        {
            DEBUGF("Clearing mem at addr=0x%0x size=0x%0x\n",
                   (unsigned int)sect_addr[i], (size_t)shdr->sh_size);
            memset((void *)sect_addr_img[i], 0, (size_t)shdr->sh_size);
        }
    }

    DEBUGF("Clearing common symbols area at addr=0x%0x size=0x%0x\n",
           (unsigned int)sect_addr[COMMON_SYMBOLS_IDX],
           (unsigned int)common_size);

    memset((void *)sect_addr_img[COMMON_SYMBOLS_IDX], 0, common_size);

if (elf_header.e_machine == EM_ARM)
    apply_veneers();

    return 0;
}
