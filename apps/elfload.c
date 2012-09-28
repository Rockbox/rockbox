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
 *    may be use to endorse or promote products derived from this software
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

#include <ctype.h>
#include <stdint.h>

#include "logdiskf.h"  /* log API */
#include "storage.h"
#include "filefuncs.h"
#include "load_code.h" /* lc_*() */
#include "audio.h"     /* sound_stop() */

#include "elf.h"       /* ELF types, defs and macros */
#include "elfload.h"

/* forward declarations */
static int load_relocatable(int fd, struct mem_info_t *m);

#define MAX_ELF_SECTIONS 32
#define COMMON_IDX 32

#define ADDRESS_SPACE_SPLIT 0x80000000
#define ADDRESS_MASK        0x7fffffff

/* The extra ptr is for COMMON symbols */
static char *sect_addr[MAX_ELF_SECTIONS+1];  /* array of section address */
static Elf32_Sym *symtab = NULL;             /* ptr to symtab */
static size_t symtab_size = 0;

static Elf32_Ehdr elf_header;                /* storage space for ELF header */

/*
 * Load the program from specified ELF file.
 */
void *elf_open(const char *filename, struct mem_info_t *m)
{
    Elf32_Ehdr *ehdr;

    int fd = open(filename, O_RDONLY);

    if (fd < 0)
    {
        ERRORF("Could not open file %s", filename);
        goto error;
    }

    /* Read elf header */
    lseek(fd, 0, SEEK_SET);
    size_t bytes_read = read(fd, &elf_header, sizeof(Elf32_Ehdr));

    if (bytes_read != sizeof(Elf32_Ehdr))
    {
        ERRORF("Error reading elf header");
        goto error_fd;
    }

    /* to simplify porting */
    ehdr = &elf_header;

    /*  Check ELF header */
    if ((ehdr->e_ident[EI_MAG0] != ELFMAG0) ||
        (ehdr->e_ident[EI_MAG1] != ELFMAG1) ||
        (ehdr->e_ident[EI_MAG2] != ELFMAG2) ||
        (ehdr->e_ident[EI_MAG3] != ELFMAG3))
    {
        ERRORF(("Invalid ELF image\n"));
        goto error_fd;
    }

    switch (ehdr->e_type)
    {
        case ET_REL:
            if (load_relocatable(fd, m) != 0)
                goto error_fd;
            break;

        default:
            ERRORF("Unsupported elf type 0x%x\n", ehdr->e_type);
            goto error_fd;
	}

    close(fd);

    /* patch plugin header */
    struct lc_header *lc_header_ptr = (struct lc_header *)m->dram;
    lc_header_ptr->load_addr = (unsigned char *)m->dram;
    lc_header_ptr->end_addr = (unsigned char *)(m->dram) + m->dram_runtime_use;

    return m->dram;

error_fd:
    close(fd);
error:
    return NULL;
}

static size_t process_common_symbols(Elf32_Shdr *shdr)
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
            NOTEF("COMMON sym[%d] st_value=0x%0x st_size=0x%0x\n",
                   i, sym->st_value, sym->st_size);

            bound = sym->st_value - 1;

            /* align according to constraint stored in st_value */
            common_offset = (common_size + bound) & ~bound;
            common_size = common_offset + sym->st_size;

            /* patch symbol entry for further use */
            sym->st_shndx = COMMON_IDX;
            sym->st_value = common_offset;
        }

        sym++;
    }

    /* Size must be multiple of 4 on ARM 
     * (most probably because some plugins don't check
     * the alignment of returned buffers. Need to investigate).
     */
    return ((common_size + 3) & ~3);
}

#ifdef ELF_USE_RELA
/* Updates symbol value *symval associated with relocation entry *rela
 * returns 0 on success, -1 on error
 */
static int get_symbol_value_rela(Elf32_Rela *rela, Elf32_Addr *symval)
{
    Elf32_Sym *sym = (symtab + ELF32_R_SYM(rela->r_info));

    NOTEF("symbol[%d]: st_name=0x%0x st_value=0x%0x "
           "st_size=0x%0x st_info=0x%0x st_shndx=0x%0x\n",
           (int)ELF32_R_SYM(rel->r_info), (unsigned int)sym->st_name,
           (unsigned int)sym->st_value, (unsigned int)sym->st_size,
           (unsigned int)sym->st_info, sym->st_shndx);

    if (sym->st_shndx != STN_UNDEF)
    {
        *symval = (Elf32_Addr)sect_addr[sym->st_shndx] + sym->st_value;
    }
    else
    {
        ERRORF("Undefined symbol\n");
        return -1;
    }

    return 0;
}
#endif

