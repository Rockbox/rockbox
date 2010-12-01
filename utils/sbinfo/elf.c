#include "elf.h"

void elf_init(struct elf_params_t *params)
{
    params->has_start_addr = false;
    params->start_addr = 0;
    params->first_section = NULL;
    params->last_section = NULL;
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

void elf_add_load_section(struct elf_params_t *params,
    uint32_t load_addr, uint32_t size, const void *section)
{
    struct elf_section_t *sec = elf_add_section(params);

    sec->type = EST_LOAD;
    sec->addr = load_addr;
    sec->size = size;
    sec->section = xmalloc(size);
    memcpy(sec->section, section, size);
}

void elf_add_fill_section(struct elf_params_t *params,
    uint32_t fill_addr, uint32_t size, uint32_t pattern)
{
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
}

void elf_output(struct elf_params_t *params, elf_write_fn_t write, void *user)
{
    Elf32_Ehdr ehdr;
    uint32_t phoff = sizeof(Elf32_Ehdr);
    uint32_t phentsize = sizeof(Elf32_Phdr);
    uint32_t phnum = 0;
    uint32_t shstrndx = SHN_UNDEF;
    struct elf_section_t *sec = params->first_section;
    uint32_t offset = 0;
    Elf32_Phdr phdr;

    while(sec)
    {
        if(sec->type == EST_LOAD)
        {
            sec->offset = offset;
            offset += sec->size;
        }
        
        phnum++;
        sec = sec->next;
    }

    memset(&ehdr, 0, EI_NIDENT);
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
    ehdr.e_phoff = phoff;
    ehdr.e_shoff = 0;
    ehdr.e_flags = 0;
    if(params->has_start_addr)
        ehdr.e_flags |= EF_ARM_HASENTRY;
    ehdr.e_ehsize = sizeof ehdr;
    ehdr.e_phentsize = phentsize;
    ehdr.e_phnum = phnum;
    ehdr.e_shentsize = 0;
    ehdr.e_shnum = 0;
    ehdr.e_shstrndx = shstrndx;

    write(user, 0, &ehdr, sizeof ehdr);

    sec = params->first_section;
    offset = phoff;
    while(sec)
    {
        sec->offset += phoff + phnum * phentsize;
        
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
    while(sec)
    {
        if(sec->type == EST_LOAD)
            write(user, sec->offset, sec->section, sec->size);
        sec = sec->next;
    }
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

void elf_release(struct elf_params_t *params)
{
    struct elf_section_t *sec, *next_sec;
    sec = params->first_section;
    while(sec)
    {
        next_sec = sec->next;
        if(sec->type == EST_LOAD)
            free(sec->section);
        free(sec);
        sec = next_sec;
    }
    params->first_section = NULL;
    params->last_section = NULL;
}