#ifdef ELF_USE_REL
/* Updates symbol value *symval associated with relocation entry *rel
 * returns 0 on success, -1 on error
 */
static int get_symbol_value_rel(Elf32_Rel *rel, Elf32_Addr *symval)
{
    Elf32_Sym *sym = (symtab + ELF32_R_SYM(rel->r_info));

    NOTEF("symbol[%d]: st_name=0x%0x st_value=0x%0x "
           "st_size=0x%0x st_info=0x%0x st_shndx=0x%0x\n",
           (int)ELF32_R_SYM(rel->r_info), (unsigned int)sym->st_name,
           (unsigned int)sym->st_value, (unsigned int)sym->st_size,
           (unsigned int)sym->st_info, sym->st_shndx);

    if (sym->st_shndx != STN_UNDEF)
    {
        *symval = (Elf32_Addr)sect_addr[sym->st_shndx] + sym->st_value;
    }
    else
    {
        ERRORF("Undefined symbol\n");
        return -1;
    }

    return 0;
}
#endif

#if defined(CPU_MIPS)
struct r_mips_hi16 {
    Elf_Addr *where;
    Elf_Addr symval;
};

#define SIGN_EXTEND16(x) ((x & 0xffff) | \
                         ((x & 0x8000) ? 0xffff0000 : 0x00000000))

#define HI16_TAB_ENTRIES 32
static int relocate_rel(Elf32_Rel *rel, char *target_sect, struct mem_info_t *m)
{
    (void)m;
    static struct r_mips_hi16 r_mips_hi16_tab[HI16_TAB_ENTRIES];
    static int next = 0;

    struct r_mips_hi16 *h;

    Elf32_Word val, instr, addlo;
    Elf32_Addr symval;
    Elf32_Addr *where = (Elf32_Addr *)(target_sect + rel->r_offset);

    switch (ELF32_R_TYPE(rel->r_info))
    {
        case R_MIPS_NONE:
            break;

        case R_MIPS_32:
            if (get_symbol_value_rel(rel, &symval))
                return -1;

            *where += symval;
            break;

        case R_MIPS_26:
            if (get_symbol_value_rel(rel, &symval))
                return -1;

            /* Only word aligned addresses can be expressed */
            if (symval % 4)
                return -2;

            /* Check for relocation overflow */
            if ((symval & 0xf0000000) != (((Elf32_Addr)where + 4) & 0xf0000000))
                return -2;

            *where = (*where & 0xfc000000 | ((symval>>2) & 0x03ffffff));
            break;

        case R_MIPS_HI16:
            /* To perform this relocation we need to know the carry from
             * R_MIPS_LO16, so save the information and perform actual
             * work in R_MIPS_LO16
             */
            if (get_symbol_value_rel(rel, &symval))
                return -1;

            /* We cannot track that much R_MIPS_HI16 entries */
            if (next >= HI16_TAB_ENTRIES)
                return -2;

            h = &r_mips_hi16_tab[next++];
            h->where = where;
            h->symval = symval;
            break;

        case R_MIPS_LO16:
            if (get_symbol_value_rel(rel, &symval))
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
            ERRORF("Unknown relocation type: %d\n", ELF32_R_TYPE(rel->r_info));
            return -3;
    }

    return 0;
}

#elif defined(CPU_SH)
static int relocate_rela(Elf32_Rela *rela,
                         char *target_sect,
                         struct mem_info_t *m)
{
    (void)m;
    Elf32_Addr symval;
    Elf32_Addr *where = (Elf32_Addr *)(target_sect + rela->r_offset);

    switch (ELF32_R_TYPE(rela->r_info))
    {
        case R_SH_NONE:
            break;

        case R_SH_DIR32:
            if (get_symbol_value_rela(rela, &symval))
                return -1;

            *where = symval + rela->r_addend;
            break;

        default:
            ERRORF("Unknown relocation type: %d\n", ELF32_R_TYPE(rela->r_info));
            return -3;
    }

    return 0;
}

#elif defined(CPU_COLDFIRE)
static int relocate_rela(Elf32_Rela *rela,
                         char *target_sect,
                         struct mem_info_t *m)
{
    (void)m;
    Elf32_Addr symval;
    Elf32_Addr *where = (Elf32_Addr *)(target_sect + rela->r_offset);

    switch (ELF32_R_TYPE(rela->r_info))
    {
        case R_68K_NONE:
            break;

        case R_68K_32:
            if (get_symbol_value_rela(rela, &symval))
                return -1;

            *where = symval + rela->r_addend;
            break;

        default:
            ERRORF("Unknown relocation type: %d\n", ELF32_R_TYPE(rela->r_info));
            return -3;
    }

    return 0;
}

#elif defined(CPU_ARM)
/* TODO currently it does not work on THUMB builds
 * ( but it whould fail gracefully as R_ARM_THM_CALL is not supported
 */
#define SIGN_EXTEND24(x) ((x & 0x00ffffff) | \
                          ((x & 0x800000) ? 0xff000000 : 0x00000000))

#define VENEER_SIZE 8    /* maybe this needs to be variable */
#define VENEER_SLOTS 16  /* TODO figure out resonable value */
enum mem_segment_t {
    DRAM_SEGMENT,
    IRAM_SEGMENT,
    UNKNOWN_SEGMENT
};

struct veneer_t
{
    Elf32_Word *where;
    Elf32_Addr target;
    enum mem_segment_t mem_segment;
};

static struct veneer_t veneer_tab[VENEER_SLOTS];
static int veneer_idx = 0;

static enum mem_segment_t get_mem_segment(Elf32_Addr addr,
                                          struct mem_info_t *m)
{
    if ((addr >= (Elf32_Addr)m->iram) &&
        (addr < (Elf32_Addr)((char *)m->iram + m->iram_size)))
        return IRAM_SEGMENT;
    else if ((addr >= (Elf32_Addr)m->dram) &&
             (addr < (Elf32_Addr)((char *)m->dram + m->dram_size)))
        return DRAM_SEGMENT;
    else
        return UNKNOWN_SEGMENT;
}

static int insert_veneer(Elf32_Addr source,
                         Elf32_Addr target,
                         Elf32_Addr *veneer_addr,
                         struct mem_info_t *m)
{
    Elf32_Addr align;
    Elf32_Word *where;
    enum mem_segment_t mem_segment = get_mem_segment(source, m);
    int i;

    /* Check if we have veneer for this target address already present */
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
    if (mem_segment == DRAM_SEGMENT)
    {
        align = (m->dram_runtime_use + 3) & ~3;

        if (align + VENEER_SIZE > m->dram_size)
        {
            ERRORF("Not enough dram mem to insert veneer\n");
            return -1;
        }

        where = (Elf32_Word *)((char *)m->dram + align);

        /* Check for overflow - it should not happen on our
         * targets but to be on the safe side...
         */
        if ((Elf32_Addr)where - source >= 0x02000000)
        {
            ERRORF("Memory for veneer is too far away\n");
            return -1;
        }

        /* Account for inserted veneer */
        m->dram_runtime_use = align + VENEER_SIZE;
    }
    else if (mem_segment == IRAM_SEGMENT)
    {
        align = (m->iram_runtime_use + 3) & ~3;

        if (align + VENEER_SIZE > m->iram_size)
        {
            ERRORF("Not enough iram mem to insert veneer\n");
            return -1;
        }

        /* We do not check for overflow as whole iram
         * is within jump range on all targets
         */
        where = (Elf32_Word *)((char *)m->iram + align);

        /* Account for inserted veneer */
        m->iram_runtime_use = align + VENEER_SIZE;
    }
    else
    {
        ERRORF("Unknown memory segment\n");
        return -1;
    }

    if (veneer_idx < VENEER_SLOTS)
    {
        veneer_tab[veneer_idx].where = where;
        veneer_tab[veneer_idx].target = target;
        *veneer_addr = (Elf32_Addr)veneer_tab[veneer_idx].where;
        veneer_idx++;

        NOTEF("Inserting veneer at addr=0x%0x target addr=0x%0x\n",
              (unsigned int)where, (unsigned int)target);
    }
    else
    {
        ERRORF("Veneer slots exchaused\n");
        return -1;
    }

    return 0;
}

/* Write veneers in memory */
static void apply_veneers(void)
{
    int i;
    struct veneer_t *v;

    NOTEF("Writing %d veneer(s) in dram\n", veneer_idx);

    for (i=0; i < veneer_idx; i++)
    {
        v = &veneer_tab[i];
        *v->where = 0xe51ff004;    /* ldr pc, [pc, #-4] */
        *(v->where+1) = v->target; /* .word TARGET_ADDRESS */
    }
}

static int relocate_rel(Elf32_Rel *rel,
                        char *target_sect,
                        struct mem_info_t *m)
{
    Elf32_Addr symval;
    Elf32_Sword addend, offset;

    Elf32_Addr *where = (Elf32_Addr *)(target_sect + rel->r_offset);

    switch (ELF32_R_TYPE(rel->r_info))
    {
        case R_ARM_NONE:
        case R_ARM_V4BX:
            break;

        case R_ARM_ABS32:
            if (get_symbol_value_rel(rel, &symval))
                return -1;

            *where += (symval & ~1);
            break;

        case R_ARM_PLT32:
        case R_ARM_CALL:
        case R_ARM_JUMP24:
            if (get_symbol_value_rel(rel, &symval))
                return -1;

            addend = SIGN_EXTEND24(*where & 0x00ffffff);

            offset = ((symval & ~1) - (Elf32_Addr)where + (addend << 2));

            /* Only word align addresses are permited */
            if (offset % 4)
            {
                return -2;
            }

            /* Check for overflow */
            if (offset >= 0x02000000 ||
                offset <= -0x02000000)
            {
                /* Target address is too far away
                 * lets try to use veneer for it
                 */
                if (insert_veneer((Elf32_Addr)where, symval, &symval, m) != 0)
                    return -2;

                offset = ((symval & ~1) - (Elf32_Addr)where + (addend << 2));
                NOTEF("patching call at addr=0x%0x, "
                      "symval=0x%0x offset=0x%0x\n",
                      (unsigned int)where, symval, offset);
            }

            *where = (*where & 0xff000000) | ((offset>>2) & 0x00ffffff);
            break;

        default:
            ERRORF("Unknown relocation type: %d\n", ELF32_R_TYPE(rel->r_info));
            return -3;
    }

    return 0;
}
#endif

#ifdef ELF_USE_RELA
static int relocate_section_rela(int fd,
                                 Elf32_Shdr *shdr,
                                 struct mem_info_t *m)
{
    Elf32_Rela rela_entry, *rela;
    int nr_reloc = (int)(shdr->sh_size / shdr->sh_entsize);
    char *target_sect = sect_addr[shdr->sh_info];

    int i;

    rela = &rela_entry;

    for (i = 0; i < nr_reloc; i++)
    {
        /* Read Elf32_Rela entry */
        lseek(fd, shdr->sh_offset + (i * sizeof(Elf32_Rela)), SEEK_SET);
        read(fd, &rela_entry, sizeof(Elf32_Rela));

        /* Call architecture specific function */
        if (relocate_rela(rela, target_sect, m) != 0)
            return -1;

    }
    return 0;
}
#endif

#ifdef ELF_USE_REL
static int relocate_section_rel(int fd, Elf32_Shdr *shdr, struct mem_info_t *m)
{
    Elf32_Rel rel_entry, *rel;
    int i;
    int nr_reloc = (int)(shdr->sh_size / shdr->sh_entsize);
    char *target_sect = sect_addr[shdr->sh_info];

    rel = &rel_entry;

    NOTEF("section[%d]: number of relocations: %d\n",
          (int)shdr->sh_info, nr_reloc);

    /* Loop through all relocation entries for the section */
    for (i = 0; i < nr_reloc; i++)
    {
        NOTEF("rel[%d] -> ", i);

        /* Read Elf32_Rel entry */
        lseek(fd, shdr->sh_offset + (i * sizeof(Elf32_Rel)), SEEK_SET);
        read(fd, &rel_entry, sizeof(Elf32_Rel));

        /* Call architecture specific function */
        if (relocate_rel(rel, target_sect, m) != 0)
        {
            return -1;
        }
    }

    return 0;
}
#endif

static int relocate_section(int fd, Elf32_Shdr *shdr, struct mem_info_t *m)
{
    int error;

    if (shdr->sh_entsize == 0)
        return 0;

    switch (shdr->sh_type)
    {
#ifdef ELF_USE_REL
        case SHT_REL:
            error = relocate_section_rel(fd, shdr, m);
            break;
#elif defined(ELF_USE_RELA)
        case SHT_RELA:
            error = relocate_section_rela(fd, shdr, m);
            break;
#endif
        default:
            error = -1;
            break;
    }

    return error;
}

static char *get_address(Elf32_Shdr *shdr, struct mem_info_t *m)
{
    if (shdr->sh_addr < ADDRESS_SPACE_SPLIT)
        return (char *)m->dram + shdr->sh_addr;
    else
        return (char *)m->iram + (shdr->sh_addr & ADDRESS_MASK);
}

static int check_section_mem_req(Elf32_Shdr *shdr,
                                 struct mem_info_t *m,
                                 int load)
{
    if (shdr->sh_addr < ADDRESS_SPACE_SPLIT)
    {
        m->dram_runtime_use += shdr->sh_size;

        /* Check mem requirements */
        if (m->dram_runtime_use > m->dram_size)
        {
            ERRORF("Not enough dram memory to load section:"
                   " need=0x%0x aval=0x%0x\n",
                   m->dram_runtime_use, m->dram_size);

            return -1;
        }

        if (load)
            m->dram_load_use += shdr->sh_size;

    }
    else
    {
        m->iram_runtime_use += shdr->sh_size;

        /* Check mem requirements */
        if (m->iram_runtime_use > m->iram_size)
        {
            ERRORF("Not enough iram memory to load section:"
                   " need=0x%0x aval=0x%0x\n",
                   m->iram_runtime_use, m->iram_size);

            return -1;
        }

        /* If we are going to use iram we need to stop playback first
         * so codec will not overwrite our claimed mem
         */
        audio_stop();
    }

    return 0;
}

static int load_relocatable(int fd, struct mem_info_t *m)
{
    size_t common_size;
    Elf32_Ehdr *ehdr;
    Elf32_Shdr section_header, *shdr;
    int i;

    ehdr = &elf_header;
    shdr = &section_header;
    m->dram_runtime_use = m->dram_load_use = 0;
    m->iram_runtime_use = 0;

    if ((int)ehdr->e_shnum > MAX_ELF_SECTIONS)
    {
        ERRORF("Can't support more then %d sections\n", MAX_ELF_SECTIONS);
        return -1;
    }

    /* Loop through all sections */
    for (i = 1; i < (int)ehdr->e_shnum; i++)
    {
        /* Get section header */
        lseek(fd, ehdr->e_shoff + (i * sizeof(Elf32_Shdr)), SEEK_SET);
        read(fd, &section_header, sizeof(Elf32_Shdr));

        sect_addr[i] = get_address(shdr, m);

        NOTEF("\nsection[%d]: sh_type=0x%0x, sh_flags=0x%0x "
              "sh_addr=0x%0x sh_offset=0x%0x sh_size=0x%0x\n",
              (int)i, (unsigned int)shdr->sh_type,
              (unsigned int)shdr->sh_flags, (unsigned int)shdr->sh_addr,
              (unsigned int)shdr->sh_offset, (unsigned int)shdr->sh_size);

        if (shdr->sh_flags & SHF_ALLOC)
        {
            if (shdr->sh_type == SHT_PROGBITS)
            {
                if (shdr->sh_size)
                {
                    if (check_section_mem_req(shdr, m, 1) != 0)
                    {
                        return -1;
                    }

                    lseek(fd, shdr->sh_offset, SEEK_SET);
                    read(fd, (void *)sect_addr[i], (size_t)shdr->sh_size);

                    NOTEF("loading section at addr=0x%0x size=0x%0x\n",
                          (unsigned int)sect_addr[i], (int)shdr->sh_size);
                } /* if (shdr->sh_size) */

                NOTEF("sect_addr[%d] = 0x%x\n", i, (unsigned int)sect_addr[i]);

            } /* if (shdr->sh_type == SHT_PROGBITS) */
            else if (shdr->sh_type == SHT_NOBITS)
            {
                /* .bss */
                if (shdr->sh_size)
                {
                    if (check_section_mem_req(shdr, m, 0) != 0)
                        return -1;

                } /* if (shdr->size) */

                NOTEF("sect_addr[%d] = 0x%x\n", i, (unsigned int)sect_addr[i]);
            }
        } /* if (shdr->sh_flags & SHF_ALLOC) */
        else
        {
            if (shdr->sh_type == SHT_SYMTAB)
            {
                /* load symtab into memory */
                if (m->dram_size > m->dram_load_use + shdr->sh_size)
                {
                    symtab = (Elf32_Sym *)((char *)m->dram +
                                                   m->dram_size -
                                                   shdr->sh_size);
                    symtab_size = shdr->sh_size;

                    NOTEF("Loading symtab at addr=0x%0x size=0x%0x\n",
                           (unsigned int)symtab, symtab_size);

                    lseek(fd, shdr->sh_offset, SEEK_SET);
                    if (read(fd, (void *)symtab, symtab_size) !=
                                 (ssize_t)symtab_size)
                    {
                        ERRORF("Error loading symtab\n");
                        return -1;
                    }
                }
                else
                {
                    ERRORF("Not enough dram mem to load symtab\n");
                    return -1;
                }

                common_size = process_common_symbols(shdr);

                if (m->dram_size > common_size + m->dram_runtime_use)
                {
                    /* plug at the end */
                    sect_addr[COMMON_IDX] = (char *)m->dram +
                                                    m->dram_runtime_use;
                    m->dram_runtime_use += common_size;
                }
                else
                {
                    ERRORF("Not enough dram memory for common symbols "
                           "need=0x%0x aval=0x%0x\n",
                           m->dram_runtime_use + common_size, m->dram_size);
                    return -1;
                }
            }
        } /* shdr->sh_flags */
    } /* for */

    if (symtab == NULL)
    {
        ERRORF("Can't find symtab.\n");
        return -1;
    }

    NOTEF("\ndram run size: 0x%x/0x%x, "
          "iram run size: 0x%x/0x%x\n",
          m->dram_runtime_use, m->dram_size,
          m->iram_runtime_use, m->iram_size);

    NOTEF("\nProcessing relocations\n");
    /* Process relocation */
    for (i = 1; i < (int)ehdr->e_shnum; i++)
    {
        lseek(fd, ehdr->e_shoff + (i * sizeof(Elf32_Shdr)), SEEK_SET);
        read(fd, &section_header, sizeof(Elf32_Shdr));

        if (shdr->sh_type == SHT_REL || shdr->sh_type == SHT_RELA)
        {
            NOTEF("\nFound relocation info in section[%d]\n", i);

            if (relocate_section(fd, shdr, m) != 0)
            {
                ERRORF("Relocation error\n");
                return -1;
            }
        }
    }

    /* Deferred clearing of .bss, .ibss, .ncbss and COMMON */
    for (i = 1; i < (int)ehdr->e_shnum; i++)
    {
        lseek(fd, ehdr->e_shoff + (i * sizeof(Elf32_Shdr)), SEEK_SET);
        read(fd, &section_header, sizeof(Elf32_Shdr));

        if ((shdr->sh_flags & SHF_ALLOC) && (shdr->sh_type == SHT_NOBITS))
        {
            NOTEF("Clearing mem at addr=0x%0x size=0x%0x\n",
                   (unsigned int)sect_addr[i], (size_t)shdr->sh_size);

            memset((void *)sect_addr[i], 0, (size_t)shdr->sh_size);
        }
    }

    NOTEF("Clearing common symbols area at addr=0x%0x size=0x%0x\n",
           (unsigned int)sect_addr[COMMON_IDX],
           (unsigned int)common_size);

    memset((void *)sect_addr[COMMON_IDX], 0, common_size);

#if defined(CPU_ARM)
    apply_veneers();
#endif
    return 0;
}
